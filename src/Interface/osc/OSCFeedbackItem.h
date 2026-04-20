/*
  ==============================================================================

    OSCFeedbackItem.h
    Created: 20 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../feedback/FeedbackItem.h"

class OSCInterface;

class OSCFeedbackItem : public FeedbackItem
{
public:
    OSCFeedbackItem(const String& name = "OSC Feedback");
    virtual ~OSCFeedbackItem();

    StringParameter* oscAddress;

    OSCInterface* oscInterface = nullptr;
    void setOSCInterface(OSCInterface* iface);
};
