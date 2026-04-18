/*
  ==============================================================================

    PluginScanner.h
    Created: 18 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class PluginSearchPath :
    public BaseItem
{
public:
    PluginSearchPath(var params = var());
    virtual ~PluginSearchPath() {}

    FileParameter* path;

    String getTypeString() const override { return "PluginSearchPath"; }
    static PluginSearchPath* create(var params) { return new PluginSearchPath(params); }
};

class PluginSearchPathManager :
    public BaseManager<PluginSearchPath>
{
public:
    PluginSearchPathManager();
    virtual ~PluginSearchPathManager() {}

    PluginSearchPath* createItem() override;
    FileSearchPath getSearchPaths() const;
};

//==============================================================================

class PluginScanWindow;

class PluginScanner :
    public ControllableContainer,
    public Thread
{
public:
    juce_DeclareSingleton(PluginScanner, true)

    PluginScanner();
    ~PluginScanner();

    AudioPluginFormatManager formatManager;
    KnownPluginList knownPluginList;

    PluginSearchPathManager searchPaths;
    Trigger* scanBtn;

    void startScan();
    void run() override;

    void savePluginList();
    void loadPluginList();

    File getPluginListFile() const;

    void showPluginMenu(std::function<void(PluginDescription)> callback);

    void onContainerTriggerTriggered(Trigger* t) override;

    std::unique_ptr<PluginScanWindow> scanWindow;
    String currentPluginBeingScanned;
    int pluginsFound = 0;

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginScanner)
};

class PluginScanWindow : public DocumentWindow, public Timer
{
public:
    PluginScanWindow(PluginScanner* scanner);
    ~PluginScanWindow();

    void closeButtonPressed() override;
    void timerCallback() override;

private:
    PluginScanner* scanner;
    Label statusLabel;
    ProgressBar progressBar;
    double progress = 0.0;
};
