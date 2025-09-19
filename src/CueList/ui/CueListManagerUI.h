/*
  ==============================================================================

    CueListManagerUI.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../CueListManager.h"
#include "CueListManagerItemUI.h"

class CueListManagerUI:
	public BaseManagerShapeShifterUI<CueListManager, CueList, CueListManagerItemUI>
{
public:
  CueListManagerUI(const String &contentName);
  ~CueListManagerUI();

  static CueListManagerUI *create(const String &name) {
    return new CueListManagerUI(name); 
  }
};