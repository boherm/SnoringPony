/*
  ==============================================================================

    ShowProperties.h
    Created: 24 Sep 2025 03:33:00pm
    Author:  boherm

  ==============================================================================
*/

#include "ShowProperties.h"

juce_ImplementSingleton(ShowProperties)

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
    showFileVersion = addStringParameter("Show file version", "Version of this project file 2", "1.0");
}

ShowProperties::~ShowProperties()
{
    delete projectName;
    delete companyName;
    delete showFileVersion;
}

void ShowProperties::clear()
{
    projectName->resetValue();
    companyName->resetValue();
    showFileVersion->resetValue();
}
