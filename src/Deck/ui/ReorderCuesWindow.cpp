/*
  ==============================================================================

    ReorderCuesWindow.cpp
    Created: 16 Dec 2025 10:31:21am
    Author:  boherm

  ==============================================================================
*/

#include "ReorderCuesWindow.h"
#include "CuesTableModel.h"
#include "juce_gui_basics/juce_gui_basics.h"

juce_ImplementSingleton(ReorderCuesWindow)

ReorderCuesWindow::ReorderCuesWindow()
{
    TextButton exitBtn("Close");
    container.addAndMakeVisible(&exitBtn);
}

ReorderCuesWindow::~ReorderCuesWindow()
{
}

void ReorderCuesWindow::paint(juce::Graphics& g)
{
}

void ReorderCuesWindow::resized()
{
}

void ReorderCuesWindow::showWindow(CuesTableModel* caller)
{
    this->caller = caller;
    setSize(250, 250);
    DialogWindow::showDialog("Reordering cues", this, &ShapeShifterManager::getInstance()->mainContainer, BG_COLOR, true);
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&container);
    viewport.setBounds(0, 0, 250, 250);
    container.setSize(250 - 10, 200);
}
