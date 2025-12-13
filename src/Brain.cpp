/*
  ==============================================================================

    Brain.cpp
    Created: 6 Nov 2025 08:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "Brain.h"
#include "PonyEngine.h"
#include "Cue/Cue.h"
#include "Cuelist/Cuelist.h"
#include "ui/panels/ShowControl.h"

juce_ImplementSingleton(Brain);

void Brain::go()
{
    const MessageManagerLock mmLock;
    Cuelist* cl = dynamic_cast<PonyEngine*>(Engine::mainEngine)->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();
    if (cl != nullptr)
        cl->go();
}

void Brain::panic()
{
    const MessageManagerLock mmLock;
    Cuelist* cl = dynamic_cast<PonyEngine*>(Engine::mainEngine)->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();
    if (cl != nullptr) {
        cl->panic();
        ShowControl::getInstance()->startPanicking();
    }
}
