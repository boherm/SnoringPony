/*
  ==============================================================================

    DCACue.cpp
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "DCACue.h"
#include "../../Interface/mixer/MixerInterface.h"
#include "../../Interface/mixer/MixerChannel.h"
#include "../../Interface/mixer/Character.h"
#include "../../Interface/mixer/MixerFX.h"
#include "../../Interface/InterfaceManager.h"
#include <set>

DCACue::DCACue(var params) :
    Cue(params)
{
    itemDataType = "DCA Cue";
    duration->isSavable = false;
    duration->hideInEditor = true;
    duration->hideInRemoteControl = true;
    retriggerStopCC->hideInEditor = true;

    targetMixer = addTargetParameter("Target Mixer", "Mixing console interface to apply this DCA assignment to", InterfaceManager::getInstance());
    targetMixer->targetType = TargetParameter::CONTAINER;
    targetMixer->maxDefaultSearchLevel = 1;

    dcaAssignments.reset(new BaseManager<DCAAssignment>("DCA Assignments"));
    dcaAssignments->selectItemWhenCreated = false;
    dcaAssignments->addBaseManagerListener(this);
    addChildControllableContainer(dcaAssignments.get());

    if (!Engine::mainEngine->isLoadingFile)
    {
        Array<MixerInterface*> mixers = InterfaceManager::getInstance()->getItemsWithType<MixerInterface>();
        if (!mixers.isEmpty())
            targetMixer->setValueFromTarget(mixers[0]);
    }
}

DCACue::~DCACue()
{
    if (dcaAssignments != nullptr) dcaAssignments->removeBaseManagerListener(this);
}

MixerInterface* DCACue::getMixer() const
{
    return targetMixer->getTargetContainerAs<MixerInterface>();
}

int DCACue::getMixerNumDCAs() const
{
    if (auto* m = getMixer()) return m->numDCAs->intValue();
    return 0;
}

DCAAssignment* DCACue::findAssignment(int dcaNumber) const
{
    for (auto* a : dcaAssignments->items)
        if (a->dcaNumber->intValue() == dcaNumber) return a;
    return nullptr;
}

DCAAssignment* DCACue::createAssignment(int dcaNumber)
{
    DCAAssignment* a = new DCAAssignment();
    a->dcaNumber->setValue(dcaNumber);
    dcaAssignments->addItem(a);
    return a;
}

void DCACue::itemAdded(DCAAssignment* a)
{
    if (a == nullptr) return;
    if (Engine::mainEngine != nullptr && Engine::mainEngine->isLoadingFile) return;

    // If another assignment already owns this DCA number, bump to the next free slot
    int current = a->dcaNumber->intValue();
    bool conflict = false;
    for (auto* other : dcaAssignments->items)
    {
        if (other == a) continue;
        if (other->dcaNumber->intValue() == current) { conflict = true; break; }
    }

    if (conflict)
    {
        int maxDCA = getMixerNumDCAs();
        if (maxDCA <= 0) maxDCA = 16;
        for (int k = 1; k <= maxDCA; ++k)
        {
            bool used = false;
            for (auto* other : dcaAssignments->items)
            {
                if (other == a) continue;
                if (other->dcaNumber->intValue() == k) { used = true; break; }
            }
            if (!used) { a->dcaNumber->setValue(k); current = k; break; }
        }
    }

    a->setNiceName("DCA " + String(current));
}

bool DCACue::isCharacterUsedInOtherDCA(Character* c, DCAAssignment* excluding) const
{
    for (auto* a : dcaAssignments->items)
    {
        if (a == excluding) continue;
        if (a->hasCharacter(c)) return true;
    }
    return false;
}

DCAAssignment* DCACue::findAssignmentContaining(Character* c) const
{
    for (auto* a : dcaAssignments->items)
        if (a->hasCharacter(c)) return a;
    return nullptr;
}

bool DCACue::isChannelUsedInOtherDCA(MixerChannel* ch, DCAAssignment* excluding) const
{
    if (ch == nullptr) return false;
    for (auto* a : dcaAssignments->items)
    {
        if (a == excluding) continue;
        for (auto* r : a->characters->items)
            if (r->getChannel() == ch) return true;
    }
    return false;
}

void DCACue::playInternal()
{
    auto* mixer = getMixer();
    if (mixer == nullptr) { endCue(); return; }

    // Line check cue (non-removable baseline): reset icons and push first
    // character name for each declared channel before applying DCA state.
    if (!userCanRemove) mixer->applyLineCheckBaseline();

    const int n = mixer->numDCAs->intValue();
    Array<Array<int>> membership;
    for (int i = 0; i < n; ++i) membership.add(Array<int>());

    StringArray names;
    for (int i = 0; i < n; ++i) names.add(String());

    std::map<int, String> activeChannelNames;
    std::map<int, std::set<int>> channelFXBuses;
    Array<bool> dcaHasFX;
    for (int i = 0; i < n; ++i) dcaHasFX.add(false);

    std::map<int, float> dcaForcedFaders;

    for (auto* a : dcaAssignments->items)
    {
        int idx = a->dcaNumber->intValue() - 1;
        if (idx < 0 || idx >= n) continue;

        for (auto* r : a->characters->items)
        {
            auto* character = r->getCharacter();
            auto* ch = r->getChannel();
            if (ch == nullptr) continue;

            int chNum = ch->channelNumber->intValue();
            membership.getReference(idx).addIfNotAlreadyThere(chNum);
            if (character != nullptr)
                activeChannelNames[chNum] = character->characterName->stringValue();

            if (auto* fx = r->getFX())
            {
                channelFXBuses[chNum].insert(fx->busNumber->intValue());
                dcaHasFX.set(idx, true);
            }
        }

        names.set(idx, a->getEffectiveDisplayName());

        if (a->forceFader->boolValue())
            dcaForcedFaders[idx] = a->faderPosition->floatValue();
    }

    mixer->applyDCAMembership(membership, names, activeChannelNames, channelFXBuses, dcaHasFX, dcaForcedFaders);
    endCue();
}

void DCACue::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == targetMixer)
    {
        refreshCharacterRefRoots();
        checkBrokenRefs();
    }
}

void DCACue::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
    Cue::onControllableFeedbackUpdateInternal(cc, c);

    for (auto* a : dcaAssignments->items)
    {
        for (auto* r : a->characters->items)
        {
            if (c == r->characterRef)
            {
                checkBrokenRefs();
                return;
            }
        }
    }
}

void DCACue::refreshCharacterRefRoots()
{
    for (auto* a : dcaAssignments->items)
        for (auto* r : a->characters->items)
            r->updateRootFromCue();
}

void DCACue::checkBrokenRefs()
{
    for (auto* a : dcaAssignments->items)
    {
        for (auto* r : a->characters->items)
        {
            if (r->getCharacter() == nullptr && r->characterRef->stringValue().isNotEmpty())
            {
                setWarningMessage("Broken character reference in DCA " + String(a->dcaNumber->intValue()));
                return;
            }
        }
    }
    clearWarning();
}

void DCACue::loadJSONDataInternal(var data)
{
    Cue::loadJSONDataInternal(data);
    refreshCharacterRefRoots();
    checkBrokenRefs();
}

String DCACue::autoDescriptionInternal()
{
    StringArray names;
    for (auto* a : dcaAssignments->items)
    {
        for (auto* r : a->characters->items)
            if (auto* c = r->getCharacter())
                names.addIfNotAlreadyThere(c->characterName->stringValue());
    }
    return names.joinIntoString(", ");
}
