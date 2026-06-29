/*
  ==============================================================================

    RFDeviceManagerItemUI.cpp
    Created: 26 Jun 2026
    Author:  boherm

  ==============================================================================
*/

#include "RFDeviceManagerItemUI.h"
#include "../RFDeviceManager.h"

RFDeviceManagerItemUI::RFDeviceManagerItemUI(RFDevice* item) :
    BaseItemUI(item)
{
    // Hide the color picker in the list (it stays available in the inspector).
    showColorUI = false;
    if (itemColorUI != nullptr) itemColorUI->setVisible(false);

    // Refresh the row when the manager's "Display presets" toggle changes.
    RFDeviceManager::getInstance()->addAsyncContainerListener(this);
}

RFDeviceManagerItemUI::~RFDeviceManagerItemUI()
{
    if (RFDeviceManager::getInstanceWithoutCreating() != nullptr)
        RFDeviceManager::getInstance()->removeAsyncContainerListener(this);
}

void RFDeviceManagerItemUI::mouseDown(const MouseEvent& e)
{
    // Clicking the preset/frequency box toggles the global display mode.
    if (valueBox.contains(e.getPosition()))
    {
        BoolParameter* dp = RFDeviceManager::getInstance()->displayPresets;
        dp->setValue(!dp->boolValue());
        return;
    }

    BaseItemUI<RFDevice>::mouseDown(e);
}

void RFDeviceManagerItemUI::newMessage(const ContainerAsyncEvent& e)
{
    BaseItemMinimalUI<RFDevice>::newMessage(e);

    if (e.targetControllable == RFDeviceManager::getInstance()->displayPresets)
        repaint();
}

void RFDeviceManagerItemUI::controllableFeedbackUpdateInternal(Controllable* c)
{
    BaseItemUI<RFDevice>::controllableFeedbackUpdateInternal(c);
    repaint();   // refresh the assigned-value box when the optimizer updates it
}

void RFDeviceManagerItemUI::paint(Graphics& g)
{
    BaseItemUI::paint(g);

    Rectangle<int> r = getLocalBounds().withTrimmedLeft(8).withTrimmedRight(22);

    const bool presets = item->getCandidateMode() == RFProfile::PRESETS
        && RFDeviceManager::getInstance()->displayPresets->boolValue();
    const double f = item->assignedFreqMHz->floatValue();

    String val, tip;
    if (f <= 0.0)
    {
        val = "--";
    }
    else if (presets)
    {
        String pn = item->assignedPresetName->stringValue();
        val = pn.isNotEmpty() ? pn : String(f, 3);
        tip = String(f, 3);   // frequency on hover in Presets mode
    }
    else
    {
        val = String(f, 3);
    }
    setTooltip(tip);

    Font valFont(14.0f);
    int valW = valFont.getStringWidth(val);
    int boxW = valW + 24;
    int boxH = jmin(r.getHeight() - 4, 22);
    Rectangle<int> box = r.removeFromRight(boxW).withSizeKeepingCentre(boxW, boxH);
    valueBox = box;   // remember for click hit-testing in mouseDown

    g.setColour(BG_COLOR.darker(.2f).withAlpha(.6f));
    g.fillRoundedRectangle(box.toFloat(), 4.0f);
    g.setColour(BG_COLOR.brighter(.1f));
    g.drawRoundedRectangle(box.toFloat().reduced(0.5f), 4.0f, 1.0f);

    g.setFont(valFont);
    g.setColour(f <= 0.0 ? Colours::white.withAlpha(.4f) : Colours::white.withAlpha(.9f));
    g.drawText(val, box, Justification::centred);
}
