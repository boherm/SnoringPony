/*
  ==============================================================================

    PluginScanner.cpp
    Created: 18 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "PluginScanner.h"

//==============================================================================
// PluginSearchPaths
//==============================================================================

PluginSearchPaths::PluginSearchPaths() :
    ControllableContainer("Search Paths")
{
    saveAndLoadRecursiveData = true;
    userCanAddControllables = true;
    userAddControllablesFilters.add(FileParameter::getTypeStringStatic());
    customUserCreateControllableFunc = &PluginSearchPaths::createItem;
}

void PluginSearchPaths::createItem(ControllableContainer* cc)
{
    FileParameter* fp = new FileParameter("Search Path", "Plugin search directory", "");
    fp->directoryMode = true;
    fp->userCanChangeName = true;
    fp->isRemovableByUser = true;
    fp->saveValueOnly = false;
    cc->addControllable(fp);
}

void PluginSearchPaths::onControllableAdded(Controllable* c)
{
    if (auto* fp = dynamic_cast<FileParameter*>(c))
    {
        fp->userCanChangeName = true;
        fp->directoryMode = true;
    }
}

FileSearchPath PluginSearchPaths::getSearchPaths() const
{
    FileSearchPath paths;

    for (auto* c : controllables)
    {
        if (auto* fp = dynamic_cast<FileParameter*>(c))
        {
            String val = fp->stringValue();
            if (val.isEmpty()) continue;

            File f = fp->getFile();
            if (!f.isDirectory())
                f = File(val);

            if (f.isDirectory())
                paths.add(f);
        }
    }
    return paths;
}

//==============================================================================
// PluginScanner
//==============================================================================

juce_ImplementSingleton(PluginScanner)

PluginScanner::PluginScanner() :
    ControllableContainer("Plugin Scanner")
{
    formatManager.addDefaultFormats();

    saveAndLoadRecursiveData = true;

    scanBtn = addTrigger("Scan Plugins", "Scan for VST3/AU plugins in the search paths");
    addChildControllableContainer(&searchPaths);

    loadPluginList();
}

PluginScanner::~PluginScanner()
{
    stopTimer();
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
    if (isTimerRunning())
        return;

    knownPluginList.clear();
    pluginsFound = 0;
    currentPluginBeingScanned = "Starting...";
    cancelled = false;
    currentFormatIndex = 0;
    activeScanner.reset();

    scanWindow = std::make_unique<PluginScanWindow>(this);
    startTimer(50);
}

void PluginScanner::timerCallback()
{
    if (cancelled)
    {
        stopTimer();
        activeScanner.reset();
        savePluginList();
        scanWindow.reset();
        NLOG("Plugin Scanner", "Scan cancelled. " + String(knownPluginList.getNumTypes()) + " plugins found.");
        return;
    }

    auto formats = formatManager.getFormats();

    // Need a new scanner for the next format?
    if (activeScanner == nullptr)
    {
        if (currentFormatIndex >= formats.size())
        {
            // All formats done
            stopTimer();
            activeScanner.reset();
            savePluginList();
            pluginsFound = knownPluginList.getNumTypes();

            scanWindow.reset();
            NLOG("Plugin Scanner", String(pluginsFound) + " plugins found.");
            return;
        }

        auto* format = formats[currentFormatIndex];

        FileSearchPath paths = searchPaths.getSearchPaths();
        auto defaultLocations = format->getDefaultLocationsToSearch();
        for (int i = 0; i < defaultLocations.getNumPaths(); i++)
            paths.addIfNotAlreadyThere(defaultLocations[i]);

        activeScanner = std::make_unique<PluginDirectoryScanner>(
            knownPluginList, *format, paths, true, File());
    }

    // Scan one plugin
    String pluginName;
    if (activeScanner->scanNextFile(true, pluginName))
    {
        currentPluginBeingScanned = pluginName;
        pluginsFound = knownPluginList.getNumTypes();

        if (scanWindow != nullptr)
            scanWindow->updateStatus(pluginName, pluginsFound);
    }
    else
    {
        // This format is done, move to the next
        activeScanner.reset();
        currentFormatIndex++;
    }
}

void PluginScanner::showPluginMenu(std::function<void(PluginDescription)> callback)
{
    if (knownPluginList.getNumTypes() == 0 && !isTimerRunning())
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
}

PluginScanWindow::~PluginScanWindow()
{
}

void PluginScanWindow::closeButtonPressed()
{
    scanner->cancelled = true;
}

void PluginScanWindow::updateStatus(const String& pluginName, int found)
{
    statusLabel.setText("Scanning: " + pluginName + " (" + String(found) + " found)", dontSendNotification);
    progress = -1.0;
}
