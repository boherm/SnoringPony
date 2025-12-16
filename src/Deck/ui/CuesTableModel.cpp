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
#include "juce_graphics/juce_graphics.h"

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

        // Ensure the next cue + 1 is visible (or the next cue if it's the last one)
        if (cl->nextCue->getTargetContainerAs<Cue>() == nullptr)
            return;

        auto nextCueIndex = cl->cues->items.indexOf(cl->nextCue->getTargetContainerAs<Cue>());
        if (nextCueIndex + 1 < cl->cues->items.size()) {
            tlb->scrollToEnsureRowIsOnscreen(nextCueIndex + 1);
        } else {
            tlb->scrollToEnsureRowIsOnscreen(nextCueIndex);
        }
    }
}

void CuesTableModel::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= cl->cues->items.size())
        return;

    g.setFont(14.0f);
    g.setColour(Colours::white);

    Cue* cue = cl->cues->items[rowNumber];
    Cue* nextCue = cl->nextCue->getTargetContainerAs<Cue>();


    // Status column
    if (StatusColumn == columnId) {
        Path myPath;

        if (nextCue == cue) {
            g.setColour(Colours::orange.brighter(0.2f));
            myPath.addRectangle(0, 0, 5, height);
            myPath.addTriangle(5, 0, 5, height, 10, height * 0.5f);
            g.fillPath(myPath);
        }

        if (cue->isPlaying->boolValue() || cue->preWaitActive->boolValue() || cue->postWaitActive->boolValue()) {
            g.setColour(Colours::green.brighter(0.2f));
            myPath.addRectangle(0, 0, 5, height);
            myPath.addTriangle(5, 0, 5, height, 10, height * 0.5f);
            g.fillPath(myPath);
        }
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
        if (!cue->preWaitCC->enabled->boolValue() || cue->preWaitDuration->doubleValue() <= 0.0)
            return;

        double timeLeft = cue->preWaitDuration->doubleValue() - cue->preWaitCurrentTime->doubleValue();
        double positionPercent = cue->preWaitCurrentTime->doubleValue() / cue->preWaitDuration->doubleValue();
        auto color = Colours::darkgoldenrod;
        String text = CuesTableModel::valueToTimeString(jmax<double>(timeLeft, 0.0));

        Rectangle<float> r = Rectangle<float>(0, 0, width, height);
        r.reduce(4, 4);

        Path myPath;
        myPath.addRectangle(r.getX(), r.getY(), r.getWidth(), r.getHeight());
        g.setColour(color);

        if (cue->isAutoStartCue()) g.setOpacity(0.5f);

        g.strokePath(myPath, PathStrokeType(1));
        g.setColour(color.withAlpha(0.3f));

        g.fillRect(r.getX(), r.getY(), r.getWidth() * positionPercent, r.getHeight());
        g.setColour(Colours::white.withAlpha(0.8f));

        if (cue->isAutoStartCue()) g.setOpacity(0.5f);

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
        if (cue->duration->doubleValue() <= 0.0)
            return;

        double timeLeft = cue->duration->doubleValue() - cue->currentTime->doubleValue();
        double positionPercent = cue->currentTime->doubleValue() / cue->duration->doubleValue();
        auto color = !cue->isPlaying->boolValue() || cue->preWaitActive->boolValue() || timeLeft > 10 ? Colours::green.brighter(0.2f) : Colours::red.brighter(0.2f);
        String text = CuesTableModel::valueToTimeString(jmax<double>(timeLeft, 0.0));

        Rectangle<float> r = Rectangle<float>(0, 0, width, height);
        r.reduce(4, 4);

        Path myPath;
        myPath.addRectangle(r.getX(), r.getY(), r.getWidth(), r.getHeight());
        g.setColour(color);

        if (cue->isAutoStartCue()) g.setOpacity(0.5f);

        g.strokePath(myPath, PathStrokeType(1));
        g.setColour(color.withAlpha(0.3f));

        g.fillRect(r.getX(), r.getY(), r.getWidth() * positionPercent, r.getHeight());
        g.setColour(Colours::white.withAlpha(0.8f));

        if (cue->isAutoStartCue()) g.setOpacity(0.5f);

        g.drawText(text, r.getX(), r.getY(), r.getWidth(), r.getHeight(), Justification::centred, true);

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
        if (!cue->postWaitCC->enabled->boolValue() || cue->postWaitDuration->doubleValue() <= 0.0)
            return;

        Rectangle<float> r = Rectangle<float>(0, 0, width, height);
        r.reduce(4, 4);

        double timeLeft = cue->postWaitDuration->doubleValue() - cue->postWaitCurrentTime->doubleValue();
        double positionPercent = cue->postWaitCurrentTime->doubleValue() / cue->postWaitDuration->doubleValue();
        auto color = Colours::darkgoldenrod;
        String text = CuesTableModel::valueToTimeString(jmax<double>(timeLeft, 0.0));


        Path myPath;
        myPath.addRectangle(r.getX(), r.getY(), r.getWidth(), r.getHeight());
        g.setColour(color);

        if (cue->isAutoStartCue()) g.setOpacity(0.5f);

        g.strokePath(myPath, PathStrokeType(1));
        g.setColour(color.withAlpha(0.3f));

        g.fillRect(r.getX(), r.getY(), r.getWidth() * positionPercent, r.getHeight());
        g.setColour(Colours::white.withAlpha(0.8f));

        if (cue->isAutoStartCue()) g.setOpacity(0.5f);

        g.drawText(text, r.getX(), r.getY(), r.getWidth(), r.getHeight(), Justification::centred, true);
        return;
    }

    // Description Column
    if (DescriptionColumn == columnId) {
        Rectangle<float> r = Rectangle<float>(0, 0, width, height);

        if (cue->getControllableByName("Loop") != nullptr && dynamic_cast<BoolParameter*>(cue->getControllableByName("Loop"))->boolValue()) {
            g.setOpacity(0.5f);
            g.drawImage(SPAssetManager::getInstance()->getLoopIcon(), r.removeFromRight(22).reduced(3), RectanglePlacement::centred, false);
        }
        g.setOpacity(1.0f);
        if (cue->isAutoStartCue())
            g.setOpacity(0.4f);

        g.drawText(cue->getDescription(), r.reduced(10, 0), Justification::centredLeft);
        return;
    }
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
    } else {
        inspectCue(rowNumber);
    }
}


void CuesTableModel::cellDoubleClicked(int rowNumber, int columnId, const MouseEvent& event)
{
    cl->nextCue->setTarget(cl->cues->items[rowNumber]);
    cl->nextCue->notifyValueChanged();
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
