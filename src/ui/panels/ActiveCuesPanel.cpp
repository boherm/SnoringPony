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
        int cc = (int)((s - t) * 100.0);
        return String::formatted("%02d:%02d.%02d", t / 60, t % 60, cc);
    }

    void drawStopCircle(Graphics& g, Rectangle<int> btn, Colour bg)
    {
        g.setColour(bg);
        g.fillEllipse(btn.toFloat());
    }

    // Centered white "X".
    void drawXIcon(Graphics& g, Rectangle<int> btn)
    {
        Rectangle<float> x = btn.toFloat().reduced(btn.getWidth() * 0.32f);
        g.setColour(Colours::white);
        g.drawLine(x.getX(), x.getY(), x.getRight(), x.getBottom(), 2.2f);
        g.drawLine(x.getRight(), x.getY(), x.getX(), x.getBottom(), 2.2f);
    }

    // Rotating spinner (fading spokes), used while the cue is fading out on panic.
    void drawSpinner(Graphics& g, Rectangle<int> btn)
    {
        Point<float> ctr = btn.getCentre().toFloat();
        float rOuter = btn.getWidth() * 0.30f;
        float rInner = btn.getWidth() * 0.15f;
        const int n = 8;
        int step = (int)(juce::Time::getMillisecondCounter() / 100) % n; // one spoke per 100 ms
        for (int k = 0; k < n; k++)
        {
            int idx = ((step - k) % n + n) % n; // 0 = leading (brightest) spoke
            float alpha = jmax(0.15f, (float)(n - idx) / (float)n);
            float a = juce::MathConstants<float>::twoPi * k / n;
            Point<float> p1 = ctr.getPointOnCircumference(rInner, a);
            Point<float> p2 = ctr.getPointOnCircumference(rOuter, a);
            g.setColour(Colours::white.withAlpha(alpha));
            g.drawLine(p1.x, p1.y, p2.x, p2.y, 1.8f);
        }
    }

    const Colour rowBaseColor = Colour::fromRGB(43, 43, 43);   // unfilled part of the row
    const Colour progressColor = Colour::fromRGB(82, 121, 63); // green progress fill (playing)
    const Colour waitColor = Colour::fromRGB(150, 112, 45);    // amber fill (pre / post-wait)
    const Colour timeColor = Colour::fromRGB(132, 214, 90);    // elapsed / remaining text
}

//==============================================================================
// ActiveCuesRows (scrollable content)
//==============================================================================

Rectangle<int> ActiveCuesRows::getStopRect(int rowIndex) const
{
    const int s = 24; // small circle, vertically centered in the row
    return Rectangle<int>(getWidth() - s - 14, rowIndex * rowH + (rowH - s) / 2, s, s);
}

// The whole row is the progress bar: clicking anywhere on it seeks. Returns the filled
// track area (mirrors the green fill drawn in paint()).
Rectangle<int> ActiveCuesRows::getProgressRect(int rowIndex) const
{
    return Rectangle<int>(0, rowIndex * rowH, getWidth(), rowH).reduced(2, 2);
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

        // Status drives the progress values and the fill color: pre/post-wait share an
        // amber background (labeled below), playback is green.
        double cur = 0.0, total = 0.0;
        Colour fillColor = progressColor;
        String waitLabel;
        if (c->preWaitActive->boolValue())
        {
            cur = c->preWaitCurrentTime->doubleValue();
            total = c->preWaitDuration->doubleValue();
            fillColor = waitColor;
            waitLabel = "Pre-wait";
        }
        else if (c->postWaitActive->boolValue() && !c->isPlaying->boolValue())
        {
            cur = c->postWaitCurrentTime->doubleValue();
            total = c->postWaitDuration->doubleValue();
            fillColor = waitColor;
            waitLabel = "Post-wait";
        }
        else
        {
            cur = c->currentTime->doubleValue();
            total = c->duration->doubleValue();
            fillColor = progressColor;
        }

        Rectangle<int> track = getProgressRect(i);

        // Row background = dark base + full-height progress fill in the status color.
        g.setColour(rowBaseColor);
        g.fillRect(track);
        if (total > 0.0)
        {
            float frac = (float)jlimit(0.0, 1.0, cur / total);
            if (frac > 0.0f)
            {
                g.setColour(fillColor);
                g.fillRect(track.withWidth(roundToInt(track.getWidth() * frac)));
            }
        }

        // Cue color strip on the far left.
        g.setColour(c->itemColor->getColor());
        g.fillRect(track.withWidth(4));

        // Text block (id + description, then elapsed / remaining times), vertically centered.
        Rectangle<int> content = track.reduced(12, 8).withTrimmedRight(getStopRect(i).getWidth() + 16);

        Rectangle<int> line1 = content.removeFromTop(24);
        g.setColour(Colours::white);
        g.setFont(Font(16.0f, Font::bold));
        g.drawText(c->id->stringValue() + "  ·  " + c->getDescription(),
                   line1, Justification::centredLeft, true);

        content.removeFromTop(2);
        Rectangle<int> line2 = content.removeFromTop(18);

        // Pre/Post-wait label, centered at the bottom between the two times.
        if (waitLabel.isNotEmpty())
        {
            g.setColour(Colours::white.withAlpha(.8f));
            g.setFont(Font(12.0f, Font::bold));
            g.drawText(waitLabel, line2, Justification::centred);
        }

        g.setColour(timeColor);
        g.setFont(Font(15.0f, Font::bold));
        g.drawText(fmtTime(cur), line2.removeFromLeft(90), Justification::centredLeft);
        if (total > 0.0)
            g.drawText("-" + fmtTime(total - cur), line2, Justification::centredRight);

        // STOP button (circular). While fading after the first click, the background
        // blinks base/red and the icon becomes a rotating spinner; otherwise a white X.
        const Colour stopBase = Colours::black.withAlpha(.45f);
        Rectangle<int> btn = getStopRect(i);
        if (c->isPanicking)
        {
            bool blinkOn = (juce::Time::getMillisecondCounter() / 400) % 2 == 0;
            drawStopCircle(g, btn, blinkOn ? Colours::red : stopBase);
            drawSpinner(g, btn);
        }
        else
        {
            drawStopCircle(g, btn, stopBase);
            drawXIcon(g, btn);
        }
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
        if (track.getWidth() <= 0 || !track.contains(e.getPosition())) continue;

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
