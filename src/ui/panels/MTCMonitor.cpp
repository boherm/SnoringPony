/*
  ==============================================================================

    MTCMonitor.cpp
    Created: 25 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "MTCMonitor.h"
#include "../../Cue/Cue.h"
#include "../../Interface/midi/MTCSender.h"
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
    startTimerHz(1000);
}

MTCMonitor::~MTCMonitor()
{
    stopTimer();
}

void MTCMonitor::paint(juce::Graphics& g)
{
    g.fillAll(BG_COLOR);

    auto& senders = MTCSender::activeSenders;

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

    for (HashMap<MIDIInterface*, MTCSender*>::Iterator it(senders); it.next();)
    {
        MIDIInterface* iface = it.getKey();
        MTCSender* sender = it.getValue();
        if (iface == nullptr || sender == nullptr || sender->owner == nullptr) continue;

        Rectangle<int> row = Rectangle<int>(0, y, getWidth(), rowH).reduced(8, 4);

        // Interface name, originating cue and frame rate on top, centered.
        g.setColour(Colours::white.withAlpha(.55f));
        g.setFont(12.0f);
        g.drawText(iface->niceName + "  -  " + sender->owner->niceName + "   (" + sender->frameRate->getValueKey() + ")",
                   row.removeFromTop(16), Justification::centred, true);

        // Big live timecode below, centered.
        g.setColour(Colours::limegreen);
        g.setFont(Font(jmin((float)row.getHeight() - 2, 30.0f), Font::bold));
        g.drawText(sender->timecodeDisplay->stringValue(), row, Justification::centred, true);

        y += rowH;
    }
}

void MTCMonitor::timerCallback()
{
    repaint();
}
