/*
  ==============================================================================

    CuesTableModel.h
    Created: 30 Sep 2025 11:01:57pm
    Author:  boherm

  ==============================================================================
*/

#include "../../MainIncludes.h"
#include "../../Cuelist/Cuelist.h"
#include "juce_gui_basics/juce_gui_basics.h"

class CuesTableModel : public TableListBoxModel
{
public:
    CuesTableModel(TableListBox* tlb, Cuelist* cl);
    ~CuesTableModel() override;

    TableListBox* tlb = nullptr;
    Cuelist* cl = nullptr;

    int getNumRows() override;
    void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate) override;
    int getColumnAutoSizeWidth(int columnId) override;
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;
    void cellClicked(int rowNumber, int columnId, const MouseEvent& event) override;
    void backgroundClicked(const MouseEvent& event) override;

    void inspectCue(int rowNumber);
    void cellDoubleClicked(int rowNumber, int columnId, const MouseEvent& event) override;

// private:
//     Array<Cue> cues;
//     void generateTestData();

// public:
//     void refreshData() { generateTestData(); }
};
