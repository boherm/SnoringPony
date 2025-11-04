/*
  ==============================================================================

    ShowProperties.h
    Created: 24 Sep 2025 03:33:00pm
    Author:  boherm

  ==============================================================================
*/

#include "ShowProperties.h"
#include "../Cuelist/CuelistManager.h"
#include "juce_organicui/controllable/Controllable.h"

ShowProperties::ShowProperties() :
    ControllableContainer("Show Properties"),
    projectName(nullptr),
    companyName(nullptr),
    showFileVersion(nullptr)
{
    saveAndLoadRecursiveData = true;
    hideInRemoteControl = true;
    defaultHideInRemoteControl = true;

    projectName = addStringParameter("Project name", "Name of the show project", "Untitled Show");
    companyName = addStringParameter("Company name", "Company name of the show project", "Unknown Company");
    showFileVersion = addStringParameter("Show file version", "Version of this project file", "1.0");

    startingNextCue = addTargetParameter("Next cue on load", "Target of the next cue to go on loading this project file", CuelistManager::getInstance());
    startingNextCue->targetType = TargetParameter::CONTAINER;
    startingNextCue->customGetTargetContainerFunc = &CuelistManager::showMenuForTargetCue;

    nextCueToGo = addTargetParameter("Next cue to go", "Target of the next cue to go", CuelistManager::getInstance());
    nextCueToGo->targetType = TargetParameter::CONTAINER;
    nextCueToGo->isSavable = false;
    nextCueToGo->customGetTargetContainerFunc = &CuelistManager::showMenuForTargetCue;
    nextCueToGo->alwaysNotify = true;
    nextCueToGo->hideInEditor = true;
}

void ShowProperties::clear()
{
    projectName->resetValue();
    companyName->resetValue();
    showFileVersion->resetValue();

    startingNextCue->resetValue();
    nextCueToGo->resetValue();
}

void ShowProperties::parameterValueChanged(Parameter* p)
{
    ControllableContainer::parameterValueChanged(p);

    if (!Engine::mainEngine->isLoadingFile) return;

    if (p == startingNextCue) {
        if (startingNextCue->getTargetContainer() != nullptr) {
            nextCueToGo->setTarget(startingNextCue->getTargetContainer());
            nextCueToGo->notifyValueChanged();
        }
    }
}
