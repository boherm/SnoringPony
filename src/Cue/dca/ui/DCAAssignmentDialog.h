/*
  ==============================================================================

    DCAAssignmentDialog.h
    Created: 14 Apr 2026
    Author:  boherm

    Modal dialog letting the user pick which characters belong to a given
    DCA within a DCACue. Characters already assigned to another DCA in the
    same cue are greyed out.

  ==============================================================================
*/

#pragma once

#include "../../../MainIncludes.h"

class DCACue;
class DCAAssignment;
class Character;

class DCAAssignmentDialog :
    public juce::Component,
    public juce::Button::Listener,
    public juce::TextEditor::Listener,
    public juce::Slider::Listener
{
public:
    DCAAssignmentDialog(DCACue* cue, DCAAssignment* assignment);
    ~DCAAssignmentDialog();

    DCACue* cue;
    DCAAssignment* assignment;

    std::unique_ptr<juce::Label> nameLabel;
    std::unique_ptr<juce::TextEditor> nameEditor;
    std::unique_ptr<juce::Label> globalFXLabel;
    std::unique_ptr<juce::TextButton> globalFXBtn;
    std::unique_ptr<juce::Label> charactersLabel;
    std::unique_ptr<juce::Viewport> listViewport;
    std::unique_ptr<juce::Component> listContent;
    juce::OwnedArray<juce::ToggleButton> charButtons;
    juce::OwnedArray<juce::TextButton> fxButtons;
    juce::Array<Character*> characters;
    std::unique_ptr<juce::ToggleButton> forceFaderBtn;
    std::unique_ptr<juce::Slider> faderSlider;
    std::unique_ptr<juce::Label> faderValueLabel;
    std::unique_ptr<juce::TextButton> closeBtn;

    void buttonClicked(juce::Button* b) override;
    void textEditorTextChanged(juce::TextEditor&) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void paint(juce::Graphics& g) override;
    void resized() override;

    void rebuildCharacterList();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DCAAssignmentDialog)
};
