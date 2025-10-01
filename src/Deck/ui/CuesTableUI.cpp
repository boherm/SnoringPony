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
    NameColumn = 1,
    TimeColumn = 2,
    DescriptionColumn = 3,
    ActiveColumn = 4
};

CuesTableUI::CuesTableUI(Cuelist* cl)
{
    this->cl = cl;

    if (cl == nullptr) {
        return;
    }

    tableModel = std::make_unique<CuesTableModel>(cl);
    lafTable = std::make_unique<LookAndFeelTable>();

    tableListBox.setLookAndFeel(lafTable.get());

    tableListBox.setAutoSizeMenuOptionShown(false);
    tableListBox.setModel(tableModel.get());
    tableListBox.setRowHeight(30);
    int flags = TableHeaderComponent::visible | TableHeaderComponent::resizable | TableHeaderComponent::appearsOnColumnMenu;
    tableListBox.getHeader().addColumn("Nom", NameColumn, 200, 50, 1500, flags & ~TableHeaderComponent::appearsOnColumnMenu);
    tableListBox.getHeader().addColumn("Temps", TimeColumn, 200, 50, 1500, flags);
    tableListBox.getHeader().addColumn("Description", DescriptionColumn, 200, 50, 1500, flags);
    // tableListBox.getHeader().addColumn("Actif", ActiveColumn, 200, 50, 1000, flags);

    addAndMakeVisible(tableListBox);

    fillText();
    resized();
}

CuesTableUI::~CuesTableUI()
{
    tableListBox.setLookAndFeel(nullptr);
    lafTable.reset();
}

void CuesTableUI::paint(Graphics& g)
{
    g.fillAll(Colours::aliceblue);
}

void CuesTableUI::resized()
{
    tableListBox.setBounds(getLocalBounds());
    tableListBox.getHeader().setColumnWidth(1, getWidth() * (1.0/12));
    tableListBox.getHeader().setColumnWidth(2, getWidth() * (2.0/12));
    tableListBox.getHeader().setColumnWidth(3, getWidth() * (9.0/12));
    // tableListBox.getHeader().setColumnWidth(4, getWidth() * (1.0/12));
}

void CuesTableUI::fillText()
{
    if (tableModel)
    {
        tableModel->refreshData();
        tableListBox.updateContent();
    }
}
