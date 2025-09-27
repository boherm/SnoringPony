/*
  ==============================================================================

    DeckViewUI.h
    Created: 27 Sep 2025 05:23:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class DeckViewUI :
    public Component
{
public:
    DeckViewUI(const String &deckName);
    ~DeckViewUI() override;

    String deckName;

    void paint (Graphics&) override;
    // void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeckViewUI)
};

