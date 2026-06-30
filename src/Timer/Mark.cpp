/*
  ==============================================================================

    Mark.cpp
    Created: 30 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "Mark.h"

Mark::Mark(var params) :
    BaseItem(params.getProperty("name", "Mark"))
{
    itemDataType = "Mark";
    canBeDisabled = false;
    showInspectorOnSelect = false;

    duration = addFloatParameter("Duration", "Time elapsed since the previous mark", 0.0, 0.0);
    duration->defaultUI = FloatParameter::TIME;
    duration->setControllableFeedbackOnly(true);
    duration->hideInEditor = true; // painted by MarkUI instead

    cumulative = addFloatParameter("Cumulative", "Total elapsed time at this mark", 0.0, 0.0);
    cumulative->defaultUI = FloatParameter::TIME;
    cumulative->setControllableFeedbackOnly(true);
    cumulative->hideInEditor = true; // painted by MarkUI instead
}

Mark::~Mark()
{
}
