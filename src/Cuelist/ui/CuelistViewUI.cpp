/*
  ==============================================================================

    CuelistViewUI.cpp
    Created: 19 Sep 2025 08:58:00am
    Author:  boherm

  ==============================================================================
*/

#include "CuelistViewUI.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_organicui/inspectable/ui/Inspector.h"
#include "juce_organicui/ui/Style.h"
#include <_stdlib.h>

// IDs des colonnes
enum ColumnIds
{
    NameColumn = 1,
    TimeColumn = 2,
    DescriptionColumn = 3,
    ActiveColumn = 4
};

CuelistViewUI::CuelistViewUI(const String &contentName) :
    ShapeShifterContent(new CuelistView(), contentName)
{
	InspectableSelectionManager::mainSelectionManager->addSelectionListener(this);
	// DashboardManager::getInstance()->addBaseManagerListener(this);
}

CuelistViewUI::~CuelistViewUI()
{
	if (InspectableSelectionManager::mainSelectionManager != nullptr) InspectableSelectionManager::mainSelectionManager->removeSelectionListener(this);
    delete contentComponent;
}

void CuelistViewUI::inspectablesSelectionChanged()
{
    Logger::writeToLog("CueListViewUI::inspectablesSelectionChanged");
    // Array<CueList*> cueLists = InspectableSelectionManager::activeSelectionManager->getInspectablesAs<CueList>();
    // if (cueLists.size() > 0)
    // {
    //     CueList * cueList = cueLists.getFirst();
    //     CueListView::getInstance()->fillText(cueList->niceName);
    //     String cueListName = "CL " + cueList->niceName;
    //     setCustomName(cueListName);
    // } else {
    //     CueListView::getInstance()->fillText("No cue list selected");
    //     setCustomName("No");
    // }
}

// Implémentation de CueListTableModel
CuelistTableModel::CuelistTableModel()
{
    generateTestData();
    LookAndFeel::getDefaultLookAndFeel().setColour(TableListBox::backgroundColourId, BG_COLOR);
}

CuelistTableModel::~CuelistTableModel()
{
}

int CuelistTableModel::getNumRows()
{
    return cueData.size();
}

void CuelistTableModel::paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.fillAll(BG_COLOR.brighter(0.2f));
    }
    else if (rowNumber % 2 == 0)
    {
        g.fillAll(BG_COLOR);
    }
    else
    {
        g.fillAll(BG_COLOR.brighter(0.1f));
    }
}

void CuelistTableModel::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
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

Component* CuelistTableModel::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate)
{
    return nullptr; // Pas de composants personnalisés pour l'instant
}

void CuelistTableModel::cellClicked(int rowNumber, int columnId, const MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        PopupMenu p;
        p.addItem(1, "Edit");
        p.addSeparator();
        p.addItem(2, "Delete");
        p.showMenuAsync(PopupMenu::Options());
        // Logger::writeToLog("CueListTableModel::cellClicked: popup menu");
        return;
    }

    Logger::writeToLog("CueListTableModel::cellClicked: rowNumber: " + String(rowNumber) + ", columnId: " + String(columnId));

    auto inspect =
        ShapeShifterManager::getInstance()->getContentForName("Inspector");
    if (inspect == nullptr)
      return;
    InspectorUI *inspectorUI =
        dynamic_cast<InspectorUI *>(inspect->contentComponent);
    // multi fixture edition disabled organic ui update
    Array<Inspectable *> newSel;
    if (CuelistManager::getInstance()->items.size() == 0) {
      return;
    }
    if (!newSel.contains(CuelistManager::getInstance()->items[0])) {
      newSel.insert(0, CuelistManager::getInstance()->items[0]);
		}
		inspectorUI->inspector->setCurrentInspectables(newSel);
}

int CuelistTableModel::getColumnAutoSizeWidth(int columnId)
{
    switch (columnId)
    {
        case NameColumn:
            return 120;
        case TimeColumn:
            return 80;
        case DescriptionColumn:
            return 200;
        case ActiveColumn:
            return 60;
        default:
            return 100;
    }
}

void CuelistTableModel::sortOrderChanged(int newSortColumnId, bool isForwards)
{
    // Implémentation du tri si nécessaire
}

void CuelistTableModel::generateTestData()
{
    cueData.clear();
    
    // Génération de données de test
    cueData.add({"Cue 1", "00:01:30", "Introduction", true});
    cueData.add({"Cue 2", "00:02:45", "Verse 1", false});
    cueData.add({"Cue 3", "00:04:12", "Chorus", true});
    cueData.add({"Cue 4", "00:05:30", "Verse 2", false});
    cueData.add({"Cue 5", "00:07:00", "Bridge", true});
    cueData.add({"Cue 6", "00:08:15", "Final Chorus", false});
    cueData.add({"Cue 7", "00:09:45", "Outro", true});
    cueData.add({"Cue 8", "00:11:00", "Applause", false});
}

// juce_ImplementSingleton(CueListView);

CuelistView::CuelistView()
{
    // Créer le modèle de données
    tableModel = std::make_unique<CuelistTableModel>();
    
    // Configurer le TableListBox
    tableListBox.setModel(tableModel.get());
    tableListBox.setRowHeight(25);
    int flags = TableHeaderComponent::visible | TableHeaderComponent::resizable | TableHeaderComponent::appearsOnColumnMenu;
    tableListBox.getHeader().addColumn("Nom", NameColumn, 120, 50, 200, flags & ~TableHeaderComponent::appearsOnColumnMenu);
    tableListBox.getHeader().addColumn("Temps", TimeColumn, 80, 50, 150, flags);
    tableListBox.getHeader().addColumn("Description", DescriptionColumn, 200, 100, 400, flags);
    tableListBox.getHeader().addColumn("Actif", ActiveColumn, 60, 50, 100, flags);
    
    addAndMakeVisible(tableListBox);
    
    fillText("No cue list selected");
    CuelistView::resized();
}

CuelistView::~CuelistView()
{
}

void CuelistView::paint (juce::Graphics& g)
{
    g.fillAll(Colours::aliceblue);
}

void CuelistView::resized()
{
    tableListBox.setBounds(getLocalBounds());
}

void CuelistView::fillText(String text)
{
    // Cette méthode peut être utilisée pour mettre à jour le titre ou d'autres informations
    // Pour l'instant, on peut l'utiliser pour régénérer les données de test
    if (tableModel)
    {
        tableModel->refreshData();
        tableListBox.updateContent();
    }
}