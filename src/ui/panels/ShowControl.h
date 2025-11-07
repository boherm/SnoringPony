/*
  ==============================================================================

    ShowControl.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class Cuelist;

class ShowControlUI : public ShapeShifterContent {
public:
    ShowControlUI(const String &contentName);
    ~ShowControlUI();

    static ShowControlUI *create(const String &name) { return new ShowControlUI(name); }
};

class ShowControl :
    public ControllableContainer,
    public Component
{
public:
    juce_DeclareSingleton(ShowControl, true);
    ShowControl();
    ~ShowControl() override;

    Cuelist* mainCuelist = nullptr;

    Trigger* paramGo; TriggerButtonUI* btnGo;
    Trigger* paramPanic; TriggerButtonUI* btnPanic;

    void paint (Graphics&) override;
    void resized() override;
    void triggerTriggered(Trigger* t) override;

    void parameterValueChanged(Parameter* p) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShowControl)
};
