/*
  ==============================================================================

    UserInputManager.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "MainIncludes.h"

class UserInputManager:
    public OSCRemoteControl::RemoteControlListener
{
public:
    juce_DeclareSingleton(UserInputManager, true);

    UserInputManager();
    ~UserInputManager();

    void processInput(String s);
    void processMessage(const juce::OSCMessage& m, const juce::String& clientId) override;
};