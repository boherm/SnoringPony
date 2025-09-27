/*
  ==============================================================================

    CuelistViewUI.h
    Created: 19 Sep 2025 08:58:00am
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../CuelistManager.h"

class CuelistViewUI : public ShapeShifterContent,
                      public InspectableSelectionManager::Listener,
                      public CuelistManager::ManagerListener {
public:
  CuelistViewUI(const String &contentName);
  ~CuelistViewUI();
  
	void inspectablesSelectionChanged() override;
  
  static CuelistViewUI *create(const String &name) {
    return new CuelistViewUI(name);
  }
};

class CuelistTableModel : public TableListBoxModel
{
public:
    CuelistTableModel();
    ~CuelistTableModel() override;

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

class CuelistView  : public Component
{
public:
    juce_DeclareSingleton(CuelistView, true);
    CuelistView();
    ~CuelistView() override;

    TableListBox tableListBox;
    std::unique_ptr<CuelistTableModel> tableModel;

    void paint (Graphics&) override;
    void resized() override;
    void fillText(String text);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CuelistView)
};