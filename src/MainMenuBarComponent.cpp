/*
  ==============================================================================

    MainMenuBarComponent.cpp
    Created: 25 Sep 2025 01:43:00pm
    Author:  boherm

  ==============================================================================
*/

#include "MainMenuBarComponent.h"
#include "PonyEngine.h"
#include "ProjectSettings/ShowProperties.h"
#include "Redundancy/RedundancyManager.h"
#include "MainComponent.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_organicui/ui/Style.h"

MainMenuBarComponent::MainMenuBarComponent(MainContentComponent* mainComp, PonyEngine* engine) :
	Component("Menu Bar")
#if !JUCE_MAC
	, menuBarComp(mainComp)
#endif
{
#if !JUCE_MAC
	addAndMakeVisible(menuBarComp);
#endif

    this->showProperties = &engine->showProperties;
    this->showProperties->projectName->addParameterListener(this);
    this->showProperties->companyName->addParameterListener(this);
    this->showProperties->showFileVersion->addParameterListener(this);

    if (auto* rm = RedundancyManager::getInstanceWithoutCreating())
    {
        rm->enabled->addParameterListener(this);
        rm->role->addParameterListener(this);
        rm->peerPresent->addParameterListener(this);
        rm->mainConflict->addParameterListener(this);
        rm->showMismatch->addParameterListener(this);
    }
}

MainMenuBarComponent::~MainMenuBarComponent()
{
    this->showProperties->projectName->removeParameterListener(this);
    this->showProperties->companyName->removeParameterListener(this);
    this->showProperties->showFileVersion->removeParameterListener(this);

    if (auto* rm = RedundancyManager::getInstanceWithoutCreating())
    {
        rm->enabled->removeParameterListener(this);
        rm->role->removeParameterListener(this);
        rm->peerPresent->removeParameterListener(this);
        rm->mainConflict->removeParameterListener(this);
        rm->showMismatch->removeParameterListener(this);
    }
}

void MainMenuBarComponent::paint(Graphics& g)
{
    g.fillAll(BG_COLOR);
}

void MainMenuBarComponent::paintOverChildren(Graphics& g)
{
    g.setColour(TEXTNAME_COLOR);

    Rectangle<int> *r = new Rectangle<int>(
            getLocalBounds().getX() + 5,
            getLocalBounds().getY(),
            getLocalBounds().getWidth() - 10,
            getLocalBounds().getHeight()
    );

    String title = this->showProperties->companyName->stringValue() + " - " +
        this->showProperties->projectName->stringValue() + " - " +
        this->showProperties->showFileVersion->stringValue();

    g.drawText(title, *r, Justification::centred, 1);

    // Redundancy badge (always shown). IDLE when off; MAIN/BACKUP (+ peer-presence dot) when on.
    // Clicking it opens the redundancy system in the inspector.
    redundancyBadgeBounds = {};
    if (auto* rm = RedundancyManager::getInstanceWithoutCreating())
    {
        const bool on = rm->isEnabled();
        const bool isMain = rm->getRole() == RedundancyManager::MAIN;
        const bool conflict = on && isMain && rm->mainConflict->boolValue();

        String label;
        Colour badgeCol;
        const bool mismatch = on && !isMain && rm->showMismatch->boolValue();
        if (!on)                   { label = "IDLE";          badgeCol = BG_COLOR.brighter(0.2f); }
        else if (conflict)         { label = "MAIN!";         badgeCol = HIGHLIGHT_COLOR; }
        else if (rm->hasTakenOver || isMain) { label = "MAIN";   badgeCol = RED_COLOR; }
        else if (mismatch)         { label = "BACKUP BAD";    badgeCol = HIGHLIGHT_COLOR; }
        else                       { label = "BACKUP";        badgeCol = AUDIO_COLOR; }

        // Full-height block sized to the text: text centred, peer-presence dot pinned right with a
        // BG_COLOR ring so it detaches from the coloured block.
        const Font badgeFont(12.0f);
        GlyphArrangement ga;
        ga.addLineOfText(badgeFont, label, 0.0f, 0.0f);
        const int textW = roundToInt(ga.getBoundingBox(0, -1, true).getWidth());

        const int pad = 10;                  // left/right padding
        const int gap = 8;                   // small gap between text and dot
        const int dotD = 8;
        const int ringW = 1;                 // BG_COLOR ring thickness
        const int outerD = dotD + 2 * ringW;

        const int bw = pad + textW + (on ? gap + dotD : 0) + pad;
        const int bx = getWidth() - bw;      // flush to the right edge

        Rectangle<int> block(bx, 0, bw, getHeight());
        g.setColour(badgeCol);
        g.fillRect(block);

        g.setColour(Colours::white);
        g.setFont(badgeFont);
        g.drawText(label, Rectangle<int>(bx + pad, 0, textW, getHeight()), Justification::centred, 1);

        if (on)
        {
            // Peer-presence dot right after the text (small gap), with a BG_COLOR ring.
            const float cx = (float)(bx + pad + textW + gap) + dotD / 2.0f;
            const float cy = (float)getHeight() / 2.0f;

            g.setColour(BG_COLOR);
            g.fillEllipse(cx + 1.0f - outerD / 2.0f, cy - outerD / 2.0f, (float)outerD, (float)outerD);

            const bool present = rm->peerPresent->boolValue();
            g.setColour(present ? Colour(0xff43a047) : Colour(0xffe53935));
            g.fillEllipse(cx + 1.0f - dotD / 2.0f, cy - dotD / 2.0f, (float)dotD, (float)dotD);
        }

        redundancyBadgeBounds = block;
    }
}

void MainMenuBarComponent::resized()
{
#if !JUCE_MAC
	Rectangle<int> r = getLocalBounds();
	r.removeFromRight(300);
	menuBarComp.setBounds(r);
#endif
}

void MainMenuBarComponent::parameterValueChanged(Parameter* parameter)
{
    repaint();
}

void MainMenuBarComponent::mouseDown(const MouseEvent& event)
{
    // Clicking the redundancy badge shows the redundancy system in the inspector; clicking
    // elsewhere in the bar shows the show properties as before.
    if (auto* rm = RedundancyManager::getInstanceWithoutCreating())
    {
        if (!redundancyBadgeBounds.isEmpty() && redundancyBadgeBounds.contains(event.getPosition()))
        {
            rm->selectThis();
            return;
        }
    }

    this->showProperties->selectThis();
}
