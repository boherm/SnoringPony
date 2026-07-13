/*
  ==============================================================================

    ShowInfos.cpp
    Created: 4 Nov 2025 10:50:23am
    Author:  boherm

  ==============================================================================
*/

#include "ShowInfos.h"
#include "../../PonyEngine.h"
#include "../../Cue/Cue.h"
#include "../../Cuelist/Cuelist.h"
#include "juce_graphics/juce_graphics.h"
#include <cmath>

//==============================================================================

ShowInfosUI::ShowInfosUI(const String &contentName):
    ShapeShifterContent(ShowInfos::getInstance(), contentName) { }

ShowInfosUI::~ShowInfosUI() { }

//==============================================================================

juce_ImplementSingleton(ShowInfos);

ShowInfos::ShowInfos():
    ControllableContainer("Show Infos")
{
    addAndMakeVisible(notesViewport);
    notesViewport.setViewedComponent(&notesView, false); // owned by us, don't delete
    notesViewport.setScrollBarsShown(true, false);        // vertical only
    notesViewport.setVisible(false);

    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
    engine->showProperties.mainCuelist->addParameterListener(this);
}

ShowInfos::~ShowInfos()
{
    if (currentCue != nullptr) {
        currentCue->id->removeParameterListener(this);
        currentCue->description->removeParameterListener(this);
        currentCue = nullptr;
    }

    if (nextCue != nullptr) {
        nextCue->id->removeParameterListener(this);
        nextCue->description->removeParameterListener(this);
        nextCue->notes->removeParameterListener(this);
        nextCue = nullptr;
    }

    if (mainCuelist != nullptr) {
        mainCuelist->currentCue->removeParameterListener(this);
        mainCuelist->nextCue->removeParameterListener(this);
        mainCuelist = nullptr;
    }

    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);
    engine->showProperties.mainCuelist->removeParameterListener(this);
}

void ShowInfos::paint(juce::Graphics &g)
{
    g.fillAll(BG_COLOR);

    auto area = getLocalBounds().reduced(5, 10);

    const int markerWidth = 20; // fixed column so both ids start at the same x

    // Current cue line: "> id: description"
    {
        auto line = area.removeFromTop(lineHeight);
        g.setFont(20.0f);
        g.setColour(Colours::white);

        g.drawText(">", line.removeFromLeft(markerWidth), Justification::topLeft);

        if (currentCue != nullptr) {
            String id = currentCue->id->stringValue() + ": ";
            g.drawText(id, line.removeFromLeft(g.getCurrentFont().getStringWidth(id)), Justification::topLeft);

            if (currentCue->getDescription().startsWith("--"))
                g.setFont(Font(20.0f, Font::italic));
            g.drawText(currentCue->getDescription(), line, Justification::topLeft);
        } else {
            g.setFont(Font(20.0f, Font::italic));
            g.setColour(Colours::white.darker(0.6f));
            g.drawText("(no cue now)", line, Justification::topLeft);
        }
    }

    // Next cue line: "| id: description"
    {
        auto line = area.removeFromTop(lineHeight);
        g.setFont(Font(20.0f, Font::plain));
        g.setColour(Colours::white.darker(0.6f));

        g.drawText("|", line.removeFromLeft(markerWidth), Justification::topLeft);

        if (nextCue != nullptr) {
            String id = nextCue->id->stringValue() + ": ";
            g.drawText(id, line.removeFromLeft(g.getCurrentFont().getStringWidth(id)), Justification::topLeft);

            if (nextCue->getDescription().startsWith("--"))
                g.setFont(Font(20.0f, Font::italic));
            g.drawText(nextCue->getDescription(), line, Justification::topLeft);
        } else {
            g.setFont(Font(20.0f, Font::italic));
            g.drawText("(no cue next)", line, Justification::topLeft);
        }
    }

    // The next cue notes are drawn by notesView inside notesViewport (see resized()).
}

void ShowInfos::resized()
{
    auto area = getLocalBounds().reduced(5, 10);
    area.removeFromTop(lineHeight * 2 + notesGap); // skip the two cue lines + gap
    notesViewport.setBounds(area.withTrimmedLeft(notesIndent));
    layoutNotes();
}

void ShowInfos::layoutNotes()
{
    int w = notesViewport.getWidth();
    if (w <= 0)
        return;

    int h = notesView.getPreferredHeight(w);
    if (h > notesViewport.getHeight()) {
        // A vertical scrollbar will appear: reflow the text in the reduced width.
        w -= notesViewport.getScrollBarThickness();
        h = notesView.getPreferredHeight(w);
    }
    notesView.setSize(w, h);
}

void ShowInfos::updateNotes()
{
    const bool has = (nextCue != nullptr);
    notesViewport.setVisible(has);
    notesView.setText(has ? nextCue->notes->stringValue() : String());
    layoutNotes();
}

//==============================================================================

void ShowInfos::NotesView::setText(const String& t)
{
    if (text == t)
        return;
    text = t;
    repaint();
}

AttributedString ShowInfos::NotesView::makeString() const
{
    AttributedString s;
    s.setText(text);
    s.setFont(Font(16.0f));
    s.setColour(Colours::white.darker(0.6f));
    s.setLineSpacing(3.0f); // ~1.2x line height for a 16px font
    s.setJustification(Justification::topLeft);
    return s;
}

int ShowInfos::NotesView::getPreferredHeight(int width) const
{
    if (text.isEmpty() || width <= 0)
        return 0;

    TextLayout layout;
    layout.createLayout(makeString(), (float) width);
    return (int) std::ceil(layout.getHeight());
}

void ShowInfos::NotesView::paint(Graphics& g)
{
    if (text.isEmpty())
        return;

    TextLayout layout;
    layout.createLayout(makeString(), (float) getWidth());
    layout.draw(g, getLocalBounds().toFloat());
}

void ShowInfos::parameterValueChanged(Parameter* p)
{
    PonyEngine* engine = dynamic_cast<PonyEngine*>(Engine::mainEngine);

    if (p == engine->showProperties.mainCuelist)
    {
        if (currentCue != nullptr) {
            currentCue->id->removeParameterListener(this);
            currentCue->description->removeParameterListener(this);
            currentCue = nullptr;
        }

        if (nextCue != nullptr) {
            nextCue->id->removeParameterListener(this);
            nextCue->description->removeParameterListener(this);
            nextCue->notes->removeParameterListener(this);
            nextCue = nullptr;
        }

        if (mainCuelist != nullptr) {
            mainCuelist->currentCue->removeParameterListener(this);
            mainCuelist->nextCue->removeParameterListener(this);
        }

        mainCuelist = engine->showProperties.mainCuelist->getTargetContainerAs<Cuelist>();

        if (mainCuelist != nullptr) {
            mainCuelist->currentCue->addParameterListener(this);
            mainCuelist->nextCue->addParameterListener(this);

            Cue* cc = mainCuelist->currentCue->getTargetContainerAs<Cue>();
            currentCue = cc;
            if (cc != nullptr) {
                cc->id->addParameterListener(this);
                cc->description->addParameterListener(this);
            }

            Cue* nc = mainCuelist->nextCue->getTargetContainerAs<Cue>();
            nextCue = nc;
            if (nc != nullptr) {
                nc->id->addParameterListener(this);
                nc->description->addParameterListener(this);
                nc->notes->addParameterListener(this);
            }
        }
    }

    if (mainCuelist && p == mainCuelist->currentCue)
    {
        if (currentCue != nullptr) {
            currentCue->id->removeParameterListener(this);
            currentCue->description->removeParameterListener(this);
        }
        Cue* cc = mainCuelist->currentCue->getTargetContainerAs<Cue>();
        currentCue = cc;
        if (cc != nullptr) {
            cc->id->addParameterListener(this);
            cc->description->addParameterListener(this);
        }
    }

    if (mainCuelist && p == mainCuelist->nextCue)
    {
        if (nextCue != nullptr) {
            nextCue->id->removeParameterListener(this);
            nextCue->description->removeParameterListener(this);
            nextCue->notes->removeParameterListener(this);
        }
        Cue* nc = mainCuelist->nextCue->getTargetContainerAs<Cue>();
        nextCue = nc;
        if (nc != nullptr) {
            nc->id->addParameterListener(this);
            nc->description->addParameterListener(this);
            nc->notes->addParameterListener(this);
        }
    }

    updateNotes();
    repaint();
}
