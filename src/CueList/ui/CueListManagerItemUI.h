/*
  ==============================================================================

    CueListManagerItemUI.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../CueList.h"

class CueListManagerItemUI :
	public BaseItemUI<CueList>
{
public:
CueListManagerItemUI(CueList *item);
  ~CueListManagerItemUI();

  void paint(Graphics &g) override;
};