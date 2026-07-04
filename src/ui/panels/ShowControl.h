/*
  ==============================================================================

    ShowControl.h
    Created: 19 Sep 2025 12:15:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "juce_organicui/controllable/parameter/BoolParameter.h"

class Cuelist;

class ShowControlUI : public ShapeShifterContent {
public:
    ShowControlUI(const String &contentName);
    ~ShowControlUI();

    static ShowControlUI *create(const String &name) { return new ShowControlUI(name); }
};

class ShowControl :
    public ControllableContainer,
    public Component,
    public Timer,
    public BaseManagerListener<Cuelist>
{
public:
    juce_DeclareSingleton(ShowControl, true);
    ShowControl();
    ~ShowControl() override;

    Cuelist* mainCuelist = nullptr;

    BoolParameter* isPanicking;
    Trigger* paramGo; TriggerButtonUI* btnGo;
    Trigger* paramPanic; TriggerButtonUI* btnPanic;

    void startPanicking();
    void stopPanicking();

    // Panic is global (all cuelists): track each cuelist's play/panic state.
    bool anyCuelistPlaying() const;
    bool anyCuelistPanicking() const;
    void addCuelistPanicListeners(Cuelist* cl);
    void removeCuelistPanicListeners(Cuelist* cl);
    void refreshPanicState();

    void itemAdded(Cuelist* c) override;
    void itemsAdded(Array<Cuelist*> items) override;
    void itemRemoved(Cuelist* c) override;
    void itemsRemoved(Array<Cuelist*> items) override;

    void paint (Graphics&) override;
    void resized() override;
    void triggerTriggered(Trigger* t) override;

    void parameterValueChanged(Parameter* p) override;

    void timerCallback() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShowControl)
};
