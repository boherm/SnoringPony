/*
  ==============================================================================

    CuelistManagerUI.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../CuelistManager.h"
#include "CuelistManagerItemUI.h"

class CuelistManagerUI:
	public BaseManagerShapeShifterUI<CuelistManager, Cuelist, CuelistManagerItemUI>
{
public:
  CuelistManagerUI(const String &contentName);
  ~CuelistManagerUI();

  static CuelistManagerUI *create(const String &name) {
    return new CuelistManagerUI(name); 
  }
};