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
#include "CuesTableUI.h"

class DeckViewUI :
    public Component,
    public ParameterListener,
    public Button::Listener,
	public ContainerAsyncListener
{
public:
    DeckViewUI(const String &deckName);
    ~DeckViewUI() override;

    std::unique_ptr<CuesTableUI> cuesTable;
    std::unique_ptr<juce::ImageButton> addItemBT;

    String deckName;
    Cuelist* currentCuelist;

    void setCurrentCuelist(Cuelist* cl);
    void paint (Graphics&) override;
    void resized() override;
    void parameterValueChanged(Parameter* parameter) override;
    void mouseDown(const MouseEvent& event);

    TargetParameter* getAssociatedTargetParameter();

    void buttonClicked(Button* button) override;

	 void newMessage(const ContainerAsyncEvent& e) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeckViewUI)
};
