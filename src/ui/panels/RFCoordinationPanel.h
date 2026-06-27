/*
  ==============================================================================

    RFCoordinationPanel.h
    Created: 26 Jun 2026
    Author:  boherm

    RF (wireless mic / IEM) frequency coordination panel. "Analyze" runs a live
    spectrum scan (Phase 1: simulated source) shown as a waterfall scrolling
    bottom-to-top, low frequencies on the left and high on the right, with a
    max-hold occupancy trace. After at least one minute of scanning, "Optimize"
    assigns a conflict-free frequency to every declared device (see the RF
    Coordination Settings in the project settings) and marks them on the spectrum.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../../RF/RFSpectrumSource.h"
#include "../../RF/FrequencyOptimizer.h"

class RFCoordinationSettings;

class RFCoordinationUI : public ShapeShifterContent {
public:
    RFCoordinationUI(const String &contentName);
    ~RFCoordinationUI();

    static RFCoordinationUI *create(const String &name) { return new RFCoordinationUI(name); }
};

class RFCoordinationPanel :
    public juce::Component,
    public juce::Timer,
    public ParameterListener,
    public EngineListener
{
public:
    juce_DeclareSingleton(RFCoordinationPanel, true);
    RFCoordinationPanel();
    ~RFCoordinationPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    void parameterValueChanged(Parameter* p) override;

    void fileLoaded() override;
    void engineCleared() override;

private:
    static constexpr double scanRequiredSec = 60.0;
    static constexpr double occupancyDbSpan = 80.0;   // reference span the cool-down fades over
    static constexpr double viewWidthMaxMHz = 10.0;   // widest view (default)
    static constexpr double viewWidthMinMHz = 0.5;    // most zoomed-in view
    double viewWidthMHz = 10.0;                         // current view width (wheel zoom)

    RFCoordinationSettings* settings = nullptr;
    std::unique_ptr<RFSpectrumSource> sourceFull;     // full band -> occupancy / optimizer
    std::unique_ptr<RFSpectrumSource> sourceDetail;   // current view -> waterfall display

    juce::TextButton analyzeBtn { "Analyze" };
    juce::TextButton optimizeBtn { "Optimize" };

    // Centre-frequency navigation, rendered as an inspector-style organic slider.
    std::unique_ptr<FloatParameter> centerParam;
    std::unique_ptr<juce::Component> centerSliderUI;
    bool canNavigate = true;       // band wider than the view (slider usable)

    std::unique_ptr<juce::Component> profileHeader;    // "RF Profiles" title bar
    std::unique_ptr<juce::Component> deviceHeader;     // "RF Devices" title bar
    std::unique_ptr<juce::Component> profileListUI;    // profiles (top of left column)
    std::unique_ptr<juce::Component> deviceListUI;     // devices (bottom of left column)
    juce::Component* profileAddBtn = nullptr;          // reparented onto the panel, on top of the header
    juce::Component* deviceAddBtn = nullptr;
    std::unique_ptr<juce::Component> deviceSplitter;   // draggable divider (left | spectrum)
    std::unique_ptr<juce::Component> profileSplitter;  // draggable divider (profiles / devices)
    int deviceListWidth = 280;
    int profileListHeight = 0;                         // 0 = use default proportion
    juce::Rectangle<int> leftColBounds;

    bool   analyzing = false;
    double scanElapsedSec = 0.0;
    double lastTickMs = 0.0;

    double bandMinMHz = 470.0, bandMaxMHz = 870.0;   // full scanned band
    double viewMinMHz = 470.0, viewMaxMHz = 870.0;   // zoomed/panned view window

    // Full-band scan -> occupancy (peak-hold) used by the strip, hover and optimizer.
    RFSpectrumSweep liveFull;
    bool   haveFull = false;
    std::vector<float> maxHoldDb;   // per full-band bin

    // Detail scan of the current view -> the waterfall image.
    RFSpectrumSweep liveDetail;
    bool   haveDetail = false;
    bool   detailDirty = false;     // view changed: retune the detail source (debounced)
    double detailRetuneAtMs = 0.0;

    bool   sourceRestartPending = false;   // gain/resolution/device changed: restart source
    double sourceRestartAtMs = 0.0;

    juce::Image waterfall;
    double wfBandMin = 0.0, wfBandMax = 0.0;   // range the waterfall image currently maps

    std::vector<float> specSmoothDb;           // temporally-smoothed FFT trace (per plot pixel)

    juce::Rectangle<int> headerArea, plotArea; // plotArea = occupancy strip + waterfall + FFT
    juce::Rectangle<int> waterArea, occArea, specArea, sliderArea;
    juce::Point<int> mousePos { -1, -1 };
    bool  mouseInPlot = false;

    bool   plotDragging = false;    // drag the plot to pan the view
    int    dragStartX = 0;
    double dragStartCenter = 0.0;

    double lastDisplaySig = -1.0;   // change-detection for idle repaints
    void*  lastSelectedDevicePtr = nullptr;   // to centre on a newly-selected device

    // Background optimization + progress (-1 = idle, 0..1 = running).
    std::unique_ptr<juce::Thread> optimizeThread;
    std::atomic<float> optimizeProgress { -1.0f };
    std::atomic<bool>  optimizeDone { false };
    FrequencyOptimizer::Result optimizeResult;
    juce::CriticalSection optimizeLock;
    void applyOptimizeResult();

    // Guided optimization scan: step scan windows over the devices' candidate ranges,
    // dwelling on each to capture occupancy, then place the devices.
    static constexpr double guidedDwellSec = 5.0;     // default dwell per scan window
    double dwellSec() const;                           // configured dwell (Scan Dwell setting)
    static constexpr double guidedWindowMHz = 10.0;   // guided scan window width
    bool   guidedActive = false;
    std::vector<std::pair<double, double>> scanWindows;
    int    scanWindowIndex = 0;
    double scanWindowStartMs = 0.0;
    RFSpectrumSweep guidedOcc;

    std::vector<std::pair<double, double>> computeScanRanges() const;
    void startGuidedScan();
    void cancelGuidedScan();
    void gotoScanWindow(int i);
    void accumulateGuided(const RFSpectrumSweep& s);
    void finishGuidedScan();
    void launchOptimize(const RFSpectrumSweep& occ);
    const RFSpectrumSweep* displaySweep() const;

    void startAnalyze();
    void stopAnalyze();
    void runOptimize();
    void clearGraphs();
    void updateButtons();
    double computeDisplaySignature() const;   // cheap hash to detect idle changes
    void createSources();                      // (re)create sources for the current source type
    void commitWaterfallColumn(const RFSpectrumSweep& s);
    void rebuildWaterfall();               // resize/retune the view image, preserving content
    int  waterfallColumns() const;         // image width (oversampled for a crisp render)
    int  waterfallRows() const;            // image height (oversampled vertically)
    double detailBinHz() const;            // simulated detail resolution for the current view
    double rtlBinHz() const;               // RTL-SDR resolution: finer as you zoom in
    void scheduleDetailRetune();           // mark the detail source for a debounced retune
    void retuneDetail();                   // (re)start the detail source over the view
    void retuneView();                     // follow the view: simulated debounce / RTL-SDR live
    void startSources();                   // configure + start both scan sources
    void setCenter(double centerMHz);             // widest view centred here, clamped to band
    void updateSliderRange();                      // sync the slider to the scanned band
    void resetView();
    void applyBandFromSettings();                 // read Scan Min/Max, reconfigure live
    double occupancyDecayRate() const;            // dB/s from the cool-down setting

    double xForFreq(double mhz) const;
    double freqForX(int x) const;
    float  yForDb(float db) const;
    static juce::Colour powerToColour(float db);

    void drawSpectrum(juce::Graphics& g);
    void drawSpectrumLine(juce::Graphics& g);   // smooth FFT trace + gradient fill (top area)
    float yForSpecDb(float db) const;
    void drawFreqAxis(juce::Graphics& g);
    void drawAssignedMarkers(juce::Graphics& g);
    void drawForbiddenZones(juce::Graphics& g);   // red hatch outside the allowed ranges

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFCoordinationPanel)
};
