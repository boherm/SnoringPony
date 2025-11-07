/*
  ==============================================================================

    CuelistEditor.h
    Created: 06 Nov 2025 06:02:00pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class Cuelist;

class CuelistEditor :
    public BaseItemEditor
{
public:
    CuelistEditor(Array<Cuelist*> cuelists, bool isRoot);
    ~CuelistEditor();

    TriggerUI* goBtnUI;
    TriggerUI* stopBtnUI;

    virtual void resizedInternalHeaderItemInternal(Rectangle<int> &r) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CuelistEditor)
};
