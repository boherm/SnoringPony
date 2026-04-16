/*
  ==============================================================================

    Brain.cpp
    Created: 6 Nov 2025 08:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "MainIncludes.h"

class Cuelist;

class Brain
{
public:
    juce_DeclareSingleton(Brain, true)
    Brain() {}
	virtual ~Brain() {}

    void go();
    void panic();
    void selectNextCue();
    void selectPreviousCue();

    static void selectNextCueOn(Cuelist* cl);
    static void selectPreviousCueOn(Cuelist* cl);
};
