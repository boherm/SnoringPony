/*
  ==============================================================================

    CueListUI.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../CueList.h"

class CueListUI :
	public BaseItemUI<CueList>
{
public:
  CueListUI(CueList *item);
  ~CueListUI();

  void paint(Graphics &g) override;
};