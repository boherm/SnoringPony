/*
  ==============================================================================

    AllowedRange.cpp
    Created: 26 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "AllowedRange.h"

AllowedRange::AllowedRange(var params) :
    BaseItem(params.getProperty("name", "Range"))
{
    itemDataType = "Allowed Range";
    canBeDisabled = false;

    minMHz = addFloatParameter("Min", "Lower edge of the allowed range, in MHz", 823.0f, 1.0f, 6000.0f);
    maxMHz = addFloatParameter("Max", "Upper edge of the allowed range, in MHz", 832.0f, 1.0f, 6000.0f);
}

AllowedRange::~AllowedRange()
{
}
