/*
  ==============================================================================

    DCAMixingCuelistEditor.h
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../ui/CuelistEditor.h"

class DCAMixingCuelist;

class DCAMixingCuelistEditor :
    public CuelistEditor
{
public:
    DCAMixingCuelistEditor(Array<DCAMixingCuelist*> cuelists, bool isRoot);
    ~DCAMixingCuelistEditor();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DCAMixingCuelistEditor)
};
