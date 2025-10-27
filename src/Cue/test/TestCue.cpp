/*
  ==============================================================================

    TestCue.cpp
    Created: 27 Oct 2025 09:32:20pm
    Author:  boherm

  ==============================================================================
*/

#include "TestCue.h"

TestCue::TestCue(var params) :
    Cue(params)
{
    objectType = "Test";
}

TestCue::~TestCue()
{
}

void TestCue::play()
{
    Logger::writeToLog("TestCue::play: " + niceName);
}
