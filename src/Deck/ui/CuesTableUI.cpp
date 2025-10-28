/*
  ==============================================================================

    CuesTableUI.cpp
    Created: 30 Sep 2025 11:01:57pm
    Author:  boherm

  ==============================================================================
*/

#include "CuesTableUI.h"
#include "juce_graphics/juce_graphics.h"

enum ColumnIds
{
    IdColumn = 1,
    TypeColumn = 2,
    DescriptionColumn = 3,
    TimeColumn = 4
};

CuesTableUI::CuesTableUI(Cuelist* cl)
{
    this->cl = cl;

    if (cl == nullptr) {
        return;
    }

    tableModel = std::make_unique<CuesTableModel>(&tableListBox, cl);
    lafTable = std::make_unique<LookAndFeelTable>();

    tableListBox.setLookAndFeel(lafTable.get());

    tableListBox.setMultipleSelectionEnabled(true);
    tableListBox.setAutoSizeMenuOptionShown(false);
    tableListBox.setModel(tableModel.get());
    tableListBox.setRowHeight(30);
    int flags = TableHeaderComponent::visible | TableHeaderComponent::resizable | TableHeaderComponent::appearsOnColumnMenu;
    tableListBox.getHeader().addColumn("#", IdColumn, 55, 55, 100, flags & ~TableHeaderComponent::appearsOnColumnMenu);
    tableListBox.getHeader().addColumn("Type", TypeColumn, 40, 40, 40, flags & ~TableHeaderComponent::resizable);
    tableListBox.getHeader().addColumn("Description", DescriptionColumn, 200, 200, 3000, flags & ~TableHeaderComponent::appearsOnColumnMenu);
    tableListBox.getHeader().addColumn("Time", TimeColumn, 200, 200, 200, flags & ~TableHeaderComponent::resizable);

    addAndMakeVisible(tableListBox);
    cl->cues.addBaseManagerListener(this);

    resized();
}

CuesTableUI::~CuesTableUI()
{
    cl->cues.removeBaseManagerListener(this);
    tableListBox.setLookAndFeel(nullptr);
    lafTable.reset();
}

void CuesTableUI::paint(Graphics& g)
{
}

void CuesTableUI::resized()
{
    int width = getWidth();
    width -= tableListBox.getViewport()->getScrollBarThickness();

    tableListBox.setBounds(getLocalBounds());

    tableListBox.getHeader().setColumnWidth(IdColumn, 55);
    width -= 55;

    if (tableListBox.getHeader().isColumnVisible(TypeColumn)) {
        tableListBox.getHeader().setColumnWidth(TypeColumn, 40);
        width -= 40;
    }

    if (tableListBox.getHeader().isColumnVisible(TimeColumn)) {
        tableListBox.getHeader().setColumnWidth(TimeColumn, 200);
        width -= 200;
    }

    tableListBox.getHeader().setColumnWidth(DescriptionColumn, width);
}
