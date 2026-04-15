/*
  ==============================================================================

    DCAMixingCuelistEditor.cpp
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "DCAMixingCuelistEditor.h"
#include "../DCAMixingCuelist.h"

DCAMixingCuelistEditor::DCAMixingCuelistEditor(Array<DCAMixingCuelist*> cuelists, bool isRoot) :
    CuelistEditor(Inspectable::getArrayAs<DCAMixingCuelist, Cuelist>(cuelists), isRoot)
{
}

DCAMixingCuelistEditor::~DCAMixingCuelistEditor()
{
}
