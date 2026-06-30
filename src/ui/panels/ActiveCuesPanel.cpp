/*
  ==============================================================================

    ActiveCuesPanel.cpp
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "ActiveCuesPanel.h"
#include "../../Cuelist/CuelistManager.h"
#include "../../Cuelist/Cuelist.h"
#include "../../Cue/CueManager.h"
#include "../../Cue/Cue.h"
#include "../../Cue/audio/AudioCue.h"
#include "../../Cue/audio/AudioFile.h"
#include "../../Cue/audio/AudioSlices.h"

namespace
{
    Array<Cue*> collectActiveCues()
    {
        Array<Cue*> result;
        for (Cuelist* cl : CuelistManager::getInstance()->items)
        {
            if (cl == nullptr || cl->cues == nullptr) continue;
            for (Cue* c : cl->cues->items)
            {
                if (c == nullptr) continue;
                if (c->isPlaying->boolValue() || c->preWaitActive->boolValue() || c->postWaitActive->boolValue())
                    result.add(c);
            }
        }
        return result;
    }

    String fmtTime(double s)
    {
        if (s < 0) s = 0;
        int t = (int)s;
        return String::formatted("%d:%02d", t / 60, t % 60);
    }

    // Rounded background + centered white square icon.
    void drawStopButton(Graphics& g, Rectangle<int> btn, Colour bg)
    {
        g.setColour(bg);
        g.fillRoundedRectangle(btn.toFloat(), 5.0f);
        float sq = jmin(btn.getWidth(), btn.getHeight()) * 0.42f;
        Rectangle<float> sr = Rectangle<float>(sq, sq).withCentre(btn.getCentre().toFloat());
        g.setColour(Colours::white);
        g.fillRoundedRectangle(sr, 1.5f);
    }
}

//==============================================================================
// ActiveCuesRows (scrollable content)
//==============================================================================

Rectangle<int> ActiveCuesRows::getStopRect(int rowIndex) const
{
    const int s = rowH - 26;
    return Rectangle<int>(getWidth() - s - 12, rowIndex * rowH + (rowH - s) / 2, s, s);
}

// Horizontal extent of the progress track for a row (mirrors the layout in paint()).
// Used to hit-test clicks and map them to a position for seeking.
Rectangle<int> ActiveCuesRows::getProgressRect(int rowIndex) const
{
    Rectangle<int> row(0, rowIndex * rowH, getWidth(), rowH);
    Rectangle<int> content = row.reduced(12, 6).withTrimmedRight(getStopRect(rowIndex).getWidth() + 20);
    content.removeFromTop(14); // cuelist name
    content.removeFromTop(20); // cue id + description
    Rectangle<int> prog = content.removeFromTop(16);
    prog.removeFromLeft(42);   // elapsed time label
    prog.removeFromRight(46);  // remaining time label
    return prog.reduced(6, 0); // matches the drawn track's horizontal extent
}

void ActiveCuesRows::paint(juce::Graphics& g)
{
    g.fillAll(BG_COLOR);

    Array<Cue*> cues = collectActiveCues();

    if (cues.isEmpty())
    {
        g.setColour(Colours::white.withAlpha(.35f));
        g.setFont(16.0f);
        g.drawText("(no cue playing)", getLocalBounds(), Justification::centred);
        return;
    }

    for (int i = 0; i < cues.size(); i++)
    {
        Cue* c = cues[i];
        Cuelist* cl = c->parentCuelist;

        Rectangle<int> row(0, i * rowH, getWidth(), rowH);

        // Row background tinted with the cuelist color + color bar on the left.
        Colour clColor = (cl != nullptr) ? cl->itemColor->getColor() : BG_COLOR.brighter(.1f);
        g.setColour(clColor.withAlpha(.18f));
        g.fillRect(row.reduced(2, 2));
        g.setColour(clColor);
        g.fillRect(row.reduced(2, 2).removeFromLeft(4));

        Rectangle<int> content = row.reduced(12, 6).withTrimmedRight(getStopRect(i).getWidth() + 20);

        // 1) Cuelist name (top).
        g.setColour(Colours::white.withAlpha(.55f));
        g.setFont(11.0f);
        g.drawText((cl != nullptr) ? cl->niceName : String("?"),
                   content.removeFromTop(14), Justification::topLeft, true);

        // 2) Cue id + description.
        g.setColour(Colours::white);
        g.setFont(Font(15.0f, Font::bold));
        g.drawText(c->id->stringValue() + "   " + c->getDescription(),
                   content.removeFromTop(20), Justification::centredLeft, true);

        // 3) Progress bar with elapsed (left) and remaining (right) times.
        double cur = 0.0, total = 0.0;
        if (c->preWaitActive->boolValue())
        {
            cur = c->preWaitCurrentTime->doubleValue();
            total = c->preWaitDuration->doubleValue();
        }
        else
        {
            cur = c->currentTime->doubleValue();
            total = c->duration->doubleValue();
        }

        Rectangle<int> prog = content.removeFromTop(16);
        if (total > 0.0)
        {
            g.setColour(Colours::white.withAlpha(.7f));
            g.setFont(11.0f);
            g.drawText(fmtTime(cur), prog.removeFromLeft(42), Justification::centredLeft);
            g.drawText("-" + fmtTime(total - cur), prog.removeFromRight(46), Justification::centredRight);

            Rectangle<float> track = prog.reduced(6, 0).withSizeKeepingCentre((float)prog.getWidth() - 12, 6.0f).toFloat();
            g.setColour(BG_COLOR.darker(.35f));
            g.fillRoundedRectangle(track, 3.0f);
            float pos = (float)jlimit(0.0, 1.0, cur / total);
            if (pos > 0.0f)
            {
                g.setColour(clColor.brighter(.2f));
                g.fillRoundedRectangle(track.withWidth(track.getWidth() * pos), 3.0f);
            }
        }

        // STOP button (square icon). While fading after the first click, blink red/orange.
        Colour stopColor = Colours::darkred;
        if (c->isPanicking)
        {
            bool blinkOn = (juce::Time::getMillisecondCounter() / 400) % 2 == 0;
            stopColor = blinkOn ? Colours::red : Colours::orange;
        }
        drawStopButton(g, getStopRect(i), stopColor);
    }
}

void ActiveCuesRows::mouseDown(const juce::MouseEvent& e)
{
    Array<Cue*> cues = collectActiveCues();
    for (int i = 0; i < cues.size(); i++)
    {
        if (getStopRect(i).contains(e.getPosition()))
        {
            cues[i]->panic(); // first click: fade then stop; second click: stop now
            repaint();
            return;
        }
    }

    // Click on the progress bar of a playing audio cue to seek it.
    for (int i = 0; i < cues.size(); i++)
    {
        Cue* c = cues[i];
        if (c->preWaitActive->boolValue()) continue; // the bar shows the pre-wait, not the audio

        AudioCue* ac = dynamic_cast<AudioCue*>(c);
        if (ac == nullptr || !ac->isPlaying->boolValue()) continue;

        Rectangle<int> track = getProgressRect(i);
        if (track.getWidth() <= 0) continue;

        // A little vertical slack so the thin bar is easy to hit.
        Rectangle<int> hit = track.withTop(track.getY() - 6).withBottom(track.getBottom() + 6);
        if (!hit.contains(e.getPosition())) continue;

        double total = c->duration->doubleValue();
        if (total <= 0.0) continue;

        double frac = jlimit(0.0, 1.0, (double)(e.getPosition().x - track.getX()) / (double)track.getWidth());

        // Map the fraction onto the cue's playable window, then seek every file there.
        double winStart = ac->slicesManager->startTime->doubleValue();
        double winEnd = ac->slicesManager->endTime->doubleValue();
        double target = winStart + frac * (winEnd - winStart);

        ac->slicesManager->resetSlices(); // drop stale repetition counters so the bar matches
        ac->filesManager->setCurrentTime(target);
        repaint();
        return;
    }
}

//==============================================================================
// ActiveCuesPanel (header + scrollable viewport)
//==============================================================================

ActiveCuesPanelUI::ActiveCuesPanelUI(const String& contentName) :
    ShapeShifterContent(ActiveCuesPanel::getInstance(), contentName)
{
}

ActiveCuesPanelUI::~ActiveCuesPanelUI()
{
}

juce_ImplementSingleton(ActiveCuesPanel);

ActiveCuesPanel::ActiveCuesPanel()
{
    viewport.setViewedComponent(&rows, false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);
    startTimerHz(10);
}

ActiveCuesPanel::~ActiveCuesPanel()
{
    stopTimer();
}

Rectangle<int> ActiveCuesPanel::getStopAllRect() const
{
    return Rectangle<int>(getWidth() - 118, 5, 108, headerH - 10);
}

void ActiveCuesPanel::resized()
{
    viewport.setBounds(0, headerH, getWidth(), getHeight() - headerH);
}

void ActiveCuesPanel::paint(juce::Graphics& g)
{
    int count = collectActiveCues().size();

    // Header with the global STOP ALL button.
    g.setColour(BG_COLOR.darker(.15f));
    g.fillRect(0, 0, getWidth(), headerH);
    g.setColour(Colours::white.withAlpha(.6f));
    g.setFont(13.0f);
    g.drawText("Active cues (" + String(count) + ")", 10, 0, getWidth() - 130, headerH, Justification::centredLeft);

    if (count > 0)
    {
        Rectangle<int> all = getStopAllRect();
        g.setColour(Colours::darkred);
        g.fillRoundedRectangle(all.toFloat(), 4.0f);
        g.setColour(Colours::white);
        g.setFont(Font(12.0f, Font::bold));
        g.drawText("STOP ALL", all, Justification::centred);
    }
}

void ActiveCuesPanel::mouseDown(const juce::MouseEvent& e)
{
    if (getStopAllRect().contains(e.getPosition()))
    {
        for (Cue* c : collectActiveCues()) c->panic();
        repaint();
    }
}

void ActiveCuesPanel::timerCallback()
{
    int count = collectActiveCues().size();
    int w = jmax(0, viewport.getMaximumVisibleWidth());
    int h = jmax(count * ActiveCuesRows::rowH, viewport.getHeight());

    if (rows.getWidth() != w || rows.getHeight() != h)
        rows.setSize(w, h);

    rows.repaint();
    repaint(); // header count
}
