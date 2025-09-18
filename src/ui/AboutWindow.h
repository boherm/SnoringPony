/*
  ==============================================================================

    AboutWindow.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class AboutWindow :
	public Component
{
public:
	AboutWindow();
	~AboutWindow();

	Image aboutImage;

	void paint(Graphics &g) override;
};
