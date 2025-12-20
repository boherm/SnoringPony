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
    preWaitActive->hideInEditor = true;

    // --- Post-Wait ----
    postWaitTimer = new Cue::CueTimer(this);
    postWaitCC = new EnablingControllableContainer("Post-wait");
    postWaitCC->enabled->setValue(false);
    postWaitCC->editorIsCollapsed = true;
    addChildControllableContainer(postWaitCC, true);

    postWaitType = postWaitCC->addEnumParameter("Type", "Define when we start the Post-wait of this cue");
    postWaitType->addOption("Immediate", PostWaitType::IMMEDIATE);
    postWaitType->addOption("After Pre-wait", PostWaitType::AFTER_PRE);
    postWaitType->addOption("After Cue", PostWaitType::AFTER_CUE);

    postWaitDuration = postWaitCC->addFloatParameter("Waiting time", "Duration before auto-follow triggers the next cue", 0.0, 0.0);
    postWaitDuration->defaultUI = FloatParameter::TIME;

    postWaitCurrentTime = postWaitCC->addFloatParameter("Current time", "Current time of the auto-follow", 0.0);
    postWaitCurrentTime->setEnabled(false);
    postWaitCurrentTime->defaultUI = FloatParameter::TIME;

    postWaitActive = postWaitCC->addBoolParameter("Active", "Auto-follow currently active", false);
    postWaitActive->setEnabled(false);
    postWaitActive->hideInEditor = true;
}

Cue::~Cue()
{
    preWaitTimer->stop();
    postWaitTimer->stop();

    delete preWaitTimer;
    delete postWaitTimer;
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
        if (postWaitCC == cc && c == postWaitCC->enabled) {
            postWaitCC->editorIsCollapsed = !postWaitCC->enabled->boolValue();
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
        autoFollowProcess(PostWaitType::AFTER_PRE);
        playInternal();
    }
    if (postWaitTimer == timer) {
        postWaitCurrentTime->setValue(0.0f);
        postWaitActive->setValue(false);
        playNextCue();
    }
}

void Cue::play()
{
    if (!canBePlayed())
        return;

    isPlaying->setValue(true);
    autoFollowProcess(PostWaitType::IMMEDIATE);
    isPanicking = false;

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
    isPanicking = true;

    if ((preWaitActive->boolValue() || postWaitActive->boolValue()) && parentCuelist->currentCue->getTargetContainerAs<Cue>() == this) {
        parentCuelist->currentCue->resetValue();
    }

    if (preWaitActive->boolValue()) {
        preWaitTimer->stop();
        preWaitCurrentTime->setValue(0.0f);
        preWaitActive->setValue(false);
        isPlaying->setValue(false);
    }

    if (postWaitActive->boolValue()) {
        postWaitTimer->stop();
        postWaitCurrentTime->setValue(0.0f);
        postWaitActive->setValue(false);
    }

    panicInternal();
}

void Cue::stop()
{
    isPanicking = false;

    if ((preWaitActive->boolValue() || postWaitActive->boolValue()) && parentCuelist->currentCue->getTargetContainerAs<Cue>() == this) {
        parentCuelist->currentCue->resetValue();
    }

    if (preWaitActive->boolValue()) {
        preWaitTimer->stop();
        preWaitCurrentTime->setValue(0.0f);
        preWaitActive->setValue(false);
        isPlaying->setValue(false);
        return;
    }

    if (postWaitActive->boolValue()) {
        postWaitTimer->stop();
        postWaitCurrentTime->setValue(0.0f);
        postWaitActive->setValue(false);
        isPlaying->setValue(false);
        return;
    }

    stopInternal();
}

void Cue::endCue()
{
    isPlaying->setValue(false);
    autoFollowProcess(PostWaitType::AFTER_CUE);

    if (!postWaitCC->enabled->boolValue()) {
        if (parentCuelist->currentCue->getTargetContainerAs<Cue>() == this) {
            parentCuelist->currentCue->resetValue();
        }
    }
}

void Cue::playNextCue()
{
    auto idx = parentCuelist->cues->items.indexOf(this);
    if (idx + 2 < parentCuelist->cues->items.size()) {
        Cue* nextCue = parentCuelist->cues->items[idx + 1];
        nextCue->play();

        if (parentCuelist->currentCue->getTargetContainerAs<Cue>() == this) {
            parentCuelist->currentCue->setValueFromTarget(nextCue);
        }
    }
}

bool Cue::isAutoStartCue()
{
    auto idx = parentCuelist->cues->items.indexOf(this);
    if (idx - 1 < 0)
        return false;
    return parentCuelist->cues->items[idx - 1]->postWaitCC->enabled->boolValue();
}

void Cue::setNextCue()
{
    auto idx = parentCuelist->cues->items.indexOf(this);
    if (idx + 1 >= parentCuelist->cues->items.size()) {
        parentCuelist->nextCue->resetValue();
        parentCuelist->nextCue->notifyValueChanged();
        return;
    }

    for (int i = idx + 1; i < parentCuelist->cues->items.size(); i++) {
        Cue* c = parentCuelist->cues->items[i];
        if (!c->isAutoStartCue()) {
            parentCuelist->nextCue->setTarget(c);
            parentCuelist->nextCue->notifyValueChanged();
            return;
        }
    }
}

void Cue::autoFollowProcess(PostWaitType type)
{
    if (!postWaitCC->enabled->boolValue())
        return;

    if (
            type == postWaitType->intValue() ||
            type == PostWaitType::IMMEDIATE && postWaitType->intValue() == PostWaitType::AFTER_PRE && !preWaitCC->enabled->boolValue()
    ) {
        if (postWaitDuration->floatValue() <= 0.0f) {
            playNextCue();
        } else {
            postWaitCurrentTime->setValue(0.0f);
            postWaitActive->setValue(true);
            postWaitTimer->start(postWaitDuration->floatValue(), postWaitCurrentTime);
        }
    }
}

String Cue::getDescription()
{
    if (description->stringValue().isEmpty())
        return autoDescriptionInternal();

    return description->stringValue();
}
