/*
  ==============================================================================

    MultiCueEditor.h
    Created: 04 Jul 2026
    Author:  boherm

    Editor shown in the Inspector when several cues are selected at once.

    It stays entirely on the app side (no framework changes): each section
    reuses the framework's GenericControllableContainerEditor to render a
    single "representative" cue, and value edits are propagated to the other
    selected cues at the model level (see MultiCueEditor::newMessage).

    Layout (Option B):
      - a "Common" section with the parameters shared by every selected cue
        (base Cue params: id, description, notes, pre/post-wait, retrigger...),
      - one section per cue type ("2 Audio Cues", "1 OSC Cue"...) with the
        parameters specific to that type.

    When every selected cue has the same type the intersection covers all
    parameters, so a single full section is shown.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "MultiCueSync.h"

class Cue;

// Filtering rules for a multi-edit section, applied recursively at every depth.
struct MultiEditFilter
{
    juce::StringArray hiddenNames;    // shortNames to hide (leaves or sub-containers), any depth
    bool hideDisabledLeaves = true;   // hide enabled==false leaves (runtime displays, e.g. current time)
};

// A GenericControllableContainerEditor that renders a container while hiding items per a
// shared MultiEditFilter (recursively into sub-containers). At the section root it also
// applies a top-level name filter (common vs type-specific split) and shows a custom title.
class FilteredCueContainerEditor :
    public GenericControllableContainerEditor
{
public:
    FilteredCueContainerEditor(ControllableContainer* container, bool isTopLevel,
                               std::shared_ptr<MultiEditFilter> filter,
                               std::function<bool(const juce::String&)> topNameFilter,
                               const juce::String& headerText);
    ~FilteredCueContainerEditor() override;

    bool isTopLevel;
    std::shared_ptr<MultiEditFilter> filter;
    std::function<bool(const juce::String&)> topNameFilter;
    juce::String headerText;

    // Enable toggle for EnablingControllableContainers (pre/post-wait, duck, retrigger,
    // MTC...): normally drawn by EnablingControllableContainerEditor, which our recursive
    // filtered editor replaces, so we redraw it here.
    std::unique_ptr<BoolToggleUI> enabledUI;

    bool shouldShowControllable(Controllable* c) override;
    bool shouldShowContainer(ControllableContainer* cc) override;
    InspectableEditor* getEditorUIForContainer(ControllableContainer* cc) override;
    void resizedInternalHeader(juce::Rectangle<int>& r) override;
    void controllableFeedbackUpdate(Controllable* c) override;

    // Number of controllables/containers that pass the filter (0 => empty section).
    int getVisibleItemCount();

    void newMessage(const ContainerAsyncEvent& e) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilteredCueContainerEditor)
};


class MultiCueEditor :
    public InspectableEditor,
    public ContainerAsyncListener
{
public:
    MultiCueEditor(juce::Array<Cue*> cues, bool isRoot);
    ~MultiCueEditor() override;

    // Only cues belonging to the same cuelist as the first one (single-cuelist rule).
    // Stored as Inspectable weak refs (WeakReference<Cue> is not available since the
    // weak-ref master lives on the Inspectable/ControllableContainer base); use
    // asCue() to access them as Cue*.
    juce::Array<juce::WeakReference<Inspectable>> cues;

    // Top-level shortNames present in every selected cue (the "common" set).
    juce::StringArray commonNames;

    // Top-level shortNames hidden in multi-edit: the generic value mirror ignores changes
    // under these (they are not user-editable here, or handled by a dedicated extra
    // editor, e.g. audio files), so programmatic changes don't get cross-written.
    juce::StringArray mirrorExcludedTopNames;

    // Representative cue of each type group; these are the cues we listen to for
    // mirroring (they are the ones actually rendered/edited).
    juce::Array<juce::WeakReference<Inspectable>> repCues;

    juce::OwnedArray<FilteredCueContainerEditor> sections;
    // Optional per-type extra editors (e.g. audio bulk output/volume).
    juce::OwnedArray<juce::Component> extras;
    // Optional per-type structural syncs (e.g. DCA assignments).
    juce::OwnedArray<MultiCueSync> syncs;
    // Sections + extras in vertical display order.
    juce::Array<juce::Component*> layoutItems;

    // True when every selected cue is a Fade cue (duration is then a shared editable field).
    bool allFade = false;

    // Re-entrancy guard while we push mirrored values.
    bool isMirroring = false;
    bool isRebuilding = false;

    void resized() override;
    void childBoundsChanged(juce::Component* c) override;

    // ContainerAsyncListener: mirror value edits from a representative cue to the
    // other selected cues in the relevant scope, with grouped undo.
    void newMessage(const ContainerAsyncEvent& e) override;

    static Cue* asCue(const juce::WeakReference<Inspectable>& w);

private:
    void buildSections();
    // Append rep's optional bulk-edit extra component (applied to scopeCues) after its section.
    void addExtraFor(Cue* rep, const juce::Array<Cue*>& scopeCues);
    // Install rep's optional structural sync (e.g. DCA assignments) across scopeCues.
    void addSyncFor(Cue* rep, const juce::Array<Cue*>& scopeCues);
    juce::Array<Cue*> cuesOfType(const juce::String& type) const;

    // Ordered, de-duplicated list of cue types across the selection (first-seen order).
    juce::StringArray getOrderedTypes() const;

    static juce::String firstAddressSegment(const juce::String& address);
    static Cue* getCueAncestor(Controllable* c);
    static juce::String pluralizeCues(int count, const juce::String& typeName);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiCueEditor)
};
