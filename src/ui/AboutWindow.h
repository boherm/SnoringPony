/*
  ==============================================================================

    AboutWindow.h
    Created: 4 May 2025 3:33:04pm
    Author:  Boris Hermans

  ==============================================================================
*/

#pragma once

class AboutWindow :
	public Component
{
public:
	AboutWindow();
	~AboutWindow();

	Image aboutImage;

	void paint(Graphics &g) override;
};
