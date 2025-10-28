/*
  ==============================================================================

    CuesTableModel.cpp
    Created: 30 Sep 2025 11:01:57pm
    Author:  boherm

  ==============================================================================
*/

#include "CuesTableModel.h"
#include "../../Cuelist/CuelistManager.h"
#include "../../ui/SPAssetManager.h"
#include "juce_gui_basics/juce_gui_basics.h"

enum ColumnIds
{
    IdColumn = 1,
    TypeColumn = 2,
    DescriptionColumn = 3,
    TimeColumn = 4
};

CuesTableModel::CuesTableModel(TableListBox* tlb, Cuelist* cl)
{
    this->cl = cl;
    this->tlb = tlb;

    if (cl == nullptr) {
        return;
    }
}

CuesTableModel::~CuesTableModel()
{
}

int CuesTableModel::getNumRows()
{
    return cl->cues.items.size();
}

void CuesTableModel::paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    Cue* cue = cl->cues.items[rowNumber];

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
        g.fillAll(cue->itemColor->getColor().darker(0.1f));
    }
}

void CuesTableModel::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= cl->cues.items.size())
        return;

    g.setFont(14.0f);
    g.setColour(Colours::white);

    Cue* cue = cl->cues.items[rowNumber];

    String text;
    Image img;
    Path myPath;
    switch (columnId)
    {
        case IdColumn:
            g.drawText(cue->id->stringValue(), 2, 0, width - 4, height, Justification::centred, true);
            text = "";
            break;

        case TypeColumn:
            img = SPAssetManager::getInstance()->getCueIcon(cue->getCueType());
            g.setOpacity(0.7f);
            g.drawImageWithin(img, 7.0, 7.0, width - 14.0, height - 14.0, RectanglePlacement::centred, false);
            g.setOpacity(1.0f);
            text = "";
            break;

        case TimeColumn:
            myPath.addRectangle(4, 3, (width - 8), height - 6);
            g.setColour(Colours::green.brighter(0.2f));
            g.strokePath(myPath, PathStrokeType(1));
            g.setColour(Colours::green.brighter(0.2f).withAlpha(0.6f));
            g.fillRect(4.0f, 3.0f, (width - 8.0f) * Random::getSystemRandom().nextFloat(), height - 6.0f);
            g.setColour(Colours::white.withAlpha(0.8f));
            text = "00:20.12";
            g.drawText(text, 4, 3, width - 8, height - 6, Justification::centred,
                    true);
            return;
            break;

        case DescriptionColumn:
            text = cue->description->stringValue();
            break;

        // case NameColumn:
        //     text = data.name;
        //     break;
        // case TimeColumn:
        //   myPath.addRectangle(4, 3, (width - 8), height - 6);
        //   g.setColour(Colours::green.brighter(0.2f));
        //   g.strokePath(myPath, PathStrokeType(1));
        //   g.setColour(Colours::green.brighter(0.2f).withAlpha(0.6f));
        //   g.fillRect(4.0f, 3.0f, (width - 8.0f) * Random::getSystemRandom().nextFloat(), height - 6.0f);
        //   g.setColour(Colours::white.withAlpha(0.8f));
        //   text = data.time;
        //   g.drawText(text, 4, 3, width - 8, height - 6, Justification::centred,
        //              true);
        //   return;
        //   break;
        // case DescriptionColumn:
        //     text = data.description;
        //     break;
        // case ActiveColumn:
        //     text = data.isActive ? "OK" : "KO";
        //     g.setColour(data.isActive ? Colours::green : Colours::red);
        //     break;
    }

    // g.setColour(Colours::white.withAlpha(0.5f));
    g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);
}

Component* CuesTableModel::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate)
{
    return nullptr;
}

void CuesTableModel::cellClicked(int rowNumber, int columnId, const MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        Logger::writeToLog("CuesTableModel::cellClicked: " + String(tlb->getSelectedRows().size()));
        PopupMenu p;
        // p.addCustomItem(0, "Cue " + String(rowNumber + 1), false, false);
        p.addItem(-1, "Play this cue");
        p.addItem(1, "Edit this cue", tlb->getSelectedRows().size() == 1);
        p.addItem(2, tlb->getSelectedRows().size() > 1 ? "Delete selected cues" : "Delete this cue");
        p.addSeparator();
        p.addItem(3, "Add new cue after");
        p.addItem(4, "Add new cue before");

        p.showMenuAsync(PopupMenu::Options(), [this, rowNumber](int result) {
            if (result == -1){
                // Play action
                if (rowNumber < cl->cues.items.size()) {
                    Cue* item = cl->cues.items[rowNumber];
                    item->play();
                }
            }
            if (result == 1) {
                // Edit action
                if (rowNumber < cl->cues.items.size()) {
                    inspectCue(rowNumber);
                }
            } else if (result == 2) {
                // Delete action
                if (rowNumber < cl->cues.items.size() && tlb->getSelectedRows().size() == 1) {
                    Cue* item = cl->cues.items[rowNumber];

                    if (item->askConfirmationBeforeRemove && GlobalSettings::getInstance()->askBeforeRemovingItems->boolValue())
                    {
                        AlertWindow::showAsync(
                            MessageBoxOptions().withIconType(AlertWindow::QuestionIcon)
                                .withTitle("Delete " + item->niceName)
                                .withMessage("Are you sure you want to delete this ?")
                                .withButton("Delete")
                                .withButton("Cancel"),
                                [item](int result)
                                {
                                    if (result != 0) item->remove();
                                }
                        );
                    }
                    else cl->cues.askForRemoveBaseItem(item);
                }
            }
        });
        return;
    }
}

void CuesTableModel::backgroundClicked(const MouseEvent& event)
{
    // if (event.mods.isPopupMenu())
    // {
    //     PopupMenu p;
    //     p.addItem(1, "Add new cue");
    //     p.showMenuAsync(PopupMenu::Options());
    //     return;
    // }
}

void CuesTableModel::cellDoubleClicked(int rowNumber, int columnId, const MouseEvent& event)
{
    inspectCue(rowNumber);
}

void CuesTableModel::inspectCue(int rowNumber)
{
    auto inspect = ShapeShifterManager::getInstance()->getContentForName("Inspector");
    if (inspect == nullptr)
        return;
    InspectorUI *inspectorUI = dynamic_cast<InspectorUI *>(inspect->contentComponent);

    inspectorUI->inspector->setCurrentInspectables(cl->cues.items[rowNumber]);
    inspectorUI->repaint();
}

int CuesTableModel::getColumnAutoSizeWidth(int columnId)
{
    // switch (columnId)
    // {
    //     case NameColumn:
    //         return 120;
    //     case TimeColumn:
    //         return 80;
    //     case DescriptionColumn:
    //         return 200;
    //     case ActiveColumn:
    //         return 60;
    //     default:
    //         return 100;
    // }
}

void CuesTableModel::sortOrderChanged(int newSortColumnId, bool isForwards)
{
}

// void CuesTableModel::generateTestData()
// {
//     cueData.clear();

    // for (auto& cue : cl->cues.items) {
    //     CueData cd;
    //     cd.name = cue->niceName;
    //     cd.time = "00:01:30";
    //     cd.description = cue->notes->getValue();
    //     cd.isActive = true;

    //     cueData.add(cd);
    // }

    // cueData.add({"Cue 1", "00:01:30", "Introduction", true});
    // cueData.add({"Cue 2", "00:02:45", "Verse 1", false});
    // cueData.add({"Cue 3", "00:04:12", "Chorus", true});
    // cueData.add({"Cue 4", "00:05:30", "Verse 2", false});
    // cueData.add({"Cue 5", "00:07:00", "Bridge", true});
    // cueData.add({"Cue 6", "00:08:15", "Final Chorus", false});
    // cueData.add({"Cue 7", "00:09:45", "Outro", true});
    // cueData.add({"Cue 8", "00:11:00", "Applause", false});
// }
