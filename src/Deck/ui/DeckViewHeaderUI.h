/*
  ==============================================================================

    DeckViewHeaderUI.h
    Created: 05 Oct 2025 12:56:20am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class Cuelist;

class DeckViewHeaderUI :
    public Component,
    public Button::Listener,
	public ContainerAsyncListener,
    public juce::Timer
{
public:
    DeckViewHeaderUI(Cuelist* cl);
    ~DeckViewHeaderUI() override;

    void timerCallback() override;
    void updatePanicAnimation();
    bool panicFlashState = false;

    std::unique_ptr<juce::ImageButton> addItemBT;
    std::unique_ptr<juce::TextButton> goBtnUI;
    std::unique_ptr<juce::TextButton> panicBtnUI;
    std::unique_ptr<juce::TextButton> mainToggleBT;
    Component* parent;
    Cuelist* currentCuelist;

    void updateMainToggleState();

    void setParent(Component* p) { parent = p; }

    void paint (Graphics&) override;
    void resized() override;
    void mouseDown(const MouseEvent& event);

    void buttonClicked(Button* button) override;
    void newMessage(const ContainerAsyncEvent& e) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeckViewHeaderUI)
};
