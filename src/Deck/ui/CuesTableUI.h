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
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_organicui/manager/BaseManagerListener.h"
#include "../../Cue/Cue.h"

class CuesTableUI :
    public Component,
    public BaseManagerListener<Cue>,
    public ContainerAsyncListener
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

    void addCuesListeners();

    void itemAdded(Cue* c)
    {
        tableListBox.updateContent();
        tableListBox.repaint();
    }

	void itemRemoved(Cue* c)
    {
        tableListBox.updateContent();
        tableListBox.repaint();
    }
	void itemsReordered()
    {
        tableListBox.updateContent();
        tableListBox.repaint();
    }

    void newMessage(const ContainerAsyncEvent& e) override
    {
        tableListBox.updateContent();
        tableListBox.repaint();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CuesTableUI)
};
