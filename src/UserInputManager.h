//
// Created by Boris Hermans on 17/09/2025.
//
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