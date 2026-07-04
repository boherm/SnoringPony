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
class Cue;

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

    // Rows currently shown: the flat cue list minus the members of collapsed groups.
    // Rebuilt on demand; every table callback indexes THIS, not cl->cues->items.
    juce::Array<Cue*> visibleCues;
    void rebuildVisibleCues();
    Cue* rowToCue(int row) const;
    int visibleRowForCue(Cue* c);

    // Cell whose DCA assignment modal is currently open (for a highlight border).
    // -1 when none. Kept in sync via the dialog's onStateChanged / onClosed hooks.
    int editingRow = -1;
    int editingDca = -1;
    juce::Component::SafePointer<juce::Component> activeDcaDialog;

    void addListeners();

    std::unique_ptr<ReorderCuesWindow> reorderWindow;

    int getNumRows() override;
    void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate) override;
    String getCellTooltip(int rowNumber, int columnId) override;
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;
    void cellClicked(int rowNumber, int columnId, const MouseEvent& event) override;
    void cellDoubleClicked(int rowNumber, int columnId, const MouseEvent& event) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void backgroundClicked(const MouseEvent& event) override;
    void deleteKeyPressed (int lastRowSelected) override;
    void returnKeyPressed (int lastRowSelected) override;

    void inspectCue(int rowNumber);
    // Cues under the current selection, or `fallback` alone if nothing is selected.
    juce::Array<Cue*> currentSelectionOr(Cue* fallback);
    // Push the current table selection to the Inspector: single row -> that cue,
    // several rows -> all of them (multi-cue editor).
    void syncSelectionToInspector();
    void askDeleteSelectedCues();

    // Drag and drop support
    var getDragSourceDescription(const SparseSet<int>& selectedRows) override;

    void parameterValueChanged(Parameter* p) override;

    static String valueToTimeString(double timeVal);

    // Draws the status "cursor" (playing = green / next = orange arrow, or split when both)
    // at (x, y) with height h. Shared by the cell renderer and the group-border overlay so
    // the cursor can be redrawn on top of a group's green box.
    static void paintStatusArrow(juce::Graphics& g, float x, float y, float h, bool playing, bool isNext);

    void newMessage(const InspectableSelectionManager::SelectionEvent& e) override;

    void reorderCues(float startId, float increment);
};
