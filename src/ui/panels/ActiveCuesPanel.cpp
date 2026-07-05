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
                bool active = c->isPlaying->boolValue() || c->preWaitActive->boolValue() || c->postWaitActive->boolValue();
                // A ducking audio cue in its post-play phase is no longer "playing" but must
                // stay listed (pre-play is already covered by isPlaying).
                if (auto* ac = dynamic_cast<AudioCue*>(c))
                    active = active || ac->duckPrePlayActive->boolValue() || ac->duckPostPlayActive->boolValue();
                if (active)
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

    const Colour rowBaseColor = Colour::fromRGB(43, 43, 43);   // unfilled part of the row
    const Colour progressColor = Colour::fromRGB(82, 121, 63); // green progress fill (playing)
    const Colour waitColor = Colour::fromRGB(150, 112, 45);    // amber fill (pre / post-wait)
    const Colour duckColor = Colour::fromRGB(60, 100, 150);    // blue fill (duck pre / post-play)
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

// Full-width draggable volume fader glued to the bottom of the row.
Rectangle<int> ActiveCuesRows::getFaderRect(int rowIndex) const
{
    const int h = 12;
    int y = rowIndex * rowH + rowH - h;
    return Rectangle<int>(0, y, getWidth(), h);
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
        AudioCue* ac = dynamic_cast<AudioCue*>(c);
        if (c->preWaitActive->boolValue())
        {
            cur = c->preWaitCurrentTime->doubleValue();
            total = c->preWaitDuration->doubleValue();
            fillColor = waitColor;
            waitLabel = "Pre-wait";
        }
        else if (ac != nullptr && ac->duckPrePlayActive->boolValue())
        {
            cur = ac->duckPrePlayCurrentTime->doubleValue();
            total = ac->duckPrePlayDuration->doubleValue();
            fillColor = duckColor;
            waitLabel = "Pre-play";
        }
        else if (ac != nullptr && ac->duckPostPlayActive->boolValue())
        {
            cur = ac->duckPostPlayCurrentTime->doubleValue();
            total = ac->duckPostPlayDuration->doubleValue();
            fillColor = duckColor;
            waitLabel = "Post-play";
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

        // Volume fader (audio / playlist cues): track + fill at the live output level + a thumb
        // at the user-set volume. The fill drops and recovers while the cue is ducked.
        if (c->hasVolumeFader())
        {
            Rectangle<int> f = getFaderRect(i);
            if (f.getWidth() > 0)
            {
                g.setColour(Colours::black.withAlpha(.4f));
                g.fillRect(f);

                float maxV = jmax(0.0001f, c->getMaxFaderVolume());
                float lvl = jlimit(0.0f, 1.0f, c->getOutputLevel() / maxV);
                if (lvl > 0.0f)
                {
                    g.setColour(timeColor.withAlpha(.85f));
                    g.fillRect(f.withWidth(roundToInt(f.getWidth() * lvl)));
                }

                float vol = jlimit(0.0f, 1.0f, c->getFaderVolume() / maxV);
                int tx = f.getX() + roundToInt(f.getWidth() * vol);
                g.setColour(Colours::white);
                g.fillRect(tx - 1, f.getY(), 2, f.getHeight());
            }
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

    // Arm dragging a volume fader (audio / playlist cues). The volume only changes once the
    // mouse actually moves, so a plain click (or the first click of a double-click) never jumps it.
    for (int i = 0; i < cues.size(); i++)
    {
        if (!cues[i]->hasVolumeFader()) continue;
        Rectangle<int> f = getFaderRect(i);
        if (f.getWidth() > 0 && f.contains(e.getPosition()))
        {
            dragFaderRow = i;
            return;
        }
    }

    // Click on the progress bar of a seekable cue (audio, group, ...) to seek it.
    for (int i = 0; i < cues.size(); i++)
    {
        Cue* c = cues[i];
        if (c->preWaitActive->boolValue()) continue; // the bar shows the pre-wait, not playback
        if (!c->canSeekTo()) continue;

        Rectangle<int> track = getProgressRect(i);
        if (track.getWidth() <= 0 || !track.contains(e.getPosition())) continue;

        double total = c->duration->doubleValue();
        if (total <= 0.0) continue;

        double frac = jlimit(0.0, 1.0, (double)(e.getPosition().x - track.getX()) / (double)track.getWidth());
        c->seekToTime(frac * total);
        repaint();
        return;
    }
}

void ActiveCuesRows::mouseDrag(const juce::MouseEvent& e)
{
    if (dragFaderRow < 0) return;

    Array<Cue*> cues = collectActiveCues();
    if (dragFaderRow >= cues.size()) { dragFaderRow = -1; return; }

    Rectangle<int> f = getFaderRect(dragFaderRow);
    if (f.getWidth() <= 0) return;

    float frac = jlimit(0.0f, 1.0f, (float)(e.getPosition().x - f.getX()) / (float)f.getWidth());
    cues[dragFaderRow]->setFaderVolume(frac * cues[dragFaderRow]->getMaxFaderVolume());
    repaint();
}

void ActiveCuesRows::mouseUp(const juce::MouseEvent&)
{
    dragFaderRow = -1;
}

juce::String ActiveCuesRows::getTooltip()
{
    Point<int> p = getMouseXYRelative();
    Array<Cue*> cues = collectActiveCues();
    for (int i = 0; i < cues.size(); i++)
    {
        if (!cues[i]->hasVolumeFader()) continue;
        if (getFaderRect(i).contains(p))
            return "Volume: " + String(cues[i]->getFaderVolume(), 2);
    }
    return {};
}

void ActiveCuesRows::mouseDoubleClick(const juce::MouseEvent& e)
{
    // Double-click a fader to type an exact volume, using the same bubble editor as the Deck.
    Array<Cue*> cues = collectActiveCues();
    for (int i = 0; i < cues.size(); i++)
    {
        if (!cues[i]->hasVolumeFader()) continue;
        Rectangle<int> f = getFaderRect(i);
        if (f.getWidth() <= 0 || !f.contains(e.getPosition())) continue;

        Cue* cue = cues[i];
        String initial = String(cue->getFaderVolume(), 2);
        std::function<void(const String&)> onCommit = [cue](const String& t)
        {
            cue->setFaderVolume(jlimit(0.0f, 1.5f, t.getFloatValue()));
        };

        auto bubble = std::make_unique<VolumeEditBubble>(initial, std::move(onCommit));
        CallOutBox::launchAsynchronously(std::move(bubble), localAreaToGlobal(f), nullptr);
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
    int count = collectActiveCues().size();
    int w = jmax(0, viewport.getMaximumVisibleWidth());
    int h = jmax(count * ActiveCuesRows::rowH, viewport.getHeight());

    if (rows.getWidth() != w || rows.getHeight() != h)
        rows.setSize(w, h);

    rows.repaint();
    repaint(); // header count
}
