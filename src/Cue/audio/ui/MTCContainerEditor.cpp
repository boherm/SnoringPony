/*
  ==============================================================================

    MTCContainerEditor.cpp
    Created: 21 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "MTCContainerEditor.h"
#include "../AudioCue.h"

MTCContainerEditor::MTCContainerEditor(Array<ControllableContainer*> cc, bool isRoot) :
    EnablingControllableContainerEditor(Inspectable::getArrayAs<ControllableContainer, EnablingControllableContainer>(cc), isRoot),
    audioCue(nullptr)
{
    if (cc.size() > 0)
        audioCue = dynamic_cast<AudioCue*>(cc[0]->parentContainer.get());

    timecodeLabel.setText("00:00:00:00", dontSendNotification);
    timecodeLabel.setColour(timecodeLabel.backgroundColourId, BG_COLOR.darker(.1f).withAlpha(.4f));
    timecodeLabel.setColour(timecodeLabel.outlineColourId, BG_COLOR.brighter(.1f));
    timecodeLabel.setColour(timecodeLabel.textColourId, Colours::white.withAlpha(.7f));
    timecodeLabel.setFont(Font(headerHeight - 6));
    timecodeLabel.setJustificationType(Justification::centred);
    timecodeLabel.setEditable(false);
    addAndMakeVisible(&timecodeLabel);
}

MTCContainerEditor::~MTCContainerEditor()
{
}

void MTCContainerEditor::resizedInternalHeader(Rectangle<int>& r)
{
    int labelWidth = timecodeLabel.getFont().getStringWidth("00:00:00:00") + 16;
    timecodeLabel.setBounds(r.removeFromRight(jmin(labelWidth, getWidth() - 100)).reduced(1, 2));
    r.removeFromRight(4);
    EnablingControllableContainerEditor::resizedInternalHeader(r);
}

void MTCContainerEditor::controllableFeedbackUpdate(Controllable* c)
{
    if (audioCue != nullptr && c == audioCue->mtcTimecodeDisplay)
    {
        String tc = audioCue->mtcTimecodeDisplay->stringValue();
        bool isRunning = (tc != "00:00:00:00");
        timecodeLabel.setText(tc, dontSendNotification);
        timecodeLabel.setColour(timecodeLabel.textColourId, isRunning ? Colours::green.brighter(.1f) : Colours::white.withAlpha(.7f));
        return;
    }

    EnablingControllableContainerEditor::controllableFeedbackUpdate(c);
}
