/*
  ==============================================================================

    PluginScanner.h
    Created: 18 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class PluginSearchPaths :
    public ControllableContainer
{
public:
    PluginSearchPaths();
    ~PluginSearchPaths() {}

    static void createItem(ControllableContainer* cc);
    void onControllableAdded(Controllable* c) override;

    FileSearchPath getSearchPaths() const;
};

//==============================================================================

class PluginScanWindow;

class PluginScanWindow;

class PluginScanner :
    public ControllableContainer,
    public Timer
{
public:
    juce_DeclareSingleton(PluginScanner, true)

    PluginScanner();
    ~PluginScanner();

    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;

    PluginSearchPaths searchPaths;
    Trigger* scanBtn;

    void startScan();
    void timerCallback() override;

    void savePluginList();
    void loadPluginList();

    File getPluginListFile() const;

    void showPluginMenu(std::function<void(PluginDescription)> callback);

    void onContainerTriggerTriggered(Trigger* t) override;

    std::unique_ptr<PluginScanWindow> scanWindow;
    String currentPluginBeingScanned;
    int pluginsFound = 0;
    bool cancelled = false;

private:
    std::unique_ptr<PluginDirectoryScanner> activeScanner;
    int currentFormatIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginScanner)
};

class PluginScanWindow : public DocumentWindow
{
public:
    PluginScanWindow(PluginScanner* scanner);
    ~PluginScanWindow();

    void closeButtonPressed() override;
    void updateStatus(const String& pluginName, int found);

private:
    PluginScanner* scanner;
    Label statusLabel;
    ProgressBar progressBar;
    double progress = 0.0;
};
