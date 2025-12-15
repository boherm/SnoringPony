/*
  ==============================================================================

    Cue.cpp
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#include "Cue.h"
#include "ui/CueEditor.h"
#include "../Cuelist/Cuelist.h"
#include "../Cue/CueManager.h"

Cue::CueTimer::CueTimer(Cue* cue)
{
    parentCue = cue;
}

Cue::CueTimer::~CueTimer()
{
}

void Cue::CueTimer::start(float duration, FloatParameter* currentTimeParam)
{
    this->duration = duration;
    this->currentTimeParam = currentTimeParam;
    this->currentTimeParam->setValue(0.0f);
    startTimer(20);
}

void Cue::CueTimer::stop()
{
    stopTimer();
    this->duration = 0.0f;
    this->currentTimeParam = nullptr;
}

void Cue::CueTimer::notify()
{
    const MessageManagerLock mmLock;
    if (parentCue != nullptr)
        parentCue->onCueTimerFinished(this);
}

void Cue::CueTimer::timerCallback()
{
    if (currentTimeParam != nullptr) {
        float currentTime = currentTimeParam->floatValue();
        currentTime += 0.02f;
        if (currentTime >= duration) {
            currentTimeParam->setValue(currentTime);
            currentTime = duration;
            notify();
            stop();
        } else {
            currentTimeParam->setValue(currentTime);
        }
    }
}

// -------------------------------------------------------

Cue::Cue(var params) :
    BaseItem(params.getProperty("name", "1.000")),
    objectType(params.getProperty("type", "Cue").toString()),
	objectData(params)

{
    saveAndLoadRecursiveData = true;
    nameCanBeChangedByUser = false;
	canBeDisabled = false;
	editorIsCollapsed = false;
	itemDataType = "Cue";
    setHasCustomColor(true);
    itemColor->hideInEditor = false;
	itemColor->setDefaultValue(BG_COLOR);
    itemColor->setControlMode(Parameter::ControlMode::REFERENCE);

    isPlaying = addBoolParameter("Is Playing", "Indicates if this cue is currently playing", false, false);
    isPlaying->isSavable = false;

    setNextBtn = addTrigger("Set next", "Trigger this cue after next");
    setNextBtn->hideInEditor = true;

    id = addFloatParameter("ID", "ID of this cue", params.getProperty("id", 1.0));
    id->lockManualControlMode = true;

    currentTime = addFloatParameter("Current time", "Current time of the cue", 0.0f);
    currentTime->isSavable = false;
    currentTime->lockManualControlMode = true;
currentTime->hideInRemoteControl = true;
    currentTime->defaultUI = FloatParameter::TIME;
    currentTime->setEnabled(false);

    duration = addFloatParameter("Duration", "Duration of the cue", params.getProperty("duration", 0.0), 0.0);
    duration->lockManualControlMode = true;
    duration->defaultUI = FloatParameter::TIME;

    description = addStringParameter("Description", "Description of the cue", params.getProperty("description", ""));
    description->lockManualControlMode = true;

	notes = addStringParameter("Notes", "Notes of the cue", params.getProperty("notes", ""));
    notes->lockManualControlMode = true;
    notes->multiline = true;

    // --- Pre-wait ----
    preWaitTimer = new Cue::CueTimer(this);
    preWaitCC = new EnablingControllableContainer("Pre-wait");
    preWaitCC->enabled->setValue(false);
    preWaitCC->editorIsCollapsed = true;
    addChildControllableContainer(preWaitCC, true);

    preWaitDuration = preWaitCC->addFloatParameter("Waiting time", "Time to wait before starting the cue", 0.0, 0.0);
    preWaitDuration->lockManualControlMode = true;
    preWaitDuration->defaultUI = FloatParameter::TIME;

    preWaitCurrentTime = preWaitCC->addFloatParameter("Current time", "Current time of the pre-wait", 0.0, 0.0);
    preWaitCurrentTime->defaultUI = FloatParameter::TIME;
    preWaitCurrentTime->setEnabled(false);

    preWaitActive = preWaitCC->addBoolParameter("Active", "Pre-wait currently active", false);
    preWaitActive->setEnabled(false);
    // preWaitActive->hideInEditor = true;

    // --- Auto-follow ----
    autoFollowTimer = new Cue::CueTimer(this);
    autoFollowCC = new EnablingControllableContainer("Auto-follow");
    autoFollowCC->enabled->setValue(false);
    autoFollowCC->editorIsCollapsed = true;
    addChildControllableContainer(autoFollowCC, true);

    autoFollowType = autoFollowCC->addEnumParameter("Type", "Type of auto-follow behavior", "Immediate");
    autoFollowType->addOption("Immediate", AutoFollowType::IMMEDIATE);
    autoFollowType->addOption("After Pre-wait", AutoFollowType::AFTER_PRE);
    autoFollowType->addOption("After Cue", AutoFollowType::AFTER_CUE);

    autoFollowDuration = autoFollowCC->addFloatParameter("Waiting time", "Duration before auto-follow triggers the next cue", 0.0, 0.0);
    autoFollowDuration->defaultUI = FloatParameter::TIME;

    autoFollowCurrentTime = autoFollowCC->addFloatParameter("Current time", "Current time of the auto-follow", 0.0);
    autoFollowCurrentTime->setEnabled(false);
    autoFollowCurrentTime->defaultUI = FloatParameter::TIME;

    autoFollowActive = autoFollowCC->addBoolParameter("Active", "Auto-follow currently active", false);
    autoFollowActive->setEnabled(false);
    // autoFollowActive->hideInEditor = true;
}

Cue::~Cue()
{
    preWaitTimer->stop();
    autoFollowTimer->stop();

    delete preWaitTimer;
    delete autoFollowTimer;
}

InspectableEditor* Cue::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
    return new CueEditor(this, isRoot);
}

void Cue::setGoNext()
{
    parentCuelist->nextCue->setTarget(this);
    parentCuelist->nextCue->notifyValueChanged();
}

void Cue::parameterValueChanged(Parameter* p)
{
    ControllableContainer::parameterValueChanged(p);

    if (p == id) {
        clearWarning();
        setNiceName(id->stringValue());
        setCustomShortName(id->stringValue());

        if (id->stringValue() != niceName) {
            setWarningMessage("A cue with this ID already exists!");
        }
    }
}

void Cue::parameterControlModeChanged(Parameter* p)
{
    if (p == itemColor && itemColor->controlMode == Parameter::ControlMode::REFERENCE)
    {
        itemColor->referenceTarget->setRootContainer(ProjectSettings::getInstance()->getControllableContainerByName("colorPresets"));
    }
}

void Cue::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
	BaseItem::onControllableFeedbackUpdateInternal(cc, c);

    if (Engine::mainEngine->isLoadingFile) {
        if (preWaitCC == cc && c == preWaitCC->enabled) {
            preWaitCC->editorIsCollapsed = !preWaitCC->enabled->boolValue();
        }
        if (autoFollowCC == cc && c == autoFollowCC->enabled) {
            autoFollowCC->editorIsCollapsed = !autoFollowCC->enabled->boolValue();
        }
    }
}

void Cue::triggerTriggered(Trigger* t)
{
    if (t == setNextBtn) {
        setGoNext();
    }
}

void Cue::onCueTimerFinished(Cue::CueTimer* timer)
{
    if (preWaitTimer == timer) {
        preWaitCurrentTime->setValue(0.0f);
        preWaitActive->setValue(false);
        autoFollowProcess(AutoFollowType::AFTER_PRE);
        playInternal();
    }
    if (autoFollowTimer == timer) {
        autoFollowCurrentTime->setValue(0.0f);
        autoFollowActive->setValue(false);
        playNextCue();
    }
}

void Cue::play()
{
    if (!canBePlayed())
        return;

    isPlaying->setValue(true);
    autoFollowProcess(AutoFollowType::IMMEDIATE);

    if (preWaitCC->enabled->boolValue()) {
        preWaitCurrentTime->setValue(0.0f);
        preWaitActive->setValue(true);
        preWaitTimer->start(preWaitDuration->floatValue(), preWaitCurrentTime);
    } else {
        playInternal();
    }
    setNextCue();
}

void Cue::panic()
{
    if (preWaitActive->boolValue()) {
        preWaitTimer->stop();
        preWaitCurrentTime->setValue(0.0f);
        preWaitActive->setValue(false);
        isPlaying->setValue(false);
    }

    if (autoFollowActive->boolValue()) {
        autoFollowTimer->stop();
        autoFollowCurrentTime->setValue(0.0f);
        autoFollowActive->setValue(false);
        isPlaying->setValue(false);
    }

    panicInternal();
}

void Cue::stop()
{
    if (preWaitActive->boolValue()) {
        preWaitTimer->stop();
        preWaitCurrentTime->setValue(0.0f);
        preWaitActive->setValue(false);
        isPlaying->setValue(false);
        return;
    }

    if (autoFollowActive->boolValue()) {
        autoFollowTimer->stop();
        autoFollowCurrentTime->setValue(0.0f);
        autoFollowActive->setValue(false);
        isPlaying->setValue(false);
        return;
    }

    stopInternal();
}

void Cue::endCue()
{
    isPlaying->setValue(false);
    autoFollowProcess(AutoFollowType::AFTER_CUE);
}

void Cue::playNextCue()
{
    auto idx = parentCuelist->cues->items.indexOf(this);
    if (idx + 1 < parentCuelist->cues->items.size())
        parentCuelist->cues->items[idx + 1]->play();
}

bool Cue::isAutoStartCue()
{
    auto idx = parentCuelist->cues->items.indexOf(this);
    if (idx - 1 < 0)
        return false;
    return parentCuelist->cues->items[idx - 1]->autoFollowCC->enabled->boolValue();
}

void Cue::setNextCue()
{
    auto idx = parentCuelist->cues->items.indexOf(this);

    for (int i = idx + 1; i < parentCuelist->cues->items.size(); i++) {
        Cue* c = parentCuelist->cues->items[i];
        if (!c->isAutoStartCue()) {
            parentCuelist->nextCue->setTarget(c);
            parentCuelist->nextCue->notifyValueChanged();
            return;
        }
    }
}

void Cue::autoFollowProcess(AutoFollowType type)
{
    if (!autoFollowCC->enabled->boolValue())
        return;

    if (
            type == autoFollowType->intValue() ||
            type == AutoFollowType::IMMEDIATE && autoFollowType->intValue() == AutoFollowType::AFTER_PRE && !preWaitCC->enabled->boolValue()
    ) {
        if (autoFollowDuration->floatValue() <= 0.0f) {
            playNextCue();
        } else {
            autoFollowCurrentTime->setValue(0.0f);
            autoFollowActive->setValue(true);
            autoFollowTimer->start(autoFollowDuration->floatValue(), autoFollowCurrentTime);
        }
    }
}
