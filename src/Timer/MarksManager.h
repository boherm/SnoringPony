/*
  ==============================================================================

    MarksManager.h
    Created: 30 Jun 2026
    Author:  boherm

    Holds the marks of a single ShowTimer. Runtime-only: excluded from the project
    save (includeInRecursiveSave = false) and not addable manually by the user.

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"
#include "Mark.h"

class MarksManager :
    public BaseManager<Mark>
{
public:
    MarksManager();
    ~MarksManager();

    // Append a mark with the given label, segment duration and cumulative time.
    Mark* addMark(const String& label, double duration, double cumulative);
};
