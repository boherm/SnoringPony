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
#include "../../Cue/group/GroupCue.h"
#include "../../Redundancy/RedundancyManager.h"
#include "juce_organicui/ui/Style.h"

namespace
{
    // A Backup in standby only mirrors the Main's state; it must not let the user drive playback
    // (panic/stop) or change cue volumes from the Active Cues panel.
    bool redundancyBlocksControl()
    {
        auto* rm = RedundancyManager::getInstanceWithoutCreating();
        return rm != nullptr && rm->isStandby();
    }

    Array<Cue*> collectActiveCues()
    {
        Array<Cue*> result;
        for (Cuelist* cl : CuelistManager::getInstance()->items)
        {
            if (cl == nullptr || cl->cues == nullptr) continue;
            for (Cue* c : cl->cues->items)
            {
                if (c == nullptr) continue;
                bool active = c->isPlaying->boolValue() || c->preWaitActive->boolValue() || c->postWaitActive->boolValue();
                // A ducking audio cue in its post-play or fade-in (restore) phase is no longer
                // "playing" but must stay listed (pre-play is already covered by isPlaying).
                if (auto* ac = dynamic_cast<AudioCue*>(c))
                    active = active || ac->duckPrePlayActive->boolValue() || ac->duckPostPlayActive->boolValue()
                                    || ac->duckFadeInActive->boolValue();
                if (active)
                    result.add(c);
            }
        }
        return result;
    }

    const Colour rowBaseColor = Colour::fromRGB(43, 43, 43);    // the item box
    const Colour progressColor = Colour::fromRGB(82, 121, 63);  // cue playback progress fill (green)
    const Colour timeColor = Colour::fromRGB(132, 214, 90);     // elapsed / remaining text (green)
    const Colour preWaitColor = Colour::fromRGB(60, 100, 150); // pre-wait progress fill (blue)
    const Colour waitColor = Colour::fromRGB(150, 112, 45);    // post-wait progress fill (amber)
    const Colour duckColor = Colour::fromRGB(90, 160, 210);    // duck phases progress fill (sky blue)

    // mm:ss.cc
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

    struct WaitPhase { double cur; double total; String label; Colour color; };

    // The cue's active wait/duck phases, top-to-bottom in lifecycle order. Each is its own bar;
    // several can run at once (e.g. duck pre-play during playback), so they stack. Only phases
    // with a real duration are included. Duck phases (audio cues) are sky blue.
    Array<WaitPhase> getWaitPhases(Cue* c)
    {
        Array<WaitPhase> phases;
        auto* ac = dynamic_cast<AudioCue*>(c);

        if (c->preWaitActive->boolValue() && c->preWaitDuration->doubleValue() > 0.0)
            phases.add({ c->preWaitCurrentTime->doubleValue(), c->preWaitDuration->doubleValue(), "Pre-wait", preWaitColor });

        // Duck: fade-out runs concurrently with the pre-play delay, so they share one bar.
        if (ac != nullptr && ac->duckPrePlayActive->boolValue() && ac->duckPrePlayDuration->doubleValue() > 0.0)
            phases.add({ ac->duckPrePlayCurrentTime->doubleValue(), ac->duckPrePlayDuration->doubleValue(), "Ducking & waiting", duckColor });

        if (c->postWaitActive->boolValue() && c->postWaitDuration->doubleValue() > 0.0)
            phases.add({ c->postWaitCurrentTime->doubleValue(), c->postWaitDuration->doubleValue(), "Post-wait", waitColor });

        if (ac != nullptr && ac->duckPostPlayActive->boolValue() && ac->duckPostPlayDuration->doubleValue() > 0.0)
            phases.add({ ac->duckPostPlayCurrentTime->doubleValue(), ac->duckPostPlayDuration->doubleValue(), "Wait after playing", duckColor });

        if (ac != nullptr && ac->duckFadeInActive->boolValue() && ac->duckFadeInDuration->doubleValue() > 0.0)
            phases.add({ ac->duckFadeInCurrentTime->doubleValue(), ac->duckFadeInDuration->doubleValue(), "Restore ducked volume", duckColor });

        return phases;
    }

    // Small pop-up text editor, styled like the Deck's cell edit bubble, launched in a CallOutBox.
    class VolumeEditBubble : public Component, private TextEditor::Listener
    {
    public:
        VolumeEditBubble(const String& initial, std::function<void(const String&)> commit) :
            onCommit(std::move(commit))
        {
            editor.setText(initial, dontSendNotification);
            editor.setSelectAllWhenFocused(true);
            editor.setEscapeAndReturnKeysConsumed(true);
            editor.setInputRestrictions(6, "0123456789.");
            editor.setJustification(Justification::centred);
            editor.setColour(TextEditor::backgroundColourId, BG_COLOR.darker(.1f));
            editor.setColour(TextEditor::textColourId, TEXT_COLOR);
            editor.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
            editor.setColour(TextEditor::focusedOutlineColourId, BG_COLOR.brighter(.3f));
            editor.setColour(TextEditor::highlightColourId, BG_COLOR.brighter(.2f));
            editor.setColour(TextEditor::highlightedTextColourId, TEXT_COLOR);
            editor.setColour(CaretComponent::caretColourId, TEXT_COLOR);
            editor.applyColourToAllText(TEXT_COLOR, true);
            editor.addListener(this);
            addAndMakeVisible(editor);
            setSize(120, 28);
        }

        void paint(Graphics& g) override { g.fillAll(BG_COLOR); }

    private:
        TextEditor editor;
        std::function<void(const String&)> onCommit;
        bool committed = false;

        void resized() override { editor.setBounds(getLocalBounds().reduced(2)); }
        void parentHierarchyChanged() override
        {
            if (isShowing())
            {
                Component::SafePointer<TextEditor> safeEd(&editor);
                MessageManager::callAsync([safeEd] { if (safeEd != nullptr) safeEd->grabKeyboardFocus(); });
            }
        }

        void commitAndDismiss()
        {
            if (committed) return;
            committed = true;
            if (onCommit) onCommit(editor.getText());
            if (auto* box = findParentComponentOfClass<CallOutBox>()) box->dismiss();
        }

        void textEditorReturnKeyPressed(TextEditor&) override { commitAndDismiss(); }
        void textEditorEscapeKeyPressed(TextEditor&) override
        {
            committed = true;
            if (auto* box = findParentComponentOfClass<CallOutBox>()) box->dismiss();
        }
        void textEditorFocusLost(TextEditor&) override { commitAndDismiss(); }
    };
}

//==============================================================================
// ActiveCuesRow (a single row)
//==============================================================================

int ActiveCuesRow::getDesiredHeight() const
{
    if (cue == nullptr)
        return rowH;

    // The vertical fader sits on the right over the full height, so it doesn't add height.
    return rowH + getWaitPhases(cue).size() * waitBarH;
}

Rectangle<int> ActiveCuesRow::getFaderRect() const
{
    if (cue == nullptr || !cue->hasVolumeFader())
        return {};

    // Vertical fader: a full-height strip on the far right of the item.
    Rectangle<int> box = getLocalBounds().reduced(0, 4);
    return box.removeFromRight(faderW);
}

Rectangle<int> ActiveCuesRow::getProgressRect() const
{
    if (cue == nullptr)
        return {};

    // Same region the progress fill is drawn on: the box minus the fader (right) and wait bars.
    Rectangle<int> box = getLocalBounds().reduced(0, 4);
    if (cue->hasVolumeFader())
        box.removeFromRight(faderW);
    box.removeFromBottom(getWaitPhases(cue).size() * waitBarH);
    return box;
}

Rectangle<int> ActiveCuesRow::getStopRect() const
{
    const int s = 22;
    Rectangle<int> box = getLocalBounds().reduced(0, 4);
    // Sit just left of the vertical fader (or the right edge when there's no fader).
    int right = box.getRight() - ((cue != nullptr && cue->hasVolumeFader()) ? faderW : 0);
    return Rectangle<int>(right - s - 8, box.getY() + 6, s, s);
}

void ActiveCuesRow::paint(juce::Graphics& g)
{
    if (cue == nullptr)
        return;

    Rectangle<int> box = getLocalBounds().reduced(0, 4);

    g.setColour(rowBaseColor);
    g.fillRect(box);

    // Vertical volume fader on the far right (full height): track + fill from the bottom at the
    // live output level + a thumb at the user-set volume. Reserved first so the text / wait bars
    // don't run under it.
    if (cue->hasVolumeFader())
    {
        Rectangle<int> f = box.removeFromRight(faderW);
        g.setColour(Colours::black.withAlpha(.4f));
        g.fillRect(f);

        float maxV = jmax(0.0001f, cue->getMaxFaderVolume());
        float lvl = jlimit(0.0f, 1.0f, cue->getOutputLevel() / maxV);
        if (lvl > 0.0f)
        {
            int fillH = roundToInt(f.getHeight() * lvl);
            g.setColour(timeColor.withAlpha(.85f));
            g.fillRect(f.getX(), f.getBottom() - fillH, f.getWidth(), fillH);
        }

        float vol = jlimit(0.0f, 1.0f, cue->getFaderVolume() / maxV);
        int ty = f.getBottom() - roundToInt(f.getHeight() * vol);
        g.setColour(Colours::white);
        g.fillRect(f.getX(), ty - 1, f.getWidth(), 2);
    }

    // Pre/post-wait bars, stacked top-to-bottom above the fader (the row grows by waitBarH per
    // active phase). Both pre- and post-wait can run at once.
    Array<WaitPhase> phases = getWaitPhases(cue);
    if (!phases.isEmpty())
    {
        Rectangle<int> waitArea = box.removeFromBottom(phases.size() * waitBarH);
        for (auto& ph : phases)
        {
            Rectangle<int> bar = waitArea.removeFromTop(waitBarH);

            // Dark base + colored progress fill (elapsed / total of the wait).
            g.setColour(rowBaseColor.darker(.35f));
            g.fillRect(bar);
            if (ph.total > 0.0)
            {
                float frac = (float)jlimit(0.0, 1.0, ph.cur / ph.total);
                if (frac > 0.0f)
                {
                    g.setColour(ph.color.darker(0.4f)); // darker so the white text stays legible
                    g.fillRect(bar.withWidth(roundToInt(bar.getWidth() * frac)));
                }
            }

            Rectangle<int> wt = bar.reduced(8, 0);
            g.setColour(Colours::white.withAlpha(.9f));
            g.setFont(Font(11.0f, Font::bold));
            g.drawText(fmtTime(ph.cur), wt, Justification::centredLeft);
            g.drawText(ph.label, wt, Justification::centred);
            g.drawText("-" + fmtTime(ph.total - ph.cur), wt, Justification::centredRight);
        }
    }

    // Cue playback progress fill. `box` is now just the content area (the fader was removed from
    // the right and the wait bars from the bottom), so the fill never overlaps them.
    {
        double total = cue->duration->doubleValue();
        if (total > 0.0)
        {
            float frac = (float)jlimit(0.0, 1.0, cue->currentTime->doubleValue() / total);
            if (frac > 0.0f)
            {
                g.setColour(progressColor);
                g.fillRect(box.withWidth(roundToInt(box.getWidth() * frac)));
            }
        }
    }

    Rectangle<int> content = box.reduced(12, 6);

    // Line 1 (top): cue number and name, small. Trimmed on the right so it doesn't run under the
    // STOP button.
    Rectangle<int> line1 = content.removeFromTop(22).withRight(getStopRect().getX() - 6);
    g.setColour(Colours::white);
    g.setFont(Font(13.0f, Font::bold));
    g.drawText(cue->id->stringValue() + "  ·  " + cue->getDescription(),
               line1, Justification::centredLeft, true);

    // Line 2 (below): elapsed (left) / remaining (right), in green.
    Rectangle<int> line2 = content.removeFromTop(28);
    double cur = cue->currentTime->doubleValue();
    double total = cue->duration->doubleValue();
    g.setColour(timeColor);
    g.setFont(Font(14.0f, Font::bold));
    g.drawText(fmtTime(cur), line2.removeFromLeft(90), Justification::centredLeft);
    if (total > 0.0)
        g.drawText("-" + fmtTime(total - cur), line2, Justification::centredRight);

    // STOP button, top-right (left of the fader). While fading out on panic, the circle blinks
    // red and the icon becomes a rotating spinner; otherwise a white X.
    {
        Rectangle<int> stop = getStopRect();
        const Colour stopBase = Colours::black.withAlpha(.45f);
        if (cue->isPanicking)
        {
            bool blinkOn = (juce::Time::getMillisecondCounter() / 400) % 2 == 0;
            drawStopCircle(g, stop, blinkOn ? RED_COLOR : stopBase);
            drawSpinner(g, stop);
        }
        else
        {
            drawStopCircle(g, stop, stopBase);
            drawXIcon(g, stop);
        }
    }

    // Cue color strip on the far left (full height), drawn without consuming the layout box.
    if (cue->itemColor->value != cue->itemColor->defaultValue) {
        g.setColour(cue->itemColor->getColor());
        g.fillRect(getLocalBounds().reduced(0, 4).withWidth(5));
    }
}

void ActiveCuesRow::mouseDown(const juce::MouseEvent& e)
{
    // In Backup standby, the row is display-only (no panic, no fader, no seek).
    if (redundancyBlocksControl()) return;

    // STOP button: first click fades out (panic), second click stops immediately.
    if (cue != nullptr && getStopRect().contains(e.getPosition()))
    {
        cue->panic();
        repaint();
        return;
    }

    // Arm dragging the fader; the volume only changes once the mouse actually moves, so a plain
    // click (or the first click of a double-click) never jumps it.
    if (getFaderRect().contains(e.getPosition()))
    {
        draggingFader = true;
        return;
    }

    // Click the progress bar to seek (audio, group, ... seekable cues), except during pre-wait.
    if (cue != nullptr && cue->canSeekTo() && !cue->preWaitActive->boolValue())
    {
        Rectangle<int> track = getProgressRect();
        double total = cue->duration->doubleValue();
        if (track.getWidth() > 0 && total > 0.0 && track.contains(e.getPosition()))
        {
            double frac = jlimit(0.0, 1.0, (double)(e.getPosition().x - track.getX()) / (double)track.getWidth());
            cue->seekToTime(frac * total);
            repaint();
        }
    }
}

void ActiveCuesRow::mouseDrag(const juce::MouseEvent& e)
{
    if (redundancyBlocksControl() || !draggingFader || cue == nullptr)
        return;

    Rectangle<int> f = getFaderRect();
    if (f.getHeight() <= 0)
        return;

    // Vertical: bottom = 0, top = max.
    float frac = jlimit(0.0f, 1.0f, (float)(f.getBottom() - e.getPosition().y) / (float)f.getHeight());
    cue->setFaderVolume(frac * cue->getMaxFaderVolume());
    repaint();
}

void ActiveCuesRow::mouseUp(const juce::MouseEvent&)
{
    draggingFader = false;
}

void ActiveCuesRow::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (redundancyBlocksControl() || cue == nullptr)
        return;

    Rectangle<int> f = getFaderRect();
    if (f.getWidth() <= 0 || !f.contains(e.getPosition()))
        return;

    // Double-click the fader to type an exact volume, using the same bubble editor as the Deck.
    Cue* c = cue;
    String initial = String(c->getFaderVolume(), 2);
    std::function<void(const String&)> onCommit = [c](const String& t)
    {
        c->setFaderVolume(jlimit(0.0f, 1.5f, t.getFloatValue()));
    };

    auto bubble = std::make_unique<VolumeEditBubble>(initial, std::move(onCommit));
    CallOutBox::launchAsynchronously(std::move(bubble), localAreaToGlobal(f), nullptr);
}

juce::String ActiveCuesRow::getTooltip()
{
    if (cue != nullptr && getFaderRect().contains(getMouseXYRelative()))
        return "Volume: " + String(cue->getFaderVolume(), 2);
    return {};
}

//==============================================================================
// ActiveCuesRows (scrollable content: one ActiveCuesRow child per active cue)
//==============================================================================

void ActiveCuesRows::setCues(const Array<Cue*>& cues, int width, int minHeight)
{
    // Match the number of child rows to the active cues.
    while (rowComps.size() < cues.size())
        addAndMakeVisible(rowComps.add(new ActiveCuesRow()));
    while (rowComps.size() > cues.size())
        rowComps.removeLast();

    // Assign each row its cue and stack them, using each row's own (variable) height.
    int y = 0;
    for (int i = 0; i < cues.size(); i++)
    {
        rowComps[i]->setCue(cues[i]);
        int h = rowComps[i]->getDesiredHeight();
        rowComps[i]->setBounds(0, y, width, h);
        y += h;
    }

    setSize(width, jmax(y, minHeight));
}

void ActiveCuesRows::paint(juce::Graphics& g)
{
    g.fillAll(BG_COLOR);

    if (rowComps.isEmpty())
    {
        g.setColour(Colours::white.withAlpha(.35f));
        g.setFont(16.0f);
        g.drawText("(no cue playing)", getLocalBounds(), Justification::centred);
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
    startTimerHz(100);
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
    if (redundancyBlocksControl()) return; // Backup standby: no STOP ALL

    if (getStopAllRect().contains(e.getPosition()))
    {
        // A sub-cue whose group is playing is panicked by that group (single fade); skip it
        // here. One launched on its own is panicked directly.
        for (Cue* c : collectActiveCues())
            if (c->parentGroup == nullptr || !c->parentGroup->isPlaying->boolValue())
                c->panic();
        repaint();
    }
}

void ActiveCuesPanel::timerCallback()
{
    // Sync the child rows to the currently active cues and lay them out.
    rows.setCues(collectActiveCues(), jmax(0, viewport.getMaximumVisibleWidth()), viewport.getHeight());
    repaint(); // header count
}
