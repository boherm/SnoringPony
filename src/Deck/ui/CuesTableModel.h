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

class ReorderCuesWindow;

class CuesTableModel :
    public TableListBoxModel,
    public ParameterListener,
    public InspectableSelectionManager::AsyncListener
{
public:
    CuesTableModel(TableListBox* tlb, Cuelist* cl);
    ~CuesTableModel() override;

    TableListBox* tlb = nullptr;
    Cuelist* cl = nullptr;

    void addListeners();

    std::unique_ptr<ReorderCuesWindow> reorderWindow;

    int getNumRows() override;
    void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate) override;
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;
    void cellClicked(int rowNumber, int columnId, const MouseEvent& event) override;
    void cellDoubleClicked(int rowNumber, int columnId, const MouseEvent& event) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void backgroundClicked(const MouseEvent& event) override;
    void deleteKeyPressed (int lastRowSelected) override;

    void inspectCue(int rowNumber);
    void askDeleteSelectedCues();

    // Drag and drop support
    var getDragSourceDescription(const SparseSet<int>& selectedRows) override;

    void parameterValueChanged(Parameter* p) override;

    static String valueToTimeString(double timeVal);

    void newMessage(const InspectableSelectionManager::SelectionEvent& e) override;

    void reorderCues(float startId, float increment);
};
