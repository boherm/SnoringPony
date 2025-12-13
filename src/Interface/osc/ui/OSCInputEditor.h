/*
  ==============================================================================

    OSCInputEditor.h
    Created: 9 Dec 2025 11:07:20pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../../MainIncludes.h"

class OSCInputEditor :
    public EnablingControllableContainerEditor
{
public:
    OSCInputEditor(Array<ControllableContainer*> cc, bool isRoot);
    ~OSCInputEditor();

    void resizedInternalHeader(Rectangle<int>& r) override;
    Label ipLabel;

    static InspectableEditor * create(bool isRoot, Array<ControllableContainer*>cc ) { return new OSCInputEditor(cc, isRoot); }
};
