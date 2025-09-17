#include "ShowControl.h"
#include "../../MainIncludes.h"

//==============================================================================
ShowControlUI::ShowControlUI(const String &contentName): ShapeShifterContent(ShowControl::getInstance(), contentName) {
}

ShowControlUI::~ShowControlUI() {
}

juce_ImplementSingleton(ShowControl);

ShowControl::ShowControl(): ControllableContainer("Show Control") {
    paramGo = addTrigger("GO", "GO the next cue");
    paramPanic = addTrigger("Panic", "Push in case of emergency!");
    btnGo = paramGo->createButtonUI();
    btnPanic = paramPanic->createButtonUI();
    addAndMakeVisible(btnGo);
    addAndMakeVisible(btnPanic);
    btnGo->customBGColor = Colour(0, 139, 0);
    btnGo->useCustomBGColor = true;

    btnPanic->customBGColor = Colour(159, 0, 0);
    btnPanic->useCustomBGColor = true;
}

ShowControl::~ShowControl() {
    delete btnGo;
    delete btnPanic;
}

void ShowControl::paint(juce::Graphics &g) {
}

void ShowControl::resized() {
    btnGo->setBounds(0, 0, getLocalBounds().getWidth() / 2, getLocalBounds().getHeight());
    btnPanic->setBounds(getLocalBounds().getWidth() / 2, 0, getLocalBounds().getWidth() / 2,
                        getLocalBounds().getHeight());
}

void ShowControl::triggerTriggered(Trigger *t) {
    Logger::getCurrentLogger()->writeToLog("Push: " + t->niceName);
    // UserInputManager::getInstance()->processInput(t->niceName);
}
