/*
  ==============================================================================

    DeckViewUI.h
    Created: 27 Sep 2025 05:23:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../../Cuelist/Cuelist.h"

class DeckViewUI :
    public Component,
    public ParameterListener
{
public:
    DeckViewUI(const String &deckName);
    ~DeckViewUI() override;

    String deckName;
    Cuelist* currentCuelist;

    void setCurrentCuelist(Cuelist* cl);
    void paint (Graphics&) override;
    // void resized() override;
    void parameterValueChanged(Parameter* parameter) override;
    void mouseDown(const MouseEvent& event);

    TargetParameter* getAssociatedTargetParameter();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeckViewUI)
};

