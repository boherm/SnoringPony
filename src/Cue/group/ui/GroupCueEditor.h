/*
  ==============================================================================

    GroupCueEditor.h
    Created: 05 Jul 2026
    Author:  boherm

    Timeline view for a GroupCue: one lane per sub-cue, positioned by its pre-wait
    offset and sized by its duration. Dragging a sub-cue horizontally sets its pre-wait.

    It is hosted in its own collapsible "Timeline" container (GroupTimelineContainer),
    added to the GroupCue only in Timeline fire mode, so it shows up as a proper titled
    section in the inspector rather than floating below the parameters.

  ==============================================================================
*/

#pragma once

#include <map>

#include "../../../MainIncludes.h"
#include "../../ui/CueEditor.h"

class GroupCue;
class Cue;

// The draggable timeline strip: a horizontal ruler + one lane per member.
class GroupTimeline :
    public juce::Component,
    public ContainerAsyncListener,
    public juce::ChangeListener,
    public juce::TooltipClient
{
public:
    GroupTimeline(GroupCue* group);
    ~GroupTimeline() override;

    GroupCue* group;

    // Height needed to show the ruler + one lane per member.
    int preferredHeight() const;

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseMagnify(const juce::MouseEvent& e, float scaleFactor) override;

    void newMessage(const ContainerAsyncEvent& e) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override; // thumbnail loaded

    // Hovered sub-cue's "id · name" — reveals it when the block is too small to read.
    juce::String getTooltip() override;

    // The timeline lives in a Viewport; the editor gives it the scroll host and the visible
    // width so it can size itself (visible width * zoom) and scroll on zoom.
    void setViewport(juce::Viewport* v) { viewport = v; }
    void layoutForViewport(int visibleWidth);
    // Invoked when the member count changes, so the host editor can re-reserve height.
    std::function<void()> onMembersChanged;

private:
    juce::Viewport* viewport = nullptr;
    double zoom = 1.0;         // 1.0 = fit the viewport width; larger = zoomed in + scroll
    int fitWidth = 0;          // viewport visible width (zoom==1 width)
    int lastMemberCount = -1;
    void applyZoom();          // resize self to fitWidth * zoom
    void zoomAt(float anchorX, double factor); // zoom keeping the time under anchorX fixed

    // Cached audio waveforms drawn inside Audio Cue blocks, keyed by file path.
    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache { 32 };
    std::map<juce::String, std::unique_ptr<juce::AudioThumbnail>> thumbnails;
    juce::AudioThumbnail* thumbnailFor(const juce::File& file);

    // The member whose lane contains point p (component coords), or nullptr.
    Cue* memberAtY(float y, int& indexOut) const;

    // Sub-cue currently being dragged, and the seconds between the grab point and its start.
    Cue* dragged = nullptr;
    double grabOffset = 0.0;

    // Timeline span (seconds) and time<->pixel mapping over the drawable area.
    double span() const;
    juce::Rectangle<float> plotArea() const;
    float timeToX(double t) const;
    double xToTime(float x) const;

    juce::Rectangle<float> laneRect(int index) const;
    juce::Rectangle<float> blockRect(Cue* m, int index) const;

    static double memberStart(const Cue* m);
    static double memberDuration(const Cue* m);
};

// ------------------------------------------------------------------------------

// A lightweight container whose sole purpose is to give the timeline a titled,
// collapsible home in the inspector. Added to the GroupCue only in Timeline mode.
class GroupTimelineContainer :
    public ControllableContainer
{
public:
    GroupTimelineContainer(GroupCue* group);
    ~GroupTimelineContainer() override;

    GroupCue* group;

    InspectableEditor* getEditorInternal(bool isRoot, juce::Array<Inspectable*> inspectables) override;
};

// ------------------------------------------------------------------------------

class GroupTimelineEditor :
    public GenericControllableContainerEditor
{
public:
    GroupTimelineEditor(juce::Array<ControllableContainer*> containers, bool isRoot);
    ~GroupTimelineEditor() override;

    GroupTimelineContainer* timelineContainer = nullptr;
    std::unique_ptr<GroupTimeline> timeline;
    juce::Viewport viewport; // hosts the timeline so it can zoom + scroll horizontally

    void resizedInternalContent(juce::Rectangle<int>& r) override;
};
