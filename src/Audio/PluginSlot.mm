/*
  ==============================================================================

    PluginSlot.cpp
    Created: 18 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "PluginSlot.h"
#include "PluginScanner.h"
#include "ui/PluginSlotEditor.h"

//==============================================================================
// PluginSlot
//==============================================================================

PluginSlot::PluginSlot(var params) :
    BaseItem("Empty Slot")
{
    canBeDisabled = true;
    userCanRemove = true;

    editorCanBeCollapsed = false;
    showWarningInUI = true;

    selectPluginBtn = addTrigger("Select Plugin", "Choose a VST/AU plugin");
    selectPluginBtn->hideInEditor = true;
    editBtn = addTrigger("Edit", "Open plugin editor window");
    editBtn->hideInEditor = true;
}

PluginSlot::~PluginSlot()
{
    closePluginEditor();
    unloadPlugin();
}

void PluginSlot::loadPlugin(const PluginDescription& desc)
{
    closePluginEditor();

    // Save current state before unloading
    String previousName = pluginDescription.name;
    if (pluginInstance != nullptr)
        pluginInstance->getStateInformation(lastKnownState);

    unloadPlugin();
    pendingDescription.reset();
    pendingState = MemoryBlock();

    pluginDescription = desc;

    @try
    {
        String errorMessage;
        pluginInstance = PluginScanner::getInstance()->formatManager.createPluginInstance(
            desc, currentSampleRate, currentBlockSize, errorMessage);

        if (pluginInstance != nullptr)
        {
            setNiceName(desc.name);
            clearWarning();

            auto layout = pluginInstance->getBusesLayout();
            auto channelSet = AudioChannelSet::canonicalChannelSet(currentNumChannels);

            if (layout.getMainInputChannels() != currentNumChannels && pluginInstance->getBus(true, 0) != nullptr)
                layout.getChannelSet(true, 0) = channelSet;
            if (layout.getMainOutputChannels() != currentNumChannels && pluginInstance->getBus(false, 0) != nullptr)
                layout.getChannelSet(false, 0) = channelSet;

            if (!pluginInstance->setBusesLayout(layout))
                NLOGWARNING(niceName, "Could not set bus layout for: " + desc.name);

            pluginInstance->prepareToPlay(currentSampleRate, currentBlockSize);

            // Restore state if replacing with same plugin name
            if (previousName == desc.name && lastKnownState.getSize() > 0)
            {
                pluginInstance->setStateInformation(lastKnownState.getData(), (int)lastKnownState.getSize());
                lastKnownState = MemoryBlock();
            }
        }
        else
        {
            NLOGWARNING(niceName, "Failed to load plugin: " + errorMessage);
            setNiceName("Error: " + desc.name);
            setWarningMessage("Plugin not found: " + desc.name);
            // lastKnownState is preserved for next attempt
        }
    }
    @catch (...)
    {
        NLOGWARNING(niceName, "Plugin crashed during load: " + desc.name);
        pluginInstance.reset();
        setNiceName("Crashed: " + desc.name);
        setWarningMessage("Plugin crashed during load: " + desc.name);
        // lastKnownState is preserved for next attempt
    }
}

void PluginSlot::unloadPlugin()
{
    if (pluginInstance != nullptr)
    {
        pluginInstance->releaseResources();
        pluginInstance.reset();
    }
}

void PluginSlot::prepareToPlay(double sampleRate, int blockSize, int numChannels)
{
    currentSampleRate = sampleRate;
    currentBlockSize = blockSize;
    currentNumChannels = numChannels;

    if (pluginInstance != nullptr)
    {
        auto layout = pluginInstance->getBusesLayout();
        auto channelSet = AudioChannelSet::canonicalChannelSet(numChannels);

        if (layout.getMainInputChannels() != numChannels && pluginInstance->getBus(true, 0) != nullptr)
            layout.getChannelSet(true, 0) = channelSet;
        if (layout.getMainOutputChannels() != numChannels && pluginInstance->getBus(false, 0) != nullptr)
            layout.getChannelSet(false, 0) = channelSet;

        pluginInstance->setBusesLayout(layout);
        pluginInstance->prepareToPlay(sampleRate, blockSize);
    }
}

void PluginSlot::releaseResources()
{
    if (pluginInstance != nullptr)
        pluginInstance->releaseResources();
}

void PluginSlot::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    if (pluginInstance == nullptr || !enabled->boolValue())
        return;

    int pluginChannels = jmax(pluginInstance->getTotalNumInputChannels(),
                              pluginInstance->getTotalNumOutputChannels());

    if (pluginChannels <= 0)
        return;

    @try
    {
        if (buffer.getNumChannels() <= pluginChannels)
        {
            pluginInstance->processBlock(buffer, midi);
        }
        else
        {
            float* channelPtrs[64];
            int numCh = jmin(pluginChannels, buffer.getNumChannels(), 64);
            for (int ch = 0; ch < numCh; ++ch)
                channelPtrs[ch] = buffer.getWritePointer(ch);

            AudioBuffer<float> subBuffer(channelPtrs, numCh, buffer.getNumSamples());
            pluginInstance->processBlock(subBuffer, midi);
        }
    }
    @catch (...)
    {
        enabled->setValue(false);
        NLOGWARNING(niceName, "Plugin crashed during processing, disabled: " + pluginDescription.name);
    }
}

void PluginSlot::showPluginEditor()
{
    if (pluginInstance == nullptr)
        return;

    if (editorWindow != nullptr)
    {
        editorWindow->toFront(true);
        return;
    }

    @try
    {
        if (auto* editor = pluginInstance->createEditorIfNeeded())
        {
            editorWindow = std::make_unique<PluginEditorWindow>(this, editor);
        }
    }
    @catch (...)
    {
        NLOGWARNING(niceName, "Plugin editor crashed: " + pluginDescription.name);
    }
}

void PluginSlot::closePluginEditor()
{
    editorWindow.reset();
}

InspectableEditor* PluginSlot::getEditorInternal(bool isRoot, Array<Inspectable*> /*inspectables*/)
{
    return new PluginSlotEditor(this, isRoot);
}

void PluginSlot::triggerTriggered(Trigger* t)
{
    BaseItem::triggerTriggered(t);

    if (t == selectPluginBtn)
    {
        PluginScanner::getInstance()->showPluginMenu([this](PluginDescription desc)
        {
            loadPlugin(desc);
        });
    }
    else if (t == editBtn)
    {
        showPluginEditor();
    }
}

var PluginSlot::getJSONData(bool includeNonOverriden)
{
    var data = BaseItem::getJSONData(includeNonOverriden);

    if (pluginInstance != nullptr)
    {
        auto descXml = pluginDescription.createXml();
        if (descXml != nullptr)
            data.getDynamicObject()->setProperty("pluginDescription", descXml->toString());

        MemoryBlock stateBlock;
        pluginInstance->getStateInformation(stateBlock);
        data.getDynamicObject()->setProperty("pluginState", stateBlock.toBase64Encoding());
    }

    return data;
}

void PluginSlot::loadJSONDataInternal(var data)
{
    BaseItem::loadJSONDataInternal(data);

    String descXmlString = data.getProperty("pluginDescription", "");
    if (descXmlString.isNotEmpty())
    {
        auto descXml = parseXML(descXmlString);
        if (descXml != nullptr)
        {
            PluginDescription desc;
            if (desc.loadFromXml(*descXml))
            {
                // Defer plugin instantiation to avoid blocking the file load
                pendingDescription = std::make_unique<PluginDescription>(desc);
                pendingState = MemoryBlock();

                String stateBase64 = data.getProperty("pluginState", "");
                if (stateBase64.isNotEmpty())
                    pendingState.fromBase64Encoding(stateBase64);

                setNiceName(desc.name + " (loading...)");
            }
        }
    }
}

void PluginSlot::loadPluginDeferred()
{
    if (pendingDescription == nullptr)
        return;

    PluginDescription desc = *pendingDescription;
    MemoryBlock state = pendingState;

    // Store as lastKnownState so loadPlugin can find it if it fails
    lastKnownState = state;

    pendingDescription.reset();
    pendingState = MemoryBlock();

    loadPlugin(desc);

    // If loadPlugin succeeded and didn't restore (different name case), apply state here
    if (pluginInstance != nullptr && state.getSize() > 0 && lastKnownState.getSize() > 0)
    {
        pluginInstance->setStateInformation(state.getData(), (int)state.getSize());
        lastKnownState = MemoryBlock();
    }
}

//==============================================================================
// PluginEditorWindow
//==============================================================================

PluginEditorWindow::PluginEditorWindow(PluginSlot* slot, AudioProcessorEditor* editor) :
    DocumentWindow(slot->niceName, Colours::darkgrey, DocumentWindow::closeButton),
    slot(slot)
{
    setContentOwned(editor, true);
    setResizable(true, false);
    setUsingNativeTitleBar(true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

PluginEditorWindow::~PluginEditorWindow()
{
    clearContentComponent();
}

void PluginEditorWindow::closeButtonPressed()
{
    slot->closePluginEditor();
}

//==============================================================================
// PluginChainManager
//==============================================================================

PluginChainManager::PluginChainManager() :
    BaseManager("Plugin Chain")
{
    selectItemWhenCreated = false;
}

PluginChainManager::~PluginChainManager()
{
}

PluginSlot* PluginChainManager::createItem()
{
    return new PluginSlot();
}

bool PluginChainManager::hasPendingLoads() const
{
    for (auto* slot : items)
        if (slot->hasPendingLoad()) return true;
    return false;
}

int PluginChainManager::getNumPendingLoads() const
{
    int count = 0;
    for (auto* slot : items)
        if (slot->hasPendingLoad()) count++;
    return count;
}

bool PluginChainManager::hasPluginWarnings() const
{
    for (auto* slot : items)
        if (slot->getWarningMessage().isNotEmpty()) return true;
    return false;
}

String PluginChainManager::getPluginWarningMessage() const
{
    StringArray warnings;
    for (auto* slot : items)
    {
        String w = slot->getWarningMessage();
        if (w.isNotEmpty())
            warnings.add(w);
    }
    return warnings.joinIntoString("\n");
}

//==============================================================================
// PluginLoader (Timer-based, runs on message thread)
//==============================================================================

PluginLoader::PluginLoader(Array<PluginSlot*> slots) :
    pendingSlots(slots),
    totalToLoad(slots.size())
{
    window = std::make_unique<PluginLoaderWindow>(this);

    if (!pendingSlots.isEmpty())
        window->updateStatus(pendingSlots[0]->getPendingPluginName(), 0, totalToLoad);

    startTimer(50);
}

PluginLoader::~PluginLoader()
{
    stopTimer();
}

void PluginLoader::timerCallback()
{
    if (cancelled || currentIndex >= pendingSlots.size())
    {
        stopTimer();
        int loaded = currentIndex;
        int total = totalToLoad;

        // Must destroy window before self - use callAsync since we're in our own timer
        window.reset();
        NLOG("Plugin Loader", String(loaded) + "/" + String(total) + " plugins loaded.");
        return;
    }

    auto* slot = pendingSlots[currentIndex];

    if (window != nullptr)
        window->updateStatus(slot->getPendingPluginName(), currentIndex, totalToLoad);

    slot->loadPluginDeferred();
    currentIndex++;

    if (window != nullptr)
        window->updateStatus(
            currentIndex < pendingSlots.size() ? pendingSlots[currentIndex]->getPendingPluginName() : "Done",
            currentIndex, totalToLoad);
}

//==============================================================================
// PluginLoaderWindow
//==============================================================================

PluginLoaderWindow::PluginLoaderWindow(PluginLoader* loader) :
    DocumentWindow("Loading Plugins...", Colours::darkgrey, DocumentWindow::closeButton),
    loader(loader),
    progressBar(progress)
{
    setSize(400, 100);
    setUsingNativeTitleBar(true);
    setResizable(false, false);

    auto* content = new Component();
    content->setSize(400, 100);

    statusLabel.setJustificationType(Justification::centredLeft);
    statusLabel.setText("Loading plugins...", dontSendNotification);
    statusLabel.setBounds(10, 10, 380, 24);
    content->addAndMakeVisible(statusLabel);

    progressBar.setBounds(10, 44, 380, 24);
    content->addAndMakeVisible(progressBar);

    setContentOwned(content, true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
    setAlwaysOnTop(true);
}

PluginLoaderWindow::~PluginLoaderWindow()
{
}

void PluginLoaderWindow::closeButtonPressed()
{
    loader->cancelled = true;
}

void PluginLoaderWindow::updateStatus(const String& pluginName, int loaded, int total)
{
    if (total > 0)
        progress = (double)loaded / (double)total;

    statusLabel.setText("Loading: " + pluginName + " (" + String(loaded) + "/" + String(total) + ")", dontSendNotification);
}
