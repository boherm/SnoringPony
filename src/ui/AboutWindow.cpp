/*
  ==============================================================================

    AboutWindow.cpp
    Created: 4 May 2025 3:33:10pm
    Author:  Boris Hermans

  ==============================================================================
*/

#include "../MainIncludes.h"
#include "AboutWindow.h"
#include "SPAssetManager.h"

AboutWindow::AboutWindow() :
	Component("About")
{
	aboutImage = SPAssetManager::getInstance()->getAboutImage();
	setSize(aboutImage.getWidth(), aboutImage.getHeight());
	setSize(800, 480);
}

AboutWindow::~AboutWindow()
{
}

void AboutWindow::paint(Graphics& g)
{
	g.fillAll(BG_COLOR.darker());
	g.drawImage(aboutImage, getLocalBounds().toFloat());

	g.setColour(TEXT_COLOR);
	g.setFont(12);
	g.drawText(
			   "version " + getApp().getApplicationVersion(),
			   getLocalBounds().removeFromBottom(30).removeFromRight(200).reduced(5).toFloat(),
			   Justification::right
	);
}
