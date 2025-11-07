/*
  ==============================================================================

    Brain.cpp
    Created: 6 Nov 2025 08:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "MainIncludes.h"

class Brain
{
public:
    juce_DeclareSingleton(Brain, true)
    Brain();
	~Brain();

    void go();
    void panic();
};
