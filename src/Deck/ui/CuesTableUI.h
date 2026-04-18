/*
  ==============================================================================

    CuesTableUI.h
    Created: 30 Sep 2025 11:01:57pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../../Cue/Cue.h"

class Cuelist;
class CuesTableModel;
class LookAndFeelTable;

class CuesTableUI :
    public Component,
    public BaseManagerListener<Cue>,
    public ContainerAsyncListener,
    public DragAndDropContainer,
    public DragAndDropTarget,
    public FileDragAndDropTarget,
    public WarningTarget::AsyncListener
{
public:
    CuesTableUI(Cuelist* cl);
    ~CuesTableUI() override;

    Cuelist* cl = nullptr;

    TableListBox tableListBox;
    std::unique_ptr<CuesTableModel> tableModel;
    std::unique_ptr<LookAndFeelTable> lafTable;

    void paint (Graphics&) override;
    void paintOverChildren (Graphics&) override;
    void resized() override;

    void itemAdded(Cue* c)
    {
        c->addAsyncWarningTargetListener(this);
        tableListBox.updateContent();
        tableListBox.repaint();
    }

    void itemsAdded(Array<Cue*> items)
    {
        for (auto* c : items)
            c->addAsyncWarningTargetListener(this);
        tableListBox.updateContent();
        tableListBox.repaint();
    }

	void itemRemoved(Cue* c)
    {
        c->removeAsyncWarningTargetListener(this);
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

    void newMessage(const WarningTarget::WarningTargetEvent& e) override
    {
        tableListBox.updateContent();
        tableListBox.repaint();
    }

    // DragAndDropTarget implementation
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    // FileDragAndDropTarget
    bool isInterestedInFileDrag (const juce::StringArray& files) override;

    void fileDragMove (const juce::StringArray& /*files*/, int /*x*/, int /*y*/) override;

    void fileDragEnter (const juce::StringArray& /*files*/, int /*x*/, int /*y*/) override;

    void fileDragExit (const juce::StringArray& /*files*/) override;

    void filesDropped (const juce::StringArray& files, int /*x*/, int /*y*/) override;

private:
    int insertIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CuesTableUI)
};
