/*
  ==============================================================================

    SPUnicode.h
    Created: 04 Jul 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace SPUnicode
{
    // Normalize a string to Unicode NFC (precomposed). macOS stores filenames in NFD
    // (e.g. "e" + combining acute instead of "é"), which JUCE's text rendering composes
    // poorly; normalizing to NFC fixes the display. No-op on other platforms.
    juce::String toNFC(const juce::String& s);
}
