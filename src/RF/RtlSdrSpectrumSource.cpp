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

void RtlSdrSpectrumSource::configure(int deviceIndex_, float gainDb_, int ppm_)
{
    deviceIndex = deviceIndex_;
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

    setRange(minMHz, maxMHz, binHz);

    {
        const juce::ScopedLock sl(lock);
        hasData = false;
    }

    running.store(true);
    startThread();
}

void RtlSdrSpectrumSource::setRange(double minMHz, double maxMHz, double binHz)
{
    double lo = juce::jmin(minMHz, maxMHz);
    double hi = juce::jmax(minMHz, maxMHz);
    if (hi - lo < 0.1) hi = lo + 0.1;
    reqMinMHz.store(lo);
    reqMaxMHz.store(hi);
    reqBinHz.store(juce::jmax(200.0, binHz));   // 200 Hz floor (fftSize cap)
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
    const double SR = 2400000.0;                 // 2.4 MS/s

    auto configure = [&](rtlsdr_dev_t* dv)
    {
        rtlsdr_set_sample_rate(dv, (uint32_t)SR);
        rtlsdr_set_freq_correction(dv, ppm);         // returns -2 if unchanged; ignored
        rtlsdr_set_tuner_gain_mode(dv, 1);           // 1 = manual
        rtlsdr_set_tuner_gain(dv, (int)(gainDb * 10.0f));  // tenths of dB
        rtlsdr_reset_buffer(dv);
    };

    rtlsdr_dev_t* d = nullptr;
    if (rtlsdr_open(&d, (uint32_t)deviceIndex) < 0 || d == nullptr)
    {
        setStatus("Cannot open RTL-SDR #" + juce::String(deviceIndex));
        running.store(false);
        return;
    }
    dev = d;
    configure(d);
    int errStreak = 0;

    const double usable = 0.75;                  // keep the central 75% of each segment
    const double stepHz = SR * usable;
    const double dbOffset = -30.0;               // rough calibration toward the display scale

    // FFT machinery; rebuilt only when the requested bin width changes.
    double curBin = -1.0;
    int fftSize = 0, fftOrder = 0;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window;
    double winPow = 1.0;
    std::vector<uint8_t> raw;
    std::vector<juce::dsp::Complex<float>> fin, fout;

    while (!threadShouldExit() && running.load())
    {
        // Re-read the requested window every sweep so the dongle follows the view
        // live (no reopen). The window is small (e.g. 4 MHz), so each sweep is fast.
        const double bmin = reqMinMHz.load();
        const double bmax = reqMaxMHz.load();
        const double bin  = reqBinHz.load();

        if (bin != curBin)
        {
            fftSize = juce::nextPowerOfTwo(juce::jlimit(512, 16384, (int)std::round(SR / bin)));
            fftOrder = (int)std::round(std::log2((double)fftSize));
            fftSize = 1 << fftOrder;
            fft.reset(new juce::dsp::FFT(fftOrder));
            window.assign((size_t)fftSize, 0.0f);
            winPow = 0.0;
            for (int i = 0; i < fftSize; ++i)
            {
                window[(size_t)i] = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)i / (float)(fftSize - 1)));
                winPow += (double)window[(size_t)i] * window[(size_t)i];
            }
            raw.assign((size_t)fftSize * 2, 0);
            fin.assign((size_t)fftSize, { 0.0f, 0.0f });
            fout.assign((size_t)fftSize, { 0.0f, 0.0f });
            curBin = bin;
        }

        const double binMHz = bin / 1.0e6;
        const int numBins = juce::jmax(1, (int)std::round((bmax - bmin) / binMHz));

        RFSpectrumSweep sweep;
        sweep.minMHz = bmin; sweep.maxMHz = bmax; sweep.binHz = bin;
        sweep.powerDb.assign((size_t)numBins, -200.0f);

        setStatus("Scanning " + juce::String(bmin, 2) + " - " + juce::String(bmax, 2)
                  + " MHz @ " + juce::String(bin / 1000.0, 1) + " kHz");

        const double startHz = bmin * 1.0e6 + stepHz * 0.5;
        const double endHz   = bmax * 1.0e6;

        for (double centerHz = startHz; centerHz < endHz + stepHz; centerHz += stepHz)
        {
            if (threadShouldExit() || !running.load()) break;
            // Abort the sweep early if the window changed, so retuning feels instant.
            if (reqMinMHz.load() != bmin || reqMaxMHz.load() != bmax || reqBinHz.load() != bin) break;

            int rc = rtlsdr_set_center_freq(d, (uint32_t)centerHz);

            int n = 0;
            rtlsdr_read_sync(d, raw.data(), (int)raw.size(), &n);   // discard (settle)
            int rr = rtlsdr_read_sync(d, raw.data(), (int)raw.size(), &n);

            if (rc < 0 || rr < 0 || n <= 0)
            {
                // USB / tuner error (e.g. -4 = device gone). After a few in a row, try
                // to recover by reopening the dongle; if that fails, report and stop
                // instead of spinning forever with no data.
                if (++errStreak >= 6)
                {
                    setStatus("RTL-SDR error - recovering...");
                    if (d != nullptr) rtlsdr_close(d);
                    d = nullptr; dev = nullptr;
                    wait(250);
                    if (threadShouldExit() || !running.load()) break;

                    if (rtlsdr_open(&d, (uint32_t)deviceIndex) < 0 || d == nullptr)
                    {
                        setStatus("RTL-SDR lost - reconnect the dongle, then Stop and Analyze again.");
                        running.store(false);
                        break;
                    }
                    dev = d;
                    configure(d);
                    errStreak = 0;
                }
                continue;
            }
            errStreak = 0;

            const int samples = juce::jmin(fftSize, n / 2);
            for (int i = 0; i < samples; ++i)
            {
                float I = ((float)raw[(size_t)(2 * i)]     - 127.5f) / 127.5f;
                float Q = ((float)raw[(size_t)(2 * i + 1)] - 127.5f) / 127.5f;
                fin[(size_t)i] = { I * window[(size_t)i], Q * window[(size_t)i] };
            }
            for (int i = samples; i < fftSize; ++i) fin[(size_t)i] = { 0.0f, 0.0f };

            fft->perform(fin.data(), fout.data(), false);

            const double norm = (double)fftSize * winPow;
            for (int k = 0; k < fftSize; ++k)
            {
                int kk = (k < fftSize / 2) ? k : k - fftSize;       // signed bin
                double offHz = (double)kk * SR / (double)fftSize;
                if (std::abs(offHz) > usable * SR * 0.5) continue;  // drop the segment edges

                double freqMHz = (centerHz + offHz) / 1.0e6;
                int b = (int)((freqMHz - bmin) / binMHz);
                if (b < 0 || b >= numBins) continue;

                double re = fout[(size_t)k].real(), im = fout[(size_t)k].imag();
                double p = (re * re + im * im) / norm;
                float db = (float)(10.0 * std::log10(juce::jmax(1.0e-12, p)) + dbOffset);

                if (db > sweep.powerDb[(size_t)b]) sweep.powerDb[(size_t)b] = db;
            }
        }

        {
            const juce::ScopedLock sl(lock);
            latest = sweep;
            hasData = true;
        }
    }

    if (d != nullptr) rtlsdr_close(d);
    dev = nullptr;
}

#else // ---- no hardware support compiled in --------------------------------

void RtlSdrSpectrumSource::run()
{
    setStatus("RTL-SDR support is not compiled in (librtlsdr not vendored).");
    running.store(false);
}

#endif
