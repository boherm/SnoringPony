/*
  ==============================================================================

    AudioWaveformSlicerEditor.h
    Created: 22 Dec 2025 04:26:23am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../../MainIncludes.h"
#include "../AudioSlices.h"

class AudioCue;
class AudioWaveformSlicer;

class AudioWaveform:
    public Component,
    public ChangeListener,
    public ContainerAsyncListener,
    public Timer
{
public:
    AudioWaveform(AudioCue* audioCue);
    virtual ~AudioWaveform();

    AudioCue* audioCue;

    int zoomLevel = 1;

    bool mouseDragging = false;
    Parameter* parameterResizing = nullptr;

    std::unique_ptr<FileChooser> chooser;
    AudioFormatManager formatManager;
    std::unique_ptr<AudioFormatReaderSource> readerSource;
    AudioTransportSource transportSource;
    AudioThumbnailCache thumbnailCache;
    AudioThumbnail thumbnail;

    double visibleStartTime = 0.0;
    double visibleEndTime = 0.0;

    void loadFile(File file);

    void paint(Graphics& g) override;
    void resized() override;

    void changeListenerCallback (ChangeBroadcaster* source) override;

    float timeToX(double t, Rectangle<float> area);
    static double chooseGridStepSeconds (double visibleSpanSeconds, int pixelWidth);
    static String formatTime(double seconds);

    void drawTimeSlices (Graphics& g, Rectangle<float> area);
    void drawSlices(Graphics& g);
    void drawCurrentTime(Graphics& g);

    bool isMouseHovering = false;
    float mouseX = 0.0f;
    float selectedX = 0.0f;
    bool shiftSelection = false;
    float startSelectionX = 0.0f;
    // void mouseEnter(const MouseEvent& event) override;
    // void mouseExit(const MouseEvent& event) override;
    void mouseMove(const MouseEvent& event) override;
    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseUp(const MouseEvent& event) override;

    // void mouseMagnify(const MouseEvent& event, float scaleFactor) override;

    void modifierKeysChanged(const ModifierKeys& modifiers) override;

    void newMessage(const ContainerAsyncEvent& e) override;

    void timerCallback() override;
	// void controllableFeedbackUpdate(ControllableContainer*, Controllable*) override;
    // void newMessage(const BaseManager<AudioSlice>::ManagerEvent& e) override;
};

// ------------------------------------------------------------------------------

class AudioWaveformSlicerEditor :
    public GenericControllableContainerEditor
{
public:
    AudioWaveformSlicerEditor(juce::Array<ControllableContainer*> containers, bool isRoot);
    virtual ~AudioWaveformSlicerEditor();

    AudioWaveformSlicer* audioWaveformContainer;
    std::unique_ptr<AudioWaveform> waveformC;

    Viewport viewport;
    TriggerButtonUI* zoomInUI = nullptr;
    TriggerButtonUI* zoomOutUI = nullptr;
    TriggerButtonUI* previewUI = nullptr;

    void refreshThumbnail();
    void resizedInternalHeader(juce::Rectangle<int>& r) override;
    void resizedInternalContent(juce::Rectangle<int>& r) override;

    void zoomIn();
    void zoomOut();
};
