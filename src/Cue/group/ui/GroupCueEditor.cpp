/*
  ==============================================================================

    GroupCueEditor.cpp
    Created: 05 Jul 2026
    Author:  boherm

  ==============================================================================
*/

#include "GroupCueEditor.h"
#include "../GroupCue.h"
#include "../../CueManager.h"
#include "../../../Cuelist/Cuelist.h"
#include "../../audio/AudioCue.h"
#include "../../audio/AudioFile.h"
#include "../../audio/AudioSlices.h"

namespace
{
    constexpr int kRuler = 18;
    constexpr int kLaneH = 46;
    constexpr int kLaneGap = 4;
    constexpr int kLeftPad = 4;
    constexpr int kRightPad = 8;
    constexpr int kLabelH = 14; // thin label strip at the top of each block

    // A "nice" tick step so the ruler shows ~8 divisions.
    double niceStep(double span)
    {
        double raw = jmax(0.0001, span / 8.0);
        double mag = std::pow(10.0, std::floor(std::log10(raw)));
        double n = raw / mag;
        double stepN = n < 1.5 ? 1.0 : n < 3.0 ? 2.0 : n < 7.0 ? 5.0 : 10.0;
        return stepN * mag;
    }

    String fmtTime(double s)
    {
        if (s < 60.0)
            return String(s, s < 10.0 ? 1 : 0) + "s";
        int m = (int) (s / 60.0);
        return String(m) + ":" + String((int) (s - m * 60.0)).paddedLeft('0', 2);
    }
}

//==============================================================================
// GroupTimeline
//==============================================================================

GroupTimeline::GroupTimeline(GroupCue* group) : group(group)
{
    formatManager.registerBasicFormats();
    if (group != nullptr && group->parentCuelist != nullptr && group->parentCuelist->cues != nullptr)
        group->parentCuelist->cues->addAsyncContainerListener(this);
}

GroupTimeline::~GroupTimeline()
{
    if (group != nullptr && group->parentCuelist != nullptr && group->parentCuelist->cues != nullptr)
        group->parentCuelist->cues->removeAsyncContainerListener(this);
}

double GroupTimeline::memberStart(const Cue* m)
{
    return m->preWaitCC->enabled->boolValue() ? m->preWaitDuration->doubleValue() : 0.0;
}

double GroupTimeline::memberDuration(const Cue* m)
{
    return m->duration->doubleValue();
}

int GroupTimeline::preferredHeight() const
{
    int n = jmax(1, group->getMembers().size());
    return kRuler + n * (kLaneH + kLaneGap) + 4;
}

double GroupTimeline::span() const
{
    return jmax(1.0, group->duration->doubleValue() * 1.08);
}

juce::Rectangle<float> GroupTimeline::plotArea() const
{
    return juce::Rectangle<float>((float) kLeftPad, (float) kRuler,
                                  jmax(1.0f, (float) getWidth() - kLeftPad - kRightPad),
                                  (float) getHeight() - kRuler);
}

float GroupTimeline::timeToX(double t) const
{
    float w = jmax(1.0f, (float) getWidth() - kLeftPad - kRightPad);
    return (float) kLeftPad + (float) (t / span()) * w;
}

double GroupTimeline::xToTime(float x) const
{
    float w = jmax(1.0f, (float) getWidth() - kLeftPad - kRightPad);
    return jmax(0.0, (double) ((x - (float) kLeftPad) / w) * span());
}

juce::Rectangle<float> GroupTimeline::laneRect(int index) const
{
    float y = (float) kRuler + index * (kLaneH + kLaneGap);
    return juce::Rectangle<float>((float) kLeftPad, y,
                                  jmax(1.0f, (float) getWidth() - kLeftPad - kRightPad), (float) kLaneH);
}

juce::Rectangle<float> GroupTimeline::blockRect(Cue* m, int index) const
{
    double start = memberStart(m);
    double dur = memberDuration(m);
    float x = timeToX(start);
    float w = jmax(3.0f, timeToX(start + dur) - x); // min width so a 0-dur cue stays grabbable
    auto lane = laneRect(index);
    return juce::Rectangle<float>(x, lane.getY(), w, lane.getHeight());
}

Cue* GroupTimeline::memberAtY(float y, int& indexOut) const
{
    auto members = group->getMembers();
    for (int i = 0; i < members.size(); i++)
    {
        auto lane = laneRect(i);
        if (y >= lane.getY() && y <= lane.getBottom()) { indexOut = i; return members[i]; }
    }
    indexOut = -1;
    return nullptr;
}

juce::AudioThumbnail* GroupTimeline::thumbnailFor(const juce::File& file)
{
    if (!file.existsAsFile()) return nullptr;

    const juce::String key = file.getFullPathName();
    auto it = thumbnails.find(key);
    if (it != thumbnails.end())
        return it->second.get();

    auto th = std::make_unique<juce::AudioThumbnail>(512, formatManager, thumbnailCache);
    th->addChangeListener(this); // repaint as the file loads
    th->setSource(new juce::FileInputSource(file));
    auto* raw = th.get();
    thumbnails[key] = std::move(th);
    return raw;
}

void GroupTimeline::changeListenerCallback(juce::ChangeBroadcaster*)
{
    repaint();
}

juce::String GroupTimeline::getTooltip()
{
    int idx = -1;
    if (Cue* m = memberAtY(getMouseXYRelative().toFloat().y, idx))
        return m->id->stringValue() + " · " + m->getDescription();
    return {};
}

void GroupTimeline::paint(juce::Graphics& g)
{
    g.fillAll(BG_COLOR.darker(0.15f));

    const double sp = span();
    const double step = niceStep(sp);

    // Ruler + gridlines.
    g.setFont(11.0f);
    for (double t = 0.0; t <= sp + 1e-6; t += step)
    {
        float x = timeToX(t);
        g.setColour(Colours::white.withAlpha(0.10f));
        g.drawVerticalLine((int) x, (float) kRuler, (float) getHeight());
        g.setColour(Colours::white.withAlpha(0.5f));
        g.drawText(fmtTime(t), (int) x + 2, 0, 50, kRuler, Justification::centredLeft, false);
    }

    auto members = group->getMembers();
    for (int i = 0; i < members.size(); i++)
    {
        Cue* m = members[i];
        auto lane = laneRect(i);
        g.setColour(Colours::black.withAlpha(0.25f));
        g.fillRect(lane);

        auto block = blockRect(m, i);
        // Custom colour, or white when the sub-cue has no colour set (still the default).
        Colour col = m->itemColor->getColor();
        if (col == BG_COLOR) col = Colours::white;

        // Subtle fill so an audio waveform stays readable underneath.
        g.setColour(col.withAlpha(0.20f));
        g.fillRoundedRectangle(block, 3.0f);

        // Reserve a thin strip at the top for the label; the waveform gets the rest.
        auto waveArea = block.withTrimmedTop((float) kLabelH);

        // Audio Cue: draw its waveform (the playable [start, end] window) below the label.
        if (auto* ac = dynamic_cast<AudioCue*>(m))
        {
            juce::File file;
            if (!ac->filesManager->items.isEmpty())
                file = ac->filesManager->items.getFirst()->audioFile->getFile();
            double ws = ac->slicesManager->startTime->doubleValue();
            double we = ac->slicesManager->endTime->doubleValue();
            if (auto* th = thumbnailFor(file))
            {
                if (th->getNumChannels() > 0 && we > ws)
                {
                    g.saveState();
                    g.reduceClipRegion(waveArea.getSmallestIntegerContainer());
                    g.setColour(col.withAlpha(0.85f));
                    th->drawChannel(g, waveArea.reduced(1.0f).toNearestInt(), ws, we, 0, 1.0f);
                    g.restoreState();
                }
            }
        }

        // Border = the sub-cue's own colour (white if unset).
        g.setColour(col);
        g.drawRoundedRectangle(block, 3.0f, 1.5f);

        // Discreet label in the top strip; the full "id · name" is on hover (getTooltip).
        g.setColour(Colours::white.withAlpha(0.9f));
        g.setFont(11.0f);
        g.drawText(m->id->stringValue() + "  " + m->getDescription(),
                   block.withHeight((float) kLabelH).reduced(5.0f, 0.0f),
                   Justification::centredLeft, true);
    }

    // Playhead while the group is running.
    if (group->isPlaying->boolValue())
    {
        float px = timeToX(group->currentTime->doubleValue());
        g.setColour(Colours::red.withAlpha(0.85f));
        g.drawVerticalLine((int) px, (float) kRuler, (float) getHeight());
    }
}

void GroupTimeline::mouseDown(const juce::MouseEvent& e)
{
    dragged = nullptr;
    auto members = group->getMembers();
    for (int i = 0; i < members.size(); i++)
    {
        auto lane = laneRect(i);
        if (e.position.y >= lane.getY() && e.position.y <= lane.getBottom())
        {
            dragged = members[i];
            grabOffset = xToTime(e.position.x) - memberStart(dragged);
            break;
        }
    }
}

void GroupTimeline::mouseDrag(const juce::MouseEvent& e)
{
    if (dragged == nullptr) return;

    double newStart = jmax(0.0, xToTime(e.position.x) - grabOffset);
    dragged->preWaitDuration->setValue((float) newStart);
    dragged->preWaitCC->enabled->setValue(newStart > 0.0001);
    repaint();
}

void GroupTimeline::mouseUp(const juce::MouseEvent&)
{
    dragged = nullptr;
}

void GroupTimeline::layoutForViewport(int visibleWidth)
{
    fitWidth = jmax(1, visibleWidth);
    applyZoom();
}

void GroupTimeline::applyZoom()
{
    setSize(jmax(fitWidth, (int) std::round(fitWidth * zoom)), preferredHeight());
}

void GroupTimeline::zoomAt(float anchorX, double factor)
{
    if (viewport == nullptr)
    {
        zoom = jlimit(1.0, 300.0, zoom * factor);
        applyZoom();
        return;
    }

    const int oldVX = viewport->getViewPositionX();
    const float screenX = anchorX - (float) oldVX;   // mouse position within the viewport
    const double tUnder = xToTime(anchorX);          // time under the mouse (absolute coords)

    zoom = jlimit(1.0, 300.0, zoom * factor);
    applyZoom();

    const float newAbsX = timeToX(tUnder);
    viewport->setViewPosition(jmax(0, (int) std::round(newAbsX - screenX)), 0);
}

void GroupTimeline::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        double dy = wheel.deltaY * (wheel.isReversed ? -1.0 : 1.0);
        zoomAt(e.position.x, std::pow(1.25, dy * 5.0));
    }
    else if (viewport != nullptr)
    {
        double delta = (wheel.deltaX != 0.0f ? wheel.deltaX : wheel.deltaY);
        viewport->setViewPosition(jmax(0, viewport->getViewPositionX() - (int) std::round(delta * 140.0)), 0);
    }
}

void GroupTimeline::mouseMagnify(const juce::MouseEvent& e, float scaleFactor)
{
    zoomAt(e.position.x, (double) scaleFactor);
}

void GroupTimeline::newMessage(const ContainerAsyncEvent& e)
{
    // Any change in the cuelist's cues (member pre-wait/duration, add/remove, playhead) →
    // redraw the timeline.
    if (group != nullptr && group->parentCuelist != nullptr && e.source == group->parentCuelist->cues)
    {
        int n = group->getMembers().size();
        if (n != lastMemberCount)
        {
            lastMemberCount = n;
            if (onMembersChanged) onMembersChanged(); // let the host re-reserve height
        }
        repaint();
    }
}

//==============================================================================
// GroupCueEditor
//==============================================================================

GroupCueEditor::GroupCueEditor(juce::Array<Cue*> cues, bool isRoot) :
    CueEditor(cues, isRoot)
{
    group = dynamic_cast<GroupCue*>(cues.getFirst());
    timeline.reset(new GroupTimeline(group));
    timeline->setViewport(&viewport);
    timeline->onMembersChanged = [this] { resized(); };

    viewport.setViewedComponent(timeline.get(), false);
    viewport.setScrollBarsShown(false, false, false, true); // horizontal scrollbar only
    viewport.setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);
    addChildComponent(viewport);

    updateTimelineVisibility();
}

GroupCueEditor::~GroupCueEditor()
{
}

void GroupCueEditor::updateTimelineVisibility()
{
    const bool show = group != nullptr && group->fireMode->intValue() == GroupCue::TIMELINE;
    viewport.setVisible(show);
}

void GroupCueEditor::resizedInternalContent(juce::Rectangle<int>& r)
{
    GenericControllableContainerEditor::resizedInternalContent(r); // the standard parameter rows

    if (viewport.isVisible())
    {
        r.removeFromTop(6);
        const int scrollbar = 10;
        auto tr = r.removeFromTop(timeline->preferredHeight() + scrollbar);
        viewport.setBounds(tr.reduced(4, 0));
        timeline->layoutForViewport(viewport.getMaximumVisibleWidth());
    }
}

void GroupCueEditor::newMessage(const ContainerAsyncEvent& e)
{
    CueEditor::newMessage(e);

    // Fire Mode toggled → show/hide the timeline and re-flow the editor height.
    if (e.type == ContainerAsyncEvent::EventType::ControllableFeedbackUpdate
        && group != nullptr && e.targetControllable == group->fireMode)
    {
        updateTimelineVisibility();
        resized();
    }
}
