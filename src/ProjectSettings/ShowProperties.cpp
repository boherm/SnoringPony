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

    startupMainCuelist = addTargetParameter("Startup main cuelist", "Target of the main cuelist to load on starting the show", CuelistManager::getInstance());
    startupMainCuelist->targetType = TargetParameter::CONTAINER;
    startupMainCuelist->maxDefaultSearchLevel = 0;

    mainCuelist = addTargetParameter("Current main cuelist", "Target the current main cuelist", CuelistManager::getInstance());
    mainCuelist->targetType = TargetParameter::CONTAINER;
    mainCuelist->maxDefaultSearchLevel = 0;
    mainCuelist->isSavable = false;
    mainCuelist->alwaysNotify = true;
}

void ShowProperties::clear()
{
    projectName->resetValue();
    companyName->resetValue();
    showFileVersion->resetValue();

    startupMainCuelist->resetValue();
    mainCuelist->resetValue();
}

void ShowProperties::parameterValueChanged(Parameter* p)
{
    ControllableContainer::parameterValueChanged(p);

    if (!Engine::mainEngine->isLoadingFile) return;

    if (p == startupMainCuelist) {
        if (startupMainCuelist->getTargetContainer() != nullptr) {
            mainCuelist->setTarget(startupMainCuelist->getTargetContainer());
            mainCuelist->notifyValueChanged();
        }
    }
}
