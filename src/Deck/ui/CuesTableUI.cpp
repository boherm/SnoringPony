/*
  ==============================================================================

    CuesTableUI.cpp
    Created: 30 Sep 2025 11:01:57pm
    Author:  boherm

  ==============================================================================
*/

#include "CuesTableUI.h"
#include "CuesTableModel.h"
#include "../../Cuelist/Cuelist.h"
#include "../../Cue/CueManager.h"
#include "../../ui/LookAndFeelTable.h"

enum ColumnIds
{
    StatusColumn = 1,
    IdColumn = 2,
    TypeColumn = 3,
    DescriptionColumn = 4,
    TimeColumn = 5,
    PreWaitColumn = 6,
    PostWaitColumn = 7
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
    tableListBox.setRowHeight(35);
    int flags = TableHeaderComponent::visible | TableHeaderComponent::resizable | TableHeaderComponent::appearsOnColumnMenu;
    tableListBox.getHeader().addColumn("", StatusColumn, 10, 10, 10, flags & ~TableHeaderComponent::appearsOnColumnMenu | ~TableHeaderComponent::resizable);
    tableListBox.getHeader().addColumn("#", IdColumn, 60, 60, 100, flags & ~TableHeaderComponent::appearsOnColumnMenu);
    tableListBox.getHeader().addColumn("Type", TypeColumn, 40, 40, 40, flags & ~TableHeaderComponent::resizable);
    tableListBox.getHeader().addColumn("Description", DescriptionColumn, 200, 200, 5000, flags & ~TableHeaderComponent::appearsOnColumnMenu);
    tableListBox.getHeader().addColumn("Pre-wait", PreWaitColumn, 130, 130, 130, flags & ~TableHeaderComponent::resizable);
    tableListBox.getHeader().addColumn("Time", TimeColumn, 130, 130, 130, flags & ~TableHeaderComponent::resizable);
    tableListBox.getHeader().addColumn("Post-wait", PostWaitColumn, 130, 130, 130, flags & ~TableHeaderComponent::resizable);

    addAndMakeVisible(tableListBox);
    cl->cues->addBaseManagerListener(this);
    cl->cues->addAsyncContainerListener(this);
    cl->cues->addAsyncWarningTargetListener(this);

    resized();
}

CuesTableUI::~CuesTableUI()
{
    cl->cues->removeAsyncContainerListener(this);
    cl->cues->removeBaseManagerListener(this);
    cl->cues->removeAsyncWarningTargetListener(this);
    tableListBox.setLookAndFeel(nullptr);
    lafTable.reset();
}

void CuesTableUI::paint(Graphics& g)
{
}

void CuesTableUI::paintOverChildren(Graphics& g)
{
    // Draw insertion line during drag
    if (insertIndex >= 0)
    {
        int rowHeight = tableListBox.getRowHeight();
        int headerHeight = tableListBox.getHeader().getHeight();

        // Calculate Y position relative to the table's visible area
        int yPos = headerHeight + (insertIndex * rowHeight) - tableListBox.getViewport()->getViewPositionY();

        // Get the table's bounds within this component
        Rectangle<int> tableBounds = tableListBox.getBounds();

        g.setColour(Colours::orange);
        g.fillRect(tableBounds.getX(), yPos - 1, tableBounds.getWidth(), 3);
    }
}

void CuesTableUI::resized()
{
    int width = getWidth();
    width -= tableListBox.getViewport()->getScrollBarThickness();

    tableListBox.setBounds(getLocalBounds());

    tableListBox.getHeader().setColumnWidth(StatusColumn, 10);
    width -= 10;

    tableListBox.getHeader().setColumnWidth(IdColumn, 60);
    width -= 60;

    if (tableListBox.getHeader().isColumnVisible(TypeColumn)) {
        tableListBox.getHeader().setColumnWidth(TypeColumn, 40);
        width -= 40;
    }

    if (tableListBox.getHeader().isColumnVisible(PreWaitColumn)) {
        tableListBox.getHeader().setColumnWidth(PreWaitColumn, 130);
        width -= 130;
    }

    if (tableListBox.getHeader().isColumnVisible(TimeColumn)) {
        tableListBox.getHeader().setColumnWidth(TimeColumn, 130);
        width -= 130;
    }

    if (tableListBox.getHeader().isColumnVisible(PostWaitColumn)) {
        tableListBox.getHeader().setColumnWidth(PostWaitColumn, 130);
        width -= 130;
    }

    tableListBox.getHeader().setColumnWidth(DescriptionColumn, width);
}

bool CuesTableUI::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    // Check if the drag source is from our table
    if (dragSourceDetails.description.isObject())
    {
        var typeVar = dragSourceDetails.description.getProperty("type", var());
        if (typeVar.toString() == "cueTableRows")
        {
            return dragSourceDetails.sourceComponent.get() == &tableListBox;
        }
    }
    return false;
}

void CuesTableUI::itemDragEnter(const SourceDetails& dragSourceDetails)
{
    insertIndex = -1;
    repaint();
}

void CuesTableUI::itemDragMove(const SourceDetails& dragSourceDetails)
{
    // Calculate where to insert based on mouse position
    Point<int> pos = tableListBox.getLocalPoint(this, dragSourceDetails.localPosition);

    int rowHeight = tableListBox.getRowHeight();
    int headerHeight = tableListBox.getHeader().getHeight();
    int numRows = tableListBox.getNumRows();

    // Adjust for header and scroll position
    int adjustedY = pos.y - headerHeight + tableListBox.getViewport()->getViewPositionY();

    // Calculate the row we're hovering over
    int row = adjustedY / rowHeight;

    // Determine if we should insert before or after this row
    int rowYPos = row * rowHeight;
    bool insertBefore = (adjustedY - rowYPos) < (rowHeight / 2);

    if (insertBefore)
        insertIndex = jlimit(0, numRows, row);
    else
        insertIndex = jlimit(0, numRows, row + 1);

    repaint();
}

void CuesTableUI::itemDragExit(const SourceDetails& dragSourceDetails)
{
    insertIndex = -1;
    repaint();
}

void CuesTableUI::itemDropped(const SourceDetails& dragSourceDetails)
{
    if (cl == nullptr || !dragSourceDetails.description.isObject())
    {
        insertIndex = -1;
        repaint();
        return;
    }

    // Get the rows being dragged
    var rowsVar = dragSourceDetails.description.getProperty("rows", var());
    if (!rowsVar.isArray())
    {
        insertIndex = -1;
        repaint();
        return;
    }

    // Extract row indices
    SparseSet<int> sourceRows;
    Array<var>* rowsArray = rowsVar.getArray();
    for (int i = 0; i < rowsArray->size(); i++)
    {
        int rowIndex = (int)(*rowsArray)[i];
        sourceRows.addRange(Range<int>(rowIndex, rowIndex + 1));
    }

    if (sourceRows.size() == 0)
    {
        insertIndex = -1;
        repaint();
        return;
    }

    // Get the cues to move
    Array<Cue*> cuesToMove;
    for (int i = 0; i < sourceRows.size(); i++)
    {
        int rowIndex = sourceRows[i];
        if (rowIndex >= 0 && rowIndex < cl->cues->items.size())
        {
            cuesToMove.add(cl->cues->items[rowIndex]);
        }
    }

    if (cuesToMove.isEmpty())
    {
        insertIndex = -1;
        repaint();
        return;
    }

    // Calculate the correct insert position
    int targetIndex = insertIndex;

    // Track the new indices for re-selection
    SparseSet<int> newSelection;

    // If we're moving a single item
    if (cuesToMove.size() == 1)
    {
        Cue* cueToMove = cuesToMove[0];
        int currentIndex = cl->cues->items.indexOf(cueToMove);

        // Adjust target index if we're moving down (after the current position)
        if (currentIndex < targetIndex)
            targetIndex--;

        // Ensure the target index is valid
        targetIndex = jlimit(0, cl->cues->items.size() - 1, targetIndex);

        // Move the cue using the BaseManager's setItemIndex method
        if (currentIndex != targetIndex)
        {
            cl->cues->setItemIndex(cueToMove, targetIndex, true);
        }

        // Add to new selection
        newSelection.addRange(Range<int>(targetIndex, targetIndex + 1));
    }
    else
    {
        // For multiple items, we need to move them carefully
        // Sort the indices to maintain relative order
        Array<int> sortedIndices;
        for (int i = 0; i < sourceRows.size(); i++)
        {
            sortedIndices.add(sourceRows[i]);
        }
        sortedIndices.sort();

        // Count how many items are before the target index
        int numBefore = 0;
        for (int idx : sortedIndices)
        {
            if (idx < targetIndex)
                numBefore++;
        }

        // Adjust target index
        int adjustedTarget = targetIndex - numBefore;

        // Move items in reverse order to avoid index shifting issues
        for (int i = sortedIndices.size() - 1; i >= 0; i--)
        {
            int currentIndex = sortedIndices[i];
            Cue* cue = cl->cues->items[currentIndex];

            int newIndex = adjustedTarget + i;
            newIndex = jlimit(0, cl->cues->items.size() - 1, newIndex);

            if (currentIndex != newIndex)
            {
                cl->cues->setItemIndex(cue, newIndex, true);
            }
        }

        // Add the range to new selection
        newSelection.addRange(Range<int>(adjustedTarget, adjustedTarget + cuesToMove.size()));
    }

    // Refresh the table
    tableListBox.updateContent();

    // Re-select the moved items at their new positions
    tableListBox.setSelectedRows(newSelection, dontSendNotification);

    tableListBox.repaint();

    insertIndex = -1;
    repaint();
}
