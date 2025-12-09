/*
  ==============================================================================

    CuesTableModel.cpp
    Created: 30 Sep 2025 11:01:57pm
    Author:  boherm

  ==============================================================================
*/

#include "CuesTableModel.h"
#include "../../Cuelist/Cuelist.h"
#include "../../Cuelist/CuelistManager.h"
#include "../../Cue/CueManager.h"
#include "../../ui/SPAssetManager.h"

enum ColumnIds
{
    StatusColumn = 1,
    IdColumn = 2,
    TypeColumn = 3,
    DescriptionColumn = 4,
    TimeColumn = 5
};

CuesTableModel::CuesTableModel(TableListBox* tlb, Cuelist* cl)
{
    this->cl = cl;
    this->tlb = tlb;

    if (cl == nullptr) {
        return;
    }

    cl->nextCue->addParameterListener(this);
}

CuesTableModel::~CuesTableModel()
{
    cl->nextCue->removeParameterListener(this);
}

int CuesTableModel::getNumRows()
{
    return cl->cues->items.size();
}

void CuesTableModel::paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    Cue* cue = cl->cues->items[rowNumber];

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
    }
}

void CuesTableModel::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= cl->cues->items.size())
        return;

    g.setFont(14.0f);

    Cue* cue = cl->cues->items[rowNumber];
    Cue* nextCue = cl->nextCue->getTargetContainerAs<Cue>();

    g.setColour(Colours::white);

    double positionPercent;
    double timeLeft;
    String text;
    Image img;
    Path myPath;
    switch (columnId)
    {
        case StatusColumn:

            if (nextCue == cue) {
                g.setColour(Colours::orange.brighter(0.2f));
                myPath.addRectangle(0, 0, 5, height);
                myPath.addTriangle(5, 0, 5, height, 10, height * 0.5f);
                g.fillPath(myPath);
            }

            if (cue->isPlaying->boolValue()) {
                g.setColour(Colours::green.brighter(0.2f));
                myPath.addRectangle(0, 0, 5, height);
                myPath.addTriangle(5, 0, 5, height, 10, height * 0.5f);
                g.fillPath(myPath);
            }

            break;

        case IdColumn:
            g.setColour(Colours::white);
            g.drawText(cue->id->stringValue(), 2, 0, width - 4, height, Justification::centred, true);
            text = "";
            break;

        case TypeColumn:
            img = SPAssetManager::getInstance()->getCueIcon(cue->getCueType());
            g.setOpacity(0.7f);

            if (cue->getWarningMessage().isNotEmpty()) {
                g.setOpacity(1.0f);
                img = AssetManager::getInstance()->warningImage;
            }

            g.drawImageWithin(img, 7.0, 7.0, width - 14.0, height - 14.0, RectanglePlacement::centred, false);
            g.setOpacity(1.0f);
            text = "";
            break;

        case TimeColumn:
            if (cue->duration->doubleValue() <= 0.0)
                return;

            timeLeft = cue->duration->doubleValue() - cue->currentTime->doubleValue();
            positionPercent = cue->currentTime->doubleValue() / cue->duration->doubleValue();

            myPath.addRectangle(4, 3, (width - 8), height - 6);
            g.setColour(timeLeft > 10 ? Colours::green.brighter(0.2f) : Colours::red.brighter(0.2f));
            g.strokePath(myPath, PathStrokeType(1));
            g.setColour(timeLeft > 10 ? Colours::green.brighter(0.2f).withAlpha(0.6f) : Colours::red.brighter(0.2f).withAlpha(0.6f));
            g.fillRect(4.0f, 3.0f, (width - 8.0f) * positionPercent, height - 6.0f);
            g.setColour(Colours::white.withAlpha(0.8f));
            text = StringUtil::valueToTimeString(jmax<double>(timeLeft, 0.0));
            g.drawText(text, 4, 3, width - 8, height - 6, Justification::centred, true);
            return;
            break;

        case DescriptionColumn:
            text = cue->description->stringValue();
            break;
    }

    g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);
}

Component* CuesTableModel::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate)
{
    return nullptr;
}

void CuesTableModel::selectedRowsChanged(int lastRowSelected)
{
    if (tlb->getSelectedRows().size() == 1) {
        inspectCue(lastRowSelected);
    }
}

void CuesTableModel::cellClicked(int rowNumber, int columnId, const MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        PopupMenu p;

        if (tlb->getSelectedRows().size() == 1) {
            Cue* selectedCue = cl->cues->items[rowNumber];
            p.addSectionHeader("Cue " + selectedCue->id->stringValue() + " - " + selectedCue->getCueType());
            // p.addItem(1, "Play this cue");
            p.addItem(2, "Go after current cue");
            p.addSeparator();
            p.addItem(3, "Edit this cue");
            p.addItem(9, "Replace with new cue");
        } else {
            p.addItem(6, "Reorder selected cues ids...");
        }
        p.addColouredItem(4, tlb->getSelectedRows().size() > 1 ? "Delete selected cues" : "Delete this cue", Colours::red);

        if (tlb->getSelectedRows().size() == 1) {
            p.addSeparator();

            p.addItem(7, "Add new cue before...");
            p.addItem(8, "Add new cue after...");
        }

        p.showMenuAsync(PopupMenu::Options(), [this, rowNumber](int result) {
            // if (result == 1){
            //     // Play action
            //     if (rowNumber < cl->cues.items.size()) {
            //         Cue* item = cl->cues.items[rowNumber];
            //         item->play();
            //     }
            // }
            if (result == 2){
                // Set go next action
                if (rowNumber < cl->cues->items.size()) {
                    Cue* item = cl->cues->items[rowNumber];
                    item->setGoNext();
                }
            }
            if (result == 3) {
                // Edit action
                if (rowNumber < cl->cues->items.size()) {
                    inspectCue(rowNumber);
                }

            } else if (result == 4) {
                // Delete action
                String title = "Delete selected cues";
                String message = "Are you sure you want to delete the selected cues?";
                SparseSet<int> selected = tlb->getSelectedRows();

                if (GlobalSettings::getInstance()->askBeforeRemovingItems->boolValue())
                {
                    if (tlb->getSelectedRows().size() == 1) {
                        Cue* item = cl->cues->items[tlb->getSelectedRows()[0]];
                        title = "Delete " + item->niceName;
                        message = "Are you sure you want to delete this cue?";
                    }

                    AlertWindow::showAsync(
                        MessageBoxOptions().withIconType(AlertWindow::QuestionIcon)
                            .withTitle(title)
                            .withMessage(message)
                            .withButton("Delete")
                            .withButton("Cancel"),
                            [selected, this](int result)
                            {
                                if (result == 0) return;

                                for (int i = selected.size() - 1; i >= 0; i--) {
                                    int r = selected[i];
                                    if (r < this->cl->cues->items.size()) {
                                        Cue* item = this->cl->cues->items[r];
                                        item->remove();
                                    }
                                }
                            }
                    );

                }
            } else if (result == 7 || result == 8) {
                // Add new cue before/after
                int newIndex = (result == 7) ? rowNumber : rowNumber + 1;
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
                this->cl->cues->factory.showCreateMenu([this, rowNumber](Cue* newCue)
                    {
                        if (newCue != nullptr)
                        {
                            Cue* oldCue = this->cl->cues->items[rowNumber];

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
                                        this->inspectCue(rowNumber);

                                    }
                            );

                        }
                    }
                );
            }
        });
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

void CuesTableModel::inspectCue(int rowNumber)
{
    auto inspect = ShapeShifterManager::getInstance()->getContentForName("Inspector");
    if (inspect == nullptr)
        return;
    InspectorUI *inspectorUI = dynamic_cast<InspectorUI *>(inspect->contentComponent);

    inspectorUI->inspector->setCurrentInspectables(cl->cues->items[rowNumber]);
    inspectorUI->repaint();
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
