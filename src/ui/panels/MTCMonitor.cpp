/*
  ==============================================================================

    MTCMonitor.cpp
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "MTCMonitor.h"
#include "../../Cue/audio/AudioCue.h"
#include "../../Interface/midi/MIDIInterface.h"

//==============================================================================
MTCMonitorUI::MTCMonitorUI(const String& contentName) :
    ShapeShifterContent(MTCMonitor::getInstance(), contentName)
{
}

MTCMonitorUI::~MTCMonitorUI()
{
}

juce_ImplementSingleton(MTCMonitor);

MTCMonitor::MTCMonitor()
{
    startTimerHz(20);
}

MTCMonitor::~MTCMonitor()
{
    stopTimer();
}

void MTCMonitor::paint(juce::Graphics& g)
{
    g.fillAll(BG_COLOR);

    auto& senders = AudioCue::activeMTCSenders;

    if (senders.size() == 0)
    {
        g.setColour(Colours::white.withAlpha(.35f));
        g.setFont(16.0f);
        g.drawText("(no MTC running)", getLocalBounds(), Justification::centred);
        return;
    }

    // Center the whole block vertically.
    int count = senders.size();
    int rowH = jmin(70, getHeight() / count);
    int y = jmax(0, (getHeight() - rowH * count) / 2);

    for (HashMap<MIDIInterface*, AudioCue*>::Iterator it(senders); it.next();)
    {
        MIDIInterface* iface = it.getKey();
        AudioCue* cue = it.getValue();
        if (iface == nullptr || cue == nullptr) continue;

        Rectangle<int> row = Rectangle<int>(0, y, getWidth(), rowH).reduced(8, 4);

        // Interface name, originating cue and frame rate on top, centered.
        g.setColour(Colours::white.withAlpha(.55f));
        g.setFont(12.0f);
        g.drawText(iface->niceName + "  -  " + cue->niceName + "   (" + cue->mtcFrameRate->getValueKey() + ")",
                   row.removeFromTop(16), Justification::centred, true);

        // Big live timecode below, centered.
        g.setColour(Colours::limegreen);
        g.setFont(Font(jmin((float)row.getHeight() - 2, 30.0f), Font::bold));
        g.drawText(cue->mtcTimecodeDisplay->stringValue(), row, Justification::centred, true);

        y += rowH;
    }
}

void MTCMonitor::timerCallback()
{
    repaint();
}
