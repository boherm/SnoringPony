/*
  ==============================================================================

    Mark.h
    Created: 30 Jun 2026
    Author:  boherm

    A Mark is a timestamped marker on a ShowTimer's timeline (e.g. one per scene).
    Its name is the label; "duration" is the time elapsed since the previous mark and
    "cumulative" the total elapsed time at that point. Marks are runtime-only: they live
    in a MarksManager that is excluded from the project save.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class Mark :
    public BaseItem
{
public:
    Mark(var params = var());
    virtual ~Mark();

    FloatParameter* duration;   // time since the previous mark (read-only)
    FloatParameter* cumulative; // total elapsed time at this mark (read-only)

    String getTypeString() const override { return "Mark"; }
    static Mark* create(var params) { return new Mark(params); }
};
