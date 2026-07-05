/*
  ==============================================================================

    CuesTableModel.cpp
    Created: 30 Sep 2025 11:01:57pm
    Author:  boherm

  ==============================================================================
*/

#include "CuesTableModel.h"
#include "ReorderCuesWindow.h"
#include "../../Cuelist/Cuelist.h"
#include "../../Cuelist/CuelistManager.h"
#include "../../Cue/CueManager.h"
#include "../../Cue/dca/DCACue.h"
#include "../../Cue/dca/DCAAssignment.h"
#include "../../Cue/dca/ui/DCAAssignmentDialog.h"
#include "../../Cue/fade/FadeCue.h"
#include "../../Cue/group/GroupCue.h"
#include "../../Interface/mixer/MixerChannel.h"
#include "../../ui/SPAssetManager.h"
#include "../../Cue/audio/AudioCue.h"
#include "../../Interface/midi/MIDIInterface.h"

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
    FirstDCAColumn = 100,    // 100..115 reserved for DCAs 1..16
    LastDCAColumn = 115
};

// Width of the fold/unfold disclosure drawn at the right edge of a group cue's
// description cell (also the click target — see cellClicked).
static constexpr int kGroupToggleWidth = 30;

namespace
{
    // Builds the tooltip text describing the cue/cuelist a DCA cue triggers, using
    // getDescription() so an automatic description is resolved correctly.
    String getTriggerTooltip(DCACue* dca)
    {
        if (dca == nullptr) return {};

        ControllableContainer* target = dca->triggerCue->getTargetContainer();
        if (auto* c = dynamic_cast<Cue*>(target))
        {
            if (c == dca) return "Triggers: self-reference!";
            String cuelistName = c->parentCuelist != nullptr ? c->parentCuelist->niceName : String("?");
            return "Triggers: " + cuelistName + " / " + c->id->stringValue() + " - " + c->getDescription();
        }
        if (auto* cl = dynamic_cast<Cuelist*>(target))
            return "Triggers: GO " + cl->niceName + " (next cue)";

        return {};
    }
}

CuesTableModel::CuesTableModel(TableListBox* tlb, Cuelist* cl)
{
    this->cl = cl;
    this->tlb = tlb;

    if (cl == nullptr) {
        return;
    }

    cl->nextCue->addParameterListener(this);

	InspectableSelectionManager::mainSelectionManager->addAsyncSelectionManagerListener(this);
}

CuesTableModel::~CuesTableModel()
{
    cl->nextCue->removeParameterListener(this);
	InspectableSelectionManager::mainSelectionManager->removeAsyncSelectionManagerListener(this);

    // Drop the callbacks a still-open DCA dialog holds into this (about-to-die) model.
    if (auto* d = dynamic_cast<DCAAssignmentDialog*>(activeDcaDialog.getComponent()))
    {
        d->onStateChanged = nullptr;
        d->onClosed = nullptr;
    }
}

void CuesTableModel::rebuildVisibleCues()
{
    visibleCues.clearQuick();
    if (cl == nullptr || cl->cues == nullptr) return;

    for (auto* c : cl->cues->items)
    {
        if (c == nullptr) continue;
        // Hide members of a collapsed group.
        if (c->parentGroup != nullptr && c->parentGroup->collapsed->boolValue())
            continue;
        visibleCues.add(c);
    }
}

Cue* CuesTableModel::rowToCue(int row) const
{
    if (row < 0 || row >= visibleCues.size()) return nullptr;
    return visibleCues[row];
}

int CuesTableModel::getNumRows()
{
    rebuildVisibleCues();
    return visibleCues.size();
}

void CuesTableModel::paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    Cue* cue = rowToCue(rowNumber);

    if (cue == nullptr)
        return;

    if (rowIsSelected)
    {
        g.fillAll(cue->itemColor->getColor().brighter(0.1f));
    }
    else if (rowNumber % 2 == 0)
    {
        g.fillAll(cue->itemColor->getColor());
    }
    else
    {
        g.fillAll(cue->itemColor->getColor().darker(0.2f));
    }
}

void CuesTableModel::parameterValueChanged(Parameter* p)
{
    if (p == cl->nextCue) {
        tlb->repaint();

        // Ensure the next cue + 1 is visible (or the next cue if it's the last one)
        if (cl->nextCue->getTargetContainerAs<Cue>() == nullptr)
            return;

        rebuildVisibleCues();
        auto nextCueIndex = visibleRowForCue(cl->nextCue->getTargetContainerAs<Cue>());
        if (nextCueIndex < 0)
            return;
        if (nextCueIndex + 1 < visibleCues.size()) {
            tlb->scrollToEnsureRowIsOnscreen(nextCueIndex + 1);
        } else {
            tlb->scrollToEnsureRowIsOnscreen(nextCueIndex);
        }
    }
}

void CuesTableModel::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= visibleCues.size())
        return;

    g.setFont(14.0f);
    g.setColour(Colours::white);

    Cue* cue = rowToCue(rowNumber);
    if (cue == nullptr)
        return;
    Cue* nextCue = cl->nextCue->getTargetContainerAs<Cue>();

    // Group cues get slightly shorter timer boxes (pre-wait / time / post-wait); normal
    // cues keep the standard height.
    const int timerBoxVInset = (dynamic_cast<GroupCue*>(cue) != nullptr) ? 7 : 4;


    // Status column
    if (StatusColumn == columnId) {
        bool isCuePlaying = cue->isPlaying->boolValue() || cue->preWaitActive->boolValue() || cue->postWaitActive->boolValue();
        bool isNextCue = (nextCue == cue);
        // Inset the cursor a few px top/bottom; group cues get a bit more so it lines up with
        // the top/bottom of their green box.
        paintStatusArrow(g, 0.0f, 1.0f, (float)height - 2.0f, isCuePlaying, isNextCue);
        return;
    }

    // Id column
    if (IdColumn == columnId) {
        g.setColour(Colours::white);
        if (cue->isAutoStartCue())
            g.setOpacity(0.4f);
        g.drawText(cue->id->stringValue(), 2, 0, width - 4, height, Justification::centred, true);
        g.setOpacity(1.0f);
        return;
    }

    // Type column
    if (TypeColumn == columnId) {
        Image img = SPAssetManager::getInstance()->getCueIcon(cue->getCueType());
        g.setOpacity(0.7f);

        if (cue->isAutoStartCue())
            g.setOpacity(0.4f);

        if (cue->getWarningMessage().isNotEmpty()) {
            g.setOpacity(1.0f);
            img = AssetManager::getInstance()->warningImage;
        }

        g.drawImageWithin(img, 7.0, 7.0, width - 14.0, height - 14.0, RectanglePlacement::centred, false);
        return;
    }

    // Pre-wait column
    if (PreWaitColumn == columnId) {
        if (!cue->preWaitCC->enabled->boolValue())
            return;

        double timeLeft = cue->preWaitDuration->doubleValue() - cue->preWaitCurrentTime->doubleValue();
        double positionPercent = cue->preWaitDuration->doubleValue() == 0 ? 0 : cue->preWaitCurrentTime->doubleValue() / cue->preWaitDuration->doubleValue();
        auto color = Colours::darkgoldenrod;
        String text = CuesTableModel::valueToTimeString(jmax<double>(timeLeft, 0.0));

        Rectangle<float> r = Rectangle<float>(0, 0, width, height);
        r.reduce(4, timerBoxVInset);

        Path myPath;
        myPath.addRectangle(r.getX(), r.getY(), r.getWidth(), r.getHeight());
        g.setColour(color);

        if (cue->isAutoStartCue()) g.setOpacity(0.5f);

        if (cue->preWaitDuration->doubleValue() > 0.0)
            g.strokePath(myPath, PathStrokeType(1));
        g.setColour(color.withAlpha(0.6f));

        if (cue->isAutoStartCue()) g.setOpacity(0.4f);

        g.fillRect(r.getX(), r.getY(), r.getWidth() * positionPercent, r.getHeight());
        g.setColour(Colours::white);

        if (cue->isAutoStartCue()) g.setOpacity(0.5f);

        if (cue->preWaitDuration->doubleValue() <= 0.0)
            g.setOpacity(0.5f);

        g.drawText(text, r.getX(), r.getY(), r.getWidth(), r.getHeight(), Justification::centred, true);

        if (cue->postWaitCC->enabled->boolValue() && cue->postWaitType->intValue() == Cue::PostWaitType::IMMEDIATE) {
            g.setFont(Font(13.0f, Font::bold));
            g.setColour(Colours::orangered);
            if (cue->isAutoStartCue()) g.setOpacity(0.5f);
            g.drawText("P", r.getX() + 6, r.getY(), 15, r.getHeight(), Justification::centred, true);
            g.drawEllipse(r.getX() + 6, r.getY() + (r.getHeight() / 2) - 7, 14, 14, 1.5f);
        }

        if (cue->postWaitCC->enabled->boolValue() && cue->postWaitType->intValue() == Cue::PostWaitType::AFTER_PRE) {
            g.setFont(Font(13.0f, Font::bold));
            g.setColour(Colours::orangered);
            if (cue->isAutoStartCue()) g.setOpacity(0.5f);
            g.drawText("P", r.getX() + r.getWidth() - 20, r.getY(), 15, r.getHeight(), Justification::centred, true);
            g.drawEllipse(r.getX() + r.getWidth() - 20, r.getY() + (r.getHeight() / 2) - 7, 14, 14, 1.5f);
        }
        return;
    }

    // Time column
    if (TimeColumn == columnId) {

        Rectangle<float> r = Rectangle<float>(0, 0, width, height);
        r.reduce(4, timerBoxVInset);

        double timeLeft = cue->duration->doubleValue() - cue->currentTime->doubleValue();
        double positionPercent = cue->duration->doubleValue() == 0 ? 0 : cue->currentTime->doubleValue() / cue->duration->doubleValue();
        auto color = !cue->isPlaying->boolValue() || cue->preWaitActive->boolValue() || timeLeft > 10 ? Colours::green.brighter(0.2f) : Colours::red.brighter(0.2f);
        String text = CuesTableModel::valueToTimeString(jmax<double>(timeLeft, 0.0));


        Path myPath;
        myPath.addRectangle(r.getX(), r.getY(), r.getWidth(), r.getHeight());
        g.setColour(color);

        if (cue->isAutoStartCue()) g.setOpacity(0.5f);

        if (cue->duration->doubleValue() > 0.0 || cue->isPlaying->boolValue() || cue->preWaitActive->boolValue() || cue->postWaitActive->boolValue())
            g.strokePath(myPath, PathStrokeType(1));
        g.setColour(color.withAlpha(0.6f));

        if (cue->isAutoStartCue()) g.setOpacity(0.4f);

        g.fillRect(r.getX(), r.getY(), r.getWidth() * positionPercent, r.getHeight());
        g.setColour(Colours::white);

        if (cue->isAutoStartCue()) g.setOpacity(0.5f);

        if (cue->duration->doubleValue() <= 0.0 && !cue->isPlaying->boolValue() && !cue->preWaitActive->boolValue() && !cue->postWaitActive->boolValue())
            g.setOpacity(0.5f);

        g.drawText(text, r.getX(), r.getY(), r.getWidth(), r.getHeight(), Justification::centred, true);

        if (!cue->preWaitCC->enabled->boolValue()) {
            if (
                    cue->postWaitCC->enabled->boolValue() && cue->postWaitType->intValue() == Cue::PostWaitType::IMMEDIATE ||
                    cue->postWaitCC->enabled->boolValue() && cue->postWaitType->intValue() == Cue::PostWaitType::AFTER_PRE
            ) {
                g.setFont(Font(13.0f, Font::bold));
                g.setColour(Colours::orangered);
                if (cue->isAutoStartCue()) g.setOpacity(0.5f);
                g.drawText("P", r.getX() + 6, r.getY(), 15, r.getHeight(), Justification::centred, true);
                g.drawEllipse(r.getX() + 6, r.getY() + (r.getHeight() / 2) - 7, 14, 14, 1.5f);
            }
        }

        if (cue->postWaitCC->enabled->boolValue() && cue->postWaitType->intValue() == Cue::PostWaitType::AFTER_CUE) {
            g.setFont(Font(13.0f, Font::bold));
            g.setColour(Colours::orangered);
            if (cue->isAutoStartCue()) g.setOpacity(0.5f);
            g.drawText("P", r.getX() + r.getWidth() - 20, r.getY(), 15, r.getHeight(), Justification::centred, true);
            g.drawEllipse(r.getX() + r.getWidth() - 20, r.getY() + (r.getHeight() / 2) - 7, 14, 14, 1.5f);
        }
        return;
    }

    // Post-wait column
    if (PostWaitColumn == columnId) {
        if (!cue->postWaitCC->enabled->boolValue())
            return;

        Rectangle<float> r = Rectangle<float>(0, 0, width, height);
        r.reduce(4, timerBoxVInset);

        double timeLeft = cue->postWaitDuration->doubleValue() - cue->postWaitCurrentTime->doubleValue();
        double positionPercent = cue->postWaitDuration->doubleValue() == 0 ? 0 : cue->postWaitCurrentTime->doubleValue() / cue->postWaitDuration->doubleValue();
        auto color = Colours::darkgoldenrod;
        String text = CuesTableModel::valueToTimeString(jmax<double>(timeLeft, 0.0));


        Path myPath;
        myPath.addRectangle(r.getX(), r.getY(), r.getWidth(), r.getHeight());
        g.setColour(color);

        if (cue->isAutoStartCue()) g.setOpacity(0.5f);

        if (cue->postWaitDuration->doubleValue() > 0.0)
            g.strokePath(myPath, PathStrokeType(1));
        g.setColour(color.withAlpha(0.6f));

        if (cue->isAutoStartCue()) g.setOpacity(0.4f);

        g.fillRect(r.getX(), r.getY(), r.getWidth() * positionPercent, r.getHeight());
        g.setColour(Colours::white);

        if (cue->isAutoStartCue()) g.setOpacity(0.5f);

        if (cue->postWaitDuration->doubleValue() <= 0.0)
            g.setOpacity(0.5f);

        g.drawText(text, r.getX(), r.getY(), r.getWidth(), r.getHeight(), Justification::centred, true);
        return;
    }

    // DCA columns (DCA Mixing Cuelist)
    if (columnId >= FirstDCAColumn && columnId <= LastDCAColumn) {
        int dcaIdx = columnId - FirstDCAColumn + 1;
        DCACue* dcaCue = dynamic_cast<DCACue*>(cue);
        if (dcaCue != nullptr) {
            DCAAssignment* a = dcaCue->findAssignment(dcaIdx);
            if (a != nullptr && !a->characters->items.isEmpty()) {
                Array<int> distinctChannels;
                bool hasFX = false;
                for (auto* r : a->characters->items) {
                    if (auto* ch = r->getChannel())
                        distinctChannels.addIfNotAlreadyThere(ch->channelNumber->intValue());
                    if (r->getFX() != nullptr) hasFX = true;
                }

                // Same assignment as the next DCA cue? (not on the last occurrence)
                bool sameAsNext = false;
                if (rowNumber + 1 < visibleCues.size())
                {
                    if (auto* nextDCA = dynamic_cast<DCACue*>(rowToCue(rowNumber + 1)))
                    {
                        if (auto* nextA = nextDCA->findAssignment(dcaIdx))
                        {
                            Array<int> nextChannels;
                            for (auto* r : nextA->characters->items)
                                if (auto* ch = r->getChannel())
                                    nextChannels.addIfNotAlreadyThere(ch->channelNumber->intValue());

                            if (nextChannels.size() == distinctChannels.size() && !nextChannels.isEmpty())
                            {
                                sameAsNext = true;
                                for (int c : distinctChannels)
                                    if (!nextChannels.contains(c)) { sameAsNext = false; break; }
                            }
                        }
                    }
                }

                if (sameAsNext)
                {
                    g.setColour(Colours::green.withAlpha(0.4f));
                    g.fillRect(0, 0, width, height);
                }
                else if (distinctChannels.size() > 1)
                {
                    g.setColour(Colours::grey.withAlpha(0.4f));
                    g.fillRect(0, 0, width, height);
                }

                g.setColour(Colours::white);
                if (cue->isAutoStartCue()) g.setOpacity(0.5f);
                g.drawText(a->getEffectiveDisplayName(), 4, 0, width - 8, height,
                           Justification::centred, true);

                if (a->forceFader->boolValue()) {
                    g.setOpacity(1.0f);
                    g.setFont(Font(10.0f, Font::bold));
                    g.setColour(Colours::red);
                    g.drawText("FDR", 2, 2, 24, 12, Justification::topLeft, false);
                }

                if (hasFX) {
                    g.setOpacity(1.0f);
                    g.setFont(Font(10.0f, Font::bold));
                    g.setColour(Colours::lightblue);
                    g.drawText("FX", width - 22, 2, 20, 12, Justification::topRight, false);
                }
            }

            // Border around the cell whose DCA modal is currently open (also for empty cells),
            // and a distinct one on the other selected DCA cues the modal would also affect.
            if (rowNumber == editingRow && dcaIdx == editingDca) {
                g.setOpacity(1.0f);
                g.setColour(Colours::orange);
                g.drawRect(0, 0, width, height, 2);
            }
            else if (editingRow != -1 && dcaIdx == editingDca && dcaCue->isSelected) {
                g.setOpacity(1.0f);
                g.setColour(Colours::deepskyblue);
                g.drawRect(0, 0, width, height, 2);
            }
        }
        return;
    }

    // Cue column (DCA Mixing Cuelist): the cue/cuelist this DCA cue also triggers on GO.
    if (TriggerCueColumn == columnId) {
        auto* dcaCue = dynamic_cast<DCACue*>(cue);
        if (dcaCue != nullptr) {
            ControllableContainer* target = dcaCue->triggerCue->getTargetContainer();
            String text;
            if (auto* tc = dynamic_cast<Cue*>(target))
                text = (tc == dcaCue) ? "!" : tc->id->stringValue();
            else if (dynamic_cast<Cuelist*>(target) != nullptr)
                text = "GO";

            if (text.isNotEmpty()) {
                Rectangle<float> r(0, 0, (float)width, (float)height);
                g.setOpacity(cue->isAutoStartCue() ? 0.4f : 1.0f);
                g.setColour(Colours::white);
                g.setFont(Font(11.0f));
                g.drawText(text, r.reduced(2, 0), Justification::centred, true);
                g.setOpacity(1.0f);
            }
        }
        return;
    }

    // Description Column
    if (DescriptionColumn == columnId) {
        Rectangle<float> r = Rectangle<float>(0, 0, width, height);

        // Group cues carry the fold/unfold disclosure at the right of the description
        // (click toggles it — see cellClicked). Drawn first so it's the rightmost badge.
        if (auto* grp = dynamic_cast<GroupCue*>(cue)) {
            auto tri = r.removeFromRight(kGroupToggleWidth);
            g.setOpacity(1.0f);
            // Match the group's box: orange sequential, sky-blue timeline, green parallel.
            const int grpMode = grp->fireMode->intValue();
            const Colour triColour = grpMode == GroupCue::SEQUENTIAL ? Colours::orange
                                   : grpMode == GroupCue::TIMELINE   ? Colours::skyblue
                                                                     : Colours::green;
            g.setColour(triColour.brighter(0.3f));
            float cx = tri.getCentreX(), cy = tri.getCentreY(), s = 6.5f;
            Path p;
            if (grp->collapsed->boolValue())
                p.addTriangle(cx - s * 0.5f, cy - s, cx - s * 0.5f, cy + s, cx + s * 0.85f, cy); // ▶
            else
                p.addTriangle(cx - s, cy - s * 0.55f, cx + s, cy - s * 0.55f, cx, cy + s * 0.85f); // ▼
            g.fillPath(p);
            g.setColour(Colours::white);
        }

        AudioCue* audioCue = dynamic_cast<AudioCue*>(cue);
        if (cue->getControllableByName("Loop") != nullptr && dynamic_cast<BoolParameter*>(cue->getControllableByName("Loop"))->boolValue()) {
            g.setOpacity(0.5f);
            g.drawImage(SPAssetManager::getInstance()->getLoopIcon(), r.removeFromRight(22).reduced(3), RectanglePlacement::centred, false);
        }

        if (audioCue != nullptr && audioCue->mtcCC->enabled->boolValue()) {
            g.setOpacity(0.5f);
            g.setColour(Colours::red);
            g.setFont(Font(11.0f, Font::bold));
            g.drawText("TC", r.removeFromRight(22), Justification::centred);
            g.setColour(Colours::white);
            g.setFont(Font(14.0f));
        }

        g.setOpacity(1.0f);
        if (cue->isAutoStartCue())
            g.setOpacity(0.4f);

        // Note cues: always italic and slightly dimmed (purely informational).
        const bool isNote = cue->getCueType() == "Note";
        if (isNote)
            g.setOpacity(0.6f);

        if (isNote || !cue->userCanRemove || cue->getDescription().startsWith("--"))
            g.setFont(Font(14.0f, Font::italic));

        // Indent grouped sub-cues so the group nesting reads visually.
        const int leftPad = (cue->parentGroup != nullptr) ? 26 : 10;
        g.drawText(cue->getDescription(), r.reduced(10, 0).withTrimmedLeft(leftPad - 10), Justification::centredLeft);
        return;
    }
}

Component* CuesTableModel::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate)
{
    return nullptr;
}

String CuesTableModel::getCellTooltip(int rowNumber, int columnId)
{
    if (rowNumber >= visibleCues.size())
        return {};

    if (columnId == TriggerCueColumn)
    {
        DCACue* dcaCue = dynamic_cast<DCACue*>(rowToCue(rowNumber));
        if (dcaCue == nullptr || dcaCue->triggerCue->getTargetContainer() == nullptr)
            return {};

        return getTriggerTooltip(dcaCue);
    }

    if (columnId == DescriptionColumn)
    {
        // Show the MTC interface when hovering the "TC" badge (right side of the cell,
        // just left of the loop icon when present).
        AudioCue* audioCue = dynamic_cast<AudioCue*>(rowToCue(rowNumber));
        if (audioCue == nullptr || !audioCue->mtcCC->enabled->boolValue())
            return {};

        bool loopOn = false;
        if (auto* loopP = dynamic_cast<BoolParameter*>(audioCue->getControllableByName("Loop")))
            loopOn = loopP->boolValue();

        Rectangle<int> cell = tlb->getCellPosition(columnId, rowNumber, true);
        int tcRight = cell.getRight() - (loopOn ? 22 : 0);
        int mouseX = tlb->getMouseXYRelative().getX();
        if (mouseX < tcRight - 22 || mouseX > tcRight)
            return {};

        MIDIInterface* iface = audioCue->getMTCInterface();
        return "MTC interface: " + (iface != nullptr ? iface->niceName : String("(none selected)"));
    }

    return {};
}

void CuesTableModel::selectedRowsChanged(int lastRowSelected)
{
    syncSelectionToInspector();
}

void CuesTableModel::syncSelectionToInspector()
{
    SparseSet<int> rows = tlb->getSelectedRows();
    if (rows.size() == 0) {
        // No row selected: also clear the inspectable selection, otherwise the previously
        // selected cue lingers (e.g. paste would still anchor to it instead of appending).
        if (auto* sm = InspectableSelectionManager::mainSelectionManager)
            if (!sm->isEmpty()) sm->clearSelection();
    } else if (rows.size() == 1) {
        inspectCue(rows[0]);
    } else if (rows.size() > 1) {
        // Multiple rows: select every corresponding cue so the Inspector shows the
        // multi-cue editor.
        Array<Inspectable*> cues;
        for (int i = 0; i < rows.size(); i++) {
            if (Cue* c = rowToCue(rows[i]))
                cues.add(c);
        }
        if (cues.size() > 1)
            InspectableSelectionManager::mainSelectionManager->selectInspectables(cues);
    }
}

// A cue's row among the currently visible rows, or -1 if it's hidden (collapsed group).
int CuesTableModel::visibleRowForCue(Cue* c)
{
    return visibleCues.indexOf(c);
}

void CuesTableModel::cellClicked(int rowNumber, int columnId, const MouseEvent& event)
{
    Cue* clicked = rowToCue(rowNumber);

    // Left-click the disclosure at the right of a group's description folds/unfolds it.
    if (!event.mods.isPopupMenu() && columnId == DescriptionColumn)
    {
        if (auto* grp = dynamic_cast<GroupCue*>(clicked))
        {
            auto cell = tlb->getCellPosition(DescriptionColumn, rowNumber, true);
            if (event.x >= cell.getRight() - kGroupToggleWidth)
            {
                grp->collapsed->setValue(!grp->collapsed->boolValue());
                tlb->updateContent();
                tlb->repaint();
                return;
            }
        }
    }

    if (event.mods.isPopupMenu())
    {
        if (clicked == nullptr) return;

        PopupMenu p;

        // Existing groups in this cuelist (targets of the "Add to group" submenu).
        Array<GroupCue*> groups;
        for (auto* c : cl->cues->items)
            if (auto* g = dynamic_cast<GroupCue*>(c))
                groups.add(g);

        bool canDelete = true;
        const bool singleSel = tlb->getSelectedRows().size() == 1;
        if (singleSel) {
            Cue* selectedCue = clicked;
            p.addSectionHeader("Cue " + selectedCue->id->stringValue() + " - " + selectedCue->getCueType());
            // p.addItem(1, "Play this cue");
            p.addItem(2, "Go after current cue");
            p.addSeparator();
            p.addItem(3, "Edit this cue");
            if (dynamic_cast<GroupCue*>(selectedCue) == nullptr)
                p.addItem(9, "Replace with new cue");
            canDelete = selectedCue->userCanRemove;
        } else {
            p.addItem(6, "Reorder selected cues ids...");
        }
        if (canDelete)
            p.addColouredItem(4, singleSel ? "Delete this cue" : "Delete selected cues", Colours::red);

        // --- Grouping ---
        p.addSeparator();
        if (dynamic_cast<GroupCue*>(clicked) != nullptr) {
            if (singleSel) p.addItem(21, "Ungroup");
        } else {
            p.addItem(22, singleSel ? "Group this cue" : "Group selected cues");
            if (!groups.isEmpty()) {
                PopupMenu sub;
                for (int i = 0; i < groups.size(); i++)
                    sub.addItem(300 + i, "Group " + groups[i]->id->stringValue() + " - " + groups[i]->getDescription());
                p.addSubMenu("Add to group", sub);
            }
            if (singleSel && clicked->parentGroup != nullptr)
                p.addItem(20, "Remove from group");
        }

        if (singleSel) {
            p.addSeparator();

            p.addItem(7, "Add new cue before...");
            p.addItem(8, "Add new cue after...");
        }

        p.showMenuAsync(PopupMenu::Options(), [this, rowNumber, clicked, groups](int result) {
            // if (result == 1){
            //     // Play action
            //     if (rowNumber < cl->cues.items.size()) {
            //         Cue* item = cl->cues.items[rowNumber];
            //         item->play();
            //     }
            // }
            if (result == 2){
                // Set go next action
                if (clicked != nullptr)
                    clicked->setGoNext();
            }
            if (result == 3) {
                // Edit action
                if (clicked != nullptr)
                    InspectableSelectionManager::mainSelectionManager->selectInspectable(clicked);

            } else if (result == 4) {
                askDeleteSelectedCues();
            } else if (result == 7 || result == 8) {
                if (clicked == nullptr) return;
                // Add new cue before/after — mapped to a real flat-list index. "After" a
                // group inserts past its whole (possibly hidden) member block.
                int clickedIdx = cl->cues->items.indexOf(clicked);
                int newIndex;
                if (result == 7) {
                    newIndex = clickedIdx;
                } else if (auto* g = dynamic_cast<GroupCue*>(clicked)) {
                    newIndex = clickedIdx + 1;
                    for (auto* m : g->getMembers())
                        newIndex = jmax(newIndex, cl->cues->items.indexOf(m) + 1);
                } else {
                    newIndex = clickedIdx + 1;
                }
                this->cl->cues->factory.showCreateMenu([this, newIndex](Cue* newCue)
                    {
                        if (newCue != nullptr)
                        {
                            var newCueData(new DynamicObject());
                            newCueData.getDynamicObject()->setProperty("index", newIndex);
                            this->cl->cues->addItem(newCue, newCueData);
                        }
                    }
                );
            } else if (result == 9) {
                // Replace with new cue
                Cue* oldCue = clicked;
                this->cl->cues->factory.showCreateMenu([this, oldCue](Cue* newCue)
                    {
                        if (newCue != nullptr && oldCue != nullptr)
                        {
                            int rowNumber = this->cl->cues->items.indexOf(oldCue);

                            if (newCue->getCueType() == oldCue->getCueType())
                            {
                                AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                                    "Replace " + oldCue->niceName,
                                    "You cannot replace a cue with another cue of the same type.");
                                return;
                            }

                            AlertWindow::showAsync(
                                MessageBoxOptions().withIconType(AlertWindow::QuestionIcon)
                                    .withTitle("Replace " + oldCue->niceName)
                                    .withMessage("Are you sure you want to replace this cue?")
                                    .withButton("Replace")
                                    .withButton("Cancel"),
                                    [this, newCue, oldCue, rowNumber](int result)
                                    {
                                        if (result == 0) return;

                                        var newCueData(new DynamicObject());
                                        newCueData.getDynamicObject()->setProperty("index", rowNumber);
                                        newCueData.getDynamicObject()->setProperty("id", oldCue->id->floatValue());
                                        newCue->itemColor->setValue(oldCue->itemColor->getValue());
                                        newCue->itemColor->referenceTarget->setValue(oldCue->itemColor->referenceTarget->getValue());
                                        newCue->description->setValue(oldCue->description->stringValue());
                                        newCue->notes->setValue(oldCue->notes->stringValue());
                                        oldCue->remove();
                                        this->cl->cues->addItem(newCue, newCueData);
                                        InspectableSelectionManager::mainSelectionManager->selectInspectable(newCue);

                                    }
                            );

                        }
                    }
                );
            } else if (result == 6) {
                reorderWindow.reset(new ReorderCuesWindow(this));
                DialogWindow::LaunchOptions dw;
                dw.content.set(reorderWindow.get(), false);
                dw.dialogTitle = "Reorder cues ID";
                dw.escapeKeyTriggersCloseButton = true;
                dw.dialogBackgroundColour = BG_COLOR;
                dw.resizable = false;
                dw.launchAsync();
            } else if (result == 20) {
                // Remove selected cue(s) from their group.
                for (Cue* c : currentSelectionOr(clicked))
                    if (c->parentGroup != nullptr) cl->cues->removeFromGroup(c);
                tlb->updateContent();
                tlb->repaint();
            } else if (result == 21) {
                // Ungroup: dissolve the group, keep its members.
                if (auto* g = dynamic_cast<GroupCue*>(clicked)) {
                    for (auto* m : g->getMembers()) cl->cues->removeFromGroup(m);
                    g->remove();
                    tlb->updateContent();
                    tlb->repaint();
                }
            } else if (result == 22) {
                // Wrap the selected cues in a brand-new group.
                Array<Cue*> members;
                for (auto* c : cl->cues->items)
                    if (currentSelectionOr(clicked).contains(c) && dynamic_cast<GroupCue*>(c) == nullptr)
                        members.add(c);
                if (members.isEmpty()) return;

                int insertIdx = cl->cues->items.size();
                for (auto* c : members) insertIdx = jmin(insertIdx, cl->cues->items.indexOf(c));

                auto* group = dynamic_cast<GroupCue*>(cl->cues->factory.create("Group Cue"));
                if (group == nullptr) return;
                var d(new DynamicObject());
                d.getDynamicObject()->setProperty("index", insertIdx);
                cl->cues->addItem(group, d);
                for (auto* c : members) cl->cues->addToGroup(c, group);

                InspectableSelectionManager::mainSelectionManager->selectInspectable(group);
                tlb->updateContent();
                tlb->repaint();
            } else if (result >= 300 && result < 300 + groups.size()) {
                // Add the selected cues to an existing group.
                GroupCue* group = groups[result - 300];
                Array<Cue*> sel = currentSelectionOr(clicked);
                for (auto* c : cl->cues->items)
                    if (sel.contains(c) && dynamic_cast<GroupCue*>(c) == nullptr)
                        cl->cues->addToGroup(c, group);
                tlb->updateContent();
                tlb->repaint();
            }
        });
    } else {
        // Respect a multi-row selection (e.g. mouse shift/cmd-click): don't collapse it
        // back to a single cue. selectedRowsChanged already updated the table selection.
        syncSelectionToInspector();
    }
}

// The cues under the current table selection, or just `fallback` if nothing is selected
// (e.g. a right-click that didn't change selection).
Array<Cue*> CuesTableModel::currentSelectionOr(Cue* fallback)
{
    Array<Cue*> out;
    auto rows = tlb->getSelectedRows();
    for (int i = 0; i < rows.size(); i++)
        if (Cue* c = rowToCue(rows[i])) out.add(c);
    if (out.isEmpty() && fallback != nullptr) out.add(fallback);
    return out;
}


namespace
{
    class CellEditBubble : public Component, private TextEditor::Listener
    {
    public:
        CellEditBubble(const String& initial, std::function<void(const String&)> commit) :
            onCommit(std::move(commit))
        {
            editor.setText(initial, dontSendNotification);
            editor.setSelectAllWhenFocused(true);
            editor.setEscapeAndReturnKeysConsumed(true);
            editor.setColour(TextEditor::backgroundColourId, BG_COLOR.darker(.1f));
            editor.setColour(TextEditor::textColourId, TEXT_COLOR);
            editor.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
            editor.setColour(TextEditor::focusedOutlineColourId, BG_COLOR.brighter(.3f));
            editor.setColour(TextEditor::highlightColourId, BG_COLOR.brighter(.2f));
            editor.setColour(TextEditor::highlightedTextColourId, TEXT_COLOR);
            editor.setColour(CaretComponent::caretColourId, TEXT_COLOR);
            editor.applyColourToAllText(TEXT_COLOR, true);
            editor.addListener(this);
            addAndMakeVisible(editor);
            setSize(200, 28);
        }

        void paint(Graphics& g) override { g.fillAll(BG_COLOR); }

    private:
        TextEditor editor;
        std::function<void(const String&)> onCommit;
        bool committed = false;

        void resized() override { editor.setBounds(getLocalBounds().reduced(2)); }
        void parentHierarchyChanged() override
        {
            if (isShowing())
            {
                Component::SafePointer<TextEditor> safeEd(&editor);
                MessageManager::callAsync([safeEd] { if (safeEd != nullptr) safeEd->grabKeyboardFocus(); });
            }
        }

        void commitAndDismiss()
        {
            if (committed) return;
            committed = true;
            if (onCommit) onCommit(editor.getText());
            if (auto* box = findParentComponentOfClass<CallOutBox>()) box->dismiss();
        }

        void textEditorReturnKeyPressed(TextEditor&) override { commitAndDismiss(); }
        void textEditorEscapeKeyPressed(TextEditor&) override
        {
            committed = true;
            if (auto* box = findParentComponentOfClass<CallOutBox>()) box->dismiss();
        }
        void textEditorFocusLost(TextEditor&) override { commitAndDismiss(); }
    };
}

void CuesTableModel::cellDoubleClicked(int rowNumber, int columnId, const MouseEvent& event)
{
    if (columnId == IdColumn || columnId == DescriptionColumn
        || columnId == PreWaitColumn || columnId == PostWaitColumn
        || columnId == TimeColumn)
    {
        if (rowNumber < 0 || rowNumber >= visibleCues.size()) return;

        Cue* c = rowToCue(rowNumber);
        if (c == nullptr) return;

        // The Time cell is only editable for FadeCue (its duration is the fade time).
        if (columnId == TimeColumn && dynamic_cast<FadeCue*>(c) == nullptr) return;

        // Grouped sub-cues can't have a post-wait, so its cell isn't editable either.
        if (columnId == PostWaitColumn && c->parentGroup != nullptr) return;

        String initial;
        std::function<void(const String&)> onCommit;

        if (columnId == IdColumn)
        {
            initial = c->id->stringValue();
            onCommit = [c](const String& t) {
                c->id->setValue(t.getFloatValue());
                c->id->notifyValueChanged();
            };
        }
        else if (columnId == DescriptionColumn)
        {
            initial = c->description->stringValue();
            onCommit = [c](const String& t) { c->description->setValue(t); };
        }
        else if (columnId == PreWaitColumn)
        {
            initial = c->preWaitCC->enabled->boolValue() ? String(c->preWaitDuration->floatValue()) : String();
            onCommit = [c](const String& t) {
                if (t.trim().isEmpty())
                {
                    c->preWaitCC->enabled->setValue(false);
                }
                else
                {
                    c->preWaitDuration->setValue(jmax(0.0f, t.getFloatValue()));
                    c->preWaitCC->enabled->setValue(true);
                }
            };
        }
        else if (columnId == PostWaitColumn)
        {
            initial = c->postWaitCC->enabled->boolValue() ? String(c->postWaitDuration->floatValue()) : String();
            onCommit = [c](const String& t) {
                if (t.trim().isEmpty())
                {
                    c->postWaitCC->enabled->setValue(false);
                }
                else
                {
                    c->postWaitDuration->setValue(jmax(0.0f, t.getFloatValue()));
                    c->postWaitCC->enabled->setValue(true);
                }
            };
        }
        else // TimeColumn (FadeCue only — see guard above)
        {
            initial = String(c->duration->floatValue());
            onCommit = [c](const String& t) {
                c->duration->setValue(jmax(0.0f, t.getFloatValue()));
            };
        }

        auto cellRect = tlb->getCellPosition(columnId, rowNumber, true);
        auto screenRect = tlb->localAreaToGlobal(cellRect);
        auto bubble = std::make_unique<CellEditBubble>(initial, std::move(onCommit));
        CallOutBox::launchAsynchronously(std::move(bubble), screenRect, nullptr);
        return;
    }

    if (columnId == TriggerCueColumn)
    {
        if (rowNumber < 0 || rowNumber >= visibleCues.size()) return;

        DCACue* dcaCue = dynamic_cast<DCACue*>(rowToCue(rowNumber));
        if (dcaCue == nullptr) return;

        // Edit the "GO cue on play" target via a bubble holding the TargetParameter's
        // own UI (click to pick from the GO menu, or clear), mirroring the description cell.
        auto cellRect = tlb->getCellPosition(columnId, rowNumber, true);
        auto screenRect = tlb->localAreaToGlobal(cellRect);
        auto ui = std::unique_ptr<Component>(dcaCue->triggerCue->createDefaultUI());
        ui->setSize(jmax(220, cellRect.getWidth()), 28);
        CallOutBox::launchAsynchronously(std::move(ui), screenRect, nullptr);
        return;
    }

    if (columnId >= FirstDCAColumn && columnId <= LastDCAColumn)
    {
        if (rowNumber < 0 || rowNumber >= visibleCues.size()) return;

        int dcaIdx = columnId - FirstDCAColumn + 1;
        DCACue* dcaCue = dynamic_cast<DCACue*>(rowToCue(rowNumber));
        if (dcaCue == nullptr) return;

        DCAAssignment* a = dcaCue->findAssignment(dcaIdx);
        if (a == nullptr) a = dcaCue->createAssignment(dcaIdx);
        if (a == nullptr) return;

        auto* dialog = new DCAAssignmentDialog(dcaCue, a);

        // Highlight the edited cell, and follow the dialog's DCA navigation / closing.
        activeDcaDialog = dialog;
        editingRow = rowNumber;
        editingDca = dcaIdx;
        dialog->onStateChanged = [this, dialog]() {
            if (activeDcaDialog.getComponent() != dialog) return; // superseded by another dialog
            editingDca = dialog->currentDca;
            if (tlb != nullptr) tlb->repaint();
        };
        dialog->onClosed = [this, dialog]() {
            if (activeDcaDialog.getComponent() != dialog) return; // a newer dialog owns the highlight
            editingRow = -1;
            editingDca = -1;
            if (tlb != nullptr) tlb->repaint();
        };
        if (tlb != nullptr) tlb->repaint();

        DialogWindow::LaunchOptions dw;
        dw.content.setOwned(dialog);
        dw.dialogTitle = "DCA " + String(dcaIdx) + " — characters";
        dw.dialogBackgroundColour = BG_COLOR;
        dw.escapeKeyTriggersCloseButton = true;
        dw.useNativeTitleBar = false;
        dw.resizable = false;
        dw.launchAsync();
        return;
    }

    if (Cue* c = rowToCue(rowNumber))
    {
        cl->nextCue->setTarget(c);
        cl->nextCue->notifyValueChanged();
    }
}

void CuesTableModel::backgroundClicked(const MouseEvent& event)
{
    tlb->deselectAllRows();

    if (event.mods.isPopupMenu()){
        cl->cues->factory.showCreateMenu([this](Cue* item)
            {
                if (item != nullptr)
                {
                    this->cl->cues->addItem(item);
                }
            }
        );
    }
}

void CuesTableModel::deleteKeyPressed(int lastRowSelected)
{
    askDeleteSelectedCues();
}

void CuesTableModel::returnKeyPressed(int lastRowSelected)
{
    Cue* c = rowToCue(lastRowSelected);
    if (c == nullptr) return;

    cl->nextCue->setTarget(c);
    cl->nextCue->notifyValueChanged();
}

void CuesTableModel::inspectCue(int rowNumber)
{
    if (Cue* c = rowToCue(rowNumber))
        InspectableSelectionManager::mainSelectionManager->selectInspectable(c);
}

void CuesTableModel::askDeleteSelectedCues()
{
    // Delete action
    String title = "Delete selected cues";
    String message = "Are you sure you want to delete the selected cues?";
    SparseSet<int> selected = tlb->getSelectedRows();

    // Filter: keep only cues that can be removed
    Array<Cue*> removable;
    for (int i = 0; i < selected.size(); ++i)
    {
        Cue* item = rowToCue(selected[i]);
        if (item != nullptr && item->userCanRemove) removable.add(item);
    }

    if (removable.isEmpty()) return;

    if (GlobalSettings::getInstance()->askBeforeRemovingItems->boolValue())
    {
        if (removable.size() == 1) {
            title = "Delete " + removable[0]->niceName;
            message = "Are you sure you want to delete this cue?";
        }

        AlertWindow::showAsync(
            MessageBoxOptions().withIconType(AlertWindow::QuestionIcon)
                .withTitle(title)
                .withMessage(message)
                .withButton("Delete")
                .withButton("Cancel"),
                [this, removable](int result)
                {
                    if (result == 0) return;
                    cl->cues->removeCues(removable); // one undo for the whole selection
                }
        );
    }
    else
    {
        cl->cues->removeCues(removable);
    }
}

void CuesTableModel::sortOrderChanged(int newSortColumnId, bool isForwards)
{
}

var CuesTableModel::getDragSourceDescription(const SparseSet<int>& selectedRows)
{
    // Return a var containing the selected row indices to enable dragging
    if (selectedRows.size() > 0)
    {
        var dragData(new DynamicObject());
        dragData.getDynamicObject()->setProperty("type", "cueTableRows");

        // Store the selected rows as an array
        Array<var> rowsArray;
        for (int i = 0; i < selectedRows.size(); i++)
        {
            rowsArray.add(selectedRows[i]);
        }
        dragData.getDynamicObject()->setProperty("rows", rowsArray);

        return dragData;
    }
    return var();
}

void CuesTableModel::paintStatusArrow(Graphics& g, float x, float y, float h, bool playing, bool isNext)
{
    auto at = AffineTransform::translation(x, y);

    if (playing && isNext)
    {
        float midY = h * 0.5f;
        Path top;
        top.addRectangle(0, 0, 5, midY);
        top.addTriangle(5, 0, 5, midY, 10, midY);
        g.setColour(Colours::green.brighter(0.2f));
        g.fillPath(top, at);

        Path bottom;
        bottom.addRectangle(0, midY, 5, midY);
        bottom.addTriangle(5, midY, 5, h, 10, midY);
        g.setColour(Colours::orange.brighter(0.2f));
        g.fillPath(bottom, at);
    }
    else if (playing || isNext)
    {
        Path p;
        p.addRectangle(0, 0, 5, h);
        p.addTriangle(5, 0, 5, h, 10, h * 0.5f);
        g.setColour((playing ? Colours::green : Colours::orange).brighter(0.2f));
        g.fillPath(p, at);
    }
}

String CuesTableModel::valueToTimeString(double timeVal)
{
    int numDecimals = 3;
	int hours = abs(trunc(timeVal / 3600));
	int minutes = abs(trunc(fmod(timeVal, 3600) / 60));
	double seconds = abs(fmod(timeVal, 60));

    if (hours > 0)
        return String::formatted("%02i:%02i:%0" + String(3 + numDecimals) + "." + String(numDecimals) + "f", hours, minutes, seconds);

    return String::formatted("%02i:%0" + String(3 + numDecimals) + "." + String(numDecimals) + "f", minutes, seconds);
}

void CuesTableModel::newMessage(const InspectableSelectionManager::SelectionEvent& e)
{
    if (e.type == InspectableSelectionManager::SelectionEvent::Type::SELECTION_CHANGED)
    {
        Cue* cue = e.selectionManager->getInspectableAs<Cue>();
        if (cue == nullptr) {
            tlb->deselectAllRows();
            tlb->repaint();
            return;
        }
    }
}

void CuesTableModel::reorderCues(float startId, float increment)
{
    // Suspend per-cue duplicate-id checks while we reassign ids (intermediate states
    // would collide), then run one full-cuelist check at the end so the reordered cues
    // are still validated against the cues that weren't part of the reorder.
    cl->cues->isReordering = true;

    float currentId = startId;
    for (int i = 0; i < tlb->getNumRows(); i++) {
        if (tlb->isRowSelected(i)) {
            Cue* cue = rowToCue(i);
            if (cue == nullptr) continue;
            cue->id->setValue(currentId);
            cue->id->notifyValueChanged();
            currentId += increment;
        }
    }

    cl->cues->isReordering = false;
    cl->cues->refreshDuplicateIdWarnings();

    tlb->repaint();
}
