/*
  ==============================================================================

    PluginSlot.h
    Created: 18 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../MainIncludes.h"

class PluginEditorWindow;

class PluginSlot :
    public BaseItem
{
public:
    PluginSlot(var params = var());
    virtual ~PluginSlot();

    PluginDescription pluginDescription;
    std::unique_ptr<AudioPluginInstance> pluginInstance;

    Trigger* selectPluginBtn;
    Trigger* editBtn;

    void loadPlugin(const PluginDescription& desc);
    void loadPluginDeferred();
    void unloadPlugin();
    bool hasPendingLoad() const { return pendingDescription != nullptr; }

    void prepareToPlay(double sampleRate, int blockSize, int numChannels);
    void releaseResources();
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi);

    void showPluginEditor();
    void closePluginEditor();

    String getTypeString() const override { return "PluginSlot"; }
    static PluginSlot* create(var params) { return new PluginSlot(params); }

    void triggerTriggered(Trigger* t) override;

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables) override;

    var getJSONData(bool includeNonOverriden = false) override;
    void loadJSONDataInternal(var data) override;

    String getPendingPluginName() const { return pendingDescription != nullptr ? pendingDescription->name : String(); }

private:
    std::unique_ptr<PluginEditorWindow> editorWindow;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    int currentNumChannels = 2;

    std::unique_ptr<PluginDescription> pendingDescription;
    MemoryBlock pendingState;
    MemoryBlock lastKnownState;

    friend class PluginLoadThread;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginSlot)
};

//==============================================================================

class PluginEditorWindow : public DocumentWindow
{
public:
    PluginEditorWindow(PluginSlot* slot, AudioProcessorEditor* editor);
    ~PluginEditorWindow();

    void closeButtonPressed() override;

private:
    PluginSlot* slot;
};

//==============================================================================

class PluginChainManager :
    public BaseManager<PluginSlot>
{
public:
    PluginChainManager();
    virtual ~PluginChainManager();

    PluginSlot* createItem() override;
    bool hasPendingLoads() const;
    int getNumPendingLoads() const;
    bool hasPluginWarnings() const;
    String getPluginWarningMessage() const;
};

class PluginLoaderWindow;

class PluginLoader : public Timer
{
public:
    PluginLoader(Array<PluginSlot*> slots);
    ~PluginLoader();

    void timerCallback() override;

    Array<PluginSlot*> pendingSlots;
    int currentIndex = 0;
    int totalToLoad = 0;
    bool cancelled = false;

    std::unique_ptr<PluginLoaderWindow> window;
};

class PluginLoaderWindow : public DocumentWindow
{
public:
    PluginLoaderWindow(PluginLoader* loader);
    ~PluginLoaderWindow();

    void closeButtonPressed() override;
    void updateStatus(const String& pluginName, int loaded, int total);

private:
    PluginLoader* loader;
    Label statusLabel;
    ProgressBar progressBar;
    double progress = 0.0;
};
