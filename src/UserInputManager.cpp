/*
  ==============================================================================

    UserInputManager.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "UserInputManager.h"
#include "MainIncludes.h"
#include "Brain.h"

juce_ImplementSingleton(UserInputManager);

void UserInputManager::processInput(String s) {
	Logger::getCurrentLogger()->writeToLog("processInput: " + s);
}

void UserInputManager::processMessage(const juce::OSCMessage& m, const juce::String& clientId)
{
    // Parse address sended
    StringArray aList;
    String address = m.getAddressPattern().toString().toLowerCase();
	aList.addTokens(address, "/", "\"");
    aList.removeEmptyStrings();
    //DBG("processMessage: " + m.getAddressPattern().toString());

    String domain = aList[0];

    // Show Control ----
    if (domain == "showcontrol") {
        if (aList.size() < 2) return;
        String action = aList[1];
        if (action == "go") { // Go action
            Brain::getInstance()->go();
        } else if (action == "panic") { // Panic action
            Brain::getInstance()->panic();
        }
    }
}
