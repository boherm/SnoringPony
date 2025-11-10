/*
  ==============================================================================

    CuesTableModel.h
    Created: 30 Sep 2025 11:01:57pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class Cuelist;

class CuesTableModel :
    public TableListBoxModel,
    public ParameterListener
{
public:
    CuesTableModel(TableListBox* tlb, Cuelist* cl);
    ~CuesTableModel() override;

    TableListBox* tlb = nullptr;
    Cuelist* cl = nullptr;

    void addListeners();

    int getNumRows() override;
    void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate) override;
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;
    void cellClicked(int rowNumber, int columnId, const MouseEvent& event) override;
    void backgroundClicked(const MouseEvent& event) override;

    void inspectCue(int rowNumber);
    // void cellDoubleClicked(int rowNumber, int columnId, const MouseEvent& event) override;

    // Drag and drop support
    var getDragSourceDescription(const SparseSet<int>& selectedRows) override;

    void parameterValueChanged(Parameter* p) override;

// private:
//     Array<Cue> cues;
//     void generateTestData();

// public:
//     void refreshData() { generateTestData(); }
};
