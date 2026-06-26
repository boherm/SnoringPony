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
#include "../../RF/SimulatedRFSpectrumSource.h"
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
        addExistingItems();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFDeviceManagerUI)
};

// Thin draggable divider between the device list and the spectrum.
class RFPanelSplitter : public juce::Component
{
public:
    std::function<void(int)> onDragTo;   // called with the cursor's X in the parent

    RFPanelSplitter() { setMouseCursor(juce::MouseCursor::LeftRightResizeCursor); }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(BG_COLOR.darker(.2f));
        g.setColour(BG_COLOR.brighter(isMouseOverOrDragging() ? .35f : .12f));
        float cx = getWidth() * 0.5f;
        g.drawLine(cx, 4.0f, cx, (float)getHeight() - 4.0f, 1.5f);
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override  { repaint(); }
    void mouseDrag(const juce::MouseEvent& e) override { if (onDragTo) onDragTo(getX() + e.x); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFPanelSplitter)
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

    sourceFull.reset(new SimulatedRFSpectrumSource());
    sourceDetail.reset(new SimulatedRFSpectrumSource());

    analyzeBtn.onClick  = [this] { if (analyzing) stopAnalyze(); else startAnalyze(); };
    optimizeBtn.onClick = [this] { runOptimize(); };

    for (auto* b : { &analyzeBtn, &optimizeBtn })
    {
        b->setColour(juce::TextButton::buttonColourId, BG_COLOR.brighter(.08f));
        b->setColour(juce::TextButton::textColourOffId, TEXT_COLOR);
        addAndMakeVisible(b);
    }
    optimizeBtn.setTooltip("Assign a conflict-free frequency to every declared device (needs >= 1 min of scan)");

    deviceListUI.reset(new RFDeviceManagerUI());
    addAndMakeVisible(deviceListUI.get());

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
    }

    if (Engine::mainEngine != nullptr)
        Engine::mainEngine->addEngineListener(this);

    applyBandFromSettings();
    updateButtons();
    startTimerHz(30);
}

RFCoordinationPanel::~RFCoordinationPanel()
{
    stopTimer();

    if (settings != nullptr)
    {
        settings->scanMinMHz->removeParameterListener(this);
        settings->scanMaxMHz->removeParameterListener(this);
    }

    if (Engine::mainEngine != nullptr)
        Engine::mainEngine->removeEngineListener(this);

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

    resetView();
    rebuildWaterfall();   // size the image to the current view before wiping
    clearGraphs();
    scanElapsedSec = 0.0;
    lastTickMs = juce::Time::getMillisecondCounterHiRes();
    analyzing = true;

    startSources();
    updateButtons();
}

void RFCoordinationPanel::startSources()
{
    // Full-band scan at the base resolution (occupancy / optimizer)...
    const double baseBinHz = 100000.0; // 100 kHz
    if (sourceFull != nullptr)
    {
        sourceFull->setReferenceBand(bandMinMHz, bandMaxMHz);
        sourceFull->start(bandMinMHz, bandMaxMHz, baseBinHz);
    }
    // ...plus a fine detail scan of just the visible range for the waterfall.
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

    // Optimize is always available: without a scan, frequencies inside the allowed
    // ranges are assumed free.
    optimizeBtn.setEnabled(true);
}

//==============================================================================

void RFCoordinationPanel::timerCallback()
{
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
        }

        // Detail scan of the visible range feeds the waterfall.
        RFSpectrumSweep detail;
        if (sourceDetail != nullptr && sourceDetail->getLatestSweep(detail))
        {
            liveDetail = detail;
            haveDetail = true;
            commitWaterfallColumn();
        }

        // Apply a pending (debounced) detail retune after the view settled.
        if (detailDirty && now >= detailRetuneAtMs)
            retuneDetail();
    }

    updateButtons();

    // While analyzing the waterfall changes every frame; otherwise only repaint
    // when something displayed actually changed (allowed ranges, device
    // enable/disable, assignments, ...), so an idle panel costs nothing.
    if (analyzing) repaint();
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

void RFCoordinationPanel::commitWaterfallColumn()
{
    if (waterfall.isNull() || !haveDetail) return;

    const int w = waterfall.getWidth();
    const int h = waterfall.getHeight();
    if (w <= 0 || h <= 0) return;

    // Scroll the existing content up by one row; the newest row appears at the
    // bottom (time flows bottom -> top).
    waterfall.moveImageSection(0, 0, 0, 1, w, h - 1);

    // The image maps the range it was last built for (wfBand == the view); sample
    // the detail sweep across it.
    const double span = wfBandMax - wfBandMin;
    for (int x = 0; x < w; ++x)
    {
        double freq = wfBandMin + ((double)x + 0.5) / (double)w * span; // low freq -> left
        float db = liveDetail.powerAt(freq);
        waterfall.setPixelAt(x, h - 1, powerToColour(db));
    }
}

int RFCoordinationPanel::waterfallColumns() const
{
    // ~2 image columns per plot pixel: crisp at any zoom, bounded RAM (the image
    // tracks the view, not the whole band).
    return juce::jlimit(256, 4096, plotArea.getWidth() * 2);
}

double RFCoordinationPanel::detailBinHz() const
{
    return (viewMaxMHz - viewMinMHz) * 1.0e6 / (double)juce::jmax(1, waterfallColumns());
}

void RFCoordinationPanel::scheduleDetailRetune()
{
    detailDirty = true;
    detailRetuneAtMs = juce::Time::getMillisecondCounterHiRes() + 120.0; // debounce zoom/pan
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
    const int h = plotArea.getHeight();
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

void RFCoordinationPanel::runOptimize()
{
    if (settings == nullptr) return;

    // Clear every device's assignment first (including disabled ones), so a device
    // that is no longer placed doesn't keep a stale frequency.
    for (auto* d : RFDeviceManager::getInstance()->items)
        d->clearAssignment();

    // Occupancy = the full-band max-hold accumulated over the scan. With no scan,
    // an empty sweep is used so every frequency inside the allowed ranges is free.
    RFSpectrumSweep occ;
    if (haveFull && !liveFull.powerDb.empty())
    {
        occ = liveFull;
        if ((int)maxHoldDb.size() == occ.numBins()) occ.powerDb = maxHoldDb;
    }

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

    FrequencyOptimizer::Result res = FrequencyOptimizer::optimize(occ, inputs, c);

    // Apply the assignments back onto the devices.
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
    float norm = juce::jlimit(0.0f, 1.0f, (db + 105.0f) / 85.0f);
    float bright = std::pow(norm, 0.5f);
    return juce::Colour::fromHSV(0.66f * (1.0f - norm), 1.0f, bright, 1.0f);
}

void RFCoordinationPanel::resetView()
{
    viewMinMHz = bandMinMHz;
    viewMaxMHz = bandMaxMHz;
}

void RFCoordinationPanel::setView(double a, double b)
{
    if (b < a) std::swap(a, b);

    const double full = juce::jmax(1.0e-6, bandMaxMHz - bandMinMHz);
    const double minSpan = juce::jmin(full, 0.5);   // never zoom tighter than 0.5 MHz

    if (b - a < minSpan) { double c = (a + b) * 0.5; a = c - minSpan * 0.5; b = c + minSpan * 0.5; }
    if (b - a > full)    { a = bandMinMHz; b = bandMaxMHz; }

    // Slide the window back inside the scanned band if it spills over an edge.
    if (a < bandMinMHz) { b += bandMinMHz - a; a = bandMinMHz; }
    if (b > bandMaxMHz) { a -= b - bandMaxMHz; b = bandMaxMHz; }

    viewMinMHz = juce::jmax(a, bandMinMHz);
    viewMaxMHz = juce::jmin(b, bandMaxMHz);
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
    if (settings == nullptr) return;
    if (p == settings->scanMinMHz || p == settings->scanMaxMHz)
        applyBandFromSettings();
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
    if (settings != nullptr && plotArea.contains(e.getPosition()))
    {
        settings->selectThis();
        repaint();
    }
}

void RFCoordinationPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (!plotArea.contains(e.getPosition())) return;

    const double span = viewMaxMHz - viewMinMHz;

    // Horizontal wheel (or Shift+wheel) pans; vertical wheel zooms on the cursor.
    const bool pan = e.mods.isShiftDown() || std::abs(wheel.deltaX) > std::abs(wheel.deltaY);

    if (pan)
    {
        double d = (std::abs(wheel.deltaX) > std::abs(wheel.deltaY)) ? wheel.deltaX : wheel.deltaY;
        if (wheel.isReversed) d = -d;
        const double shift = d * span * 0.5;
        setView(viewMinMHz + shift, viewMaxMHz + shift);
    }
    else
    {
        double dy = wheel.deltaY;
        if (wheel.isReversed) dy = -dy;

        const double full = bandMaxMHz - bandMinMHz;
        const double newSpan = juce::jlimit(juce::jmin(full, 0.5), full, span * std::exp(-dy * 1.5));

        // Keep the frequency under the cursor anchored while zooming.
        const double fAnchor = freqForX(e.getPosition().x);
        const double frac = (fAnchor - viewMinMHz) / juce::jmax(1.0e-9, span);
        const double a = fAnchor - frac * newSpan;
        setView(a, a + newSpan);
    }

    rebuildWaterfall();      // immediate (stretched) preview of the new view
    scheduleDetailRetune();  // fine rescan of the new view, debounced
    repaint();
}

void RFCoordinationPanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (plotArea.contains(e.getPosition()))
    {
        resetView();
        rebuildWaterfall();
        scheduleDetailRetune();
        repaint();
    }
}

//==============================================================================
// Layout

void RFCoordinationPanel::resized()
{
    auto r = getLocalBounds();

    // Device manager fills the whole left side; the header (buttons + status) sits
    // only above the spectrum on the right.
    auto leftCol = r.removeFromLeft(deviceListWidth);
    if (deviceListUI != nullptr) deviceListUI->setBounds(leftCol);
    if (deviceSplitter != nullptr) deviceSplitter->setBounds(r.removeFromLeft(6));

    headerArea = r.removeFromTop(34);
    plotArea = r.reduced(2);

    auto hr = headerArea.reduced(6, 5);
    analyzeBtn.setBounds(hr.removeFromLeft(90));
    hr.removeFromLeft(6);
    optimizeBtn.setBounds(hr.removeFromLeft(90));

    if (plotArea.getHeight() > 0
        && (waterfall.isNull()
            || waterfall.getHeight() != plotArea.getHeight()
            || waterfall.getWidth() != waterfallColumns()))
    {
        rebuildWaterfall();      // keep existing content, fit the new size
        scheduleDetailRetune();  // match the detail resolution to the new width
    }
}

//==============================================================================
// Paint

void RFCoordinationPanel::drawFreqAxis(juce::Graphics& g)
{
    g.setFont(9.0f);

    // Choose a "nice" tick step so we get ~8-10 labels across the band.
    double span = bandMaxMHz - bandMinMHz;
    double rough = span / 9.0;
    double pow10 = std::pow(10.0, std::floor(std::log10(juce::jmax(1.0e-6, rough))));
    double step = pow10;
    for (double mult : { 1.0, 2.0, 5.0, 10.0 }) { if (pow10 * mult >= rough) { step = pow10 * mult; break; } }

    double first = std::ceil(bandMinMHz / step) * step;
    for (double f = first; f <= bandMaxMHz + 1e-9; f += step)
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

void RFCoordinationPanel::drawSpectrum(juce::Graphics& g)
{
    g.setColour(juce::Colours::black);
    g.fillRect(plotArea);

    if (!waterfall.isNull())
    {
        // The image maps [wfBandMin, wfBandMax]; draw the part covered by the current
        // view (usually the whole image, but a stretched preview right after a zoom
        // before the detail rescan lands).
        const double full = juce::jmax(1.0e-6, wfBandMax - wfBandMin);
        const int iw = waterfall.getWidth();
        const int ih = waterfall.getHeight();
        const int sx = (int)std::round((viewMinMHz - wfBandMin) / full * iw);
        const int sw = juce::jmax(1, (int)std::round((viewMaxMHz - viewMinMHz) / full * iw));
        g.drawImage(waterfall,
                    plotArea.getX(), plotArea.getY(), plotArea.getWidth(), plotArea.getHeight(),
                    juce::jlimit(0, juce::jmax(0, iw - 1), sx), 0, juce::jmin(sw, iw), ih);
    }

    // Occupancy strip (full-band peak-hold) along the top edge, so occupied regions
    // stay visible even as the waterfall scrolls.
    if (haveFull && (int)maxHoldDb.size() > 0)
    {
        const int stripH = 10;
        for (int x = plotArea.getX(); x < plotArea.getRight(); ++x)
        {
            double freq = freqForX(x);
            int bin = juce::jlimit(0, (int)maxHoldDb.size() - 1,
                                   (int)((freq - liveFull.minMHz) / liveFull.binMHz()));
            g.setColour(powerToColour(maxHoldDb[(size_t)bin]));
            g.drawVerticalLine(x, (float)plotArea.getY(), (float)(plotArea.getY() + stripH));
        }
        g.setColour(juce::Colours::white.withAlpha(.5f));
        g.setFont(9.0f);
        g.drawText("occupancy (recent peak)", plotArea.getX() + 3, plotArea.getY() + stripH + 1, 160, 11, Justification::topLeft);
    }

    drawForbiddenZones(g);
    drawFreqAxis(g);
    drawAssignedMarkers(g);

    // Zoom hint (only when not showing the full band).
    if (viewMaxMHz - viewMinMHz < (bandMaxMHz - bandMinMHz) - 1.0e-3)
    {
        g.setColour(juce::Colours::white.withAlpha(.5f));
        g.setFont(9.0f);
        g.drawText(String(viewMinMHz, 2) + " - " + String(viewMaxMHz, 2) + " MHz  (double-click to reset)",
                   plotArea.getX(), plotArea.getY() + 1, plotArea.getWidth() - 4, 11, Justification::topRight);
    }
}

void RFCoordinationPanel::paint(juce::Graphics& g)
{
    g.fillAll(BG_COLOR);

    // Header background + status text.
    g.setColour(BG_COLOR.darker(.1f));
    g.fillRect(headerArea);

    {
        int total = (int)scanElapsedSec;
        String t = String::formatted("%02d:%02d:%02d", total / 3600, (total % 3600) / 60, total % 60);
        if (settings != nullptr) t = settings->sourceType->getValueKey() + " - " + t;

        g.setColour(Colours::white.withAlpha(analyzing ? .85f : .5f));
        g.setFont(Font(14.0f, Font::bold));
        g.drawText(t, headerArea.withTrimmedRight(10), Justification::centredRight);
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
