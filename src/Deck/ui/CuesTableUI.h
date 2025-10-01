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
#include "../../ui/LookAndFeelTable.h"
#include "../../Cuelist/Cuelist.h"

class CuesTableUI :
    public Component
{
public:
    CuesTableUI(Cuelist* cl);
    ~CuesTableUI() override;

    Cuelist* cl = nullptr;

    TableListBox tableListBox;
    std::unique_ptr<CuesTableModel> tableModel;
    std::unique_ptr<LookAndFeelTable> lafTable;

    void paint (Graphics&) override;
    void resized() override;
    void fillText();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CuesTableUI)
};
