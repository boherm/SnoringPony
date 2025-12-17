/*
  ==============================================================================

    ReorderCuesWindow.h
    Created: 16 Dec 2025 10:31:21am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "CuesTableModel.h"
#include "juce_gui_basics/juce_gui_basics.h"

class Cuelist;

class ReorderCuesWindow :
    public Component //,
    // public Button::Listener
{
public:
    juce_DeclareSingleton(ReorderCuesWindow, true);
    ReorderCuesWindow();
    ~ReorderCuesWindow() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    CuesTableModel* caller = nullptr;

    // void closeWindow();
    // bool windowOpened = false;

    Viewport viewport;
    Component container;
    TextButton exitBtn;

    // void loadCuelist(Cuelist* c);
    // void showWindow();

    // OwnedArray<TextButton> buttons;
    // Array<float> cueIds;
    // Cuelist* currentTarget = nullptr;

    // TextButton exitBtn;
    void showWindow(CuesTableModel* caller);
    // void fillButtons(Cuelist * c);
    // void loadCuelist(Cuelist * c, bool triggerGoWhenSelected = false);
    // void buttonClicked(Button*);
    // bool triggerGo = false;

    // int posX = 0;
    // int posY = 0;

    // juce::CriticalSection buttonsLock;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReorderCuesWindow)
};
