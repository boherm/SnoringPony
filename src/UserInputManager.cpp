/*
  ==============================================================================

    UserInputManager.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "UserInputManager.h"
#include "MainIncludes.h"

juce_ImplementSingleton(UserInputManager);

UserInputManager::UserInputManager()
{
}

UserInputManager::~UserInputManager()
{
}

void UserInputManager::processInput(String s) {
	Logger::getCurrentLogger()->writeToLog("processInput: " + s);
}

void UserInputManager::processMessage(const juce::OSCMessage& m, const juce::String& clientId)
{
	Logger::getCurrentLogger()->writeToLog("processMessage: " + m.getAddressPattern().toString());
}