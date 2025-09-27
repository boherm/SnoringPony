/*
  ==============================================================================

    DeckUI.h
    Created: 27 Sep 2025 05:23:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class DeckUI : 
    public ShapeShifterContent {
    public:
        DeckUI(const String &contentName);
        ~DeckUI();

        static DeckUI *create(const String &name) {
            return new DeckUI(name);
        }
};
