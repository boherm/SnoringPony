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

class Cuelist;

class ReorderCuesWindow :
    public Component,
    public Button::Listener
{
public:
    ReorderCuesWindow(CuesTableModel* caller);
    ~ReorderCuesWindow() {}

    void resized() override;

    CuesTableModel* caller = nullptr;

    TextButton closeBtn {"Close"};
    TextButton reorderBtn {"Reorder Cues"};
    Component bottomArea;

    Label startIdLabel { {}, "Start ID" };
    TextEditor startIdEdit;
    Label incrementLabel { {}, "Increment" };
    TextEditor incrementEdit;

    void buttonClicked(Button*);

    float getFloatFromEditor(const juce::TextEditor& ed, float fallback) const;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReorderCuesWindow)
};
