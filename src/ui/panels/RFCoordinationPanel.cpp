/*
  ==============================================================================

    RFCoordinationPanel.cpp
    Created: 26 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "RFCoordinationPanel.h"
#include "../../RF/RFCoordinationSettings.h"
#include "../../RF/RFDeviceManager.h"
#include "../../RF/RFProfileManager.h"
#include "../../RF/SimulatedRFSpectrumSource.h"
#include "../../RF/RtlSdrSpectrumSource.h"
#include "../../RF/FrequencyOptimizer.h"
#include "../../RF/ui/RFDeviceManagerItemUI.h"
#include <algorithm>

//==============================================================================
// Embedded device-list manager UI (shows the assigned frequency / preset per row).
class RFDeviceManagerUI :
    public BaseManagerUI<RFDeviceManager, RFDevice, RFDeviceManagerItemUI>
{
public:
    RFDeviceManagerUI() :
        BaseManagerUI("RF Devices", RFDeviceManager::getInstance())
    {
        addItemText = "Add a device (mic / IEM)";
        noItemText = "Add the wireless devices to coordinate.";
        setShowSearchBar(false);
        bringToFrontOnSelect = false;   // keep the header bar (sibling, on top) visible on click
        addExistingItems();
    }

    // The "+" button, so the panel can float it on top of the header bar.
    juce::Component* getAddButton() const { return addItemBT.get(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFDeviceManagerUI)
};

// Embedded profiles manager UI (generic editor; range/presets edited in the inspector).
class RFProfileManagerUI :
    public BaseManagerUI<RFProfileManager, RFProfile, BaseItemUI<RFProfile>>
{
public:
    RFProfileManagerUI() :
        BaseManagerUI("RF Profiles", RFProfileManager::getInstance())
    {
        addItemText = "Add a frequency profile";
        noItemText = "Add profiles (range / presets), then assign one to each device.";
        setShowSearchBar(false);
        bringToFrontOnSelect = false;   // keep the header bar (sibling, on top) visible on click
        addExistingItems();
    }

    // The "+" button, so the panel can float it on top of the header bar.
    juce::Component* getAddButton() const { return addItemBT.get(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFProfileManagerUI)
};

// Thin draggable divider. Vertical = left|right (drag X); horizontal = top/bottom (drag Y).
class RFPanelSplitter : public juce::Component
{
public:
    std::function<void(int)> onDragTo;   // called with the cursor's pos (X if vertical, Y if horizontal) in the parent

    explicit RFPanelSplitter(bool verticalDivider = true) : vertical(verticalDivider)
    {
        setMouseCursor(vertical ? juce::MouseCursor::LeftRightResizeCursor
                                : juce::MouseCursor::UpDownResizeCursor);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(BG_COLOR.darker(.2f));
        g.setColour(BG_COLOR.brighter(isMouseOverOrDragging() ? .35f : .12f));
        if (vertical)
        {
            float cx = getWidth() * 0.5f;
            g.drawLine(cx, 4.0f, cx, (float)getHeight() - 4.0f, 1.5f);
        }
        else
        {
            float cy = getHeight() * 0.5f;
            g.drawLine(4.0f, cy, (float)getWidth() - 4.0f, cy, 1.5f);
        }
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override  { repaint(); }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (onDragTo) onDragTo(vertical ? getX() + e.x : getY() + e.y);
    }

private:
    bool vertical = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFPanelSplitter)
};

// Section title bar, matching organic UI's container-editor header look:
// a filled rounded bar with the name on the left.
class RFSectionHeader : public juce::Component
{
public:
    explicit RFSectionHeader(const juce::String& title) : text(title) {}

    void paint(juce::Graphics& g) override
    {
        const juce::Colour contour = BG_COLOR.brighter(.1f);
        g.setColour(contour.withMultipliedAlpha(.8f));
        g.fillRect(getLocalBounds().toFloat());

        g.setColour(contour.brighter(3.0f));
        g.setFont(g.getCurrentFont().withHeight((float)GlobalSettings::getInstance()->fontSize->floatValue()));
        g.drawText(text, getLocalBounds().reduced(8, 0), juce::Justification::centredLeft, true);
    }

private:
    juce::String text;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFSectionHeader)
};

//==============================================================================
RFCoordinationUI::RFCoordinationUI(const String& contentName) :
    ShapeShifterContent(RFCoordinationPanel::getInstance(), contentName)
{
}

RFCoordinationUI::~RFCoordinationUI()
{
}

juce_ImplementSingleton(RFCoordinationPanel);

RFCoordinationPanel::RFCoordinationPanel()
{
    settings = dynamic_cast<RFCoordinationSettings*>(
        ProjectSettings::getInstance()->getControllableContainerByName("rfCoordinationSettings"));

    createSources();

    analyzeBtn.onClick  = [this] { if (analyzing) stopAnalyze(); else startAnalyze(); };
    optimizeBtn.onClick = [this] { runOptimize(); };

    for (auto* b : { &analyzeBtn, &optimizeBtn })
    {
        b->setColour(juce::TextButton::buttonColourId, BG_COLOR.brighter(.08f));
        b->setColour(juce::TextButton::textColourOffId, TEXT_COLOR);
        addAndMakeVisible(b);
    }
    optimizeBtn.setTooltip("Assign a conflict-free frequency to every declared device (needs >= 1 min of scan)");

    // Inspector-style organic slider for the centre frequency.
    centerParam.reset(new FloatParameter("Center", "Centre frequency of the view, in MHz", 670.0f, 1.0f, 6000.0f));
    centerParam->defaultUI = FloatParameter::SLIDER;
    centerParam->addParameterListener(this);
    {
        auto* sliderUI = centerParam->createSlider();
        sliderUI->showLabel = false;            // no "Center" label
        sliderUI->suffix = " MHz";              // value suffix
        sliderUI->useCustomFGColor = true;      // this slider only (no global change)
        sliderUI->customFGColor = juce::Colours::black.withAlpha(0.35f);
        centerSliderUI.reset(sliderUI);
    }
    addAndMakeVisible(centerSliderUI.get());

    // Managers first, then their headers, so the title bars sit in front of (on top
    // of) the manager boxes: the manager extends up under the title and "wraps" it.
    profileListUI.reset(new RFProfileManagerUI());
    addAndMakeVisible(profileListUI.get());
    deviceListUI.reset(new RFDeviceManagerUI());
    addAndMakeVisible(deviceListUI.get());

    profileHeader.reset(new RFSectionHeader("RF Profiles"));
    addAndMakeVisible(profileHeader.get());
    deviceHeader.reset(new RFSectionHeader("RF Devices"));
    addAndMakeVisible(deviceHeader.get());

    // Float each manager's "+" button on top of its header bar: reparent it onto the
    // panel (added last -> in front of the headers) so the header background stays a
    // continuous bar underneath. The buttons stay owned by their managers.
    profileAddBtn = static_cast<RFProfileManagerUI*>(profileListUI.get())->getAddButton();
    deviceAddBtn  = static_cast<RFDeviceManagerUI*>(deviceListUI.get())->getAddButton();
    if (profileAddBtn != nullptr) addAndMakeVisible(profileAddBtn);
    if (deviceAddBtn  != nullptr) addAndMakeVisible(deviceAddBtn);

    auto* hsp = new RFPanelSplitter(false);   // horizontal divider: profiles / devices
    hsp->onDragTo = [this](int parentY)
    {
        if (leftColBounds.getHeight() > 0)
        {
            int avail = leftColBounds.getHeight() - 6;
            profileListHeight = juce::jlimit(80, juce::jmax(120, avail - 80),
                                             parentY - leftColBounds.getY());
            resized();
            repaint();
        }
    };
    profileSplitter.reset(hsp);
    addAndMakeVisible(profileSplitter.get());

    auto* sp = new RFPanelSplitter();
    sp->onDragTo = [this](int parentX)
    {
        deviceListWidth = juce::jlimit(160, juce::jmax(200, getWidth() - 220), parentX);
        resized();
        repaint();
    };
    deviceSplitter.reset(sp);
    addAndMakeVisible(deviceSplitter.get());

    if (settings != nullptr)
    {
        settings->scanMinMHz->addParameterListener(this);
        settings->scanMaxMHz->addParameterListener(this);
        settings->source->sourceType->addParameterListener(this);
        settings->source->device->addParameterListener(this);
        settings->source->gainDb->addParameterListener(this);
        settings->source->ppmCorrection->addParameterListener(this);
        settings->source->scanBinKHz->addParameterListener(this);
    }

    if (Engine::mainEngine != nullptr)
        Engine::mainEngine->addEngineListener(this);

    applyBandFromSettings();
    resetView();            // initialise the slider range/value
    updateButtons();
    startTimerHz(30);
}

RFCoordinationPanel::~RFCoordinationPanel()
{
    stopTimer();

    if (centerParam != nullptr) centerParam->removeParameterListener(this);

    if (settings != nullptr)
    {
        settings->scanMinMHz->removeParameterListener(this);
        settings->scanMaxMHz->removeParameterListener(this);
        settings->source->sourceType->removeParameterListener(this);
        settings->source->device->removeParameterListener(this);
        settings->source->gainDb->removeParameterListener(this);
        settings->source->ppmCorrection->removeParameterListener(this);
        settings->source->scanBinKHz->removeParameterListener(this);
    }

    if (Engine::mainEngine != nullptr)
        Engine::mainEngine->removeEngineListener(this);

    if (optimizeThread != nullptr) optimizeThread->stopThread(2000);

    if (sourceFull != nullptr)   sourceFull->stop();
    if (sourceDetail != nullptr) sourceDetail->stop();
}

//==============================================================================

void RFCoordinationPanel::startAnalyze()
{
    if (settings != nullptr)
    {
        bandMinMHz = settings->scanMinMHz->floatValue();
        bandMaxMHz = settings->scanMaxMHz->floatValue();
    }
    if (bandMaxMHz - bandMinMHz < 1.0) bandMaxMHz = bandMinMHz + 1.0;

    // Keep the current view/centre (don't recentre on Analyze); just clamp it to the band.
    updateSliderRange();
    setCenter(centerParam->floatValue());
    rebuildWaterfall();   // size the image to the current view before wiping
    clearGraphs();
    scanElapsedSec = 0.0;
    lastTickMs = juce::Time::getMillisecondCounterHiRes();
    analyzing = true;

    startSources();
    updateButtons();
}

void RFCoordinationPanel::createSources()
{
    if (sourceFull != nullptr)   sourceFull->stop();
    if (sourceDetail != nullptr) sourceDetail->stop();

    const int src = settings != nullptr ? (int)settings->source->sourceType->getValueData() : 0;

    if (src == RFSourceSettings::RTLSDR)
    {
        // One physical dongle: a single source feeds both occupancy and the waterfall.
        sourceFull.reset(new RtlSdrSpectrumSource());
        sourceDetail.reset();
    }
    else
    {
        // Simulated: cheap two-source hybrid (full band + fine detail of the view).
        sourceFull.reset(new SimulatedRFSpectrumSource());
        sourceDetail.reset(new SimulatedRFSpectrumSource());
    }
}

void RFCoordinationPanel::startSources()
{
    const int src = settings != nullptr ? (int)settings->source->sourceType->getValueData() : 0;

    if (src == RFSourceSettings::RTLSDR)
    {
        // Single fine full-band scan; the waterfall samples it across the view, so
        // zoom stays sharp without a second (impossible) simultaneous sweep.
        if (auto* rtl = dynamic_cast<RtlSdrSpectrumSource*>(sourceFull.get()))
        {
            rtl->configure(settings->source->selectedDeviceIndex(),
                           settings->source->gainDb->floatValue(), settings->source->ppmCorrection->intValue());
            // Live mode: scan only the visible window (fast frames, no freeze);
            // the slider/wheel retune it. Full-band occupancy comes from the guided
            // optimization scan.
            rtl->start(viewMinMHz, viewMaxMHz, rtlBinHz());
        }
        return;
    }

    // Simulated: full band at base resolution + fine detail of the visible range.
    const double baseBinHz = 100000.0; // 100 kHz
    if (sourceFull != nullptr)
    {
        sourceFull->setReferenceBand(bandMinMHz, bandMaxMHz);
        sourceFull->start(bandMinMHz, bandMaxMHz, baseBinHz);
    }
    retuneDetail();
}

void RFCoordinationPanel::stopAnalyze()
{
    analyzing = false;
    if (sourceFull != nullptr)   sourceFull->stop();
    if (sourceDetail != nullptr) sourceDetail->stop();
    detailDirty = false;
    updateButtons();
}

void RFCoordinationPanel::clearGraphs()
{
    if (!waterfall.isNull())
        waterfall.clear(waterfall.getBounds(), juce::Colours::black);
    wfBandMin = viewMinMHz;
    wfBandMax = viewMaxMHz;
    std::fill(maxHoldDb.begin(), maxHoldDb.end(), -200.0f);
    specSmoothDb.clear();
    haveFull = false;
    haveDetail = false;
    scanElapsedSec = 0.0;
    repaint();
}

void RFCoordinationPanel::fileLoaded()    { stopAnalyze(); clearGraphs(); }
void RFCoordinationPanel::engineCleared() { stopAnalyze(); clearGraphs(); }

void RFCoordinationPanel::updateButtons()
{
    analyzeBtn.setButtonText(analyzing ? "Stop" : "Analyze");
    optimizeBtn.setButtonText(guidedActive ? "Cancel" : "Optimize");
    optimizeBtn.setEnabled(true);
}

//==============================================================================

void RFCoordinationPanel::timerCallback()
{
    // RTL-SDR fatal error: the source thread stopped itself -> stop cleanly so the
    // error message (set on the source) is shown instead of a frozen display.
    if (analyzing && sourceDetail == nullptr && sourceFull != nullptr && !sourceFull->isRunning())
    {
        if (guidedActive) cancelGuidedScan();
        stopAnalyze();
    }

    if (analyzing)
    {
        double now = juce::Time::getMillisecondCounterHiRes();
        double dtSec = (now - lastTickMs) / 1000.0;
        scanElapsedSec += dtSec;
        lastTickMs = now;

        // Full-band occupancy (peak-hold with cool-down).
        RFSpectrumSweep full;
        if (sourceFull != nullptr && sourceFull->getLatestSweep(full))
        {
            liveFull = full;
            haveFull = true;

            if ((int)maxHoldDb.size() != liveFull.numBins())
                maxHoldDb.assign((size_t)liveFull.numBins(), -200.0f);

            // A swept / transient signal fades over time, while a continuously-present
            // carrier stays lit.
            const float decay = (float)(occupancyDecayRate() * dtSec);
            for (int i = 0; i < liveFull.numBins(); ++i)
                maxHoldDb[(size_t)i] = juce::jmax(liveFull.powerDb[(size_t)i], maxHoldDb[(size_t)i] - decay);

            // With no separate detail source (RTL-SDR: one device), the fine full-band
            // sweep also feeds the waterfall.
            if (sourceDetail == nullptr) commitWaterfallColumn(liveFull);
        }

        // Detail scan of the visible range feeds the waterfall (simulated hybrid).
        RFSpectrumSweep detail;
        if (sourceDetail != nullptr && sourceDetail->getLatestSweep(detail))
        {
            liveDetail = detail;
            haveDetail = true;
            commitWaterfallColumn(liveDetail);
        }

        // Apply a pending (debounced) detail retune after the view settled.
        if (sourceDetail != nullptr && detailDirty && now >= detailRetuneAtMs)
            retuneDetail();

        // Apply pending RTL-SDR gain/resolution/device changes (debounced restart).
        if (sourceRestartPending && now >= sourceRestartAtMs)
        {
            sourceRestartPending = false;
            startSources();
            haveFull = false;
            std::fill(maxHoldDb.begin(), maxHoldDb.end(), -200.0f);
            specSmoothDb.clear();
        }

        // Temporally-smoothed FFT trace (per plot pixel) for a clean, SDR++-like line.
        const RFSpectrumSweep* disp = (sourceDetail != nullptr && haveDetail) ? &liveDetail
                                    : (haveFull ? &liveFull : nullptr);
        if (disp != nullptr && plotArea.getWidth() > 0)
        {
            const int W = plotArea.getWidth();
            const bool seed = (int)specSmoothDb.size() != W;
            if (seed) specSmoothDb.assign((size_t)W, -200.0f);
            const float alpha = 0.35f;
            for (int i = 0; i < W; ++i)
            {
                float db = disp->powerAtInterp(freqForX(plotArea.getX() + i));
                float& s = specSmoothDb[(size_t)i];
                s = seed ? db : s + (db - s) * alpha;
            }
        }

        // Live mode: centre the view on a newly-selected device so it's easy to see
        // its spectrum (not during the guided scan, which drives the view itself).
        if (!guidedActive)
        {
            RFDevice* sel = nullptr;
            for (auto* d : RFDeviceManager::getInstance()->items)
                if (d->isSelected) { sel = d; break; }

            if (sel != nullptr && (void*)sel != lastSelectedDevicePtr)
            {
                double f = sel->assignedFreqMHz->floatValue();
                if (f <= 0.0)   // not assigned yet: use the centre of its candidates
                {
                    auto cands = sel->getCandidates();
                    if (!cands.empty())
                    {
                        double lo = cands.front().freqMHz, hi = lo;
                        for (auto& c : cands) { lo = juce::jmin(lo, c.freqMHz); hi = juce::jmax(hi, c.freqMHz); }
                        f = (lo + hi) * 0.5;
                    }
                }
                if (f > 0.0) centerParam->setValue(f);   // notifies -> centre + retune + redraw
            }
            lastSelectedDevicePtr = (void*)sel;
        }

        // Guided scan: accumulate the current window's occupancy, advance after the dwell.
        if (guidedActive)
        {
            if (auto* ds = displaySweep()) accumulateGuided(*ds);
            if (now - scanWindowStartMs >= dwellSec() * 1000.0)
            {
                ++scanWindowIndex;
                if (scanWindowIndex >= (int)scanWindows.size()) finishGuidedScan();
                else gotoScanWindow(scanWindowIndex);
            }
        }
    }

    updateButtons();

    // Background optimization finished: apply its result on the message thread.
    if (optimizeDone.exchange(false))
    {
        optimizeProgress.store(-1.0f);
        applyOptimizeResult();
    }
    const bool optimizing = optimizeProgress.load() >= 0.0f;

    // While analyzing or optimizing the panel changes every frame; otherwise only
    // repaint when something displayed actually changed, so an idle panel costs nothing.
    if (analyzing || optimizing) repaint();
    else
    {
        double sig = computeDisplaySignature();
        if (sig != lastDisplaySig) { lastDisplaySig = sig; repaint(); }
    }
}

double RFCoordinationPanel::computeDisplaySignature() const
{
    double sig = viewMinMHz * 1.0 + viewMaxMHz * 7.0 + bandMinMHz * 13.0 + bandMaxMHz * 17.0;

    if (settings != nullptr)
    {
        sig += settings->allowedRanges->items.size() * 1009.0;
        int i = 1;
        for (auto* r : settings->allowedRanges->items)
            sig += (r->minMHz->floatValue() * 31.0 + r->maxMHz->floatValue() * 131.0) * (double)(i++);

        sig += settings->isSelected ? 77777.0 : 0.0;   // update the selection border
    }

    auto* mgr = RFDeviceManager::getInstanceWithoutCreating();
    if (mgr != nullptr)
    {
        sig += mgr->items.size() * 2003.0;
        int i = 1;
        for (auto* d : mgr->items)
        {
            sig += (double)i * (d->enabled->boolValue() ? 3.0 : 5.0);
            sig += d->isSelected ? 311.0 * (double)i : 0.0;   // highlight the selected marker
            sig += d->assignedFreqMHz->floatValue() * 0.123 * (double)i;
            sig += (double)d->assignedPresetName->stringValue().hashCode() * 1e-6;
            sig += (double)d->niceName.hashCode() * 1e-7;
            ++i;
        }
    }
    return sig;
}

void RFCoordinationPanel::commitWaterfallColumn(const RFSpectrumSweep& s)
{
    if (waterfall.isNull() || s.powerDb.empty()) return;

    const int w = waterfall.getWidth();
    const int h = waterfall.getHeight();
    if (w <= 0 || h <= 0) return;

    // Scroll the existing content up by one row; the newest row appears at the
    // bottom (time flows bottom -> top).
    waterfall.moveImageSection(0, 0, 0, 1, w, h - 1);

    // The image maps the range it was last built for (wfBand == the view); sample
    // the sweep across it.
    const double span = wfBandMax - wfBandMin;
    for (int x = 0; x < w; ++x)
    {
        double freq = wfBandMin + ((double)x + 0.5) / (double)w * span; // low freq -> left
        float db = s.powerAtInterp(freq);   // interpolated -> no blocky columns
        waterfall.setPixelAt(x, h - 1, powerToColour(db));
    }
}

int RFCoordinationPanel::waterfallColumns() const
{
    // Oversample horizontally (~3x plot pixels) for a crisp render at any zoom,
    // bounded RAM (the image tracks the view, not the whole band).
    return juce::jlimit(512, 6144, plotArea.getWidth() * 3);
}

int RFCoordinationPanel::waterfallRows() const
{
    // Oversample vertically too, so the time axis stays smooth on hi-DPI displays.
    return juce::jlimit(256, 2048, waterArea.getHeight() * 2);
}

double RFCoordinationPanel::detailBinHz() const
{
    return (viewMaxMHz - viewMinMHz) * 1.0e6 / (double)juce::jmax(1, waterfallColumns());
}

double RFCoordinationPanel::rtlBinHz() const
{
    // Scan Resolution is the bin width at the widest (default) view; zooming in
    // narrows the window, so the dongle bin width scales down proportionally.
    double base = settings != nullptr ? settings->source->scanBinKHz->floatValue() * 1000.0 : 1000.0;
    return base * (viewWidthMHz / viewWidthMaxMHz);
}

void RFCoordinationPanel::scheduleDetailRetune()
{
    detailDirty = true;
    detailRetuneAtMs = juce::Time::getMillisecondCounterHiRes() + 120.0; // debounce zoom/pan
}

void RFCoordinationPanel::retuneView()
{
    if (sourceDetail != nullptr)        // simulated: debounced detail-source restart
    {
        scheduleDetailRetune();
        return;
    }
    // RTL-SDR: retune the dongle to the new window live (cheap, no reopen).
    if (analyzing)
        if (auto* rtl = dynamic_cast<RtlSdrSpectrumSource*>(sourceFull.get()))
            rtl->setRange(viewMinMHz, viewMaxMHz, rtlBinHz());
}

void RFCoordinationPanel::retuneDetail()
{
    detailDirty = false;
    if (!analyzing || sourceDetail == nullptr) return;

    sourceDetail->setReferenceBand(bandMinMHz, bandMaxMHz);   // same absolute carriers as full
    sourceDetail->start(viewMinMHz, viewMaxMHz, detailBinHz());
    haveDetail = false;   // wait for the first fine sweep
}

void RFCoordinationPanel::rebuildWaterfall()
{
    const int w = waterfallColumns();
    const int h = waterfallRows();
    if (w <= 0 || h <= 0) return;

    juce::Image img(juce::Image::RGB, w, h, true);
    img.clear(img.getBounds(), juce::Colours::black);

    // The image maps the current view; carry over any overlapping content from the
    // previous view so a zoom/pan/resize doesn't wipe the captured spectrum.
    if (!waterfall.isNull() && wfBandMax > wfBandMin)
    {
        const double ovLo = juce::jmax(wfBandMin, viewMinMHz);
        const double ovHi = juce::jmin(wfBandMax, viewMaxMHz);
        if (ovHi > ovLo)
        {
            const int ow = waterfall.getWidth(), oh = waterfall.getHeight();
            const double oFull = wfBandMax - wfBandMin;
            const double nFull = juce::jmax(1.0e-6, viewMaxMHz - viewMinMHz);

            const int srcX = (int)std::round((ovLo - wfBandMin) / oFull * ow);
            const int srcW = juce::jmax(1, (int)std::round((ovHi - ovLo) / oFull * ow));
            const int dstX = (int)std::round((ovLo - viewMinMHz) / nFull * w);
            const int dstW = juce::jmax(1, (int)std::round((ovHi - ovLo) / nFull * w));

            juce::Graphics ig(img);
            ig.drawImage(waterfall, dstX, 0, dstW, h,
                         juce::jlimit(0, juce::jmax(0, ow - 1), srcX), 0, juce::jmin(srcW, ow), oh);
        }
    }

    waterfall = img;
    wfBandMin = viewMinMHz;
    wfBandMax = viewMaxMHz;
}

//==============================================================================
// Background optimization job: runs the (multi-pass) optimizer off the message
// thread and reports progress; the panel applies the result when it completes.
class RFOptimizeJob : public juce::Thread
{
public:
    RFOptimizeJob(RFSpectrumSweep occ_, std::vector<FrequencyOptimizer::DeviceInput> in_,
                  FrequencyOptimizer::Constraints c_, double maxSeconds_,
                  std::atomic<float>& prog_, std::atomic<bool>& done_,
                  FrequencyOptimizer::Result& out_, juce::CriticalSection& lock_) :
        juce::Thread("RF Optimize"),
        occ(std::move(occ_)), inputs(std::move(in_)), c(c_), maxSeconds(maxSeconds_),
        prog(prog_), done(done_), out(out_), lk(lock_) {}

    void run() override
    {
        auto res = FrequencyOptimizer::optimize(occ, inputs, c, maxSeconds,
            [this](float p) { prog.store(p); return !threadShouldExit(); });
        { juce::ScopedLock sl(lk); out = res; }
        prog.store(1.0f);
        done.store(true);
    }

private:
    RFSpectrumSweep occ;
    std::vector<FrequencyOptimizer::DeviceInput> inputs;
    FrequencyOptimizer::Constraints c;
    double maxSeconds;
    std::atomic<float>& prog;
    std::atomic<bool>&  done;
    FrequencyOptimizer::Result& out;
    juce::CriticalSection& lk;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFOptimizeJob)
};

void RFCoordinationPanel::runOptimize()
{
    if (settings == nullptr) return;
    if (optimizeThread != nullptr && optimizeThread->isThreadRunning()) return;   // already running

    if (guidedActive) { cancelGuidedScan(); return; }   // toggle: cancel an ongoing scan

    const bool rtlsdr = (int)settings->source->sourceType->getValueData() == RFSourceSettings::RTLSDR;

    if (rtlsdr)
    {
        const bool haveDongle = RtlSdrSpectrumSource::isSupported()
                             && !RtlSdrSpectrumSource::listDevices().isEmpty();
        if (!haveDongle)
        {
            // No dongle: offer to optimize assuming a clear spectrum. Frequencies are
            // still spaced apart and kept free of intermodulation, but no real-world
            // occupancy is taken into account (empty occupancy = everything free).
            juce::NativeMessageBox::showYesNoBox(
                juce::AlertWindow::QuestionIcon,
                "No RTL-SDR dongle detected",
                "No RTL-SDR dongle was found.\n\n"
                "Optimize anyway, assuming the spectrum is clear? Frequencies will still "
                "be spaced apart and kept free of intermodulation, but real-world "
                "occupancy will not be measured.",
                this,
                juce::ModalCallbackFunction::create([this](int result)
                {
                    if (result == 1)                      // Yes
                        launchOptimize(RFSpectrumSweep());
                }));
            return;
        }

        // RTL-SDR: walk the devices' ranges in windows to capture occupancy, then place.
        startGuidedScan();
        return;
    }

    // Simulated: optimize straight away on the current occupancy (no guided scan;
    // the full band is already available). Empty occupancy = everything free.
    RFSpectrumSweep occ;
    if (haveFull && !liveFull.powerDb.empty())
    {
        occ = liveFull;
        if ((int)maxHoldDb.size() == occ.numBins()) occ.powerDb = maxHoldDb;
    }
    launchOptimize(occ);
}

void RFCoordinationPanel::launchOptimize(const RFSpectrumSweep& occ)
{
    if (settings == nullptr) return;
    if (optimizeThread != nullptr && optimizeThread->isThreadRunning()) return;

    // Clear every device's assignment first (including disabled ones), so a device
    // that is no longer placed doesn't keep a stale frequency.
    for (auto* d : RFDeviceManager::getInstance()->items)
        d->clearAssignment();

    std::vector<FrequencyOptimizer::DeviceInput> inputs;
    for (auto* d : RFDeviceManager::getInstance()->items)
    {
        if (!d->enabled->boolValue()) continue;
        FrequencyOptimizer::DeviceInput in;
        in.deviceId = d->niceName;
        for (const auto& cand : d->getCandidates())
            in.candidates.push_back({ cand.freqMHz, cand.presetName });
        inputs.push_back(std::move(in));
    }

    if (inputs.empty())
    {
        NLOGWARNING("RF Coordination", "No enabled device to coordinate.");
        repaint();
        return;
    }

    FrequencyOptimizer::Constraints c;
    c.minSpacingKHz     = settings->minSpacingKHz->floatValue();
    c.guardKHz          = settings->guardKHz->floatValue();
    c.imdMode           = (int)settings->imdOrder->getValueData();
    c.occupancyMarginDb = settings->occupancyMarginDb->floatValue();
    for (auto* r : settings->allowedRanges->items)
    {
        double lo = r->minMHz->floatValue(), hi = r->maxMHz->floatValue();
        if (hi < lo) std::swap(lo, hi);
        c.allowedRanges.push_back({ lo, hi });
    }

    // No fixed pass count: the optimizer keeps trying (random-restart) until every
    // placeable device is placed, it plateaus, or this safety time budget elapses.
    const double maxSeconds = 15.0;
    optimizeProgress.store(0.0f);
    optimizeDone.store(false);
    optimizeThread.reset(new RFOptimizeJob(occ, std::move(inputs), c, maxSeconds,
                                           optimizeProgress, optimizeDone, optimizeResult, optimizeLock));
    optimizeThread->startThread();
    repaint();
}

void RFCoordinationPanel::applyOptimizeResult()
{
    FrequencyOptimizer::Result res;
    { juce::ScopedLock sl(optimizeLock); res = optimizeResult; }

    int placed = 0;
    for (const auto& a : res.assignments)
    {
        RFDevice* dev = nullptr;
        for (auto* d : RFDeviceManager::getInstance()->items)
            if (d->niceName == a.deviceId) { dev = d; break; }
        if (dev == nullptr) continue;

        if (a.placed) { dev->setAssignment(a.freqMHz, a.presetName); ++placed; }
        else            dev->clearAssignment();
    }

    NLOG("RF Coordination", "Optimized: " + String(placed) + "/" + String((int)res.assignments.size()) + " device(s) assigned.");
    for (auto& w : res.warnings)
        NLOGWARNING("RF Coordination", w);

    repaint();
}

//==============================================================================
// Guided optimization scan

const RFSpectrumSweep* RFCoordinationPanel::displaySweep() const
{
    if (sourceDetail != nullptr) return haveDetail ? &liveDetail : nullptr;   // simulated
    return haveFull ? &liveFull : nullptr;                                     // RTL-SDR
}

double RFCoordinationPanel::dwellSec() const
{
    return settings != nullptr ? (double)settings->scanDwellSec->floatValue() : guidedDwellSec;
}

std::vector<std::pair<double, double>> RFCoordinationPanel::computeScanRanges() const
{
    std::vector<std::pair<double, double>> r;
    const double pad = 0.5;   // MHz of context around each assignable frequency

    // Scan only around the device's actually assignable frequencies (a Range mode
    // grid collapses into its contiguous span; sparse Presets give a few small
    // windows instead of the whole min..max span). No dependency on Scan Min/Max.
    for (auto* d : RFDeviceManager::getInstance()->items)
    {
        if (!d->enabled->boolValue()) continue;
        for (auto& c : d->getCandidates())
            r.push_back({ c.freqMHz - pad, c.freqMHz + pad });
    }

    // Restrict to the allowed ranges (placement is only legal there).
    if (settings != nullptr && !settings->allowedRanges->items.isEmpty())
    {
        std::vector<std::pair<double, double>> allowed;
        for (auto* a : settings->allowedRanges->items)
        {
            double lo = a->minMHz->floatValue(), hi = a->maxMHz->floatValue();
            if (hi < lo) std::swap(lo, hi);
            allowed.push_back({ lo, hi });
        }
        std::vector<std::pair<double, double>> inter;
        for (auto& rg : r)
            for (auto& al : allowed)
            {
                double lo = juce::jmax(rg.first, al.first), hi = juce::jmin(rg.second, al.second);
                if (hi > lo) inter.push_back({ lo, hi });
            }
        r.swap(inter);
    }

    // Merge overlaps (Range grids collapse into their span; nearby Presets merge).
    r.erase(std::remove_if(r.begin(), r.end(), [](auto& p) { return p.second <= p.first; }), r.end());
    std::sort(r.begin(), r.end());

    std::vector<std::pair<double, double>> merged;
    for (auto& rg : r)
    {
        if (!merged.empty() && rg.first <= merged.back().second + 1.0e-6)
            merged.back().second = juce::jmax(merged.back().second, rg.second);
        else
            merged.push_back(rg);
    }
    return merged;
}

void RFCoordinationPanel::startGuidedScan()
{
    auto ranges = computeScanRanges();
    if (ranges.empty())
    {
        // Nothing to scan: place directly (allowed ranges assumed free).
        NLOGWARNING("RF Coordination", "No device ranges to scan - placing directly.");
        launchOptimize(RFSpectrumSweep());
        return;
    }

    // Split the ranges into guided scan windows.
    scanWindows.clear();
    for (auto& rg : ranges)
        for (double w = rg.first; w < rg.second - 1.0e-6; w += guidedWindowMHz)
            scanWindows.push_back({ w, juce::jmin(w + guidedWindowMHz, rg.second) });

    if (scanWindows.empty()) return;
    if (scanWindows.size() > 200) scanWindows.resize(200);   // safety cap

    NLOG("RF Coordination", "Guided scan: " + String((int)scanWindows.size()) + " window(s) x "
         + String(dwellSec(), 0) + "s = ~" + String((int)std::ceil(scanWindows.size() * dwellSec())) + "s total.");

    // Occupancy map over the whole union extent (coarse: occupancy doesn't need 1 kHz).
    const double umin = ranges.front().first, umax = ranges.back().second;
    guidedOcc = RFSpectrumSweep();
    guidedOcc.minMHz = umin; guidedOcc.maxMHz = umax; guidedOcc.binHz = 25000.0;
    guidedOcc.powerDb.assign((size_t)juce::jmax(1, (int)std::round((umax - umin) / 0.025)), -200.0f);

    if (!analyzing) startAnalyze();   // make sure the source is running
    guidedActive = true;
    scanWindowIndex = 0;
    gotoScanWindow(0);
    repaint();
}

void RFCoordinationPanel::gotoScanWindow(int i)
{
    auto win = scanWindows[(size_t)i];
    // Show the window directly (may be wider than the live view and outside the
    // navigation band, so set the view bounds directly rather than via setCenter).
    viewMinMHz = win.first;
    viewMaxMHz = win.second;
    centerParam->setValue((win.first + win.second) * 0.5, true);   // cosmetic
    rebuildWaterfall();

    // Retune the source to scan this window at the configured resolution.
    const double bin = settings != nullptr ? settings->source->scanBinKHz->floatValue() * 1000.0 : 1000.0;
    if (sourceDetail != nullptr)
    {
        sourceDetail->setReferenceBand(bandMinMHz, bandMaxMHz);
        sourceDetail->start(viewMinMHz, viewMaxMHz, detailBinHz());
    }
    else if (auto* rtl = dynamic_cast<RtlSdrSpectrumSource*>(sourceFull.get()))
    {
        rtl->setRange(viewMinMHz, viewMaxMHz, bin);
    }

    specSmoothDb.clear();
    scanWindowStartMs = juce::Time::getMillisecondCounterHiRes();
}

void RFCoordinationPanel::accumulateGuided(const RFSpectrumSweep& s)
{
    if (s.powerDb.empty() || guidedOcc.powerDb.empty()) return;
    const double occBinMHz = guidedOcc.binMHz();
    for (int i = 0; i < s.numBins(); ++i)
    {
        double f = s.freqForBin(i);
        int b = (int)((f - guidedOcc.minMHz) / occBinMHz);
        if (b < 0 || b >= guidedOcc.numBins()) continue;
        if (s.powerDb[(size_t)i] > guidedOcc.powerDb[(size_t)b]) guidedOcc.powerDb[(size_t)b] = s.powerDb[(size_t)i];
    }
}

void RFCoordinationPanel::cancelGuidedScan()
{
    guidedActive = false;
    NLOG("RF Coordination", "Guided scan cancelled.");
    repaint();
}

void RFCoordinationPanel::finishGuidedScan()
{
    guidedActive = false;
    NLOG("RF Coordination", "Guided scan complete - placing devices.");
    launchOptimize(guidedOcc);   // uses the captured occupancy
    stopAnalyze();               // stop scanning the dongle now that we're done
}

//==============================================================================
// Mapping helpers

double RFCoordinationPanel::xForFreq(double mhz) const
{
    double span = juce::jmax(1.0e-6, viewMaxMHz - viewMinMHz);
    double frac = (mhz - viewMinMHz) / span;
    return plotArea.getX() + frac * plotArea.getWidth();
}

double RFCoordinationPanel::freqForX(int x) const
{
    double frac = (double)(x - plotArea.getX()) / (double)juce::jmax(1, plotArea.getWidth());
    return viewMinMHz + juce::jlimit(0.0, 1.0, frac) * (viewMaxMHz - viewMinMHz);
}

float RFCoordinationPanel::yForDb(float db) const
{
    const float dbMin = -105.0f, dbMax = -20.0f;
    float frac = juce::jlimit(0.0f, 1.0f, (db - dbMin) / (dbMax - dbMin));
    return (float)plotArea.getBottom() - frac * (float)plotArea.getHeight();
}

juce::Colour RFCoordinationPanel::powerToColour(float db)
{
    // Perceptual waterfall colormap (SDR++-style): black -> blue -> cyan -> green ->
    // yellow -> orange -> red -> white. Smoothly interpolated for a clean gradient.
    static const struct { float p; uint8_t r, g, b; } stops[] = {
        { 0.00f,  10,  14,  34 },   // noise floor: dark blue (visible, not pure black)
        { 0.12f,   0,  20,  80 },
        { 0.28f,   0,  40, 170 },
        { 0.44f,   0, 165, 200 },
        { 0.58f,  30, 200,  90 },
        { 0.72f, 225, 215,  45 },
        { 0.86f, 250, 130,  25 },
        { 0.95f, 245,  45,  35 },
        { 1.00f, 255, 255, 255 },
    };

    float norm = juce::jlimit(0.0f, 1.0f, (db + 100.0f) / 80.0f);   // -100..-20 dB
    int n = (int)(sizeof(stops) / sizeof(stops[0]));
    for (int i = 1; i < n; ++i)
    {
        if (norm <= stops[i].p)
        {
            float t = (norm - stops[i - 1].p) / juce::jmax(1.0e-6f, stops[i].p - stops[i - 1].p);
            auto lerp = [t](uint8_t a, uint8_t b) { return (uint8_t)(a + (b - a) * t); };
            return juce::Colour(lerp(stops[i - 1].r, stops[i].r),
                                lerp(stops[i - 1].g, stops[i].g),
                                lerp(stops[i - 1].b, stops[i].b));
        }
    }
    return juce::Colour(stops[n - 1].r, stops[n - 1].g, stops[n - 1].b);
}

float RFCoordinationPanel::yForSpecDb(float db) const
{
    const float dbMin = -105.0f, dbMax = -20.0f;
    float frac = juce::jlimit(0.0f, 1.0f, (db - dbMin) / (dbMax - dbMin));
    return (float)specArea.getBottom() - frac * (float)specArea.getHeight();
}

void RFCoordinationPanel::setCenter(double centerMHz)
{
    const double full = bandMaxMHz - bandMinMHz;
    if (full <= viewWidthMHz)   // band narrower than the view: show all of it
    {
        viewMinMHz = bandMinMHz;
        viewMaxMHz = bandMaxMHz;
        return;
    }
    const double half = viewWidthMHz * 0.5;
    double c = juce::jlimit(bandMinMHz + half, bandMaxMHz - half, centerMHz);
    viewMinMHz = c - half;
    viewMaxMHz = c + half;
}

void RFCoordinationPanel::updateSliderRange()
{
    const double full = bandMaxMHz - bandMinMHz;
    const double half = viewWidthMHz * 0.5;
    if (full <= viewWidthMHz)
    {
        centerParam->setRange(bandMinMHz, bandMaxMHz);
        centerParam->setEnabled(false);
        canNavigate = false;
    }
    else
    {
        centerParam->setRange(bandMinMHz + half, bandMaxMHz - half);
        centerParam->setEnabled(true);
        canNavigate = true;
    }
}

void RFCoordinationPanel::resetView()
{
    viewWidthMHz = viewWidthMaxMHz;   // start at the widest view
    updateSliderRange();
    const double c = (bandMinMHz + bandMaxMHz) * 0.5;
    setCenter(c);
    centerParam->setValue(c, true);   // silent
}

double RFCoordinationPanel::occupancyDecayRate() const
{
    double cool = (settings != nullptr) ? settings->occupancyCooldownSec->floatValue() : 10.0;
    return occupancyDbSpan / juce::jmax(0.5, cool);
}

void RFCoordinationPanel::applyBandFromSettings()
{
    if (settings == nullptr) return;

    double a = settings->scanMinMHz->floatValue();
    double b = settings->scanMaxMHz->floatValue();
    if (b < a) std::swap(a, b);
    if (b - a < 1.0) b = a + 1.0;

    if (bandMinMHz == a && bandMaxMHz == b) return;   // no actual change

    bandMinMHz = a;
    bandMaxMHz = b;
    resetView();

    // Remap the already-captured spectrum onto the new view instead of wiping it.
    rebuildWaterfall();

    // Occupancy is tied to the full-band bins, which change with the band, so it
    // rebuilds from the next sweep; hide it until then.
    haveFull = false;
    std::fill(maxHoldDb.begin(), maxHoldDb.end(), -200.0f);
    specSmoothDb.clear();

    // If a scan is running, retune both sources to the new band/view.
    if (analyzing)
    {
        startSources();
        lastTickMs = juce::Time::getMillisecondCounterHiRes();
    }

    repaint();
}

void RFCoordinationPanel::parameterValueChanged(Parameter* p)
{
    if (p == centerParam.get())
    {
        setCenter(centerParam->floatValue());
        rebuildWaterfall();
        retuneView();
        specSmoothDb.clear();
        repaint();
        return;
    }

    if (settings == nullptr) return;
    if (p == settings->scanMinMHz || p == settings->scanMaxMHz)
        applyBandFromSettings();
    else if (p == settings->source->sourceType)
    {
        // Auto-refresh the dongle list when switching to RTL-SDR.
        if ((int)settings->source->sourceType->getValueData() == RFSourceSettings::RTLSDR)
            settings->source->populateDevices();

        stopAnalyze();      // switch source: stop, rebuild, wait for a new Analyze
        createSources();
        clearGraphs();
        repaint();
    }
    else if (p == settings->source->scanBinKHz)
    {
        // Resolution: applied live to the running window scan (no reopen).
        retuneView();
    }
    else if (p == settings->source->device || p == settings->source->gainDb || p == settings->source->ppmCorrection)
    {
        // Gain / device / ppm need the device reopened: debounced restart so dragging
        // a slider doesn't reopen the dongle on every step.
        if (analyzing && sourceDetail == nullptr)   // RTL-SDR path
        {
            sourceRestartPending = true;
            sourceRestartAtMs = juce::Time::getMillisecondCounterHiRes() + 300.0;
        }
    }
}

//==============================================================================
// Mouse

void RFCoordinationPanel::mouseMove(const juce::MouseEvent& e)
{
    mousePos = e.getPosition();
    mouseInPlot = plotArea.contains(mousePos);
    repaint();
}

void RFCoordinationPanel::mouseExit(const juce::MouseEvent&)
{
    mouseInPlot = false;
    repaint();
}

void RFCoordinationPanel::mouseDown(const juce::MouseEvent& e)
{
    // Clicking the analyzer selects the RF Coordination project settings, so its
    // inspector (scan band, constraints, allowed ranges...) opens - and the plot
    // shows the same selection border as the device list.
    if (plotArea.contains(e.getPosition()))
    {
        if (settings != nullptr) settings->selectThis();
        // Start a pan-drag from here.
        plotDragging = canNavigate;
        dragStartX = e.getPosition().x;
        dragStartCenter = centerParam->floatValue();
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        repaint();
    }
}

void RFCoordinationPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (!plotDragging) return;

    // Drag to pan: move the centre frequency opposite to the drag (content follows
    // the cursor). The wheel is reserved for zoom only.
    double dFreq = -(double)(e.getPosition().x - dragStartX)
                   * (viewMaxMHz - viewMinMHz) / (double)juce::jmax(1, plotArea.getWidth());
    centerParam->setValue(dragStartCenter + dFreq);   // notifies -> setCenter + redraw
}

void RFCoordinationPanel::mouseUp(const juce::MouseEvent&)
{
    plotDragging = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void RFCoordinationPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (!plotArea.contains(e.getPosition())) return;

    // The wheel zooms the view width between 0.5 and 10 MHz (the dongle resolution
    // follows, finer when zoomed), anchored on the frequency under the cursor.
    double dy = wheel.deltaY;
    if (wheel.isReversed) dy = -dy;
    if (dy == 0.0) return;

    const double fAnchor = freqForX(e.getPosition().x);
    const double fracX = (double)(e.getPosition().x - plotArea.getX()) / (double)juce::jmax(1, plotArea.getWidth());

    viewWidthMHz = juce::jlimit(viewWidthMinMHz, viewWidthMaxMHz, viewWidthMHz * std::exp(-dy * 0.8));
    updateSliderRange();

    // Keep fAnchor under the cursor: centre = anchor - (fracX - 0.5) * width.
    centerParam->setValue(fAnchor - (fracX - 0.5) * viewWidthMHz, true);   // silent (clamped)
    setCenter(centerParam->floatValue());

    rebuildWaterfall();
    retuneView();
    specSmoothDb.clear();
    repaint();
}

void RFCoordinationPanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!plotArea.contains(e.getPosition())) return;

    // Reset the zoom to the full (widest) view, centred on the frequency under the cursor.
    const double fAnchor = freqForX(e.getPosition().x);
    viewWidthMHz = viewWidthMaxMHz;
    updateSliderRange();
    centerParam->setValue(fAnchor, true);   // silent
    setCenter(centerParam->floatValue());

    rebuildWaterfall();
    retuneView();
    specSmoothDb.clear();
    repaint();
}

//==============================================================================
// Layout

void RFCoordinationPanel::resized()
{
    auto r = getLocalBounds();

    // Device manager fills the whole left side; the header (buttons + status) sits
    // only above the spectrum on the right.
    auto leftCol = r.removeFromLeft(deviceListWidth);
    leftColBounds = leftCol;
    const int headerH = (int)GlobalSettings::getInstance()->fontSize->floatValue() + 8;
    // Profiles on top, devices below, with a draggable divider between them.
    int avail = leftCol.getHeight() - 6;
    if (profileListHeight <= 0) profileListHeight = avail * 2 / 5;       // default proportion
    profileListHeight = juce::jlimit(80, juce::jmax(120, avail - 80), profileListHeight);

    // Each manager fills its whole area; a full-width title bar overlays the top so the
    // manager box wraps around it, and the "+" button floats on top of the bar (right).
    auto layoutHeader = [headerH](juce::Rectangle<int> bar, juce::Component* header, juce::Component* addBtn)
    {
        if (header != nullptr) header->setBounds(bar.reduced(2, 1));
        if (addBtn != nullptr) addBtn->setBounds(bar.removeFromRight(headerH).reduced(3));
    };

    auto profileArea = leftCol.removeFromTop(profileListHeight);
    if (profileListUI != nullptr) profileListUI->setBounds(profileArea);
    layoutHeader(profileArea.removeFromTop(headerH), profileHeader.get(), profileAddBtn);

    if (profileSplitter != nullptr) profileSplitter->setBounds(leftCol.removeFromTop(6));

    if (deviceListUI != nullptr) deviceListUI->setBounds(leftCol);
    layoutHeader(leftCol.removeFromTop(headerH), deviceHeader.get(), deviceAddBtn);
    if (deviceSplitter != nullptr) deviceSplitter->setBounds(r.removeFromLeft(6));

    headerArea = r.removeFromTop(34);
    sliderArea = r.removeFromBottom(28);     // frequency navigation slider
    plotArea = r.reduced(2);

    // Occupancy strip on top, then the large waterfall, then a small FFT trace.
    juce::Rectangle<int> rem = plotArea;
    occArea  = rem.removeFromTop(12);
    specArea = rem.removeFromBottom(juce::jmin(90, rem.getHeight() / 4));
    waterArea = rem;

    auto hr = headerArea.reduced(6, 5);
    analyzeBtn.setBounds(hr.removeFromLeft(90));
    hr.removeFromLeft(6);
    optimizeBtn.setBounds(hr.removeFromLeft(90));

    if (centerSliderUI != nullptr) centerSliderUI->setBounds(sliderArea.reduced(8, 3));

    specSmoothDb.clear();   // x->freq mapping changed

    if (waterArea.getHeight() > 0
        && (waterfall.isNull()
            || waterfall.getHeight() != waterfallRows()
            || waterfall.getWidth() != waterfallColumns()))
    {
        rebuildWaterfall();      // keep existing content, fit the new size
        retuneView();            // match the detail resolution to the new width
    }
}

//==============================================================================
// Paint

void RFCoordinationPanel::drawFreqAxis(juce::Graphics& g)
{
    g.setFont(9.0f);

    // Choose a "nice" tick step so we get ~6-8 labels across the current view.
    double span = viewMaxMHz - viewMinMHz;
    double rough = span / 7.0;
    double pow10 = std::pow(10.0, std::floor(std::log10(juce::jmax(1.0e-6, rough))));
    double step = pow10;
    for (double mult : { 1.0, 2.0, 5.0, 10.0 }) { if (pow10 * mult >= rough) { step = pow10 * mult; break; } }

    double first = std::ceil(viewMinMHz / step) * step;
    for (double f = first; f <= viewMaxMHz + 1e-9; f += step)
    {
        int x = (int)xForFreq(f);
        g.setColour(juce::Colours::white.withAlpha(.10f));
        g.drawVerticalLine(x, (float)plotArea.getY(), (float)plotArea.getBottom());
        g.setColour(juce::Colours::white.withAlpha(.5f));
        int lx = juce::jlimit(plotArea.getX(), plotArea.getRight() - 44, x - 22);
        g.drawText(String(f, (step < 1.0) ? 2 : 0) + " MHz", lx, plotArea.getBottom() - 12, 44, 11, Justification::centred);
    }
}

void RFCoordinationPanel::drawAssignedMarkers(juce::Graphics& g)
{
    const int hoverPx = 6;   // make the thin marker easier to hover
    auto* mgr = RFDeviceManager::getInstance();
    int shownRow = 0;
    for (auto* d : mgr->items)
    {
        if (!d->enabled->boolValue()) continue;
        double f = d->assignedFreqMHz->floatValue();
        if (f < viewMinMHz || f > viewMaxMHz) continue;

        int x = (int)xForFreq(f);
        const bool hovered = mouseInPlot && std::abs(mousePos.x - x) <= hoverPx;
        const bool selected = d->isSelected;

        // Dark by default; the selected device's marker takes the inspector's
        // highlight colour so it's easy to spot which bar it is.
        juce::Colour col = selected ? HIGHLIGHT_COLOR : juce::Colours::orange.darker(0.7f);
        if (hovered) col = col.brighter(0.4f);
        g.setColour(col);

        if (selected || hovered)
            g.fillRect(x - 1, plotArea.getY(), 2, plotArea.getHeight());   // thicker = emphasised
        else
            g.drawVerticalLine(x, (float)plotArea.getY(), (float)plotArea.getBottom());

        // Label shown when hovering the marker or when the device is selected.
        if (!hovered && !selected) continue;

        String label = d->niceName;
        if (d->assignedPresetName->stringValue().isNotEmpty())
            label += " (" + d->assignedPresetName->stringValue() + ")";
        label += "  " + String(f, 3);

        g.setFont(Font(11.0f, Font::bold));
        int tw = g.getCurrentFont().getStringWidth(label) + 8;
        int ly = plotArea.getY() + 4 + (shownRow % 4) * 15;
        int lx = juce::jlimit(plotArea.getX(), plotArea.getRight() - tw, x + 3);
        g.setColour(juce::Colours::black.withAlpha(.75f));
        g.fillRoundedRectangle((float)lx, (float)ly, (float)tw, 14.0f, 3.0f);
        g.setColour(col);
        g.drawText(label, lx + 4, ly, tw - 4, 14, Justification::centredLeft);
        ++shownRow;
    }
}

void RFCoordinationPanel::drawForbiddenZones(juce::Graphics& g)
{
    if (settings == nullptr) return;
    auto& items = settings->allowedRanges->items;
    if (items.isEmpty()) return;   // no range declared -> assignment unrestricted

    // Merge the allowed ranges, then shade the gaps inside the current view.
    std::vector<std::pair<double, double>> allowed;
    for (auto* r : items)
    {
        double lo = r->minMHz->floatValue(), hi = r->maxMHz->floatValue();
        if (hi < lo) std::swap(lo, hi);
        allowed.push_back({ lo, hi });
    }
    std::sort(allowed.begin(), allowed.end());

    auto hatch = [&](double f0, double f1)
    {
        int x0 = juce::jlimit(plotArea.getX(), plotArea.getRight(), (int)xForFreq(f0));
        int x1 = juce::jlimit(plotArea.getX(), plotArea.getRight(), (int)xForFreq(f1));
        if (x1 - x0 < 1) return;
        juce::Rectangle<int> rr(x0, plotArea.getY(), x1 - x0, plotArea.getHeight());

        g.setColour(juce::Colours::red.withAlpha(.14f));
        g.fillRect(rr);

        g.saveState();
        g.reduceClipRegion(rr);
        g.setColour(juce::Colours::red.withAlpha(.30f));
        const int h = rr.getHeight();
        for (int x = rr.getX() - h; x < rr.getRight(); x += 9)
            g.drawLine((float)x, (float)rr.getBottom(), (float)(x + h), (float)rr.getY(), 1.0f);
        g.restoreState();

        g.setColour(juce::Colours::red.withAlpha(.5f));
        g.drawVerticalLine(x0, (float)plotArea.getY(), (float)plotArea.getBottom());
        g.drawVerticalLine(x1, (float)plotArea.getY(), (float)plotArea.getBottom());
    };

    double pos = viewMinMHz;
    for (const auto& a : allowed)
    {
        if (a.first > pos) hatch(pos, juce::jmin(a.first, viewMaxMHz));
        pos = juce::jmax(pos, a.second);
        if (pos >= viewMaxMHz) break;
    }
    if (pos < viewMaxMHz) hatch(pos, viewMaxMHz);
}

void RFCoordinationPanel::drawSpectrumLine(juce::Graphics& g)
{
    auto a = specArea;
    if (a.getWidth() <= 0 || a.getHeight() <= 0) return;

    const juce::Colour lineCol(0xff5fd0ff);   // bright cyan trace (SDR++-ish)

    // Subtle tinted background so the FFT panel reads even when flat (not pure void).
    g.setColour(juce::Colour(0xff0b0d18));
    g.fillRect(a);

    // dB grid + labels.
    g.setFont(9.0f);
    for (float db = -20.0f; db >= -105.0f; db -= 20.0f)
    {
        int y = (int)yForSpecDb(db);
        g.setColour(juce::Colours::white.withAlpha(.07f));
        g.drawHorizontalLine(y, (float)a.getX(), (float)a.getRight());
        g.setColour(juce::Colours::white.withAlpha(.35f));
        g.drawText(juce::String((int)db), a.getX() + 2, y - 10, 30, 10, Justification::topLeft);
    }

    const int W = a.getWidth();

    // Max-hold (occupancy) as a faint peak line.
    if (haveFull && (int)maxHoldDb.size() > 0)
    {
        juce::Path mp; bool started = false;
        for (int i = 0; i < W; ++i)
        {
            int x = a.getX() + i;
            double f = freqForX(x);
            int bin = juce::jlimit(0, (int)maxHoldDb.size() - 1,
                                   (int)((f - liveFull.minMHz) / liveFull.binMHz()));
            float y = yForSpecDb(maxHoldDb[(size_t)bin]);
            if (!started) { mp.startNewSubPath((float)x, y); started = true; }
            else          mp.lineTo((float)x, y);
        }
        g.setColour(juce::Colours::orange.withAlpha(.45f));
        g.strokePath(mp, juce::PathStrokeType(1.0f));
    }

    // Live, temporally-smoothed FFT trace with a gradient fill underneath.
    if ((int)specSmoothDb.size() >= W && W > 1)
    {
        juce::Path lp; float firstX = (float)a.getX(), lastX = firstX;
        for (int i = 0; i < W; ++i)
        {
            float x = (float)(a.getX() + i);
            float y = yForSpecDb(specSmoothDb[(size_t)i]);
            if (i == 0) lp.startNewSubPath(x, y);
            else        lp.lineTo(x, y);
            lastX = x;
        }

        juce::Path fill = lp;
        fill.lineTo(lastX, (float)a.getBottom());
        fill.lineTo(firstX, (float)a.getBottom());
        fill.closeSubPath();
        juce::ColourGradient grad(lineCol.withAlpha(.35f), 0.0f, (float)a.getY(),
                                  lineCol.withAlpha(0.0f), 0.0f, (float)a.getBottom(), false);
        g.setGradientFill(grad);
        g.fillPath(fill);

        g.setColour(lineCol);
        g.strokePath(lp, juce::PathStrokeType(1.5f));
    }
}

void RFCoordinationPanel::drawSpectrum(juce::Graphics& g)
{
    g.setColour(juce::Colours::black);
    g.fillRect(plotArea);

    if (!waterfall.isNull())
    {
        // The image maps [wfBandMin, wfBandMax]; draw the part covered by the current
        // view (usually the whole image, but a stretched preview right after a zoom
        // before the detail rescan lands). High-quality resampling avoids the blocky
        // look when the image is scaled to the plot.
        const double full = juce::jmax(1.0e-6, wfBandMax - wfBandMin);
        const int iw = waterfall.getWidth();
        const int ih = waterfall.getHeight();
        const int sx = (int)std::round((viewMinMHz - wfBandMin) / full * iw);
        const int sw = juce::jmax(1, (int)std::round((viewMaxMHz - viewMinMHz) / full * iw));
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(waterfall,
                    waterArea.getX(), waterArea.getY(), waterArea.getWidth(), waterArea.getHeight(),
                    juce::jlimit(0, juce::jmax(0, iw - 1), sx), 0, juce::jmin(sw, iw), ih);
    }

    // Occupancy strip (full-band peak-hold), interpolated so it's smooth too.
    if (haveFull && (int)maxHoldDb.size() > 0 && occArea.getHeight() > 0)
    {
        RFSpectrumSweep occ = liveFull;
        if ((int)maxHoldDb.size() == occ.numBins()) occ.powerDb = maxHoldDb;
        for (int x = occArea.getX(); x < occArea.getRight(); ++x)
        {
            g.setColour(powerToColour(occ.powerAtInterp(freqForX(x))));
            g.drawVerticalLine(x, (float)occArea.getY(), (float)occArea.getBottom());
        }
    }

    drawSpectrumLine(g);

    drawForbiddenZones(g);
    drawFreqAxis(g);
    drawAssignedMarkers(g);
}

void RFCoordinationPanel::paint(juce::Graphics& g)
{
    g.fillAll(BG_COLOR);

    // Header background + status text.
    g.setColour(BG_COLOR.darker(.1f));
    g.fillRect(headerArea);

    {
        const float prog = optimizeProgress.load();
        if (guidedActive && ! scanWindows.empty())
        {
            // Guided-scan progress: which window, dwell countdown, overall bar.
            const double dwellElapsed = juce::jlimit(0.0, dwellSec(),
                (juce::Time::getMillisecondCounterHiRes() - scanWindowStartMs) / 1000.0);
            const float overall = juce::jlimit(0.0f, 1.0f,
                (float)(scanWindowIndex + dwellElapsed / dwellSec()) / (float)scanWindows.size());
            auto win = scanWindows[(size_t)juce::jlimit(0, (int)scanWindows.size() - 1, scanWindowIndex)];

            auto bar = headerArea.withTrimmedRight(10).removeFromRight(380).reduced(0, 9);
            g.setColour(Colours::black.withAlpha(.4f));
            g.fillRoundedRectangle(bar.toFloat(), 3.0f);
            g.setColour(Colours::orange.withAlpha(.8f));
            g.fillRoundedRectangle(bar.withWidth((int)(bar.getWidth() * overall)).toFloat(), 3.0f);
            const int remaining = (int)std::ceil((scanWindows.size() - scanWindowIndex - 1) * dwellSec()
                                                  + (dwellSec() - dwellElapsed));
            g.setColour(Colours::white);
            g.setFont(Font(11.0f, Font::bold));
            g.drawText("Scan " + String(scanWindowIndex + 1) + "/" + String((int)scanWindows.size())
                       + " -  " + String(win.first, 1) + "-" + String(win.second, 1) + " MHz - "
                       + "  " + String(remaining) + "s left...",
                       bar, Justification::centred);
        }
        else if (prog >= 0.0f)
        {
            // Optimization progress bar (right side of the header).
            auto bar = headerArea.withTrimmedRight(10).removeFromRight(200).reduced(0, 9);
            g.setColour(Colours::black.withAlpha(.4f));
            g.fillRoundedRectangle(bar.toFloat(), 3.0f);
            auto fill = bar.withWidth((int)(bar.getWidth() * juce::jlimit(0.0f, 1.0f, prog)));
            g.setColour(Colours::limegreen.withAlpha(.8f));
            g.fillRoundedRectangle(fill.toFloat(), 3.0f);
            g.setColour(Colours::white);
            g.setFont(Font(12.0f, Font::bold));
            g.drawText("Optimizing  " + String((int)(prog * 100.0f)) + "%", bar, Justification::centred);
        }
        else
        {
            // Surface an RTL-SDR error/status (dongle not found, lost, recovering...)
            // even after it stopped, so the user knows what happened.
            String rtlStatus;
            if (auto* rtl = dynamic_cast<RtlSdrSpectrumSource*>(sourceFull.get()))
            {
                String s = rtl->getStatusMessage();
                bool isError = s.containsIgnoreCase("error") || s.containsIgnoreCase("lost")
                            || s.containsIgnoreCase("cannot") || s.containsIgnoreCase("not compiled");
                if ((analyzing && !haveFull) || isError) rtlStatus = s;
            }

            if (rtlStatus.isNotEmpty())
            {
                g.setColour(Colours::orange.withAlpha(.9f));
                g.setFont(Font(12.0f, Font::bold));
                g.drawText(rtlStatus, headerArea.withTrimmedLeft(192).withTrimmedRight(10), Justification::centredRight);
            }
            else
            {
                int total = (int)scanElapsedSec;
                String t = String::formatted("%02d:%02d:%02d", total / 3600, (total % 3600) / 60, total % 60);
                if (settings != nullptr) t = settings->source->sourceType->getValueKey() + " - " + t;

                g.setColour(Colours::white.withAlpha(analyzing ? .85f : .5f));
                g.setFont(Font(14.0f, Font::bold));
                g.drawText(t, headerArea.withTrimmedRight(10), Justification::centredRight);
            }
        }
    }

    drawSpectrum(g);

    if (!analyzing && !haveFull && !haveDetail)
    {
        g.setColour(Colours::white.withAlpha(.4f));
        g.setFont(15.0f);
        g.drawText("Stopped - press Analyze", plotArea, Justification::centred);
    }

    // Hover: vertical crosshair + frequency / level readout.
    if (mouseInPlot)
    {
        double freq = freqForX(mousePos.x);
        g.setColour(Colours::white.withAlpha(.5f));
        g.drawVerticalLine(mousePos.x, (float)plotArea.getY(), (float)plotArea.getBottom());

        String label = String(freq, 3) + " MHz";
        if (haveFull && (int)maxHoldDb.size() > 0)
        {
            int bin = juce::jlimit(0, (int)maxHoldDb.size() - 1,
                                   (int)((freq - liveFull.minMHz) / liveFull.binMHz()));
            label += "   " + String(maxHoldDb[(size_t)bin], 1) + " dB";
        }

        g.setFont(Font(12.0f, Font::bold));
        int tw = g.getCurrentFont().getStringWidth(label) + 12;
        juce::Rectangle<int> box(juce::jlimit(plotArea.getX(), plotArea.getRight() - tw, mousePos.x + 8),
                                 juce::jlimit(plotArea.getY(), plotArea.getBottom() - 18, mousePos.y - 18),
                                 tw, 16);
        g.setColour(Colours::black.withAlpha(.7f));
        g.fillRoundedRectangle(box.toFloat(), 3.0f);
        g.setColour(Colours::white);
        g.drawText(label, box, Justification::centred);
    }

    // Selection border, matching the device list's (BaseManagerUI uses
    // LIGHTCONTOUR_COLOR, rounded corner 4, thin stroke), shown when the RF
    // settings are selected in the inspector.
    if (settings != nullptr && settings->isSelected)
    {
        g.setColour(LIGHTCONTOUR_COLOR);
        g.drawRoundedRectangle(plotArea.toFloat().reduced(0.5f), 2.0f, 1.0f);
    }
}
