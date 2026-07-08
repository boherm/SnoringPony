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
#include "../../Cue/group/GroupCue.h"
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
    TriggerCueColumn = 8,
    FirstDCAColumn = 100,
    LastDCAColumn = 115
};

static MixerInterface* findMixer(Cuelist* cl)
{
    if (dynamic_cast<DCAMixingCuelist*>(cl) == nullptr) return nullptr;

    for (auto* c : cl->cues->items)
    {
        if (auto* dc = dynamic_cast<DCACue*>(c))
        {
            if (auto* m = dc->getMixer())
                return m;
        }
    }
    return nullptr;
}

static int deriveDCACount(Cuelist* cl)
{
    if (dynamic_cast<DCAMixingCuelist*>(cl) == nullptr) return 0;
    if (auto* m = findMixer(cl)) return m->getNumDCAs();
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

    rebuildColumns();

    addAndMakeVisible(tableListBox);
    cl->cues->addBaseManagerListener(this);
    cl->cues->addAsyncContainerListener(this);
    cl->cues->addAsyncWarningTargetListener(this);

    for (auto* c : cl->cues->items)
        c->addAsyncWarningTargetListener(this);

    // Watch the mixer so the DCA columns refresh live when "Num DCAs" changes.
    // Listening on the MixerInterface (not the inner settings) survives vendor rebuilds.
    if (auto* m = findMixer(cl))
    {
        watchedMixer = m;
        m->addControllableContainerListener(this);
    }

    resized();
}

void CuesTableUI::rebuildColumns()
{
    auto& header = tableListBox.getHeader();
    header.removeAllColumns();

    int flags = TableHeaderComponent::visible | TableHeaderComponent::resizable | TableHeaderComponent::appearsOnColumnMenu;
    int numDCAs = deriveDCACount(cl);
    dcaColumnCount = numDCAs;
    bool isDCAMixing = numDCAs > 0;

    header.addColumn("", StatusColumn, 10, 10, 10, flags & ~TableHeaderComponent::appearsOnColumnMenu | ~TableHeaderComponent::resizable);
    header.addColumn("#", IdColumn, 60, 60, 100, flags & ~TableHeaderComponent::appearsOnColumnMenu);
    header.addColumn("Type", TypeColumn, 40, 40, 40, flags & ~TableHeaderComponent::resizable);
    header.addColumn("Description", DescriptionColumn, 200, 200, 5000, flags & ~TableHeaderComponent::appearsOnColumnMenu);
    if (isDCAMixing)
        header.addColumn("Cue", TriggerCueColumn, 70, 50, 120, flags & ~TableHeaderComponent::appearsOnColumnMenu);
    header.addColumn("Pre-wait", PreWaitColumn, 130, 130, 130, flags & ~TableHeaderComponent::resizable);

    for (int i = 1; i <= numDCAs; ++i)
    {
        header.addColumn("DCA " + String(i),
            FirstDCAColumn + i - 1, 100, 60, 300,
            flags & ~TableHeaderComponent::appearsOnColumnMenu);
    }

    if (!isDCAMixing)
        header.addColumn("Time", TimeColumn, 130, 130, 130, flags & ~TableHeaderComponent::resizable);
    header.addColumn("Post-wait", PostWaitColumn, 130, 130, 130, flags & ~TableHeaderComponent::resizable);

    tableListBox.updateContent();
    resized();
    tableListBox.repaint();
}

void CuesTableUI::controllableFeedbackUpdate(ControllableContainer* cc, Controllable* c)
{
    // Only react to the mixer's "Num DCAs" parameter, and only when the count actually
    // changes (feedback updates also fire for unrelated params, e.g. console meters).
    if (c != nullptr && c->niceName == "Num DCAs" && deriveDCACount(cl) != dcaColumnCount)
        rebuildColumns();
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
    if (watchedMixer != nullptr)
        watchedMixer->removeControllableContainerListener(this);
    tableListBox.setLookAndFeel(nullptr);
    lafTable.reset();
}

void CuesTableUI::paint(Graphics& g)
{
}

bool CuesTableUI::keyPressed(const KeyPress& key)
{
    // Cmd/Ctrl+V: paste into this cuelist's CueManager. Works whether or not a cue is
    // selected (CueManager::addItemsFromClipboard picks the anchor / append accordingly).
    if (key == KeyPress('v', ModifierKeys::commandModifier, 0))
    {
        if (cl != nullptr && cl->cues != nullptr)
        {
            cl->cues->askForPaste();
            return true;
        }
    }

    // Cmd/Ctrl+C: copy the selected cues, expanding any group to include its sub-cues (the
    // global Copy command would only take the group itself). Paste re-creates them all.
    if (key == KeyPress('c', ModifierKeys::commandModifier, 0))
    {
        if (cl != nullptr && cl->cues != nullptr && tableModel != nullptr)
        {
            Array<Cue*> sel;
            auto rows = tableListBox.getSelectedRows();
            for (int i = 0; i < rows.size(); i++)
                if (Cue* c = tableModel->rowToCue(rows[i])) sel.add(c);

            if (!sel.isEmpty())
            {
                cl->cues->copyCues(sel);
                return true;
            }
        }
    }

    // Right/Left arrow on a selected group cue: unfold / fold it.
    if (key == KeyPress::rightKey || key == KeyPress::leftKey)
    {
        const bool collapse = (key == KeyPress::leftKey);
        auto rows = tableListBox.getSelectedRows();
        bool acted = false;
        for (int i = 0; i < rows.size(); i++)
        {
            if (auto* grp = dynamic_cast<GroupCue*>(tableModel->rowToCue(rows[i])))
            {
                grp->collapsed->setValue(collapse);
                acted = true;
            }
        }
        if (acted)
        {
            tableListBox.updateContent();
            tableListBox.repaint();
            return true;
        }
    }

    return false;
}

void CuesTableUI::paintOverChildren(Graphics& g)
{
    // Draw a green box around each group block (header + its visible members).
    if (tableModel != nullptr)
    {
        tableModel->rebuildVisibleCues();
        auto& vc = tableModel->visibleCues;

        const int rowHeight = tableListBox.getRowHeight();
        const int headerHeight = tableListBox.getHeader().getHeight();
        const int scrollY = tableListBox.getViewport()->getViewPositionY();
        const Rectangle<int> tb = tableListBox.getBounds();

        Cue* nextCue = (cl != nullptr) ? cl->nextCue->getTargetContainerAs<Cue>() : nullptr;

        g.saveState();
        g.reduceClipRegion(tb.getX(), tb.getY() + headerHeight, tb.getWidth(), jmax(0, tb.getHeight() - headerHeight));

        for (int r = 0; r < vc.size(); r++)
        {
            auto* grp = dynamic_cast<GroupCue*>(vc[r]);
            if (grp == nullptr) continue;

            int rEnd = r;
            if (!grp->collapsed->boolValue())
            {
                int k = r + 1;
                while (k < vc.size() && vc[k]->parentGroup == grp) { rEnd = k; k++; }
            }

            float yTop = (float) (tb.getY() + headerHeight + r * rowHeight - scrollY);
            float yBot = (float) (tb.getY() + headerHeight + (rEnd + 1) * rowHeight - scrollY);
            // Keep the right edge clear of the vertical scrollbar (when shown) so the border
            // isn't overlapped by it; leave a small gap too.
            auto* vp = tableListBox.getViewport();
            const float rightInset = 7.0f;
            // Inset the box so two consecutive groups leave a clean gap instead of merging
            // their touching borders into one thick band; rounded for a softer look.
            Rectangle<float> box((float) tb.getX() + 1.0f, yTop + 2.0f,
                                 (float) tb.getWidth() - 1.0f - rightInset, (yBot - yTop) - 4.0f);
            // Box colour by mode: sequential = orange, timeline = sky-blue, parallel = green.
            const int mode = grp->fireMode->intValue();
            const Colour boxColour = mode == GroupCue::SEQUENTIAL ? Colours::orange
                                   : mode == GroupCue::TIMELINE   ? Colours::skyblue
                                                                  : Colours::green;
            g.setColour(boxColour.brighter(0.1f));
            g.drawRoundedRectangle(box, 3.0f, 1.0f);

            // Redraw the status cursor over the box so it isn't cut by the green border.
            for (int rr = r; rr <= rEnd; rr++)
            {
                Cue* c = vc[rr];
                bool playing = c->isPlaying->boolValue() || c->preWaitActive->boolValue() || c->postWaitActive->boolValue();
                bool isNext = (c == nextCue);
                if (playing || isNext)
                {
                    float ry = (float) (tb.getY() + headerHeight + rr * rowHeight - scrollY);
                    CuesTableModel::paintStatusArrow(g, (float) tb.getX(), ry + 1.5f, (float) rowHeight - 3.0f, playing, isNext);
                }
            }
        }

        g.restoreState();
    }

    if (cl != nullptr && cl->cues != nullptr && cl->cues->items.isEmpty())
    {
        const int headerHeight = tableListBox.getHeader().getHeight();
        const Rectangle<int> tb = tableListBox.getBounds();
        Rectangle<int> body(tb.getX(), tb.getY() + headerHeight, tb.getWidth(), jmax(0, tb.getHeight() - headerHeight));

        g.setColour(Colours::white.withAlpha(0.4f));
        g.drawFittedText("Press the right mouse button to create a new cue to this cuelist.", body, Justification::centred, true);
    }

    // Draw insertion line during drag
    if (insertIndex >= 0)
    {
        int rowHeight = tableListBox.getRowHeight();
        int headerHeight = tableListBox.getHeader().getHeight();

        // Calculate Y position relative to the table's visible area
        int yPos = headerHeight + (insertIndex * rowHeight) - tableListBox.getViewport()->getViewPositionY();

        // Get the table's bounds within this component
        Rectangle<int> tableBounds = tableListBox.getBounds();

        int xLeft = tableBounds.getX();
        int w = tableBounds.getWidth();
        if (insertIntoGroup)
        {
            // Indented + green: the drop nests into insertGroup.
            int indent = groupNestThresholdX();
            xLeft += indent;
            w -= indent;
            g.setColour(Colours::green.brighter(0.2f));
        }
        else
        {
            g.setColour(Colours::orange);
        }
        g.fillRect(xLeft, yPos - 1, w, 3);
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

    if (tableListBox.getHeader().isColumnVisible(TriggerCueColumn)) {
        tableListBox.getHeader().setColumnWidth(TriggerCueColumn, 70);
        width -= 70;
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
    insertIntoGroup = false;
    insertGroup = nullptr;
    repaint();
}

int CuesTableUI::groupNestThresholdX() const
{
    // Nesting kicks in once the mouse passes the start of the Description column plus the
    // member indent — matching where nested cues are drawn.
    int descIdx = tableListBox.getHeader().getIndexOfColumnId(DescriptionColumn, true);
    if (descIdx < 0) return 120;
    return tableListBox.getHeader().getColumnPosition(descIdx).getX() + 20;
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

    // Decide whether this insertion point nests into a group. At a group's bottom edge the
    // horizontal position chooses (drag right = inside, left = outside); in the middle of a
    // group it's always inside.
    insertGroup = nullptr;
    insertIntoGroup = false;
    if (tableModel != nullptr)
    {
        tableModel->rebuildVisibleCues();
        auto& vc = tableModel->visibleCues;

        GroupCue* cand = nullptr;
        if (insertIndex > 0 && insertIndex - 1 < vc.size())
        {
            Cue* pred = vc[insertIndex - 1];
            if (auto* g = dynamic_cast<GroupCue*>(pred)) cand = g;
            else if (pred->parentGroup != nullptr)       cand = pred->parentGroup;
        }

        if (cand != nullptr)
        {
            Cue* succ = (insertIndex < vc.size()) ? vc[insertIndex] : nullptr;
            const bool succInGroup = (succ != nullptr && succ->parentGroup == cand);
            insertIntoGroup = succInGroup || (pos.x >= groupNestThresholdX());
            insertGroup = cand;
        }
    }

    repaint();
}

void CuesTableUI::itemDragExit(const SourceDetails& dragSourceDetails)
{
    insertIndex = -1;
    insertIntoGroup = false;
    insertGroup = nullptr;
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

    // Map the dragged VISIBLE rows to cues (skip protected/pinned cues).
    Array<Cue*> dragged;
    for (int i = 0; i < sourceRows.size(); i++)
    {
        if (Cue* c = tableModel->rowToCue(sourceRows[i]))
            if (c->canBeReorderedInEditor)
                dragged.add(c);
    }

    if (dragged.isEmpty())
    {
        insertIndex = -1;
        repaint();
        return;
    }

    // A dragged group carries its members: they move as one block and stay grouped.
    Array<GroupCue*> movedGroups;
    Array<Cue*> block;
    for (Cue* c : dragged)
    {
        block.addIfNotAlreadyThere(c);
        if (auto* g = dynamic_cast<GroupCue*>(c))
        {
            movedGroups.add(g);
            for (auto* m : g->getMembers())
                block.addIfNotAlreadyThere(m);
        }
    }

    auto& vc = tableModel->visibleCues;

    // The cue the block should land in front of (nullptr = end of list).
    Cue* anchor = (insertIndex >= 0 && insertIndex < vc.size()) ? vc[insertIndex] : nullptr;
    while (anchor != nullptr && block.contains(anchor))  // don't anchor to a moving cue
    {
        int ai = vc.indexOf(anchor);
        anchor = (ai + 1 < vc.size()) ? vc[ai + 1] : nullptr;
    }

    // Group membership was decided during the drag (indented line = nest into insertGroup).
    // No nesting: a moving group is never dropped inside another group.
    GroupCue* dropGroup = insertIntoGroup ? insertGroup : nullptr;
    if (!movedGroups.isEmpty()) dropGroup = nullptr;

    // Reorder the block in front of the anchor, preserving flat-list order.
    Array<Cue*> ordered;
    for (auto* c : cl->cues->items)
        if (block.contains(c)) ordered.add(c);

    for (Cue* c : ordered)
    {
        int anchorIdx = anchor != nullptr ? cl->cues->items.indexOf(anchor) : cl->cues->items.size();
        int cur = cl->cues->items.indexOf(c);
        int dest = (cur < anchorIdx) ? anchorIdx - 1 : anchorIdx;
        dest = jlimit(0, cl->cues->items.size() - 1, dest);
        if (cur != dest)
            cl->cues->setItemIndex(c, dest, true);
    }

    // Apply grouping to the dropped cues (members that travel with their own group keep it).
    for (Cue* c : ordered)
    {
        if (dynamic_cast<GroupCue*>(c) != nullptr) continue;
        if (c->parentGroup != nullptr && movedGroups.contains(c->parentGroup)) continue;

        // reposition=false: the drop already placed the cue contiguously at the right spot.
        if (dropGroup != nullptr)        cl->cues->addToGroup(c, dropGroup, false);
        else if (c->parentGroup != nullptr) cl->cues->removeFromGroup(c);
    }

    // Refresh and re-select the moved cues at their new visible rows.
    tableListBox.updateContent();
    tableModel->rebuildVisibleCues();
    SparseSet<int> newSelection;
    for (Cue* c : ordered)
    {
        int r = tableModel->visibleRowForCue(c);
        if (r >= 0) newSelection.addRange(Range<int>(r, r + 1));
    }
    tableListBox.setSelectedRows(newSelection, dontSendNotification);
    tableListBox.repaint();

    insertIndex = -1;
    insertIntoGroup = false;
    insertGroup = nullptr;
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
