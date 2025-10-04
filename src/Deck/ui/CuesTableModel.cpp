/*
  ==============================================================================

    CuesTableModel.cpp
    Created: 30 Sep 2025 11:01:57pm
    Author:  boherm

  ==============================================================================
*/

#include "CuesTableModel.h"
#include "../../Cuelist/CuelistManager.h"

enum ColumnIds
{
    NameColumn = 1,
    TimeColumn = 2,
    DescriptionColumn = 3,
    ActiveColumn = 4
};

CuesTableModel::CuesTableModel(Cuelist* cl)
{
    this->cl = cl;

    if (cl == nullptr) {
        return;
    }
    generateTestData();
}

CuesTableModel::~CuesTableModel()
{
}

int CuesTableModel::getNumRows()
{
    return cl->cues.items.size();
    // return cueData.size();
}

void CuesTableModel::paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.fillAll(BG_COLOR.brighter(0.1f));
    }
    else if (rowNumber % 2 == 0)
    {
        g.fillAll(BG_COLOR);
    }
    else
    {
        g.fillAll(BG_COLOR.brighter(0.05f));
    }
}

void CuesTableModel::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= cueData.size())
        return;

    const CueData& data = cueData[rowNumber];
    g.setFont(14.0f);
    g.setColour(Colours::white);


    String text;
    Path myPath;
    switch (columnId)
    {
        case NameColumn:
            text = data.name;
            break;
        case TimeColumn:
          myPath.addRectangle(4, 3, (width - 8), height - 6);
          g.setColour(Colours::green.brighter(0.2f));
          g.strokePath(myPath, PathStrokeType(1));
          g.setColour(Colours::green.brighter(0.2f).withAlpha(0.6f));
          g.fillRect(4.0f, 3.0f, (width - 8.0f) * Random::getSystemRandom().nextFloat(), height - 6.0f);
          g.setColour(Colours::white.withAlpha(0.8f));
          text = data.time;
          g.drawText(text, 4, 3, width - 8, height - 6, Justification::centred,
                     true);
          return;
          break;
        case DescriptionColumn:
            text = data.description;
            break;
        case ActiveColumn:
            text = data.isActive ? "OK" : "KO";
            g.setColour(data.isActive ? Colours::green : Colours::red);
            break;
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
        PopupMenu p;
        p.addItem(1, "Edit this cue");
        p.addItem(2, "Delete this cue");
        p.addSeparator();
        p.addItem(3, "Add new cue after");
        p.addItem(4, "Add new cue before");

        p.showMenuAsync(PopupMenu::Options(), [this, rowNumber](int result) {
            if (result == 1) {
                // Edit action
                if (rowNumber < cl->cues.items.size()) {
                    inspectCue(rowNumber);
                }
            } else if (result == 2) {
                // Delete action
                if (rowNumber < cl->cues.items.size()) {
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

void CuesTableModel::generateTestData()
{
    cueData.clear();

    for (auto& cue : cl->cues.items) {
        CueData cd;
        cd.name = cue->niceName;
        cd.time = "00:01:30";
        cd.description = cue->description->getValue();
        cd.isActive = true;

        cueData.add(cd);
    }

    // cueData.add({"Cue 1", "00:01:30", "Introduction", true});
    // cueData.add({"Cue 2", "00:02:45", "Verse 1", false});
    // cueData.add({"Cue 3", "00:04:12", "Chorus", true});
    // cueData.add({"Cue 4", "00:05:30", "Verse 2", false});
    // cueData.add({"Cue 5", "00:07:00", "Bridge", true});
    // cueData.add({"Cue 6", "00:08:15", "Final Chorus", false});
    // cueData.add({"Cue 7", "00:09:45", "Outro", true});
    // cueData.add({"Cue 8", "00:11:00", "Applause", false});
}
