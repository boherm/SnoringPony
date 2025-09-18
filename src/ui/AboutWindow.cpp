/*
  ==============================================================================

    AboutWindow.cpp
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

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
