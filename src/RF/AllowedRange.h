/*
  ==============================================================================

    AllowedRange.h
    Created: 26 Jun 2026
    Author:  boherm

    A legally usable frequency range for assignment (e.g. 823-832 MHz). The
    optimizer only assigns inside the declared ranges; the spectrum marks
    everything outside them as forbidden. With no range declared, assignment is
    unrestricted.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class AllowedRange :
    public BaseItem
{
public:
    AllowedRange(var params = var());
    virtual ~AllowedRange();

    FloatParameter* minMHz;
    FloatParameter* maxMHz;

    String getTypeString() const override { return "Allowed Range"; }
    static AllowedRange* create(var params) { return new AllowedRange(params); }
};
