/*
  ==============================================================================

    CuesTableModel.h
    Created: 30 Sep 2025 11:01:57pm
    Author:  boherm

  ==============================================================================
*/

#include "../../MainIncludes.h"
#include "../../Cuelist/Cuelist.h"

class CuesTableModel : public TableListBoxModel
{
public:
    CuesTableModel(Cuelist* cl);
    ~CuesTableModel() override;

    Cuelist* cl = nullptr;

    int getNumRows() override;
    void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate) override;
    int getColumnAutoSizeWidth(int columnId) override;
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;
    void cellClicked(int rowNumber, int columnId, const MouseEvent& event) override;

private:
    struct CueData
    {
        String name;
        String time;
        String description;
        bool isActive;
    };

    Array<CueData> cueData;
    void generateTestData();

public:
    void refreshData() { generateTestData(); }
};
