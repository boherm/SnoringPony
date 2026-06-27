/*
  ==============================================================================

    RtlSdrSpectrumSource.cpp
    Created: 26 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "RtlSdrSpectrumSource.h"

#if SP_HAS_RTLSDR
 #include <rtl-sdr.h>
#endif

RtlSdrSpectrumSource::RtlSdrSpectrumSource() :
    juce::Thread("RTL-SDR Sweep")
{
}

RtlSdrSpectrumSource::~RtlSdrSpectrumSource()
{
    stop();
}

bool RtlSdrSpectrumSource::isSupported()
{
#if SP_HAS_RTLSDR
    return true;
#else
    return false;
#endif
}

juce::StringArray RtlSdrSpectrumSource::listDevices()
{
    juce::StringArray names;
#if SP_HAS_RTLSDR
    const uint32_t n = rtlsdr_get_device_count();
    for (uint32_t i = 0; i < n; ++i)
        names.add(juce::String(i) + ": " + juce::String(rtlsdr_get_device_name(i)));
#endif
    return names;
}

void RtlSdrSpectrumSource::configure(int deviceIndex_, bool autoGain_, float gainDb_, int ppm_)
{
    deviceIndex = deviceIndex_;
    autoGain = autoGain_;
    gainDb = gainDb_;
    ppm = ppm_;
}

void RtlSdrSpectrumSource::setStatus(const juce::String& s)
{
    const juce::ScopedLock sl(statusLock);
    status = s;
}

juce::String RtlSdrSpectrumSource::getStatusMessage() const
{
    const juce::ScopedLock sl(statusLock);
    return status;
}

void RtlSdrSpectrumSource::start(double minMHz, double maxMHz, double binHz)
{
    stop();

    bandMinMHz = juce::jmin(minMHz, maxMHz);
    bandMaxMHz = juce::jmax(minMHz, maxMHz);
    if (bandMaxMHz - bandMinMHz < 0.1) bandMaxMHz = bandMinMHz + 0.1;
    targetBinHz = juce::jmax(1000.0, binHz);

    {
        const juce::ScopedLock sl(lock);
        hasData = false;
    }

    running.store(true);
    startThread();
}

void RtlSdrSpectrumSource::stop()
{
    running.store(false);
    stopThread(1500);
}

bool RtlSdrSpectrumSource::getLatestSweep(RFSpectrumSweep& out)
{
    const juce::ScopedLock sl(lock);
    if (!hasData) return false;
    out = latest;
    return true;
}

#if SP_HAS_RTLSDR

void RtlSdrSpectrumSource::run()
{
    rtlsdr_dev_t* d = nullptr;
    if (rtlsdr_open(&d, (uint32_t)deviceIndex) < 0 || d == nullptr)
    {
        setStatus("Cannot open RTL-SDR #" + juce::String(deviceIndex));
        running.store(false);
        return;
    }
    dev = d;

    const double SR = 2400000.0;                 // 2.4 MS/s
    rtlsdr_set_sample_rate(d, (uint32_t)SR);
    rtlsdr_set_freq_correction(d, ppm);          // returns -2 if unchanged; ignored
    if (autoGain)
        rtlsdr_set_tuner_gain_mode(d, 0);        // 0 = automatic
    else
    {
        rtlsdr_set_tuner_gain_mode(d, 1);
        rtlsdr_set_tuner_gain(d, (int)(gainDb * 10.0f));  // tenths of dB
    }
    rtlsdr_reset_buffer(d);

    // FFT size from the requested bin width (bin width = SR / fftSize).
    int fftSize = juce::nextPowerOfTwo(juce::jlimit(512, 16384, (int)std::round(SR / targetBinHz)));
    int fftOrder = (int)std::round(std::log2((double)fftSize));
    fftSize = 1 << fftOrder;

    juce::dsp::FFT fft(fftOrder);
    std::vector<float> window((size_t)fftSize);
    double winPow = 0.0;
    for (int i = 0; i < fftSize; ++i)
    {
        window[(size_t)i] = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)i / (float)(fftSize - 1)));
        winPow += (double)window[(size_t)i] * window[(size_t)i];
    }

    std::vector<uint8_t> raw((size_t)fftSize * 2);
    std::vector<juce::dsp::Complex<float>> fin((size_t)fftSize), fout((size_t)fftSize);

    const double usable = 0.75;                  // keep the central 75% of each segment
    const double stepHz = SR * usable;
    const double binMHz = targetBinHz / 1.0e6;
    const int numBins = juce::jmax(1, (int)std::round((bandMaxMHz - bandMinMHz) / binMHz));

    // Rough calibration so the noise floor lands in the display's -105..-20 range.
    const double dbOffset = -30.0;

    setStatus("Scanning " + juce::String(bandMinMHz, 1) + " - " + juce::String(bandMaxMHz, 1) + " MHz");

    while (!threadShouldExit() && running.load())
    {
        RFSpectrumSweep sweep;
        sweep.minMHz = bandMinMHz;
        sweep.maxMHz = bandMaxMHz;
        sweep.binHz  = targetBinHz;
        sweep.powerDb.assign((size_t)numBins, -200.0f);

        const double startHz = bandMinMHz * 1.0e6 + stepHz * 0.5;
        const double endHz   = bandMaxMHz * 1.0e6;

        for (double centerHz = startHz; centerHz < endHz + stepHz; centerHz += stepHz)
        {
            if (threadShouldExit() || !running.load()) break;

            rtlsdr_set_center_freq(d, (uint32_t)centerHz);

            // Discard one buffer so the PLL/AGC settle, then read the real one.
            int n = 0;
            rtlsdr_read_sync(d, raw.data(), (int)raw.size(), &n);
            if (rtlsdr_read_sync(d, raw.data(), (int)raw.size(), &n) < 0 || n <= 0)
                continue;

            const int samples = juce::jmin(fftSize, n / 2);
            for (int i = 0; i < samples; ++i)
            {
                float I = ((float)raw[(size_t)(2 * i)]     - 127.5f) / 127.5f;
                float Q = ((float)raw[(size_t)(2 * i + 1)] - 127.5f) / 127.5f;
                fin[(size_t)i] = { I * window[(size_t)i], Q * window[(size_t)i] };
            }
            for (int i = samples; i < fftSize; ++i) fin[(size_t)i] = { 0.0f, 0.0f };

            fft.perform(fin.data(), fout.data(), false);

            // Complex FFT: bin k < N/2 -> positive offset, k >= N/2 -> negative.
            const double norm = (double)fftSize * winPow;
            for (int k = 0; k < fftSize; ++k)
            {
                int kk = (k < fftSize / 2) ? k : k - fftSize;       // signed bin
                double offHz = (double)kk * SR / (double)fftSize;
                if (std::abs(offHz) > usable * SR * 0.5) continue;  // drop the segment edges

                double freqMHz = (centerHz + offHz) / 1.0e6;
                int bin = (int)((freqMHz - bandMinMHz) / binMHz);
                if (bin < 0 || bin >= numBins) continue;

                double re = fout[(size_t)k].real(), im = fout[(size_t)k].imag();
                double p = (re * re + im * im) / norm;
                float db = (float)(10.0 * std::log10(juce::jmax(1.0e-12, p)) + dbOffset);

                if (db > sweep.powerDb[(size_t)bin]) sweep.powerDb[(size_t)bin] = db;
            }
        }

        {
            const juce::ScopedLock sl(lock);
            latest = sweep;
            hasData = true;
        }
    }

    rtlsdr_close(d);
    dev = nullptr;
}

#else // ---- no hardware support compiled in --------------------------------

void RtlSdrSpectrumSource::run()
{
    setStatus("RTL-SDR support is not compiled in (librtlsdr not vendored).");
    running.store(false);
}

#endif
