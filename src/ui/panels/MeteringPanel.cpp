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

    if (settings != nullptr)
    {
        settings->calibration->addParameterListener(this);
        settings->weighting->addParameterListener(this);
        settings->threshold->addParameterListener(this);
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
        settings->verticalOrientation->removeParameterListener(this);
        settings->active->removeParameterListener(this);
    }
}

//==============================================================================

void MeteringPanel::clearSpectrogram()
{
    if (!spectro.isNull())
        spectro.clear(spectro.getBounds(), Colours::black);
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
    verticalTime = settings->verticalOrientation->boolValue();
    startStopBtn.setButtonText(settings->active->boolValue() ? "Stop" : "Start");
}

void MeteringPanel::parameterValueChanged(Parameter* p)
{
    if (settings == nullptr) return;

    if (p == settings->calibration) calibrationDb = settings->calibration->floatValue();
    else if (p == settings->threshold) thresholdDb = settings->threshold->floatValue();
    else if (p == settings->weighting) weightingType = (int)settings->weighting->getValueData();
    else if (p == settings->verticalOrientation) { verticalTime = settings->verticalOrientation->boolValue(); resized(); }
    else if (p == settings->active)
    {
        startStopBtn.setButtonText(settings->active->boolValue() ? "Stop" : "Start");
        // On (re)start, wipe the previously generated spectrogram so old data isn't shown.
        if (settings->active->boolValue())
            clearSpectrogram();
    }

    repaint();
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

    const bool haveImage = !spectro.isNull() && spectroImageArea.getWidth() > 0;
    const int W = haveImage ? spectro.getWidth() : 1;
    const int H = haveImage ? spectro.getHeight() : 1;
    const double columnDuration = 60.0 / juce::jmax(1, W);
    const double hopTime = (double)fftSize / sr;
    const double msScale = 1.0 / ((double)fftSize * windowPower);
    const double fastAlpha = 1.0 - std::exp(-hopTime / 0.125);

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
    return weightingType == 0 ? "dB(A)" : (weightingType == 1 ? "dB(B)" : "dB(C)");
}

void MeteringPanel::timerCallback()
{
    pullAndProcess();

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

//==============================================================================
// Layout

void MeteringPanel::resized()
{
    auto r = getLocalBounds();

    // Start/Stop button, top-right corner overlay.
    startStopBtn.setBounds(getWidth() - 64, 4, 58, 22);

    auto leftCol = r.removeFromLeft(178);
    meterArea = leftCol.removeFromRight(58);
    readoutArea = leftCol;
    r.removeFromLeft(2);

    if (verticalTime)
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
        spectro = juce::Image(juce::Image::RGB, w, h, true);
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

    auto drawBig = [&](juce::Rectangle<int> a, const String& title, float value)
    {
        a = a.withTrimmedLeft(8).withTrimmedRight(4).withTrimmedTop(4).withTrimmedBottom(4);
        g.setColour(Colours::white.withAlpha(.55f));
        g.setFont(11.0f);
        g.drawText(title, a.removeFromTop(15), Justification::topLeft);

        bool over = running && value >= thresholdDb;
        float fontSize = (float)juce::jlimit(15, 30, a.getHeight() - 2);
        g.setColour(over ? Colours::red : Colours::white);
        g.setFont(Font(fontSize, Font::bold));
        String v = (!running || value <= -119.0f) ? String("--.-") : String(value, 1);
        g.drawText(v, a, Justification::centredRight);

        g.setColour(Colours::white.withAlpha(.5f));
        g.setFont(10.0f);
        g.drawText(w, a, Justification::bottomLeft);
    };

    drawBig(nowR, "Now", splNow);
    drawBig(avgR, "LAeq 15 min", splLaeq);

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

        if (verticalTime)
        {
            int x = spectroImageArea.getX() + (int)(frac * spectroImageArea.getWidth());
            g.setColour(Colours::white.withAlpha(.08f));
            g.drawVerticalLine(x, (float)spectroImageArea.getY(), (float)spectroImageArea.getBottom());
            g.setColour(Colours::white.withAlpha(.45f));
            g.drawText(label, x - 16, freqAxisArea.getY(), 32, freqAxisArea.getHeight(), Justification::centred);
        }
        else
        {
            int y = spectroImageArea.getBottom() - (int)(frac * spectroImageArea.getHeight());
            g.setColour(Colours::white.withAlpha(.08f));
            g.drawHorizontalLine(y, (float)spectroImageArea.getX(), (float)spectroImageArea.getRight());
            g.setColour(Colours::white.withAlpha(.45f));
            g.drawText(label, freqAxisArea.getX(), y - 6, freqAxisArea.getWidth() - 2, 12, Justification::centredRight);
        }
    }
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

    if (!spectro.isNull())
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
            g.drawImageAt(spectro, spectroImageArea.getX(), spectroImageArea.getY());
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
        if (verticalTime)
        {
            double frac = (double)(mousePos.x - spectroImageArea.getX()) / (double)juce::jmax(1, spectroImageArea.getWidth());
            freq = freqFromFrac(juce::jlimit(0.0, 1.0, frac), sr);
            g.setColour(Colours::white.withAlpha(.5f));
            g.drawVerticalLine(mousePos.x, (float)spectroImageArea.getY(), (float)spectroImageArea.getBottom());
        }
        else
        {
            double frac = 1.0 - (double)(mousePos.y - spectroImageArea.getY()) / (double)juce::jmax(1, spectroImageArea.getHeight());
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
}
