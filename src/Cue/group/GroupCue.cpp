/*
  ==============================================================================

    GroupCue.cpp
    Created: 04 Jul 2026
    Author:  boherm

  ==============================================================================
*/

#include "GroupCue.h"
#include "../../Cuelist/Cuelist.h"
#include "../CueManager.h"
#include "ui/GroupCueEditor.h"

GroupCue::GroupCue(var params) :
    Cue(params)
{
    itemDataType = "Group Cue";

    // The group itself has no playable duration; only its members do.
    duration->isSavable = false;
    duration->hideInEditor = true;
    duration->hideInRemoteControl = true;
    currentTime->hideInEditor = true;

    fireMode = addEnumParameter("Fire Mode", "How the group triggers its sub-cues when GO'd");
    fireMode->addOption("Parallel (all at once)", PARALLEL);
    fireMode->addOption("Sequential", SEQUENTIAL);
    fireMode->addOption("Timeline", TIMELINE);

    loop = addBoolParameter("Loop", "If enabled, the group restarts from the top when it completes.", false);

    shuffle = addBoolParameter("Shuffle", "Sequential mode: play the sub-cues in a new random order on each GO.", false);
    shuffle->hideInEditor = fireMode->intValue() != SEQUENTIAL; // only shown in Sequential

    collapsed = addBoolParameter("Collapsed", "Whether the group is folded in the deck", false);
    collapsed->hideInEditor = true;

    groupUID = addStringParameter("Group UID", "Internal: stable identity of this group", Uuid().toString());
    groupUID->hideInEditor = true;
    groupUID->hideInRemoteControl = true;

    // The timeline lives in its own collapsible container, shown only in Timeline mode.
    timelineContainer.reset(new GroupTimelineContainer(this));
    updateTimelineContainer();
}

GroupCue::~GroupCue()
{
    stopListening();

    // Detach members before the base destructors run, so their parentGroup pointer can't
    // dangle while this group is torn down. Undoing the creation of a group deletes the
    // group but leaves its members in the cuelist; a selection-destroyed callback repaints
    // the deck mid-destruction (rebuildVisibleCues reads members' parentGroup) and would
    // otherwise touch this freed group. parentGroupUID is left intact so a redo re-links
    // them via resolveGroupMemberships().
    if (parentCuelist != nullptr && parentCuelist->cues != nullptr)
        for (auto* c : parentCuelist->cues->items)
            if (c != nullptr && c->parentGroup == this)
            {
                c->parentGroup = nullptr;
                c->notSetNextCueAuto = false;
                c->applyGroupMembershipConstraints(); // restore normal (post-wait) behaviour
            }
}

void GroupCue::updateTimelineContainer()
{
    if (timelineContainer == nullptr)
        return;

    const bool wantTimeline = fireMode->intValue() == TIMELINE;
    const bool isChild = controllableContainers.contains(timelineContainer.get());

    if (wantTimeline && !isChild)
        addChildControllableContainer(timelineContainer.get());
    else if (!wantTimeline && isChild)
        removeChildControllableContainer(timelineContainer.get());
}

void GroupCue::updateShuffleVisibility()
{
    if (shuffle == nullptr)
        return;

    const bool wantHidden = fireMode->intValue() != SEQUENTIAL;
    if (shuffle->hideInEditor == wantHidden)
        return;

    shuffle->hideInEditor = wantHidden;
    // Rebuild the inspector so the row appears / disappears (not just a value refresh).
    queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
}

Array<Cue*> GroupCue::getMembers() const
{
    Array<Cue*> members;
    if (parentCuelist == nullptr || parentCuelist->cues == nullptr)
        return members;

    for (auto* c : parentCuelist->cues->items)
        if (c != nullptr && c->parentGroup == this)
            members.add(c);

    return members;
}

void GroupCue::reshuffle()
{
    shuffleOrder = getMembers();
    juce::Random& rnd = juce::Random::getSystemRandom();
    for (int i = shuffleOrder.size() - 1; i > 0; i--) // Fisher-Yates
        shuffleOrder.swap(i, rnd.nextInt(i + 1));
}

void GroupCue::ensureShuffleOrder()
{
    Array<Cue*> members = getMembers();

    // Already the same set? keep the current (possibly shuffled) order.
    bool sameSet = shuffleOrder.size() == members.size();
    if (sameSet)
        for (auto* m : members)
            if (!shuffleOrder.contains(m)) { sameSet = false; break; }
    if (sameSet)
        return;

    // Membership changed since the last shuffle: drop removed members, append added ones
    // (keeping the existing random positions; a GO will re-shuffle the whole thing anyway).
    for (int i = shuffleOrder.size(); --i >= 0;)
        if (!members.contains(shuffleOrder[i]))
            shuffleOrder.remove(i);
    for (auto* m : members)
        if (!shuffleOrder.contains(m))
            shuffleOrder.add(m);
}

int GroupCue::getShuffleOrderIndex(const Cue* m)
{
    if (!shuffle->boolValue() || fireMode->intValue() != SEQUENTIAL)
        return 0;

    ensureShuffleOrder();
    return shuffleOrder.indexOf(const_cast<Cue*>(m)) + 1; // 1-based; 0 if not a member
}

bool GroupCue::isMemberActive(const Cue* m)
{
    return m != nullptr &&
        (m->isPlaying->boolValue() || m->preWaitActive->boolValue() || m->postWaitActive->boolValue());
}

void GroupCue::ensureListening()
{
    if (!listening && parentCuelist != nullptr && parentCuelist->cues != nullptr)
    {
        parentCuelist->cues->addAsyncContainerListener(this);
        listening = true;
    }
}

void GroupCue::stopListening()
{
    if (listening && parentCuelist != nullptr && parentCuelist->cues != nullptr)
        parentCuelist->cues->removeAsyncContainerListener(this);
    listening = false;
}

void GroupCue::playInternal()
{
    Array<Cue*> members = getMembers();
    if (members.isEmpty())
    {
        endCue();
        return;
    }

    launchMembers(true); // a real GO: draw a fresh shuffle order
}

void GroupCue::launchMembers(bool freshShuffle)
{
    // Fresh start: clear any leftover sequential state from a previous (possibly aborted
    // by stop/panic, or a completed loop iteration) run, so it always begins at the first
    // member.
    sequencing = false;
    seqIndex = -1;
    seqMembers.clearQuick();
    settleUntilMs = 0;
    aborting = false;
    retriggering = false;
    loopPending = false;

    Array<Cue*> members = getMembers();

    // Watch member state so the group ends exactly when its cues do (incl. panic/stop).
    ensureListening();

    // Clear any stale stop/panic flag left on a member (e.g. after a previous hard-stop),
    // so tick() doesn't read it as "this member was just stopped" and end the group early.
    for (auto* m : members)
        m->wasStoppedManually = false;

    if (fireMode->intValue() == SEQUENTIAL)
    {
        if (shuffle->boolValue())
        {
            // A real GO re-shuffles; a loop restart keeps the order from the initial GO.
            if (freshShuffle || shuffleOrder.isEmpty())
                reshuffle();
            else
                ensureShuffleOrder(); // keep the order, just reconcile membership
            seqMembers = shuffleOrder;
        }
        else
            seqMembers = members;

        sequencing = true;
        seqIndex = 0;
        seqMembers[0]->play();
    }
    else
    {
        sequencing = false;
        for (auto* m : members)
            m->play();
    }

    // Drive the group's currentTime (progress bar) from here.
    startProgressClock(0.0);

    // Resolve any members that finished instantly (e.g. Note/Go sub-cues).
    tick();
}

void GroupCue::tick()
{
    if (!isPlaying->boolValue())
        return;

    // Being stopped/panicked: end as soon as every member has finished (faded out),
    // regardless of the wall-clock timeline.
    if (aborting)
    {
        for (auto* m : getMembers())
            if (isMemberActive(m))
                return;
        finishGroup();
        return;
    }

    const double eps = 0.05;
    const double ct = currentTime->doubleValue();
    const double total = duration->doubleValue();

    // Right after a seek, members' transports blip isPlaying off/on: don't let that
    // transient advance or end the group.
    const bool settling = ((juce::int64) Time::getMillisecondCounter()) < settleUntilMs;

    if (sequencing)
    {
        Cue* cur = (seqIndex >= 0 && seqIndex < seqMembers.size()) ? seqMembers[seqIndex] : nullptr;
        if (cur == nullptr)
        {
            if (!settling && ct >= total - eps) finishGroup();
            return;
        }

        // Current member still running: keep waiting. Checked first so an active member
        // (incl. one fading on panic, or a freshly launched one with a stale stop-flag) can
        // never be mistaken for "finished".
        if (isMemberActive(cur))
            return;

        // Inactive, but within the post-seek settle window: treat as still playing.
        if (settling)
            return;

        // Inactive because it was stopped/panicked from the Active Cues panel: abort the
        // sequence (never launch the next one).
        if (cur->wasStoppedManually)
        {
            finishGroup();
            return;
        }

        // Inactive, but the group's own timeline hasn't reached this member's end yet: the
        // inactivity is spurious (a seek reposition, or the member finished a touch early).
        // The wall-clock timeline is the source of truth, so keep waiting.
        double spanEnd = 0.0;
        for (int k = 0; k <= seqIndex && k < seqMembers.size(); k++)
            spanEnd += memberSpan(seqMembers[k]);
        if (ct < spanEnd - eps)
            return;

        // Genuinely done: advance to the next member (or end after the last).
        if (seqIndex + 1 < seqMembers.size())
        {
            seqIndex++;
            seqMembers[seqIndex]->play();
            tick(); // re-evaluate (chains instant sub-cues, catches an immediate stop)
            return;
        }

        finishGroup();
        return;
    }

    // Parallel: end once no member is active AND the timeline has run out (guards seek blips).
    for (auto* m : getMembers())
        if (isMemberActive(m))
            return;

    if (!settling && ct >= total - eps)
        finishGroup();
}

void GroupCue::finishGroup()
{
    const bool wasAborting = aborting;
    const bool wasRetriggering = retriggering;

    // Loop: a group that completed on its own (not stopped/panicked) restarts from the top.
    // Deferred to the next timer tick so a group of instant sub-cues loops at timer rate
    // rather than recursing through launchMembers()/tick()/finishGroup().
    if (!wasAborting && loop->boolValue() && !getMembers().isEmpty())
    {
        aborting = false;
        loopPending = true;
        startTimerHz(30); // ensure the timer is running to pick up the restart
        return;
    }

    sequencing = false;
    aborting = false;
    retriggering = false;
    loopPending = false;
    isPanicking = false;
    seqIndex = -1;
    seqMembers.clearQuick();
    stopTimer();
    currentTime->setValue(0.0);

    // A group ended by a stop/panic must not fire its post-wait auto-follow (a stopped
    // normal cue never reaches its auto-follow either). A natural completion still does —
    // and so does a "Stop on retrigger", which mirrors a normal cue's retrigger-stop.
    suppressAutoFollow = wasAborting && !wasRetriggering;
    // Keep the subscription (cheap; newMessage early-outs while not playing) rather than
    // removing a listener from inside its own callback. It's dropped in the destructor.
    endCue();
    suppressAutoFollow = false;
}

void GroupCue::newMessage(const ContainerAsyncEvent& e)
{
    if (parentCuelist == nullptr)
        return;
    if (e.source != parentCuelist->cues || e.type != ContainerAsyncEvent::EventType::ControllableFeedbackUpdate)
        return;
    if (e.targetControllable == nullptr)
        return;

    const String n = e.targetControllable->niceName;
    const bool timingParam = (n == "Duration" || n == "Waiting time" || n == "Type" || n == "Enabled");
    const bool stateParam  = (n == "Is Playing" || n == "Active");
    if (!timingParam && !stateParam)
        return;

    // Which cue owns the changed controllable? Only react to our own members.
    Cue* owner = nullptr;
    for (ControllableContainer* cc = e.targetControllable->parentContainer.get(); cc != nullptr; cc = cc->parentContainer.get())
        if (auto* c = dynamic_cast<Cue*>(cc)) { owner = c; break; }
    if (owner == nullptr || owner->parentGroup != this)
        return;

    if (timingParam)
        refreshDuration();

    if (stateParam && isPlaying->boolValue())
        tick();
}

double GroupCue::memberSpan(const Cue* m)
{
    const double pre  = m->preWaitCC->enabled->boolValue()  ? m->preWaitDuration->doubleValue()  : 0.0;
    const double dur  = m->duration->doubleValue();
    const double post = m->postWaitCC->enabled->boolValue() ? m->postWaitDuration->doubleValue() : 0.0;

    // A post-wait AFTER_CUE runs after the cue (extends the tail); IMMEDIATE / AFTER_PRE
    // run alongside the cue, so they only extend it when longer than the cue itself.
    if (m->postWaitCC->enabled->boolValue())
        return (m->postWaitType->intValue() == Cue::PostWaitType::AFTER_CUE)
                   ? pre + dur + post
                   : pre + jmax(dur, post);
    return pre + dur;
}

double GroupCue::computeDuration() const
{
    bool sequential = fireMode->intValue() == SEQUENTIAL;
    double total = 0.0, longest = 0.0;

    for (auto* m : getMembers())
    {
        double span = memberSpan(m);
        total += span;
        longest = jmax(longest, span);
    }

    return sequential ? total : longest;
}

void GroupCue::startProgressClock(double fromTime)
{
    baseElapsed = jlimit(0.0, duration->doubleValue(), fromTime);
    playStartMs = (juce::int64) Time::getMillisecondCounter();
    currentTime->setValue(baseElapsed);
    startTimerHz(30);
}

void GroupCue::timerCallback()
{
    if (!isPlaying->boolValue())
    {
        stopTimer();
        return;
    }

    // A looping group finished its previous pass: restart it now (off the finishGroup call
    // stack). Keep the same shuffle order — a loop restart is not a new GO.
    if (loopPending)
    {
        loopPending = false;
        launchMembers(false);
        return;
    }

    double elapsed = baseElapsed + ((juce::int64) Time::getMillisecondCounter() - playStartMs) / 1000.0;
    currentTime->setValue(jlimit(0.0, duration->doubleValue(), elapsed));

    // Re-evaluate completion here too, so a genuine finish is caught even if the only
    // member event fell inside the post-seek settle window (which tick() ignores).
    tick();
}

bool GroupCue::canSeekTo() const
{
    return isPlaying->boolValue() && duration->doubleValue() > 0.0 && !getMembers().isEmpty();
}

void GroupCue::seekToTime(double t)
{
    Array<Cue*> members = getMembers();
    const double total = duration->doubleValue();
    if (members.isEmpty() || total <= 0.0)
        return;

    const double target = jlimit(0.0, total, t);
    const bool sequential = fireMode->intValue() == SEQUENTIAL;

    // Don't let the pending stops advance the sequence.
    sequencing = false;
    ensureListening();

    // Place member `m` at time `within` on its own timeline [0, pre + dur]: into the
    // pre-wait window (resume the wait at the right point) or into playback (skip the
    // pre-wait, seek the cue to the offset).
    auto startMemberAt = [](Cue* m, double within)
    {
        m->wasStoppedManually = false; // we're (re)starting it, not stopping it
        const double pre = m->preWaitCC->enabled->boolValue() ? m->preWaitDuration->doubleValue() : 0.0;

        if (within < pre)
        {
            m->startPreWaitAt(within);
            return;
        }

        const double cueOffset = jlimit(0.0, m->duration->doubleValue(), within - pre);
        if (m->isPlaying->boolValue() && !m->preWaitActive->boolValue())
        {
            if (m->canSeekTo()) m->seekToTime(cueOffset); // already playing: just re-seek
        }
        else
        {
            m->playSkippingPreWait();
            if (m->canSeekTo()) m->seekToTime(cueOffset);
        }
    };

    if (sequential)
    {
        // Locate the member whose span contains the target time.
        int idx = members.size() - 1;
        double spanStart = 0.0, acc = 0.0;
        for (int k = 0; k < members.size(); k++)
        {
            double s = memberSpan(members[k]);
            if (target < acc + s || k == members.size() - 1) { idx = k; spanStart = acc; break; }
            acc += s;
        }

        Cue* targetMember = members[idx];
        // Hard-stop every OTHER member; leave the target one running (just re-seek it).
        for (auto* m : members)
            if (m != targetMember && isMemberActive(m))
                m->stop();

        seqMembers = members;
        sequencing = true;
        seqIndex = idx;
        startMemberAt(targetMember, target - spanStart);
    }
    else
    {
        // Parallel: members whose span is already past run seeked; the rest are stopped.
        for (auto* m : members)
        {
            if (memberSpan(m) > target)
                startMemberAt(m, target);
            else if (isMemberActive(m))
                m->stop();
        }
    }

    startProgressClock(target);

    // Ignore the transport-reposition isPlaying blips for a short window.
    settleUntilMs = (juce::int64) Time::getMillisecondCounter() + 300;

    tick(); // wrap up if the seek landed at/after the end
}

void GroupCue::refreshDuration()
{
    // Keep watching members so the duration stays current as their timing changes.
    ensureListening();
    duration->setValue(computeDuration());
}

void GroupCue::parameterValueChanged(Parameter* p)
{
    Cue::parameterValueChanged(p);

    if (p == fireMode)
    {
        refreshDuration();
        updateTimelineContainer(); // show/hide the timeline section with the mode
        updateShuffleVisibility(); // Shuffle is Sequential-only
    }

    if (p == shuffle && shuffle->boolValue())
        reshuffle(); // give the deck badges an order to show right away
}

void GroupCue::stopInternal()
{
    // Abort: end the group once its members have stopped, don't advance the sequence.
    aborting = true;
    for (auto* m : getMembers())
        if (m->isPlaying->boolValue())
            m->stop();
    tick();
}

void GroupCue::retriggerStop()
{
    // Re-GO on a playing group with "Stop on retrigger": abort (no sequence advance, no
    // post-wait follow) and fade every active member over the GROUP's fade time. The group
    // ends once all members have settled (via tick()/finishGroup), exactly like a stop/panic.
    const double fadeTime = retriggerStopFadeOut->doubleValue();

    aborting = true;
    retriggering = true; // keep the post-wait after-cue, unlike a plain stop/panic
    isRetriggerStopping = true;
    for (auto* m : getMembers())
        if (isMemberActive(m))
            m->fadeAndStop(fadeTime);
    isRetriggerStopping = false;

    tick(); // ends immediately if nothing was active or the fade time is 0
}

void GroupCue::panicInternal()
{
    // Abort with a fade: panic each member (each once — the cuelist/panel skip grouped
    // members so they aren't double-panicked into a hard cut), then end once faded out.
    aborting = true;
    for (auto* m : getMembers())
        if (isMemberActive(m))
            m->panic();
    tick();
}

String GroupCue::autoDescriptionInternal()
{
    int n = getMembers().size();
    String mode = fireMode->intValue() == SEQUENTIAL ? "Sequential"
                : fireMode->intValue() == TIMELINE   ? "Timeline"
                                                      : "Parallel";
    return mode + " group (" + String(n) + (n == 1 ? " cue)" : " cues)");
}
