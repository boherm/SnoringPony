/*
  ==============================================================================

    PluginScanner.cpp
    Created: 18 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "PluginScanner.h"

//==============================================================================
// PluginSearchPath
//==============================================================================

PluginSearchPath::PluginSearchPath(var params) :
    BaseItem("Search Path")
{
    userCanRemove = true;
    path = addFileParameter("Path", "Plugin search directory");
    path->directoryMode = true;
}

//==============================================================================
// PluginSearchPathManager
//==============================================================================

PluginSearchPathManager::PluginSearchPathManager() :
    BaseManager("Search Paths")
{
    selectItemWhenCreated = false;
}

PluginSearchPath* PluginSearchPathManager::createItem()
{
    return new PluginSearchPath();
}

FileSearchPath PluginSearchPathManager::getSearchPaths() const
{
    FileSearchPath paths;

    for (auto* item : items)
    {
        File f = item->path->getFile();
        if (f.isDirectory())
            paths.add(f);
    }

    return paths;
}

//==============================================================================
// PluginScanner
//==============================================================================

juce_ImplementSingleton(PluginScanner)

PluginScanner::PluginScanner() :
    ControllableContainer("Plugin Scanner"),
    Thread("Plugin Scanner Thread")
{
    formatManager.addDefaultFormats();

    saveAndLoadRecursiveData = true;

    scanBtn = addTrigger("Scan Plugins", "Scan for VST3/AU plugins in the search paths");
    addChildControllableContainer(&searchPaths);

    loadPluginList();
}

PluginScanner::~PluginScanner()
{
    stopThread(5000);
    savePluginList();
    clearSingletonInstance();
}

File PluginScanner::getPluginListFile() const
{
    return File::getSpecialLocation(File::userDocumentsDirectory)
        .getChildFile("SnoringPony")
        .getChildFile("pluginList.xml");
}

void PluginScanner::loadPluginList()
{
    File listFile = getPluginListFile();
    if (listFile.existsAsFile())
    {
        auto xml = parseXML(listFile);
        if (xml != nullptr)
            knownPluginList.recreateFromXml(*xml);
    }
}

void PluginScanner::savePluginList()
{
    File listFile = getPluginListFile();
    listFile.getParentDirectory().createDirectory();

    auto xml = knownPluginList.createXml();
    if (xml != nullptr)
        xml->writeTo(listFile);
}

void PluginScanner::startScan()
{
    if (isThreadRunning())
        return;

    pluginsFound = 0;
    currentPluginBeingScanned = "Starting...";

    MessageManager::callAsync([this]()
    {
        scanWindow = std::make_unique<PluginScanWindow>(this);
    });

    startThread();
}

void PluginScanner::run()
{
    knownPluginList.clear();

    for (auto* format : formatManager.getFormats())
    {
        if (threadShouldExit())
            break;

        FileSearchPath paths = searchPaths.getSearchPaths();

        auto defaultLocations = format->getDefaultLocationsToSearch();
        for (int i = 0; i < defaultLocations.getNumPaths(); i++)
            paths.addIfNotAlreadyThere(defaultLocations[i]);

        PluginDirectoryScanner scanner(
            knownPluginList,
            *format,
            paths,
            true,
            File()
        );

        String pluginName;
        while (scanner.scanNextFile(true, pluginName))
        {
            if (threadShouldExit())
                break;

            currentPluginBeingScanned = pluginName;
            pluginsFound = knownPluginList.getNumTypes();
        }
    }

    savePluginList();
    pluginsFound = knownPluginList.getNumTypes();
    currentPluginBeingScanned = "";

    MessageManager::callAsync([this]()
    {
        scanWindow.reset();
        NLOG("Plugin Scanner", String(knownPluginList.getNumTypes()) + " plugins found.");
    });
}

void PluginScanner::showPluginMenu(std::function<void(PluginDescription)> callback)
{
    if (knownPluginList.getNumTypes() == 0 && !isThreadRunning())
    {
        startScan();
        return;
    }

    PopupMenu menu;

    auto types = knownPluginList.getTypes();
    knownPluginList.addToMenu(menu, types, KnownPluginList::sortByManufacturer);

    int rescanId = 100000;
    menu.addSeparator();
    menu.addItem(rescanId, "Rescan plugins...");

    menu.showMenuAsync(PopupMenu::Options(), [this, callback, types, rescanId](int result)
    {
        if (result == rescanId)
        {
            startScan();
            return;
        }

        if (result > 0)
        {
            int index = knownPluginList.getIndexChosenByMenu(types, result);
            if (index >= 0 && index < types.size())
                callback(types[index]);
        }
    });
}

void PluginScanner::onContainerTriggerTriggered(Trigger* t)
{
    ControllableContainer::onContainerTriggerTriggered(t);

    if (t == scanBtn)
    {
        startScan();
    }
}

//==============================================================================
// PluginScanWindow
//==============================================================================

PluginScanWindow::PluginScanWindow(PluginScanner* scanner) :
    DocumentWindow("Scanning Plugins...", Colours::darkgrey, DocumentWindow::closeButton),
    scanner(scanner),
    progressBar(progress)
{
    setSize(400, 100);
    setUsingNativeTitleBar(true);
    setResizable(false, false);

    auto* content = new Component();
    content->setSize(400, 100);

    statusLabel.setJustificationType(Justification::centredLeft);
    statusLabel.setText("Starting scan...", dontSendNotification);
    statusLabel.setBounds(10, 10, 380, 24);
    content->addAndMakeVisible(statusLabel);

    progressBar.setBounds(10, 44, 380, 24);
    content->addAndMakeVisible(progressBar);

    setContentOwned(content, true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
    setAlwaysOnTop(true);

    startTimerHz(10);
}

PluginScanWindow::~PluginScanWindow()
{
    stopTimer();
}

void PluginScanWindow::closeButtonPressed()
{
    scanner->signalThreadShouldExit();
}

void PluginScanWindow::timerCallback()
{
    String pluginName = scanner->currentPluginBeingScanned;
    int found = scanner->pluginsFound;

    if (pluginName.isNotEmpty())
        statusLabel.setText("Scanning: " + pluginName + " (" + String(found) + " found)", dontSendNotification);
    else
        statusLabel.setText("Finishing... (" + String(found) + " found)", dontSendNotification);

    progress = -1.0; // indeterminate
}
