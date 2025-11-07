/*
  ==============================================================================

    CuelistManagerItemUI.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../Cuelist.h"
#include "../../Cue/Cue.h"

class CuelistManagerItemUI :
	public BaseItemUI<Cuelist>,
    public BaseManagerListener<Cue>
{
    public:
        CuelistManagerItemUI(Cuelist *item);
        ~CuelistManagerItemUI();

        void paint(Graphics &g) override;

        void itemAdded(Cue*) { repaint(); }
        void itemRemoved(Cue*) { repaint(); }
};
