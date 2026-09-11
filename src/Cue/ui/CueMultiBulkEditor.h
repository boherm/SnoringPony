/*
  ==============================================================================

    CueMultiBulkEditor.h
    Created: 11 Sep 2026
    Author:  boherm

    Base class for the bulk-edit blocks of the multi-cue editor.

    A bulk block is backed by a detached "proxy" container: the controls the user
    sees belong to that proxy, not to any cue, and each edit is pushed to the real
    parameters of every cue in scope in a single undo step. That way a control can
    exist without mapping to one particular cue (e.g. an "unset" volume that writes
    nothing until it is moved).

    It is an InspectableEditor on purpose: the Inspector culls off-screen editors
    (InspectableEditor::updateVisibility) and only recurses into children that are
    themselves InspectableEditors, so a plain Component here leaves the inner editor
    stuck invisible. Its inspectable is the item it stands in for (the audio files
    manager, the volume parameter...), which also lets the hosting section drive its
    visibility from `inspectable->hideInEditor` like any other row.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class Cue;

class CueMultiBulkEditor :
    public InspectableEditor,
    public ContainerAsyncListener
{
public:
    // `anchor` is the inspectable this block is rendered in place of (its position in the
    // section, and the visibility source). `proxyName` names the proxy container, shown as
    // the block header when `renderAsContainer` is true; when false the proxy parameters are
    // drawn as bare rows, so a substituted parameter looks like a regular parameter line.
    CueMultiBulkEditor(Inspectable* anchor, const juce::Array<Cue*>& scopeCues,
                       const juce::String& proxyName, bool renderAsContainer);
    ~CueMultiBulkEditor() override;

    // The cues this block drives (weak, as Inspectable).
    juce::Array<juce::WeakReference<Inspectable>> cues;

    // Detached proxy container backing the visible controls.
    std::unique_ptr<ControllableContainer> proxy;
    bool renderAsContainer;

    std::unique_ptr<GenericControllableContainerEditor> proxyEditor; // renderAsContainer
    juce::OwnedArray<InspectableEditor> paramEditors;                // !renderAsContainer

    // True while we push proxy values onto the cues, to ignore our own echoes.
    bool isApplying = false;

    void resized() override;
    void childBoundsChanged(juce::Component* c) override;
    void newMessage(const ContainerAsyncEvent& e) override;

protected:
    // Call once the proxy parameters have been added, at the end of the subclass ctor.
    void buildProxyEditor();

    // Write `newVal` to every target parameter in one grouped undo step. Targets already
    // holding that value are skipped, and so are parameters that are not in MANUAL control
    // mode (a volume linked to a volume preset must keep its reference).
    void applyToParameters(const juce::Array<Parameter*>& targets, const juce::var& newVal,
                           const juce::String& undoLabel);

    // Called when the user edits `changedProxyParam`; push it to the cues.
    virtual void applyProxyChange(Parameter* changedProxyParam) = 0;

private:
    bool isLayingOut = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CueMultiBulkEditor)
};
