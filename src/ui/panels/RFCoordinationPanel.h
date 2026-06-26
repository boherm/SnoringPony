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
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    void parameterValueChanged(Parameter* p) override;

    void fileLoaded() override;
    void engineCleared() override;

private:
    static constexpr double scanRequiredSec = 60.0;
    static constexpr double occupancyDbSpan = 80.0;   // reference span the cool-down fades over

    RFCoordinationSettings* settings = nullptr;
    std::unique_ptr<RFSpectrumSource> sourceFull;     // full band -> occupancy / optimizer
    std::unique_ptr<RFSpectrumSource> sourceDetail;   // current view -> waterfall display

    juce::TextButton analyzeBtn { "Analyze" };
    juce::TextButton optimizeBtn { "Optimize" };

    std::unique_ptr<juce::Component> deviceListUI;     // embedded RFDeviceManager UI (left)
    std::unique_ptr<juce::Component> deviceSplitter;   // draggable divider
    int deviceListWidth = 280;

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

    juce::Image waterfall;
    double wfBandMin = 0.0, wfBandMax = 0.0;   // range the waterfall image currently maps

    juce::Rectangle<int> headerArea, plotArea;
    juce::Point<int> mousePos { -1, -1 };
    bool  mouseInPlot = false;

    double lastDisplaySig = -1.0;   // change-detection for idle repaints

    void startAnalyze();
    void stopAnalyze();
    void runOptimize();
    void clearGraphs();
    void updateButtons();
    double computeDisplaySignature() const;   // cheap hash to detect idle changes
    void commitWaterfallColumn();
    void rebuildWaterfall();               // resize/retune the view image, preserving content
    int  waterfallColumns() const;         // image width: ~2 px-worth of bins over the view
    double detailBinHz() const;            // detail resolution for the current view
    void scheduleDetailRetune();           // mark the detail source for a debounced retune
    void retuneDetail();                   // (re)start the detail source over the view
    void startSources();                   // configure + start both scan sources
    void setView(double minMHz, double maxMHz);   // clamps to the scanned band
    void resetView();
    void applyBandFromSettings();                 // read Scan Min/Max, reconfigure live
    double occupancyDecayRate() const;            // dB/s from the cool-down setting

    double xForFreq(double mhz) const;
    double freqForX(int x) const;
    float  yForDb(float db) const;
    static juce::Colour powerToColour(float db);

    void drawSpectrum(juce::Graphics& g);
    void drawFreqAxis(juce::Graphics& g);
    void drawAssignedMarkers(juce::Graphics& g);
    void drawForbiddenZones(juce::Graphics& g);   // red hatch outside the allowed ranges

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFCoordinationPanel)
};
