/*
  ==============================================================================

    MeteringPanel.cpp
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "MeteringPanel.h"
#include "../../ProjectSettings/MeteringSettings.h"

//==============================================================================
MeteringUI::MeteringUI(const String& contentName) :
    ShapeShifterContent(MeteringPanel::getInstance(), contentName)
{
}

MeteringUI::~MeteringUI()
{
}

juce_ImplementSingleton(MeteringPanel);

MeteringPanel::MeteringPanel()
{
    fifoData.assign((size_t)fifo.getTotalSize(), 0.0f);
    fftBuffer.assign((size_t)fftSize * 2, 0.0f);
    pendingColumn.assign((size_t)numBins, 0.0f);
    weightingGain.assign((size_t)numBins, 1.0f);
    laeqRing.assign((size_t)laeqSeconds, 0.0f);

    numRtaBands = (int)std::round(std::log2(rtaFMax / rtaFMin) * rtaBandsPerOctave) + 1;
    rtaCenters.resize((size_t)numRtaBands);
    for (int b = 0; b < numRtaBands; ++b)
        rtaCenters[(size_t)b] = rtaFMin * std::pow(2.0, (double)b / rtaBandsPerOctave);
    rtaBandLo.assign((size_t)numRtaBands, 1);
    rtaBandHi.assign((size_t)numRtaBands, 0);
    rtaSmooth.assign((size_t)numRtaBands, 0.0f);
    rtaPeak.assign((size_t)numRtaBands, 0.0f);

    window.resize((size_t)fftSize);
    windowPower = 0.0;
    for (int i = 0; i < fftSize; ++i)
    {
        window[(size_t)i] = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)i / (float)(fftSize - 1)));
        windowPower += (double)window[(size_t)i] * window[(size_t)i];
    }

    settings = dynamic_cast<MeteringSettings*>(ProjectSettings::getInstance()->getControllableContainerByName("meteringSettings"));

    startStopBtn.setTooltip("Start / stop the metering input");
    startStopBtn.onClick = [this] { if (settings) settings->active->setValue(!settings->active->boolValue()); };
    addAndMakeVisible(startStopBtn);

    modeBtn.setTooltip("Switch between the spectrogram and the RTA");
    modeBtn.onClick = [this] { if (settings) settings->displayMode->setNext(true); };
    addAndMakeVisible(modeBtn);

    if (settings != nullptr)
    {
        settings->calibration->addParameterListener(this);
        settings->weighting->addParameterListener(this);
        settings->threshold->addParameterListener(this);
        settings->displayMode->addParameterListener(this);
        settings->verticalOrientation->addParameterListener(this);
        settings->active->addParameterListener(this);

        settings->deviceManager.addAudioCallback(this);
    }

    if (Engine::mainEngine != nullptr)
        Engine::mainEngine->addEngineListener(this);

    applySettings();
    startTimerHz(60);
}

MeteringPanel::~MeteringPanel()
{
    stopTimer();

    if (Engine::mainEngine != nullptr)
        Engine::mainEngine->removeEngineListener(this);

    if (settings != nullptr)
    {
        settings->deviceManager.removeAudioCallback(this);

        settings->calibration->removeParameterListener(this);
        settings->weighting->removeParameterListener(this);
        settings->threshold->removeParameterListener(this);
        settings->displayMode->removeParameterListener(this);
        settings->verticalOrientation->removeParameterListener(this);
        settings->active->removeParameterListener(this);
    }
}

//==============================================================================

void MeteringPanel::clearSpectrogram()
{
    if (!spectro.isNull())
        spectro.clear(spectro.getBounds(), Colours::black);
    std::fill(rtaSmooth.begin(), rtaSmooth.end(), 0.0f);
    std::fill(rtaPeak.begin(), rtaPeak.end(), 0.0f);
    repaint();
}

// Wipe the spectrogram when a file is (re)loaded or a new/blank project is created,
// so leftover data from the previous project isn't shown.
void MeteringPanel::fileLoaded()   { clearSpectrogram(); }
void MeteringPanel::engineCleared() { clearSpectrogram(); }

void MeteringPanel::applySettings()
{
    if (settings == nullptr) return;

    calibrationDb = settings->calibration->floatValue();
    thresholdDb = settings->threshold->floatValue();
    weightingType = (int)settings->weighting->getValueData();
    displayMode = (int)settings->displayMode->getValueData();
    verticalTime = settings->verticalOrientation->boolValue();
    startStopBtn.setButtonText(settings->active->boolValue() ? "Stop" : "Start");
    modeBtn.setButtonText(displayMode == (int)MeteringSettings::RTA ? "RTA" : "Spectro");
}

void MeteringPanel::parameterValueChanged(Parameter* p)
{
    if (settings == nullptr) return;

    if (p == settings->calibration) calibrationDb = settings->calibration->floatValue();
    else if (p == settings->threshold) thresholdDb = settings->threshold->floatValue();
    else if (p == settings->weighting) weightingType = (int)settings->weighting->getValueData();
    else if (p == settings->displayMode)
    {
        displayMode = (int)settings->displayMode->getValueData();
        modeBtn.setButtonText(displayMode == (int)MeteringSettings::RTA ? "RTA" : "Spectro");
        resized();
    }
    else if (p == settings->verticalOrientation) { verticalTime = settings->verticalOrientation->boolValue(); resized(); }
    else if (p == settings->active)
    {
        startStopBtn.setButtonText(settings->active->boolValue() ? "Stop" : "Start");
        // On (re)start, wipe the previously generated spectrogram and reset the held
        // Max/Peak so old data isn't shown.
        if (settings->active->boolValue())
        {
            clearSpectrogram();
            resetMaxPeak();
        }
    }

    repaint();
}

float MeteringPanel::getMeasuredDbFs() const
{
    return (float)(10.0 * std::log10(juce::jmax(1e-12, splFastMs)));
}

//==============================================================================
// Audio thread

void MeteringPanel::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device != nullptr) sampleRate.store(device->getCurrentSampleRate());
    fifo.reset();
    deviceRunning.store(true);
}

void MeteringPanel::audioDeviceStopped()
{
    deviceRunning.store(false);
}

void MeteringPanel::audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
                                                     float* const* outputChannelData, int numOutputChannels,
                                                     int numSamples, const juce::AudioIODeviceCallbackContext&)
{
    for (int ch = 0; ch < numOutputChannels; ++ch)
        if (outputChannelData[ch] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);

    if (numInputChannels <= 0 || inputChannelData == nullptr) return;

    int start1, size1, start2, size2;
    fifo.prepareToWrite(numSamples, start1, size1, start2, size2);

    float peak = 0.0f;
    int written = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f; int n = 0;
        for (int ch = 0; ch < numInputChannels; ++ch)
            if (inputChannelData[ch] != nullptr) { mono += inputChannelData[ch][i]; ++n; }
        if (n > 0) mono /= (float)n;

        peak = juce::jmax(peak, std::abs(mono));

        if (written < size1) fifoData[(size_t)(start1 + written)] = mono;
        else if (written - size1 < size2) fifoData[(size_t)(start2 + (written - size1))] = mono;
        ++written;
    }
    fifo.finishedWrite(juce::jmin(written, size1 + size2));

    if (peak > inputPeak.load()) inputPeak.store(peak);
}

//==============================================================================
// Weighting

float MeteringPanel::weightingLinear(double f, int type)
{
    if (f <= 0.0) return 0.0f;
    const double f2 = f * f;
    const double c1 = 20.598997 * 20.598997;
    const double c2 = 107.65265 * 107.65265;
    const double c3 = 737.86223 * 737.86223;
    const double c4 = 12194.217 * 12194.217;
    const double c5 = 158.48932 * 158.48932;

    if (type == 0) // A
    {
        double ra = (c4 * f2 * f2) / ((f2 + c1) * std::sqrt((f2 + c2) * (f2 + c3)) * (f2 + c4));
        return (float)(ra * 1.2589254);
    }
    if (type == 2) // C
    {
        double rc = (c4 * f2) / ((f2 + c1) * (f2 + c4));
        return (float)(rc * 1.0069317);
    }
    // B
    double rb = (c4 * f2 * f) / ((f2 + c1) * std::sqrt(f2 + c5) * (f2 + c4));
    return (float)(rb * 1.0197611);
}

void MeteringPanel::ensureWeighting(double sr)
{
    if (weightingType == weightingComputedType && sr == weightingComputedSr) return;
    for (int b = 0; b < numBins; ++b)
        weightingGain[(size_t)b] = weightingLinear((double)b * sr / (double)fftSize, weightingType);
    weightingComputedType = weightingType;
    weightingComputedSr = sr;
}

void MeteringPanel::ensureRtaBands(double sr)
{
    if (sr == rtaBandsComputedSr) return;

    const double edge = std::pow(2.0, 0.5 / (double)rtaBandsPerOctave); // half a band
    for (int b = 0; b < numRtaBands; ++b)
    {
        const double centre = rtaCenters[(size_t)b];
        if (centre > sr * 0.5) // beyond Nyquist: mark empty (lo > hi) so it's skipped
        {
            rtaBandLo[(size_t)b] = 1;
            rtaBandHi[(size_t)b] = 0;
            continue;
        }

        int lo = (int)std::ceil ((centre / edge) * (double)fftSize / sr);
        int hi = (int)std::floor((centre * edge) * (double)fftSize / sr);
        if (lo > hi) // band narrower than the FFT bin spacing: sample the nearest bin
            lo = hi = (int)std::lround(centre * (double)fftSize / sr);

        rtaBandLo[(size_t)b] = juce::jlimit(1, numBins - 1, lo);
        rtaBandHi[(size_t)b] = juce::jlimit(1, numBins - 1, hi);
    }
    rtaBandsComputedSr = sr;
}

//==============================================================================
// Message thread DSP

void MeteringPanel::pullAndProcess()
{
    int ready = fifo.getNumReady();
    if (ready > 0)
    {
        int start1, size1, start2, size2;
        fifo.prepareToRead(ready, start1, size1, start2, size2);
        sampleAccum.insert(sampleAccum.end(), fifoData.begin() + start1, fifoData.begin() + start1 + size1);
        sampleAccum.insert(sampleAccum.end(), fifoData.begin() + start2, fifoData.begin() + start2 + size2);
        fifo.finishedRead(size1 + size2);
    }

    const double sr = sampleRate.load();
    ensureWeighting(sr);
    ensureRtaBands(sr);

    const bool haveImage = !spectro.isNull() && spectroImageArea.getWidth() > 0;
    const int W = haveImage ? spectro.getWidth() : 1;
    const int H = haveImage ? spectro.getHeight() : 1;
    const double columnDuration = 60.0 / juce::jmax(1, W);
    const double hopTime = (double)fftSize / sr;
    const double msScale = 1.0 / ((double)fftSize * windowPower);
    const double fastAlpha = 1.0 - std::exp(-hopTime / 0.125);
    const float  rtaAttackA  = (float)(1.0 - std::exp(-hopTime / 0.050));
    const float  rtaReleaseA = (float)(1.0 - std::exp(-hopTime / 0.300));

    while ((int)sampleAccum.size() >= fftSize)
    {
        for (int i = 0; i < fftSize; ++i)
            fftBuffer[(size_t)i] = sampleAccum[(size_t)i] * window[(size_t)i];
        std::fill(fftBuffer.begin() + fftSize, fftBuffer.end(), 0.0f);

        fft.performFrequencyOnlyForwardTransform(fftBuffer.data());

        double w0 = weightingGain[0] * fftBuffer[0];
        double energy = w0 * w0;
        for (int b = 1; b < numBins; ++b)
        {
            double wm = (double)weightingGain[(size_t)b] * fftBuffer[(size_t)b];
            energy += 2.0 * wm * wm;
            if (haveImage)
                pendingColumn[(size_t)b] = juce::jmax(pendingColumn[(size_t)b], fftBuffer[(size_t)b]);
        }
        if (haveImage) pendingColumn[0] = juce::jmax(pendingColumn[0], fftBuffer[0]);

        // RTA: aggregate bins into bands and store the band mean-square (power),
        // window-corrected and A/B/C-weighted like the SPL meter so it maps to dB SPL.
        // Wide bands (>= 2 FFT bins) sum their bins; narrow low-end bands, which are
        // thinner than the FFT bin spacing, instead interpolate the weighted power at
        // their centre frequency so adjacent bands differ smoothly rather than snapping
        // to the same bin (which produced a visible low-frequency staircase).
        for (int band = 0; band < numRtaBands; ++band)
        {
            int lo = rtaBandLo[(size_t)band], hi = rtaBandHi[(size_t)band];
            double bandEnergy = 0.0;
            if (hi > lo)
            {
                for (int bin = lo; bin <= hi; ++bin)
                {
                    double wm = (double)weightingGain[(size_t)bin] * fftBuffer[(size_t)bin];
                    bandEnergy += wm * wm;
                }
            }
            else if (hi == lo) // <= 1 bin wide: interpolate the weighted power at the centre
            {
                double xc = rtaCenters[(size_t)band] * (double)fftSize / sr;
                int k = juce::jlimit(1, numBins - 2, (int)xc);
                double frac = juce::jlimit(0.0, 1.0, xc - (double)k);
                double w0 = (double)weightingGain[(size_t)k]       * fftBuffer[(size_t)k];
                double w1 = (double)weightingGain[(size_t)(k + 1)] * fftBuffer[(size_t)(k + 1)];
                bandEnergy = w0 * w0 + (w1 * w1 - w0 * w0) * frac;
            }
            // (hi < lo: band beyond Nyquist -> bandEnergy stays 0)

            float p = (float)(2.0 * bandEnergy * msScale); // band mean-square (bins are all >= 1)
            float prev = rtaSmooth[(size_t)band];
            rtaSmooth[(size_t)band] = prev + (p - prev) * (p > prev ? rtaAttackA : rtaReleaseA);
            rtaPeak[(size_t)band] = juce::jmax(rtaPeak[(size_t)band], rtaSmooth[(size_t)band]);
        }

        double ms = energy * msScale;
        splFastMs += (ms - splFastMs) * fastAlpha;

        laeqSecEnergy += ms * hopTime;
        laeqSecTime += hopTime;
        if (laeqSecTime >= 1.0)
        {
            laeqRing[(size_t)laeqPos] = (float)(laeqSecEnergy / laeqSecTime);
            laeqPos = (laeqPos + 1) % laeqSeconds;
            laeqCount = juce::jmin(laeqSeconds, laeqCount + 1);
            laeqSecEnergy = 0.0; laeqSecTime = 0.0;
        }

        pendingColumnTime += hopTime;
        sampleAccum.erase(sampleAccum.begin(), sampleAccum.begin() + fftSize);

        if (haveImage)
            while (pendingColumnTime >= columnDuration)
            {
                commitColumn(W, H, sr);
                pendingColumnTime -= columnDuration;
            }
        else
            pendingColumnTime = 0.0;
    }

    if (!haveImage && (int)sampleAccum.size() > fftSize)
        sampleAccum.erase(sampleAccum.begin(), sampleAccum.end() - fftSize);
}

void MeteringPanel::commitColumn(int w, int h, double sr)
{
    spectro.moveImageSection(0, 0, 1, 0, w - 1, h);

    for (int y = 0; y < h; ++y)
    {
        double frac = 1.0 - (double)y / (double)juce::jmax(1, h - 1);
        double freq = freqFromFrac(frac, sr);
        int bin = (int)(freq * (double)fftSize / sr + 0.5);
        bin = juce::jlimit(1, numBins - 1, bin);

        float mag = pendingColumn[(size_t)bin] * (2.0f / (float)fftSize);
        spectro.setPixelAt(w - 1, y, magnitudeToColour(mag));
    }

    std::fill(pendingColumn.begin(), pendingColumn.end(), 0.0f);
}

double MeteringPanel::fracFromFreq(double f, double sr) const
{
    double fMin = 20.0, fMax = juce::jmax(40.0, sr * 0.5);
    return juce::jlimit(0.0, 1.0, std::log(f / fMin) / std::log(fMax / fMin));
}

double MeteringPanel::freqFromFrac(double frac, double sr) const
{
    double fMin = 20.0, fMax = juce::jmax(40.0, sr * 0.5);
    return fMin * std::exp(std::log(fMax / fMin) * frac);
}

juce::Colour MeteringPanel::magnitudeToColour(float mag)
{
    float db = juce::Decibels::gainToDecibels(mag, -100.0f);
    float norm = juce::jlimit(0.0f, 1.0f, (db + 80.0f) / 80.0f);
    // Gamma-boost the brightness so low/mid-level detail isn't rendered dim/washed-out.
    // Hue stays tied to the raw level so the colour-to-level mapping is unchanged.
    float bright = std::pow(norm, 0.5f);
    return juce::Colour::fromHSV(0.66f * (1.0f - norm), 1.0f, bright, 1.0f);
}

String MeteringPanel::weightingLabel() const
{
    return weightingType == 0 ? "dB SPL (A)" : (weightingType == 1 ? "dB SPL (B)" : "dB SPL (C)");
}

void MeteringPanel::timerCallback()
{
    pullAndProcess();

    // RTA peak hold decays ~15 dB/s, never falling below the current smoothed level.
    const float rtaPeakDecay = 0.972f; // per 60 Hz tick
    for (int b = 0; b < numRtaBands; ++b)
        rtaPeak[(size_t)b] = juce::jmax(rtaPeak[(size_t)b] * rtaPeakDecay, rtaSmooth[(size_t)b]);

    float peak = inputPeak.exchange(0.0f);
    float db = juce::Decibels::gainToDecibels(peak, -100.0f);
    if (db > meterDb) meterDb = db;
    else meterDb = juce::jmax(db, meterDb - 0.6f);

    splNow = (float)(10.0 * std::log10(juce::jmax(1e-12, splFastMs))) + calibrationDb;

    if (laeqCount > 0)
    {
        double sum = 0.0;
        for (int i = 0; i < laeqCount; ++i) sum += laeqRing[(size_t)i];
        splLaeq = (float)(10.0 * std::log10(juce::jmax(1e-12, sum / laeqCount))) + calibrationDb;
    }
    else splLaeq = -120.0f;

    // Hold the maximum (weighted) Now and the true peak (unweighted) while running.
    if (deviceRunning.load())
    {
        splMax = juce::jmax(splMax, splNow);
        float peakSpl = 20.0f * std::log10(juce::jmax(1e-9f, peak)) + calibrationDb;
        splPeak = juce::jmax(splPeak, peakSpl);
    }

    // Over-threshold alert: keep the red border for ~350 ms after dropping back below.
    if (deviceRunning.load() && splNow >= thresholdDb)
        alertHoldTicks = 21;            // ~0.35 s at 60 Hz
    else if (alertHoldTicks > 0)
        --alertHoldTicks;

    repaint();
}

//==============================================================================
// Mouse

void MeteringPanel::mouseMove(const juce::MouseEvent& e)
{
    mousePos = e.getPosition();
    mouseInSpectro = spectroImageArea.contains(mousePos);
    repaint();
}

void MeteringPanel::mouseExit(const juce::MouseEvent&)
{
    mouseInSpectro = false;
    repaint();
}

void MeteringPanel::mouseDown(const juce::MouseEvent& e)
{
    // Clicking either held readout resets both, via the settings trigger.
    if (settings != nullptr && (maxClickArea.contains(e.getPosition()) || peakClickArea.contains(e.getPosition())))
        settings->resetMaxPeak->trigger();
}

//==============================================================================
// Layout

void MeteringPanel::resized()
{
    auto r = getLocalBounds();

    // Mode + Start/Stop buttons, top-right corner overlay.
    startStopBtn.setBounds(getWidth() - 64, 4, 58, 22);
    modeBtn.setBounds(getWidth() - 64 - 4 - 78, 4, 78, 22);

    auto leftCol = r.removeFromLeft(140);
    meterArea = leftCol.removeFromLeft(58);
    readoutArea = leftCol;
    r.removeFromLeft(2);

    if (freqOnX())
    {
        freqAxisArea = r.removeFromBottom(16);
        spectroImageArea = r;
    }
    else
    {
        freqAxisArea = r.removeFromLeft(34);
        spectroImageArea = r;
    }

    int w = verticalTime ? spectroImageArea.getHeight() : spectroImageArea.getWidth();
    int h = verticalTime ? spectroImageArea.getWidth()  : spectroImageArea.getHeight();
    if (w > 0 && h > 0 && (spectro.getWidth() != w || spectro.getHeight() != h))
    {
        // Rescale the existing spectrogram into the new size instead of starting
        // from a blank image, so resizing the panel keeps the current content.
        // The buffer always stores time on X and frequency on Y, so a plain
        // rescale stays correct in both orientations.
        juce::Image resized(juce::Image::RGB, w, h, true);
        if (!spectro.isNull())
        {
            juce::Graphics ig(resized);
            ig.drawImage(spectro, 0, 0, w, h, 0, 0, spectro.getWidth(), spectro.getHeight());
        }
        spectro = resized;
    }
}

//==============================================================================
// Paint

void MeteringPanel::drawReadout(juce::Graphics& g)
{
    g.setColour(BG_COLOR.darker(.08f));
    g.fillRect(readoutArea);

    bool running = deviceRunning.load();
    String w = weightingLabel();

    auto area = readoutArea;
    auto nowR = area.removeFromTop(area.getHeight() / 2);
    auto avgR = area;

    auto drawBig = [&](juce::Rectangle<int> a, const String& title, float value,
                       const String& subLabel, float subValue, juce::Rectangle<int>& clickAreaOut)
    {
        a = a.withTrimmedLeft(6).withTrimmedRight(6).withTrimmedTop(4).withTrimmedBottom(4);

        // Title (top, centred).
        g.setColour(Colours::white.withAlpha(.55f));
        g.setFont(11.0f);
        g.drawText(title, a.removeFromTop(14), Justification::centred);

        // Held secondary value (Max / Peak) at the bottom - prominent and clickable to reset.
        clickAreaOut = a.removeFromBottom(18);
        bool subOver = running && subValue >= thresholdDb;
        String sv = (!running || subValue <= -119.0f) ? String("--.-") : String(subValue, 1);
        g.setColour(subOver ? Colours::red : Colours::white);
        g.setFont(Font(14.0f, Font::bold));
        g.drawText(subLabel + " " + sv, clickAreaOut, Justification::centred);

        // Big measured value with the weighting label directly beneath it, the pair
        // vertically centred in the remaining space.
        bool over = running && value >= thresholdDb;
        const int wH = 13;
        int blockH = juce::jmin(a.getHeight(), 34 + wH);
        auto block = a.withSizeKeepingCentre(a.getWidth(), blockH);
        int vH = juce::jmax(14, blockH - wH);

        String v = (!running || value <= -119.0f) ? String("--.-") : String(value, 1);
        g.setColour(over ? Colours::red : Colours::white);
        g.setFont(Font((float)juce::jlimit(14, 30, vH - 2), Font::bold));
        g.drawText(v, block.removeFromTop(vH), Justification::centred);

        g.setColour(Colours::white.withAlpha(.5f));
        g.setFont(10.0f);
        g.drawText(w, block, Justification::centred);
    };

    drawBig(nowR, "Now", splNow, "Max", splMax, maxClickArea);
    drawBig(avgR, "LAeq 15 min", splLaeq, "Pk", splPeak, peakClickArea);

    g.setColour(BG_COLOR.brighter(.1f));
    g.drawHorizontalLine(nowR.getBottom(), (float)readoutArea.getX() + 6, (float)readoutArea.getRight() - 6);
}

void MeteringPanel::drawMeter(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(BG_COLOR.darker(.1f));
    g.fillRect(area);

    g.setColour(Colours::white.withAlpha(.6f));
    g.setFont(10.0f);
    g.drawText("dBFS", area.getX(), area.getY() + 2, area.getWidth(), 12, Justification::centred);

    auto bar = area.reduced(12, 0).withTrimmedTop(16).withTrimmedBottom(16).withTrimmedRight(16);
    g.setColour(Colours::black.withAlpha(.5f));
    g.fillRect(bar);

    const float minDb = -60.0f, maxDb = 0.0f;
    float norm = juce::jlimit(0.0f, 1.0f, (meterDb - minDb) / (maxDb - minDb));
    int fillH = (int)(bar.getHeight() * norm);
    juce::Rectangle<int> fill = bar.withTop(bar.getBottom() - fillH);

    juce::ColourGradient grad(Colours::limegreen, (float)bar.getX(), (float)bar.getBottom(),
                              Colours::red, (float)bar.getX(), (float)bar.getY(), false);
    grad.addColour(0.7, Colours::yellow);
    g.setGradientFill(grad);
    g.fillRect(fill);

    g.setColour(Colours::white.withAlpha(.5f));
    g.setFont(9.0f);
    for (int dbTick = 0; dbTick >= -60; dbTick -= 12)
    {
        float t = (float)(dbTick - minDb) / (maxDb - minDb);
        int yy = bar.getBottom() - (int)(bar.getHeight() * t);
        g.drawText(String(dbTick), bar.getRight() + 1, yy - 6, 16, 12, Justification::centredLeft);
    }
}

void MeteringPanel::drawFreqAxis(juce::Graphics& g, double sr)
{
    static const int ticks[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    double fMax = juce::jmax(40.0, sr * 0.5);

    g.setFont(9.0f);
    for (int f : ticks)
    {
        if (f < 20 || f > fMax) continue;
        double frac = fracFromFreq((double)f, sr);
        String label = (f >= 1000) ? String(f / 1000) + "k" : String(f);

        if (freqOnX())
        {
            int x = spectroImageArea.getX() + (int)(frac * spectroImageArea.getWidth());
            g.setColour(Colours::white.withAlpha(.08f));
            g.drawVerticalLine(x, (float)spectroImageArea.getY(), (float)spectroImageArea.getBottom());
            g.setColour(Colours::white.withAlpha(.45f));
            g.drawText(label, x - 16, freqAxisArea.getY(), 32, freqAxisArea.getHeight(), Justification::centred);
        }
        else
        {
            int y = spectroImageArea.getY() + (int)(frac * spectroImageArea.getHeight());
            g.setColour(Colours::white.withAlpha(.08f));
            g.drawHorizontalLine(y, (float)spectroImageArea.getX(), (float)spectroImageArea.getRight());
            g.setColour(Colours::white.withAlpha(.45f));
            g.drawText(label, freqAxisArea.getX(), y - 6, freqAxisArea.getWidth() - 2, 12, Justification::centredRight);
        }
    }
}

void MeteringPanel::drawRTA(juce::Graphics& g, double sr)
{
    auto area = spectroImageArea;
    if (area.getWidth() <= 0 || area.getHeight() <= 0) return;

    const float left = (float)area.getX();
    const float right = (float)area.getRight();
    const float bottom = (float)area.getBottom();

    // Band mean-square (power) -> dB SPL via the calibration (dB SPL = dBFS + calibration),
    // shown on a fixed 30..120 dB SPL scale.
    const float rtaSplMin = 30.0f, rtaSplMax = 120.0f;
    auto splFor = [this](float power)
    {
        return 10.0f * std::log10(juce::jmax(power, 1e-12f)) + calibrationDb;
    };
    auto yForSpl = [&](float spl)
    {
        float frac = juce::jlimit(0.0f, 1.0f, (spl - rtaSplMin) / (rtaSplMax - rtaSplMin));
        return bottom - frac * (float)area.getHeight();
    };
    auto yFor = [&](float power) { return yForSpl(splFor(power)); };

    // Horizontal level grid + dB SPL labels (every 15 dB).
    g.setFont(9.0f);
    for (float spl = rtaSplMax; spl >= rtaSplMin - 0.1f; spl -= 15.0f)
    {
        int y = (int)yForSpl(spl);
        g.setColour(Colours::white.withAlpha(.08f));
        g.drawHorizontalLine(y, left, right);
        g.setColour(Colours::white.withAlpha(.35f));
        g.drawText(String((int)spl), area.getX() + 2, y + 1, 32, 11, Justification::topLeft);
    }
    g.setColour(Colours::white.withAlpha(.45f));
    g.drawText("dB SPL", area.getRight() - 52, area.getY() + 1, 50, 11, Justification::topRight);

    // Smoothed curve + peak-hold curve across the in-range band centres.
    juce::Path curve, peakPath;
    bool started = false;
    float firstX = left, lastX = left;
    for (int b = 0; b < numRtaBands; ++b)
    {
        if (rtaBandLo[(size_t)b] > rtaBandHi[(size_t)b]) continue; // beyond Nyquist
        float x = left + (float)fracFromFreq(rtaCenters[(size_t)b], sr) * (float)area.getWidth();
        float yS = yFor(rtaSmooth[(size_t)b]);
        float yP = yFor(rtaPeak[(size_t)b]);

        if (!started) { curve.startNewSubPath(x, yS); peakPath.startNewSubPath(x, yP); started = true; firstX = x; }
        else          { curve.lineTo(x, yS);          peakPath.lineTo(x, yP); }
        lastX = x;
    }
    if (!started) return;

    // Filled area under the smoothed curve.
    juce::Path filled = curve;
    filled.lineTo(lastX, bottom);
    filled.lineTo(firstX, bottom);
    filled.closeSubPath();
    g.setColour(Colours::limegreen.withAlpha(.18f));
    g.fillPath(filled);

    g.setColour(Colours::limegreen);
    g.strokePath(curve, juce::PathStrokeType(1.5f));

    g.setColour(Colours::white.withAlpha(.7f));
    g.strokePath(peakPath, juce::PathStrokeType(1.0f));
}

void MeteringPanel::paint(juce::Graphics& g)
{
    g.fillAll(BG_COLOR);

    drawReadout(g);
    drawMeter(g, meterArea);

    double sr = sampleRate.load();

    // Black background for the whole spectrogram area, so the not-yet-filled
    // part matches the active spectrogram instead of the panel's grey.
    g.setColour(Colours::black);
    g.fillRect(spectroImageArea);

    if (displayMode == 1)
    {
        drawRTA(g, sr);
    }
    else if (!spectro.isNull())
    {
        if (verticalTime)
        {
            int h = spectro.getHeight();
            juce::AffineTransform t(0.0f, -1.0f, (float)(spectroImageArea.getX() + h - 1),
                                    1.0f, 0.0f, (float)spectroImageArea.getY());
            g.drawImageTransformed(spectro, t);
        }
        else
        {
            // New columns are committed at the buffer's right edge (x = w-1) and
            // scroll left. Mirror on X so the newest data emerges on the LEFT, next to
            // the frequency axis - matching the vertical layout, where the newest row
            // emerges at the bottom next to its axis. Mirror on Y too so low
            // frequencies sit at the top and highs at the bottom (the buffer stores
            // highs at y=0 for the vertical path).
            int w = spectro.getWidth();
            int h = spectro.getHeight();
            juce::AffineTransform t(-1.0f, 0.0f, (float)(spectroImageArea.getX() + w - 1),
                                    0.0f, -1.0f, (float)(spectroImageArea.getY() + h - 1));
            g.drawImageTransformed(spectro, t);
        }
    }

    drawFreqAxis(g, sr);

    bool active = (settings != nullptr && settings->active->boolValue());
    if (!active)
    {
        g.setColour(Colours::white.withAlpha(.4f));
        g.setFont(15.0f);
        g.drawText("Stopped - press Start", spectroImageArea, Justification::centred);
        return;
    }
    if (!deviceRunning.load())
    {
        g.setColour(Colours::orangered.withAlpha(.85f));
        g.setFont(14.0f);
        g.drawText("No audio input (check microphone / permissions)", spectroImageArea, Justification::centred);
        return;
    }

    if (mouseInSpectro)
    {
        double freq;
        if (freqOnX())
        {
            double frac = (double)(mousePos.x - spectroImageArea.getX()) / (double)juce::jmax(1, spectroImageArea.getWidth());
            freq = freqFromFrac(juce::jlimit(0.0, 1.0, frac), sr);
            g.setColour(Colours::white.withAlpha(.5f));
            g.drawVerticalLine(mousePos.x, (float)spectroImageArea.getY(), (float)spectroImageArea.getBottom());
        }
        else
        {
            double frac = (double)(mousePos.y - spectroImageArea.getY()) / (double)juce::jmax(1, spectroImageArea.getHeight());
            freq = freqFromFrac(juce::jlimit(0.0, 1.0, frac), sr);
            g.setColour(Colours::white.withAlpha(.5f));
            g.drawHorizontalLine(mousePos.y, (float)spectroImageArea.getX(), (float)spectroImageArea.getRight());
        }

        String label = (freq >= 1000.0) ? String(freq / 1000.0, 2) + " kHz" : String((int)(freq + 0.5)) + " Hz";
        g.setFont(Font(12.0f, Font::bold));
        int tw = g.getCurrentFont().getStringWidth(label) + 12;
        juce::Rectangle<int> box(juce::jlimit(spectroImageArea.getX(), spectroImageArea.getRight() - tw, mousePos.x + 8),
                                 juce::jlimit(spectroImageArea.getY(), spectroImageArea.getBottom() - 18, mousePos.y - 18),
                                 tw, 16);
        g.setColour(Colours::black.withAlpha(.7f));
        g.fillRoundedRectangle(box.toFloat(), 3.0f);
        g.setColour(Colours::white);
        g.drawText(label, box, Justification::centred);
    }

    // Strong over-threshold alert: a thick red border around the whole panel,
    // held briefly (see alertHoldTicks) so it doesn't flicker.
    if (alertHoldTicks > 0)
    {
        g.setColour(Colours::red);
        g.drawRect(getLocalBounds(), 4);
    }
}
