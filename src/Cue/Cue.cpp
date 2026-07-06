/*
  ==============================================================================

    Cue.cpp
    Created: 01 Oct 2025 11:45:43pm
    Author:  boherm

  ==============================================================================
*/

#include "Cue.h"
#include "group/GroupCue.h"
#include "ui/CueEditor.h"
#include "ui/MultiCueEditor.h"
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

    // UID of the group this cue belongs to (empty = ungrouped). Persisted so membership
    // survives save/load; the parentGroup pointer is resolved from it after load.
    parentGroupUID = addStringParameter("Parent Group UID", "Internal: UID of the owning group cue", "");
    parentGroupUID->hideInEditor = true;
    parentGroupUID->hideInRemoteControl = true;

    previewBtn = addTrigger("Preview", "Preview this cue");
    previewBtn->hideInEditor = true;

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
    preWaitCurrentTime->isSavable = false;

    preWaitActive = preWaitCC->addBoolParameter("Active", "Pre-wait currently active", false);
    preWaitActive->setEnabled(false);
    preWaitActive->hideInEditor = true;
    preWaitActive->isSavable = false;

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
    postWaitCurrentTime->isSavable = false;

    postWaitActive = postWaitCC->addBoolParameter("Active", "Auto-follow currently active", false);
    postWaitActive->setEnabled(false);
    postWaitActive->hideInEditor = true;
    postWaitActive->isSavable = false;

    // --- Stop on retrigger ---
    retriggerStopCC = new EnablingControllableContainer("Stop on retrigger");
    retriggerStopCC->enabled->setValue(false);
    retriggerStopCC->editorIsCollapsed = true;
    addChildControllableContainer(retriggerStopCC, true);

    retriggerStopFadeOut = retriggerStopCC->addFloatParameter("Fade Out", "Fade out duration before stopping", 0.0, 0.0);
    retriggerStopFadeOut->defaultUI = FloatParameter::TIME;
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
    if (inspectables.size() > 1)
    {
        // Multi-selection: edit every selected cue at once. Restricted to cues of the
        // same cuelist as this one (single-cuelist rule).
        Array<Cue*> sameCuelistCues;
        for (auto* c : Inspectable::getArrayAs<Inspectable, Cue>(inspectables))
            if (c != nullptr && c->parentCuelist == parentCuelist)
                sameCuelistCues.add(c);

        if (sameCuelistCues.size() > 1) return new MultiCueEditor(sameCuelistCues, isRoot);
    }

    return new CueEditor(this, isRoot);
}

void Cue::setGoNext()
{
    parentCuelist->nextCue->setTarget(this);
    parentCuelist->nextCue->notifyValueChanged();
}

void Cue::applyGroupMembershipConstraints()
{
    const bool grouped = parentGroup != nullptr;
    postWaitCC->hideInEditor = grouped;
    if (grouped)
        postWaitCC->enabled->setValue(false);
}

void Cue::parameterValueChanged(Parameter* p)
{
    ControllableContainer::parameterValueChanged(p);

    if (p == id) {
        setNiceName(id->stringValue());
        setCustomShortName(id->stringValue());

        // Re-evaluate duplicate-id warnings across the whole cuelist, so changing one
        // cue's id also updates a cue it previously (or now) collides with. Skipped
        // during a reorder (one check runs at the end) and during file load.
        if (parentCuelist != nullptr && parentCuelist->cues != nullptr
            && !parentCuelist->cues->isReordering
            && (Engine::mainEngine == nullptr || !Engine::mainEngine->isLoadingFile))
        {
            parentCuelist->cues->refreshDuplicateIdWarnings();
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
        if (retriggerStopCC == cc && c == retriggerStopCC->enabled) {
            retriggerStopCC->editorIsCollapsed = !retriggerStopCC->enabled->boolValue();
        }
    }
}

void Cue::triggerTriggered(Trigger* t)
{
    if (t == setNextBtn) {
        setGoNext();
    }
    if (t == previewBtn) {
        preview();
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
    if (isPlaying->boolValue() && retriggerStopCC->enabled->boolValue())
    {
        retriggerStop();
        if (!notSetNextCueAuto)
            setNextCue();
        return;
    }

    if (!canBePlayed())
        return;

    isPlaying->setValue(true);
    wasStoppedManually = false;
    autoFollowProcess(PostWaitType::IMMEDIATE);
    isPanicking = false;

    if (preWaitCC->enabled->boolValue()) {
        preWaitCurrentTime->setValue(0.0f);
        preWaitActive->setValue(true);
        preWaitTimer->start(preWaitDuration->floatValue(), preWaitCurrentTime);
    } else {
        playInternal();
    }

    if (!retriggerStopCC->enabled->boolValue() && !notSetNextCueAuto)
        setNextCue();
}

void Cue::preview()
{
    if (!canBePreviewed())
        return;

    previewInternal();
}

void Cue::playSkippingPreWait()
{
    // Currently waiting: cancel the pre-wait and begin the cue right now (isPlaying was
    // already set when the wait started).
    if (preWaitActive->boolValue())
    {
        preWaitTimer->stop();
        preWaitCurrentTime->setValue(0.0f);
        preWaitActive->setValue(false);
        playInternal();
        return;
    }

    if (isPlaying->boolValue())
        return; // already playing the cue

    if (!canBePlayed())
        return;

    isPlaying->setValue(true);
    wasStoppedManually = false;
    isPanicking = false;
    playInternal();

    if (!retriggerStopCC->enabled->boolValue() && !notSetNextCueAuto)
        setNextCue();
}

void Cue::startPreWaitAt(double elapsed)
{
    if (!preWaitCC->enabled->boolValue())
    {
        playSkippingPreWait();
        return;
    }

    // If the cue portion is running, stop it first so we can drop back into the wait.
    if (isPlaying->boolValue() && !preWaitActive->boolValue())
        stopInternal();

    if (!isPlaying->boolValue())
    {
        if (!canBePlayed())
            return;
        isPlaying->setValue(true);
        wasStoppedManually = false;
        isPanicking = false;
        if (!retriggerStopCC->enabled->boolValue() && !notSetNextCueAuto)
            setNextCue();
    }

    // start() resets the timer to 0; set the elapsed afterwards so the wait resumes there.
    preWaitActive->setValue(true);
    preWaitTimer->start(preWaitDuration->floatValue(), preWaitCurrentTime);
    preWaitCurrentTime->setValue((float) jlimit(0.0, (double) preWaitDuration->doubleValue(), elapsed));
}

void Cue::panic()
{
    wasStoppedManually = true;
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

void Cue::retriggerStop()
{
    isRetriggerStopping = true;
    stop();
    isRetriggerStopping = false;
    endCue();
}

void Cue::fadeAndStop(double)
{
    // Nothing to fade for a generic cue: stop right away. Audio-backed cues override this.
    stop();
}

void Cue::stop()
{
    wasStoppedManually = true;
    isPanicking = false;
    isPreviewing = false;

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
    if (isPreviewing) {
        isPreviewing = false;
        return;
    }

    bool wasRetriggerStop = isRetriggerStopping;

    isPlaying->setValue(false);
    autoFollowProcess(PostWaitType::AFTER_CUE);

    if (!postWaitCC->enabled->boolValue() && !keepCurrentCueOnEnd) {
        if (parentCuelist->currentCue->getTargetContainerAs<Cue>() == this) {
            parentCuelist->currentCue->resetValue();
        }
    }

    // When a cue with "Stop on retrigger" finishes naturally (not by retrigger),
    // advance the nextCue pointer so the operator sees the next cue ready.
    if (retriggerStopCC->enabled->boolValue() && !wasRetriggerStop && !notSetNextCueAuto)
        setNextCue();
}

void Cue::playNextCue()
{
    // A grouped sub-cue never advances the cuelist: its group owns sequencing.
    if (isGroupMember())
        return;

    auto idx = parentCuelist->cues->items.indexOf(this);
    // Auto-follow to the next playable cue, skipping notes (never auto-played).
    for (int i = idx + 1; i < parentCuelist->cues->items.size(); i++) {
        Cue* nextCue = parentCuelist->cues->items[i];
        if (!nextCue->canBeNextCueAuto())
            continue;

        nextCue->play();

        if (parentCuelist->currentCue->getTargetContainerAs<Cue>() == this) {
            parentCuelist->currentCue->setValueFromTarget(nextCue);
        }
        return;
    }
}

bool Cue::isAutoStartCue()
{
    // A grouped sub-cue is fired by its group, never auto-started by a post-wait chain.
    if (isGroupMember())
        return false;

    auto idx = parentCuelist->cues->items.indexOf(this);
    if (idx - 1 < 0)
        return false;

    Cue* pred = parentCuelist->cues->items[idx - 1];
    // The auto-follow skips a group's members, so the cue that would auto-start `this` is
    // the group itself (not its last member): check the group's post-wait, not the member's.
    if (pred->isGroupMember())
        pred = pred->parentGroup;

    return pred != nullptr && pred->postWaitCC->enabled->boolValue();
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
        if (!c->isAutoStartCue() && c->canBeNextCueAuto()) {
            parentCuelist->nextCue->setTarget(c);
            parentCuelist->nextCue->notifyValueChanged();
            return;
        }
    }

    // Every following cue auto-starts (post-wait chain to the end): there is no cue left
    // to GO manually, so clear the next cue instead of leaving a stale one.
    parentCuelist->nextCue->resetValue();
    parentCuelist->nextCue->notifyValueChanged();
}

void Cue::autoFollowProcess(PostWaitType type)
{
    if (suppressAutoFollow)
        return;

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
