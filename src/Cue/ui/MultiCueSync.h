/*
  ==============================================================================

    MultiCueSync.h
    Created: 04 Jul 2026
    Author:  boherm

    Base for a per-type "structural" multi-edit sync: mirrors state the generic
    value mirror (MultiCueEditor::newMessage) cannot handle, e.g. adding/removing
    items in a manager. A concrete sync listens to the representative cue and
    applies changes to the other selected cues of the same type. It is owned by
    the MultiCueEditor for the lifetime of the selection.

  ==============================================================================
*/

#pragma once

class MultiCueSync
{
public:
    virtual ~MultiCueSync() {}
};
