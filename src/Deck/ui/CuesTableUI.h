/*
  ==============================================================================

    CuesTableUI.h
    Created: 30 Sep 2025 11:01:57pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "CuesTableModel.h"

class CuesTableUI :
    public Component
{
public:
    CuesTableUI();
    ~CuesTableUI() override;

    TableListBox tableListBox;
    std::unique_ptr<CuesTableModel> tableModel;

    void paint (Graphics&) override;
    void resized() override;
    void fillText();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CuesTableUI)
};
