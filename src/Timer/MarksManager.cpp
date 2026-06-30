/*
  ==============================================================================

    MarksManager.cpp
    Created: 30 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "MarksManager.h"

MarksManager::MarksManager() :
    BaseManager<Mark>("Marks")
{
    itemDataType = "Mark";
    userCanAddItemsManually = false; // marks are only created by the timer (cue / Mark button)
    selectItemWhenCreated = false;
    includeInRecursiveSave = false;  // runtime-only, never saved (like the elapsed time)
}

MarksManager::~MarksManager()
{
}

Mark* MarksManager::addMark(const String& label, double duration, double cumulative)
{
    Mark* m = new Mark();
    addItem(m, var(), false); // no undo for runtime marks

    m->setNiceName(label);
    m->duration->setValue(duration);
    m->cumulative->setValue(cumulative);
    return m;
}
