/*
  ==============================================================================

    ShowInfos.h
    Created: 4 Nov 2025 10:50:23am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class Cuelist;
class Cue;

class ShowInfosUI : public ShapeShifterContent {
public:
    ShowInfosUI(const String &contentName);
    ~ShowInfosUI();

    static ShowInfosUI *create(const String &name) { return new ShowInfosUI(name); }
};

class ShowInfos :
    public ControllableContainer,
    public Component
{
public:
    juce_DeclareSingleton(ShowInfos, true);
    ShowInfos();
    ~ShowInfos() override;

    Cuelist* mainCuelist = nullptr;
    Cue* currentCue = nullptr;
    Cue* nextCue = nullptr;

    void paint (Graphics&) override;
    void resized() override;

    void parameterValueChanged(Parameter* p) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShowInfos)
};
