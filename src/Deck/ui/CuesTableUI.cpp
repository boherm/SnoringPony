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
#include "../../Cuelist/dca/DCAMixingCuelist.h"
#include "../../Cue/CueManager.h"
#include "../../Cue/dca/DCACue.h"
#include "../../Interface/mixer/MixerInterface.h"
#include "../../ui/LookAndFeelTable.h"
#include "juce_gui_basics/juce_gui_basics.h"

enum ColumnIds
{
    StatusColumn = 1,
    IdColumn = 2,
    TypeColumn = 3,
    DescriptionColumn = 4,
    TimeColumn = 5,
    PreWaitColumn = 6,
    PostWaitColumn = 7,
    FirstDCAColumn = 100,
    LastDCAColumn = 115
};

static int deriveDCACount(Cuelist* cl)
{
    if (dynamic_cast<DCAMixingCuelist*>(cl) == nullptr) return 0;

    for (auto* c : cl->cues->items)
    {
        if (auto* dc = dynamic_cast<DCACue*>(c))
        {
            if (auto* m = dc->getMixer())
                return m->getNumDCAs();
        }
    }
    return 8;
}

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
    int numDCAs = deriveDCACount(cl);
    bool isDCAMixing = numDCAs > 0;

    tableListBox.getHeader().addColumn("", StatusColumn, 10, 10, 10, flags & ~TableHeaderComponent::appearsOnColumnMenu | ~TableHeaderComponent::resizable);
    tableListBox.getHeader().addColumn("#", IdColumn, 60, 60, 100, flags & ~TableHeaderComponent::appearsOnColumnMenu);
    tableListBox.getHeader().addColumn("Type", TypeColumn, 40, 40, 40, flags & ~TableHeaderComponent::resizable);
    tableListBox.getHeader().addColumn("Description", DescriptionColumn, 200, 200, 5000, flags & ~TableHeaderComponent::appearsOnColumnMenu);
    tableListBox.getHeader().addColumn("Pre-wait", PreWaitColumn, 130, 130, 130, flags & ~TableHeaderComponent::resizable);

    for (int i = 1; i <= numDCAs; ++i)
    {
        tableListBox.getHeader().addColumn("DCA " + String(i),
            FirstDCAColumn + i - 1, 100, 60, 300,
            flags & ~TableHeaderComponent::appearsOnColumnMenu);
    }

    if (!isDCAMixing)
        tableListBox.getHeader().addColumn("Time", TimeColumn, 130, 130, 130, flags & ~TableHeaderComponent::resizable);
    tableListBox.getHeader().addColumn("Post-wait", PostWaitColumn, 130, 130, 130, flags & ~TableHeaderComponent::resizable);

    addAndMakeVisible(tableListBox);
    cl->cues->addBaseManagerListener(this);
    cl->cues->addAsyncContainerListener(this);
    cl->cues->addAsyncWarningTargetListener(this);

    for (auto* c : cl->cues->items)
        c->addAsyncWarningTargetListener(this);

    resized();
}

CuesTableUI::~CuesTableUI()
{
    if (cl != nullptr && cl->cues != nullptr)
    {
        for (auto* c : cl->cues->items)
            c->removeAsyncWarningTargetListener(this);

        cl->cues->removeAsyncContainerListener(this);
        cl->cues->removeBaseManagerListener(this);
        cl->cues->removeAsyncWarningTargetListener(this);
    }
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

    for (int id = FirstDCAColumn; id <= LastDCAColumn; ++id)
    {
        if (tableListBox.getHeader().isColumnVisible(id))
            width -= tableListBox.getHeader().getColumnWidth(id);
    }

    tableListBox.getHeader().setColumnWidth(DescriptionColumn, jmax(80, width));
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

    // Get the cues to move (skip protected cues — they are pinned in place)
    Array<Cue*> cuesToMove;
    for (int i = 0; i < sourceRows.size(); i++)
    {
        int rowIndex = sourceRows[i];
        if (rowIndex >= 0 && rowIndex < cl->cues->items.size())
        {
            Cue* c = cl->cues->items[rowIndex];
            if (!c->canBeReorderedInEditor) continue;
            cuesToMove.add(c);
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

    // Prevent inserting above a pinned cue at index 0
    if (!cl->cues->items.isEmpty() && !cl->cues->items.getFirst()->canBeReorderedInEditor)
        targetIndex = jmax(targetIndex, 1);

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

bool CuesTableUI::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Filtre optionnel : par extension, etc.
    for (auto& f : files)
        if (f.endsWithIgnoreCase(".mp3") || f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".aiff") || f.endsWithIgnoreCase(".flac") || f.endsWithIgnoreCase(".ogg"))
            return true;

    return false; // ou true si tu acceptes tout
}

void CuesTableUI::fileDragMove (const juce::StringArray& /*files*/, int /*x*/, int /*y*/)
{
    // TODO: Implement this for placing new cues precisely in cuelist
}

void CuesTableUI::fileDragEnter (const juce::StringArray& /*files*/, int /*x*/, int /*y*/)
{
    // TODO: Implement this for visual feedback
}

void CuesTableUI::fileDragExit (const juce::StringArray& /*files*/)
{
    // TODO: Implement this for visual feedback
}

void CuesTableUI::filesDropped (const juce::StringArray& files, int /*x*/, int /*y*/)
{
    PopupMenu menu;
    menu.addItem(1, "Add files to one Audio Cue");
    menu.addItem(2, "Add files to multiple Audio Cues");
    menu.addSeparator();
    menu.addItem(3, "Add files to a Playlist Cue");

    menu.showMenuAsync(PopupMenu::Options(),
        [this, files](int result)
        {
            if (result == 0)
                return; // User cancelled

            if (result == 1)
                cl->cues->createOneAudioCueFromFiles(files);
            else if (result == 2)
                cl->cues->createMultipleAudioCueFromFiles(files);
            else if (result == 3)
                cl->cues->createPlaylistCueFromFiles(files);

            tableListBox.updateContent();
            tableListBox.repaint();
        });
}
