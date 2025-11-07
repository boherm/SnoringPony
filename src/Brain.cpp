/*
  ==============================================================================

    Brain.cpp
    Created: 6 Nov 2025 08:15:00am
    Author:  boherm

  ==============================================================================
*/

#include "Brain.h"
#include "Cuelist/CuelistManager.h"
#include "PonyEngine.h"
#include "Cue/Cue.h"
#include "Cuelist/CuelistManager.h"

juce_ImplementSingleton(Brain);

Brain::Brain() { }

Brain::~Brain() { }

void Brain::go()
{
    Cuelist* cl = dynamic_cast<PonyEngine*>(Engine::mainEngine)->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();
    if (cl)
        cl->go();
}

void Brain::panic()
{
    Cuelist* cl = dynamic_cast<PonyEngine*>(Engine::mainEngine)->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();
    if (cl)
        cl->stop();
}
