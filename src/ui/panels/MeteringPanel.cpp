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
    laeqRing.assign((size_t)laeqMaxSeconds, 0.0f);
    splHistory.assign((size_t)splGraphSamples, -120.0f);

    numRtaBands = (int)std::round(std::log2(rtaFMax / rtaFMin) * rtaBandsPerOctave) + 1;
    rtaCenters.resize((size_t)numRtaBands);
    for (int b = 0; b < numRtaBands; ++b)
        rtaCenters[(size_t)b] = rtaFMin * std::pow(2.0, (double)b / rtaBandsPerOctave);
    rtaBandLo.assign((size_t)numRtaBands, 1);
    rtaBandHi.assign((size_t)numRtaBands, 0);
    rtaSmooth.assign((size_t)numRtaBands, 0.0f);
    rtaPeak.assign((size_t)numRtaBands, 0.0f);
    rtaReference.assign((size_t)numRtaBands, 0.0f);

    window.resize((size_t)fftSize);
    windowPower = 0.0;
    for (int i = 0; i < fftSize; ++i)
    {
        window[(size_t)i] = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)i / (float)(fftSize - 1)));
        windowPower += (double)window[(size_t)i] * window[(size_t)i];
    }

    settings = dynamic_cast<MeteringSettings*>(ProjectSettings::getInstance()->getControllableContainerByName("meteringSettings"));

    startStopBtn.setTooltip("Start / stop the metering input  (right-click the panel for display options)");
    startStopBtn.onClick = [this] { if (settings) settings->active->setValue(!settings->active->boolValue()); };
    addAndMakeVisible(startStopBtn);

    if (settings != nullptr)
    {
        settings->calibration->addParameterListener(this);
        settings->weighting->addParameterListener(this);
        settings->laeqWindow->addParameterListener(this);
        settings->threshold->addParameterListener(this);
        settings->displayMode->addParameterListener(this);
        settings->displayMode2->addParameterListener(this);
        settings->splitPosition->addParameterListener(this);
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
        settings->laeqWindow->removeParameterListener(this);
        settings->threshold->removeParameterListener(this);
        settings->displayMode->removeParameterListener(this);
        settings->displayMode2->removeParameterListener(this);
        settings->splitPosition->removeParameterListener(this);
        settings->verticalOrientation->removeParameterListener(this);
        settings->active->removeParameterListener(this);
    }
}

//==============================================================================

void MeteringPanel::clearGraphs()
{
    if (!spectro.isNull())
        spectro.clear(spectro.getBounds(), Colours::black);
    std::fill(rtaSmooth.begin(), rtaSmooth.end(), 0.0f);
    std::fill(rtaPeak.begin(), rtaPeak.end(), 0.0f);
    rtaHasReference = false;
    std::fill(splHistory.begin(), splHistory.end(), -120.0f);
    splHistoryPos = 0; splHistoryCount = 0; splGraphTick = 0; splBucketMax = -120.0f;

    // Discard any in-flight audio so it can't repopulate the just-cleared graphs
    // (e.g. leftover FIFO samples committed by the next pullAndProcess after a New).
    sampleAccum.clear();
    std::fill(pendingColumn.begin(), pendingColumn.end(), 0.0f);
    pendingColumnTime = 0.0;
    if (!deviceRunning.load()) fifo.reset();   // safe only when the audio thread is stopped

    repaint();
}

// Wipe all graphs when a file is (re)loaded or a new/blank project is created,
// so leftover data from the previous project isn't shown.
void MeteringPanel::fileLoaded()   { clearGraphs(); }
void MeteringPanel::engineCleared() { clearGraphs(); }

void MeteringPanel::applySettings()
{
    if (settings == nullptr) return;

    calibrationDb = settings->calibration->floatValue();
    thresholdDb = settings->threshold->floatValue();
    weightingType = (int)settings->weighting->getValueData();
    laeqWindowSec = (int)settings->laeqWindow->getValueData();
    displayMode = (int)settings->displayMode->getValueData();
    displayMode2 = (int)settings->displayMode2->getValueData();
    splitPos = settings->splitPosition->floatValue();
    verticalTime = settings->verticalOrientation->boolValue();
    updatePlayPauseIcon();
}

void MeteringPanel::parameterValueChanged(Parameter* p)
{
    if (settings == nullptr) return;

    if (p == settings->calibration) calibrationDb = settings->calibration->floatValue();
    else if (p == settings->threshold) thresholdDb = settings->threshold->floatValue();
    else if (p == settings->weighting) weightingType = (int)settings->weighting->getValueData();
    else if (p == settings->laeqWindow) laeqWindowSec = (int)settings->laeqWindow->getValueData();
    else if (p == settings->displayMode)
    {
        displayMode = (int)settings->displayMode->getValueData();
        resized();
    }
    else if (p == settings->displayMode2)
    {
        displayMode2 = (int)settings->displayMode2->getValueData();
        resized();
    }
    else if (p == settings->splitPosition)
    {
        splitPos = settings->splitPosition->floatValue();
        resized();
    }
    else if (p == settings->verticalOrientation) { verticalTime = settings->verticalOrientation->boolValue(); resized(); }
    else if (p == settings->active)
    {
        updatePlayPauseIcon();
        // On (re)start, wipe all graphs and reset the held Max/Peak so old data isn't shown.
        if (settings->active->boolValue())
        {
            clearGraphs();
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
            laeqPos = (laeqPos + 1) % laeqMaxSeconds;
            laeqCount = juce::jmin(laeqMaxSeconds, laeqCount + 1);
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

    // Average the most recent laeqWindowSec one-second slots (or fewer if not yet filled).
    int n = juce::jmin(laeqWindowSec, laeqCount);
    if (n > 0)
    {
        double sum = 0.0;
        for (int i = 0; i < n; ++i)
            sum += laeqRing[(size_t)((laeqPos - 1 - i + laeqMaxSeconds) % laeqMaxSeconds)];
        splLaeq = (float)(10.0 * std::log10(juce::jmax(1e-12, sum / n))) + calibrationDb;
    }
    else splLaeq = -120.0f;

    // Max holds the highest (weighted) Now reading; Peak is a true-peak (unweighted)
    // meter that catches transients then falls back slowly (~12 dB/s) so it tracks
    // recent peaks instead of latching forever on a one-off transient.
    if (deviceRunning.load())
    {
        splMax = juce::jmax(splMax, splNow);
        float peakSpl = 20.0f * std::log10(juce::jmax(1e-9f, peak)) + calibrationDb;
        if (peakSpl > splPeak) splPeak = peakSpl;
        else                   splPeak = juce::jmax(peakSpl, splPeak - 0.2f); // 0.2 dB/tick at 60 Hz
    }

    // Over-threshold alert: keep the red border for ~350 ms after dropping back below.
    if (deviceRunning.load() && splNow >= thresholdDb)
        alertHoldTicks = 21;            // ~0.35 s at 60 Hz
    else if (alertHoldTicks > 0)
        --alertHoldTicks;

    // dB SPL graph: store the peak Now level over each ~0.5 s slot, so the graph's
    // peaks line up with the held Max instead of smoothing transients away.
    if (deviceRunning.load())
    {
        splBucketMax = juce::jmax(splBucketMax, splNow);
        if (++splGraphTick >= 30)                 // ~0.5 s at 60 Hz
        {
            splHistory[(size_t)splHistoryPos] = splBucketMax;
            splHistoryPos = (splHistoryPos + 1) % splGraphSamples;
            splHistoryCount = juce::jmin(splGraphSamples, splHistoryCount + 1);
            splBucketMax = -120.0f;
            splGraphTick = 0;
        }
    }

    // While the input is running the graphs change every frame; once stopped only
    // a few held values decay (meter, RTA peak, alert border), so repaint only
    // until they settle and then leave the panel idle (no needless CPU).
    if (deviceRunning.load()) repaint();
    else
    {
        double sig = computeMeterSignature();
        if (sig != lastMeterSig) { lastMeterSig = sig; repaint(); }
    }
}

double MeteringPanel::computeMeterSignature() const
{
    double sig = (double)meterDb + (double)alertHoldTicks * 1000.0;
    for (int b = 0; b < numRtaBands; ++b) sig += rtaPeak[(size_t)b];
    if (settings != nullptr && settings->isSelected) sig += 12345.0;   // selection border
    return sig;
}

//==============================================================================
// Mouse

bool MeteringPanel::isOverSplitter(juce::Point<int> p) const
{
    return numCols == 2 && std::abs(p.x - splitterX) <= 4
        && p.y >= centralArea.getY() && p.y < centralArea.getBottom();
}

void MeteringPanel::mouseMove(const juce::MouseEvent& e)
{
    mousePos = e.getPosition();
    setMouseCursor(isOverSplitter(mousePos) ? juce::MouseCursor::LeftRightResizeCursor
                                            : juce::MouseCursor::NormalCursor);
    repaint();
}

void MeteringPanel::mouseExit(const juce::MouseEvent&)
{
    mousePos = { -1, -1 };
    repaint();
}

void MeteringPanel::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())   // right-click anywhere: display options
    {
        showContextMenu();
        return;
    }

    // Grab the splitter to resize the two columns.
    if (isOverSplitter(e.getPosition()))
    {
        draggingSplitter = true;
        return;
    }

    // Clicking either held readout resets both, via the settings trigger.
    if (settings != nullptr && (maxClickArea.contains(e.getPosition()) || peakClickArea.contains(e.getPosition())))
    {
        settings->resetMaxPeak->trigger();
        return;
    }

    // Clicking anywhere else selects the Metering settings so its inspector opens
    // (and the panel shows a selection border).
    if (settings != nullptr)
    {
        settings->selectThis();
        repaint();
    }
}

void MeteringPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (!draggingSplitter || centralArea.getWidth() <= 0) return;

    float f = (float)(e.getPosition().x - centralArea.getX()) / (float)centralArea.getWidth();
    splitPos = juce::jlimit(0.15f, 0.85f, f);
    resized();   // relayout live; the parameter (saved value) is committed on mouseUp
    repaint();
}

void MeteringPanel::mouseUp(const juce::MouseEvent&)
{
    if (draggingSplitter)
    {
        draggingSplitter = false;
        if (settings != nullptr) settings->splitPosition->setValue(splitPos); // persist once
        repaint();
    }
}

void MeteringPanel::updatePlayPauseIcon()
{
    bool running = settings != nullptr && settings->active->boolValue();
    juce::Path p;
    if (running)   // pause: two bars
    {
        p.addRectangle(0.0f, 0.0f, 0.34f, 1.0f);
        p.addRectangle(0.62f, 0.0f, 0.34f, 1.0f);
    }
    else           // play: right-pointing triangle
    {
        p.addTriangle(0.0f, 0.0f, 0.0f, 1.0f, 0.95f, 0.5f);
    }

    juce::DrawablePath icon;
    icon.setPath(p);
    icon.setFill(juce::Colours::white);
    startStopBtn.setImages(&icon);
}

void MeteringPanel::showContextMenu()
{
    if (settings == nullptr) return;

    // Each submenu mirrors the matching EnumParameter (kept in Project Settings);
    // selecting an item just drives that parameter, so both stay in sync.
    auto addEnumItems = [](juce::PopupMenu& sub, EnumParameter* p, int base)
    {
        juce::StringArray keys = p->getAllKeys();
        juce::String cur = p->getValueKey();
        for (int i = 0; i < keys.size(); ++i)
            sub.addItem(base + i, keys[i], true, keys[i] == cur);
    };

    juce::PopupMenu left, right, weight, laeq;
    addEnumItems(left, settings->displayMode, 100);
    addEnumItems(right, settings->displayMode2, 150);
    addEnumItems(weight, settings->weighting, 200);
    addEnumItems(laeq, settings->laeqWindow, 300);

    juce::PopupMenu ref;
    ref.addItem(400, "Capture current curve");
    ref.addItem(401, "Clear", rtaHasReference);

    juce::PopupMenu m;
    m.addSubMenu("Left column", left);
    m.addSubMenu("Right column", right);
    m.addSubMenu("Weighting", weight);
    m.addSubMenu("LAeq window", laeq);
    m.addSubMenu("RTA reference", ref);

    m.showMenuAsync(juce::PopupMenu::Options().withMousePosition(),
        [this](int result)
        {
            if (result == 0 || settings == nullptr) return;
            if (result == 400) { captureRtaReference(); return; }
            if (result == 401) { clearRtaReference();   return; }

            EnumParameter* p = nullptr; int base = 0;
            if      (result < 150) { p = settings->displayMode;  base = 100; }
            else if (result < 200) { p = settings->displayMode2; base = 150; }
            else if (result < 300) { p = settings->weighting;    base = 200; }
            else                   { p = settings->laeqWindow;   base = 300; }

            juce::StringArray keys = p->getAllKeys();
            int idx = result - base;
            if (idx >= 0 && idx < keys.size()) p->setValueWithKey(keys[idx]);
        });
}

void MeteringPanel::captureRtaReference()
{
    rtaReference = rtaSmooth;
    rtaHasReference = true;
    repaint();
}

void MeteringPanel::clearRtaReference()
{
    rtaHasReference = false;
    repaint();
}

//==============================================================================
// Layout

void MeteringPanel::resized()
{
    auto r = getLocalBounds();

    // dB SPL readouts (Now / LAeq) on the left.
    readoutArea = r.removeFromLeft(82);
    r.removeFromLeft(2);

    // dBFS meter on the right, leaving room at the bottom for the Start/Stop button.
    meterArea = r.removeFromRight(58).withTrimmedBottom(26);
    r.removeFromRight(2);

    // Start/Stop (play/pause) icon button, bottom-right, centred under the dBFS meter.
    startStopBtn.setBounds(getWidth() - 41, getHeight() - 24, 24, 20);

    // Central plot region, split into one or two columns. All scales are drawn inside
    // the plots, so each column uses its whole area with no reserved axis gutter.
    centralArea = r;
    colType[0] = displayMode;
    colType[1] = displayMode2;

    if (colType[1] < 0)
    {
        numCols = 1;
        colArea[0] = centralArea;
        colArea[1] = juce::Rectangle<int>();
        splitterX = centralArea.getRight();
    }
    else
    {
        numCols = 2;
        const int sx = centralArea.getX() + juce::roundToInt(centralArea.getWidth() * juce::jlimit(0.15f, 0.85f, splitPos));
        splitterX = sx;
        const int half = 3; // splitter half-width
        colArea[0] = centralArea.withRight(sx - half);
        colArea[1] = centralArea.withLeft(sx + half);
    }

    // The spectrogram image is sized to whichever column displays it (left preferred).
    spectroImageArea = juce::Rectangle<int>();
    for (int i = 0; i < numCols; ++i)
        if (colType[i] == 0) { spectroImageArea = colArea[i]; break; }

    if (!spectroImageArea.isEmpty())
    {
        int w = verticalTime ? spectroImageArea.getHeight() : spectroImageArea.getWidth();
        int h = verticalTime ? spectroImageArea.getWidth()  : spectroImageArea.getHeight();
        if (w > 0 && h > 0 && (spectro.getWidth() != w || spectro.getHeight() != h))
        {
            // Rescale the existing spectrogram into the new size instead of starting
            // from a blank image, so resizing keeps the current content. The buffer
            // always stores time on X and frequency on Y, so a plain rescale stays
            // correct in both orientations.
            juce::Image resized(juce::Image::RGB, w, h, true);
            if (!spectro.isNull())
            {
                juce::Graphics ig(resized);
                ig.drawImage(spectro, 0, 0, w, h, 0, 0, spectro.getWidth(), spectro.getHeight());
            }
            spectro = resized;
        }
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
    // Give the Now box (which also carries Max + Peak) more room than the LAeq box.
    auto nowR = area.removeFromTop(area.getHeight() * 5 / 8);
    auto avgR = area;

    // One small held readout line (Max / Peak), clickable to reset.
    auto drawSub = [&](juce::Rectangle<int> r, const String& label, float val)
    {
        bool over = running && val >= thresholdDb;
        String sv = (!running || val <= -119.0f) ? String("--.-") : String(val, 1);
        g.setColour(over ? Colours::red : Colours::white);
        g.setFont(Font(12.0f, Font::plain));
        g.drawText(label + " " + sv, r, Justification::centred);
    };

    auto drawBig = [&](juce::Rectangle<int> a, const String& title, float value,
                       const String& sub1, float sub1v, juce::Rectangle<int>* click1,
                       const String& sub2, float sub2v, juce::Rectangle<int>* click2)
    {
        a = a.withTrimmedLeft(6).withTrimmedRight(6).withTrimmedTop(4).withTrimmedBottom(4);

        // Title (top, centred).
        g.setColour(Colours::white.withAlpha(.55f));
        g.setFont(11.0f);
        g.drawText(title, a.removeFromTop(14), Justification::centred);

        // Held values stacked at the bottom (sub2 below sub1).
        if (sub2.isNotEmpty()) { auto r = a.removeFromBottom(16); if (click2) *click2 = r; drawSub(r, sub2, sub2v); }
        if (sub1.isNotEmpty()) { auto r = a.removeFromBottom(16); if (click1) *click1 = r; drawSub(r, sub1, sub1v); }

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

    // Now box carries the live readout plus the held Max and Peak; the LAeq box stays clean.
    drawBig(nowR, "Now", splNow, "max", splMax, &maxClickArea, "peak", splPeak, &peakClickArea);
    drawBig(avgR, "LAeq " + String(laeqWindowSec / 60) + " min", splLaeq, "", 0.0f, nullptr, "", 0.0f, nullptr);

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

    auto bar = area.reduced(12, 0).withTrimmedTop(24).withTrimmedBottom(16).withTrimmedRight(16);
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

void MeteringPanel::drawFreqAxis(juce::Graphics& g, juce::Rectangle<int> area, double sr, bool freqOnX)
{
    static const int ticks[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    double fMax = juce::jmax(40.0, sr * 0.5);

    g.setFont(9.0f);
    for (int f : ticks)
    {
        if (f < 20 || f > fMax) continue;
        double frac = fracFromFreq((double)f, sr);
        String label = (f >= 1000) ? String(f / 1000) + "k" : String(f);

        if (freqOnX)
        {
            int x = area.getX() + (int)(frac * area.getWidth());
            g.setColour(Colours::white.withAlpha(.08f));
            g.drawVerticalLine(x, (float)area.getY(), (float)area.getBottom());
            g.setColour(Colours::white.withAlpha(.45f));
            int lx = juce::jlimit(area.getX(), area.getRight() - 32, x - 16); // keep edge labels in view
            g.drawText(label, lx, area.getBottom() - 12, 32, 11, Justification::centred);
        }
        else
        {
            int y = area.getY() + (int)(frac * area.getHeight());
            g.setColour(Colours::white.withAlpha(.08f));
            g.drawHorizontalLine(y, (float)area.getX(), (float)area.getRight());
            g.setColour(Colours::white.withAlpha(.45f));
            int ly = juce::jlimit(area.getY(), area.getBottom() - 12, y - 6); // keep edge labels in view
            g.drawText(label, area.getX() + 3, ly, 32, 12, Justification::centredLeft);
        }
    }
}

void MeteringPanel::drawRTA(juce::Graphics& g, juce::Rectangle<int> area, double sr)
{
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
        g.setColour(Colours::white.withAlpha(.45f));
        int ly = juce::jlimit(area.getY(), area.getBottom() - 12, y - 6); // keep edge labels in view
        g.drawText(String((int)spl), area.getX() + 3, ly, 30, 12, Justification::centredLeft);
    }
    g.setColour(Colours::white.withAlpha(.45f));
    g.drawText("dB SPL", area.getRight() - 52, area.getY() + 1, 50, 11, Justification::topRight);

    // Smoothed curve + peak-hold curve (+ optional reference) across the in-range bands.
    juce::Path curve, peakPath, refPath;
    bool started = false;
    float firstX = left, lastX = left;
    for (int b = 0; b < numRtaBands; ++b)
    {
        if (rtaBandLo[(size_t)b] > rtaBandHi[(size_t)b]) continue; // beyond Nyquist
        float x = left + (float)fracFromFreq(rtaCenters[(size_t)b], sr) * (float)area.getWidth();
        float yS = yFor(rtaSmooth[(size_t)b]);
        float yP = yFor(rtaPeak[(size_t)b]);
        float yR = rtaHasReference ? yFor(rtaReference[(size_t)b]) : 0.0f;

        if (!started)
        {
            curve.startNewSubPath(x, yS);
            peakPath.startNewSubPath(x, yP);
            if (rtaHasReference) refPath.startNewSubPath(x, yR);
            started = true; firstX = x;
        }
        else
        {
            curve.lineTo(x, yS);
            peakPath.lineTo(x, yP);
            if (rtaHasReference) refPath.lineTo(x, yR);
        }
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

    // Captured reference curve overlaid for comparison.
    if (rtaHasReference)
    {
        g.setColour(Colours::orange.withAlpha(.9f));
        g.strokePath(refPath, juce::PathStrokeType(1.2f));
        g.drawText("ref", area.getRight() - 86, area.getY() + 1, 28, 11, Justification::topRight);
    }
}

void MeteringPanel::drawSplGraph(juce::Graphics& g, juce::Rectangle<int> area)
{
    if (area.getWidth() <= 0 || area.getHeight() <= 0) return;

    const float left = (float)area.getX();
    const float right = (float)area.getRight();
    const float bottom = (float)area.getBottom();
    const float height = (float)area.getHeight();

    const float splMin = 30.0f, splMax = 120.0f;
    auto yForSpl = [&](float spl)
    {
        float frac = juce::jlimit(0.0f, 1.0f, (spl - splMin) / (splMax - splMin));
        return bottom - frac * height;
    };

    // Level grid + dB SPL labels (every 15 dB).
    g.setFont(9.0f);
    for (float spl = splMax; spl >= splMin - 0.1f; spl -= 15.0f)
    {
        int y = (int)yForSpl(spl);
        g.setColour(Colours::white.withAlpha(.08f));
        g.drawHorizontalLine(y, left, right);
        g.setColour(Colours::white.withAlpha(.45f));
        int ly = juce::jlimit(area.getY(), area.getBottom() - 12, y - 6); // keep edge labels in view
        g.drawText(String((int)spl), area.getX() + 3, ly, 30, 12, Justification::centredLeft);
    }
    g.setColour(Colours::white.withAlpha(.45f));
    g.drawText("dB SPL  (last 5 min)", area.getRight() - 120, area.getY() + 1, 118, 11, Justification::topRight);

    // Threshold reference line.
    {
        int yT = (int)yForSpl(thresholdDb);
        if (yT > area.getY() && yT < area.getBottom())
        {
            g.setColour(Colours::red.withAlpha(.35f));
            g.drawHorizontalLine(yT, left, right);
        }
    }

    // SPL history line: newest at the left, scrolling right as time passes - matching
    // the horizontal spectrogram's direction for consistency.
    if (splHistoryCount > 1)
    {
        float step = (float)area.getWidth() / (float)(splGraphSamples - 1);
        juce::Path p;
        for (int k = 0; k < splHistoryCount; ++k)   // k = age, 0 = newest
        {
            int idx = (splHistoryPos - 1 - k + splGraphSamples) % splGraphSamples;
            float x = left + (float)k * step;
            float y = yForSpl(splHistory[(size_t)idx]);
            if (k == 0) p.startNewSubPath(x, y);
            else        p.lineTo(x, y);
        }
        g.setColour(Colours::limegreen);
        g.strokePath(p, juce::PathStrokeType(1.5f));
    }
}

int MeteringPanel::columnAt(juce::Point<int> p) const
{
    for (int i = 0; i < numCols; ++i)
        if (colArea[i].contains(p)) return i;
    return -1;
}

void MeteringPanel::drawColumn(juce::Graphics& g, int idx, double sr)
{
    juce::Rectangle<int> area = colArea[idx];
    if (area.isEmpty()) return;
    int type = colType[idx];

    // Black background so the not-yet-filled part matches the active graph.
    g.setColour(Colours::black);
    g.fillRect(area);

    if (type == 2)
    {
        drawSplGraph(g, area);
    }
    else if (type == 1)
    {
        drawRTA(g, area, sr);
    }
    else // spectrogram
    {
        // The buffer is sized to the first spectrogram column; scale it into this column
        // so a second spectrogram column shows the same data (rescaled) instead of being
        // empty. For the primary column the scale factors are 1.
        if (!spectro.isNull())
        {
            int imgW = juce::jmax(1, spectro.getWidth());
            int imgH = juce::jmax(1, spectro.getHeight());
            if (verticalTime)
            {
                // Image: X = time, Y = frequency; displayed rotated (time vertical).
                float sw = (float)area.getWidth()  / (float)imgH; // frequency -> horizontal
                float sh = (float)area.getHeight() / (float)imgW; // time -> vertical
                juce::AffineTransform t(0.0f, -sw, (float)area.getRight(), sh, 0.0f, (float)area.getY());
                g.drawImageTransformed(spectro, t);
            }
            else
            {
                // Newest data on the LEFT (mirror X), low frequencies at the TOP (mirror Y).
                float sx = (float)area.getWidth()  / (float)imgW;
                float sy = (float)area.getHeight() / (float)imgH;
                juce::AffineTransform t(-sx, 0.0f, (float)area.getRight(), 0.0f, -sy, (float)area.getBottom());
                g.drawImageTransformed(spectro, t);
            }
        }
        drawFreqAxis(g, area, sr, verticalTime);
    }
}

void MeteringPanel::drawSplitter(juce::Graphics& g)
{
    g.setColour(BG_COLOR.brighter(draggingSplitter ? .55f : .28f));
    g.fillRect(splitterX - 1, centralArea.getY(), 2, centralArea.getHeight());

    g.setColour(Colours::white.withAlpha(draggingSplitter ? .65f : .35f));
    int cy = centralArea.getCentreY();
    for (int k = -1; k <= 1; ++k)
        g.fillRect(splitterX - 1, cy + k * 6 - 1, 2, 2);
}

void MeteringPanel::drawHover(juce::Graphics& g, double sr)
{
    int idx = columnAt(mousePos);
    if (idx < 0) return;

    juce::Rectangle<int> area = colArea[idx];
    int type = colType[idx];

    auto drawLabelBox = [&](const String& label)
    {
        g.setFont(Font(12.0f, Font::bold));
        int tw = g.getCurrentFont().getStringWidth(label) + 12;
        juce::Rectangle<int> box(juce::jlimit(area.getX(), area.getRight() - tw, mousePos.x + 8),
                                 juce::jlimit(area.getY(), area.getBottom() - 18, mousePos.y - 18),
                                 tw, 16);
        g.setColour(Colours::black.withAlpha(.7f));
        g.fillRoundedRectangle(box.toFloat(), 3.0f);
        g.setColour(Colours::white);
        g.drawText(label, box, Justification::centred);
    };

    if (type == 2)   // dB SPL graph: read the level at the cursor's vertical position.
    {
        double frac = (double)(area.getBottom() - mousePos.y) / (double)juce::jmax(1, area.getHeight());
        double spl = 30.0 + juce::jlimit(0.0, 1.0, frac) * (120.0 - 30.0);
        g.setColour(Colours::white.withAlpha(.5f));
        g.drawHorizontalLine(mousePos.y, (float)area.getX(), (float)area.getRight());
        drawLabelBox(String(spl, 1) + " dB SPL");
        return;
    }

    // Spectrogram / RTA: read the frequency at the cursor.
    double freq;
    if (freqOnXFor(type))
    {
        double frac = (double)(mousePos.x - area.getX()) / (double)juce::jmax(1, area.getWidth());
        freq = freqFromFrac(juce::jlimit(0.0, 1.0, frac), sr);
        g.setColour(Colours::white.withAlpha(.5f));
        g.drawVerticalLine(mousePos.x, (float)area.getY(), (float)area.getBottom());
    }
    else
    {
        double frac = (double)(mousePos.y - area.getY()) / (double)juce::jmax(1, area.getHeight());
        freq = freqFromFrac(juce::jlimit(0.0, 1.0, frac), sr);
        g.setColour(Colours::white.withAlpha(.5f));
        g.drawHorizontalLine(mousePos.y, (float)area.getX(), (float)area.getRight());
    }

    String label = (freq >= 1000.0) ? String(freq / 1000.0, 2) + " kHz" : String((int)(freq + 0.5)) + " Hz";
    if (type == 1 && numRtaBands > 0)   // RTA: also show the level of the band under the cursor
    {
        int band = juce::jlimit(0, numRtaBands - 1,
                                (int)std::lround(std::log2(freq / rtaFMin) * rtaBandsPerOctave));
        float spl = 10.0f * std::log10(juce::jmax(rtaSmooth[(size_t)band], 1e-12f)) + calibrationDb;
        label += "   " + String(spl, 1) + " dB SPL";
    }
    drawLabelBox(label);
}

void MeteringPanel::paint(juce::Graphics& g)
{
    g.fillAll(BG_COLOR);

    drawReadout(g);
    drawMeter(g, meterArea);

    double sr = sampleRate.load();

    // Draw each column (dark background + its graph), then the splitter handle.
    for (int i = 0; i < numCols; ++i)
        drawColumn(g, i, sr);

    if (numCols == 2)
        drawSplitter(g);

    // Selection border (drawn before the early returns below so it shows even when
    // the input is stopped), shown when the Metering settings are selected.
    if (settings != nullptr && settings->isSelected)
    {
        g.setColour(LIGHTCONTOUR_COLOR);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.5f), 2.0f, 1.5f);
    }

    bool active = (settings != nullptr && settings->active->boolValue());
    if (!active)
        return;
    if (!deviceRunning.load())
    {
        g.setColour(Colours::orangered.withAlpha(.85f));
        g.setFont(14.0f);
        g.drawText("No audio input (check microphone / permissions)", centralArea, Justification::centred);
        return;
    }

    drawHover(g, sr);

    // Strong over-threshold alert: a thick red border around the whole panel,
    // held briefly (see alertHoldTicks) so it doesn't flicker.
    if (alertHoldTicks > 0)
    {
        g.setColour(Colours::red);
        g.drawRect(getLocalBounds(), 4);
    }
}
