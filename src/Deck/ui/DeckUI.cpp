/*
  ==============================================================================

    DeckUI.cpp
    Created: 27 Sep 2025 05:23:00am
    Author:  boherm

  ==============================================================================
*/

#include "DeckUI.h"
#include "DeckViewUI.h"

DeckUI::DeckUI(const String &contentName) :
    ShapeShifterContent(new DeckViewUI(contentName), contentName)
{
    Logger::writeToLog("DeckUI created: " + contentName);
}

DeckUI::~DeckUI()
{
    delete contentComponent;
}
