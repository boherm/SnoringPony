/*
  ==============================================================================

    OSCFeedbackItem.cpp
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "OSCFeedbackItem.h"
#include "OSCInterface.h"

OSCFeedbackItem::OSCFeedbackItem(const String& name) :
    FeedbackItem(name)
{
    oscAddress = addStringParameter("OSC Address", "OSC address pattern for this feedback", "/feedback");
}

OSCFeedbackItem::~OSCFeedbackItem()
{
}

void OSCFeedbackItem::setOSCInterface(OSCInterface* iface)
{
    oscInterface = iface;
    bindCuelist();
}
