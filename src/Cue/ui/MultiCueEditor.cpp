/*
  ==============================================================================

    MultiCueEditor.cpp
    Created: 04 Jul 2026
    Author:  boherm

  ==============================================================================
*/

#include "MultiCueEditor.h"
#include "../Cue.h"
#include "../../Cuelist/Cuelist.h"
#include "CueUnsetValueEditor.h"

// ============================ FilteredCueContainerEditor ============================

FilteredCueContainerEditor::FilteredCueContainerEditor(ControllableContainer* container, bool isTopLevel,
                                                       std::shared_ptr<MultiEditFilter> filter,
                                                       std::function<bool(const String&)> topNameFilter,
                                                       const String& headerText,
                                                       MultiEditSubstitutions substitutions) :
    // buildAtCreation = false so filtering and substitutions (virtual overrides) are in
    // place before we build.
    GenericControllableContainerEditor(Array<ControllableContainer*>({ container }), false, false),
    isTopLevel(isTopLevel),
    filter(filter),
    topNameFilter(topNameFilter),
    headerText(headerText),
    substitutions(substitutions)
{
    containerLabel.setEditable(false, false);
    if (headerText.isNotEmpty()) containerLabel.setText(headerText, dontSendNotification);
    setDragAndDropEnabled(false);

    // Cues are removable-by-user, so the base container editor adds a remove button in
    // its header. In a multi-edit section that button would delete the representative
    // cue, which we never want here: drop it.
    if (removeBT != nullptr)
    {
        removeChildComponent(removeBT.get());
        removeBT.reset();
    }

    // Redraw the enable toggle for enabling containers (lost because we bypass
    // EnablingControllableContainerEditor to inject filtering).
    if (auto* ecc = dynamic_cast<EnablingControllableContainer*>(this->container.get()))
    {
        if (ecc->canBeDisabled)
        {
            enabledUI.reset(ecc->enabled->createToggle(ImageCache::getFromMemory(OrganicUIBinaryData::power_png, OrganicUIBinaryData::power_pngSize)));
            addAndMakeVisible(enabledUI.get());
        }
    }

    resetAndBuild();
}

FilteredCueContainerEditor::~FilteredCueContainerEditor()
{
}

bool FilteredCueContainerEditor::shouldShowControllable(Controllable* c)
{
    if (!GenericControllableContainerEditor::shouldShowControllable(c)) return false;
    if (isTopLevel && topNameFilter != nullptr && !topNameFilter(c->shortName)) return false;
    if (isTopLevel && substitutions.names.contains(c->shortName)) return true;
    if (filter != nullptr)
    {
        if (filter->hiddenNames.contains(c->shortName)) return false;
        if (filter->hideDisabledLeaves && !c->enabled) return false;
    }
    return true;
}

bool FilteredCueContainerEditor::shouldShowContainer(ControllableContainer* cc)
{
    if (!GenericControllableContainerEditor::shouldShowContainer(cc)) return false;
    if (isTopLevel && topNameFilter != nullptr && !topNameFilter(cc->shortName)) return false;
    if (isTopLevel && substitutions.names.contains(cc->shortName)) return true;
    if (filter != nullptr && filter->hiddenNames.contains(cc->shortName)) return false;
    return true;
}

InspectableEditor* FilteredCueContainerEditor::getEditorUIForControllable(Controllable* c)
{
    if (isTopLevel && substitutions.forControllable != nullptr && substitutions.names.contains(c->shortName))
        if (InspectableEditor* ed = substitutions.forControllable(c)) return ed;

    return GenericControllableContainerEditor::getEditorUIForControllable(c);
}

InspectableEditor* FilteredCueContainerEditor::getEditorUIForContainer(ControllableContainer* cc)
{
    if (isTopLevel && substitutions.forContainer != nullptr && substitutions.names.contains(cc->shortName))
        if (InspectableEditor* ed = substitutions.forContainer(cc)) return ed;

    // Recurse the filter only into plain setting groups (pre/post-wait, duck, MTC...) to
    // hide their runtime/current-time leaves. Managers (also EnablingControllableContainer,
    // but BaseItemListener) and other containers keep their natural editor so their own UI
    // (add buttons, item lists, custom item editors) is preserved.
    const bool isPlainEnabling = dynamic_cast<EnablingControllableContainer*>(cc) != nullptr
                                 && dynamic_cast<BaseItemListener*>(cc) == nullptr;

    if (isPlainEnabling)
        return new FilteredCueContainerEditor(cc, false, filter, nullptr, String());

    return GenericControllableContainerEditor::getEditorUIForContainer(cc);
}

void FilteredCueContainerEditor::resizedInternalHeader(juce::Rectangle<int>& r)
{
    if (!container.wasObjectDeleted())
        if (auto* ecc = dynamic_cast<EnablingControllableContainer*>(container.get()))
            if (ecc->canBeDisabled && enabledUI != nullptr)
                enabledUI->setBounds(r.removeFromLeft(r.getHeight()).reduced(2));

    GenericControllableContainerEditor::resizedInternalHeader(r);
}

void FilteredCueContainerEditor::controllableFeedbackUpdate(Controllable* c)
{
    if (c == nullptr || container.wasObjectDeleted()) return;
    if (auto* ecc = dynamic_cast<EnablingControllableContainer*>(container.get()))
        if (c == ecc->enabled) setCollapsed(!ecc->enabled->boolValue(), true);
}

int FilteredCueContainerEditor::getVisibleItemCount()
{
    if (container.wasObjectDeleted()) return 0;

    int count = 0;
    for (auto& c : container->controllables)
        if (c != nullptr && shouldShowControllable(c)) count++;
    for (auto& cc : container->controllableContainers)
        if (!cc.wasObjectDeleted() && cc != nullptr && shouldShowContainer(cc)) count++;
    return count;
}

void FilteredCueContainerEditor::newMessage(const ContainerAsyncEvent& e)
{
    GenericControllableContainerEditor::newMessage(e);

    // The base handler resets the label to the cue's niceName when its address (id)
    // changes; keep our section title instead.
    if (headerText.isNotEmpty() && e.type == ContainerAsyncEvent::ChildAddressChanged)
        containerLabel.setText(headerText, dontSendNotification);
}


// ================================ MultiCueEditor ================================

Cue* MultiCueEditor::asCue(const WeakReference<Inspectable>& w)
{
    if (w.wasObjectDeleted() || w == nullptr) return nullptr;
    return dynamic_cast<Cue*>(w.get());
}

MultiCueEditor::MultiCueEditor(Array<Cue*> allCues, bool isRoot) :
    InspectableEditor(Inspectable::getArrayAs<Cue, Inspectable>(allCues), isRoot)
{
    // Single-cuelist rule: only keep cues sharing the first cue's cuelist.
    Cuelist* refCuelist = allCues.isEmpty() ? nullptr : allCues.getFirst()->parentCuelist;
    for (auto& c : allCues)
        if (c != nullptr && c->parentCuelist == refCuelist)
            cues.add(c);

    // Top-level shortNames common to every selected cue.
    if (Cue* first = cues.isEmpty() ? nullptr : asCue(cues.getFirst()))
    {
        for (auto& c : first->controllables)
            if (c != nullptr) commonNames.addIfNotAlreadyThere(c->shortName);
        for (auto& cc : first->controllableContainers)
            if (!cc.wasObjectDeleted() && cc != nullptr) commonNames.addIfNotAlreadyThere(cc->shortName);

        for (int i = 1; i < cues.size(); i++)
        {
            Cue* cue = asCue(cues[i]);
            if (cue == nullptr) continue;

            StringArray keep;
            for (auto& name : commonNames)
                if (cue->getControllableByName(name, false) != nullptr
                    || cue->getControllableContainerByName(name, false) != nullptr)
                    keep.add(name);
            commonNames = keep;
        }
    }

    buildSections();

    // Listen to every rendered (representative) cue for value mirroring.
    for (auto* s : sections)
    {
        if (s == nullptr || s->container.wasObjectDeleted()) continue;
        Cue* repCue = dynamic_cast<Cue*>(s->container.get());
        if (repCue == nullptr) continue;

        bool alreadyListening = false;
        for (auto& r : repCues) if (r == repCue) { alreadyListening = true; break; }
        if (alreadyListening) continue;

        repCues.add(repCue);
        repCue->addAsyncContainerListener(this);
    }

    setSize(100, 10);
}

MultiCueEditor::~MultiCueEditor()
{
    for (auto& r : repCues)
        if (Cue* cue = asCue(r))
            cue->removeAsyncContainerListener(this);

    sections.clear();
}

StringArray MultiCueEditor::getOrderedTypes() const
{
    StringArray types;
    for (auto& c : cues)
        if (Cue* cue = asCue(c))
            types.addIfNotAlreadyThere(cue->getCueType());
    return types;
}

Array<Cue*> MultiCueEditor::cuesOfType(const String& type) const
{
    Array<Cue*> result;
    for (auto& c : cues)
        if (Cue* cue = asCue(c))
            if (cue->getCueType() == type) result.add(cue);
    return result;
}

MultiEditSubstitutions MultiCueEditor::makeSubstitutions(Cue* rep, const Array<Cue*>& scopeCues)
{
    MultiEditSubstitutions subs;
    if (rep == nullptr) return subs;

    // The type's bulk block, rendered where the container it stands in for would be (the
    // audio file list, so the block lands right before the plugin chain).
    const String anchorName = rep->getMultiEditExtraEditorAnchorName();
    if (anchorName.isNotEmpty())
    {
        subs.names.add(anchorName);
        Array<Cue*> scope = scopeCues;
        subs.forContainer = [rep, scope](ControllableContainer*) { return rep->createMultiEditExtraEditor(scope); };
    }

    // Parameters whose shared value would be misleading: replaced by an unset proxy.
    StringArray unsetNames;
    for (auto& name : rep->getMultiEditUnsetProxyNames())
    {
        Controllable* c = rep->getControllableByName(name, false);
        if (c != nullptr && CueUnsetValueEditor::canSubstitute(c)) unsetNames.add(name);
    }

    if (!unsetNames.isEmpty())
    {
        subs.names.addArray(unsetNames);
        Array<Cue*> scope = scopeCues;
        subs.forControllable = [scope](Controllable* c) -> InspectableEditor*
        {
            Parameter* p = dynamic_cast<Parameter*>(c);
            return p != nullptr ? new CueUnsetValueEditor(p, scope) : nullptr;
        };
    }

    mirrorExcludedTopNames.addArray(subs.names);
    return subs;
}

void MultiCueEditor::addSyncFor(Cue* rep, const Array<Cue*>& scopeCues)
{
    if (rep == nullptr) return;
    if (MultiCueSync* s = rep->createMultiEditSync(scopeCues)) syncs.add(s);
}

void MultiCueEditor::buildSections()
{
    isRebuilding = true;
    sections.clear();
    syncs.clear();
    layoutItems.clear();

    Cue* firstCue = cues.isEmpty() ? nullptr : asCue(cues.getFirst());
    if (firstCue == nullptr)
    {
        isRebuilding = false;
        return;
    }

    StringArray types = getOrderedTypes();
    const bool allSameType = types.size() == 1;

    // Duration is only meaningful to edit as a group for Fade cues (it is their fade time).
    // When every cue is a Fade cue it is shown as a shared field; in a mixed selection it
    // is shown only inside the Fade section; for non-fade cues it is auto-computed, so hidden.
    allFade = true;
    for (auto& c : cues)
        if (Cue* cue = asCue(c)) { if (cue->getCueType() != "Fade") { allFade = false; break; } }

    // Names the generic mirror must ignore (hidden / not user-editable in multi, or driven
    // by a dedicated extra editor such as the audio files bulk controls). Duration is
    // handled specially in newMessage (Fade-only), so it is not listed here.
    mirrorExcludedTopNames.clearQuick();
    mirrorExcludedTopNames.addArray({ "id", "isPlaying", "currentTime" });
    for (auto& c : cues)
        if (Cue* cue = asCue(c))
        {
            mirrorExcludedTopNames.addArray(cue->getMultiEditHiddenControllableNames());
            mirrorExcludedTopNames.addArray(cue->getMultiEditMirrorExcludedNames());
        }

    // Top-level name filters: common vs type-specific split.
    auto commonFilter = [this](const String& name) { return commonNames.contains(name); };
    auto specificFilter = [this](const String& name) { return !commonNames.contains(name); };

    // Per-section recursive filter: hides per-cue/runtime fields (id, is playing, the
    // current-time displays via hideDisabledLeaves) plus the type's own hidden lists.
    const bool allFadeLocal = allFade;
    auto makeFilter = [allFadeLocal](Cue* rep) {
        auto f = std::make_shared<MultiEditFilter>();
        f->hiddenNames.addArray({ "id", "isPlaying" });
        if (!allFadeLocal) f->hiddenNames.add("duration");
        if (rep != nullptr) f->hiddenNames.addArray(rep->getMultiEditHiddenControllableNames());
        return f;
    };

    // --- Common section (also the single full section when all cues share a type) ---
    String commonTitle = allSameType
        ? pluralizeCues(cues.size(), firstCue->getCueType())
        : "Common (" + String(cues.size()) + " cues)";

    // The common section is shared by every selected cue, so its substitutions apply to all
    // of them; when all cues share a type it IS that type's section, so its bulk block and
    // sync (audio files, DCA assignments) belong right here too.
    Array<Cue*> commonScope;
    for (auto& c : cues) if (Cue* cue = asCue(c)) commonScope.add(cue);

    auto* commonSection = new FilteredCueContainerEditor(firstCue, true, makeFilter(firstCue), commonFilter, commonTitle,
                                                         makeSubstitutions(firstCue, commonScope));
    sections.add(commonSection);
    addAndMakeVisible(commonSection);
    layoutItems.add(commonSection);

    if (allSameType) addSyncFor(firstCue, cuesOfType(firstCue->getCueType()));

    // --- Per-type sections with the type-specific parameters ---
    if (!allSameType)
    {
        for (auto& type : types)
        {
            Array<Cue*> typeCues = cuesOfType(type);
            Cue* rep = typeCues.isEmpty() ? nullptr : typeCues.getFirst();
            if (rep == nullptr) continue;

            auto sectionFilter = makeFilter(rep);
            std::function<bool(const String&)> topFilter = specificFilter;

            // The Fade section additionally shows duration (its fade time), even though
            // duration is a common field, so it can be edited across the fade cues.
            if (type == "Fade")
            {
                sectionFilter->hiddenNames.removeString("duration");
                topFilter = [this](const String& name) { return name == "duration" || !commonNames.contains(name); };
            }

            auto* section = new FilteredCueContainerEditor(rep, true, sectionFilter, topFilter, pluralizeCues(typeCues.size(), type),
                                                           makeSubstitutions(rep, typeCues));
            if (section->getVisibleItemCount() == 0)
            {
                delete section; // no type-specific params to show
            }
            else
            {
                sections.add(section);
                addAndMakeVisible(section);
                layoutItems.add(section);
            }

            addSyncFor(rep, typeCues);
        }
    }

    isRebuilding = false;
}

void MultiCueEditor::resized()
{
    if (isRebuilding) return;

    int w = getWidth();
    if (w == 0) return;

    const int gap = 8;
    int y = 0;

    isRebuilding = true;
    for (auto* s : layoutItems)
    {
        if (s == nullptr) continue;
        // Set the width first so the item recomputes its own height synchronously,
        // then position it and advance using the settled height.
        s->setSize(w, s->getHeight());
        s->setTopLeftPosition(0, y);
        y += s->getHeight() + gap;
    }
    isRebuilding = false;

    int totalH = jmax(y, 10);
    if (getHeight() != totalH) setSize(w, totalH);
}

void MultiCueEditor::childBoundsChanged(juce::Component* c)
{
    if (isRebuilding) return;
    if (getWidth() == 0) return;
    resized();
}

void MultiCueEditor::newMessage(const ContainerAsyncEvent& e)
{
    if (isMirroring) return;
    if (e.type != ContainerAsyncEvent::ControllableFeedbackUpdate) return;

    Controllable* c = e.targetControllable;
    if (c == nullptr || e.targetControllable.wasObjectDeleted()) return;

    Parameter* src = dynamic_cast<Parameter*>(c);
    if (src == nullptr) return;

    // Only mirror user-editable, persisted parameters. Runtime feedback (currentTime,
    // isPlaying, the *active flags) is excluded via isSavable/enabled below.
    // Hidden params are skipped too, EXCEPT the enabled toggle of an
    // EnablingControllableContainer (pre/post-wait, duck, stop-on-retrigger, MTC...):
    // it is drawn as a header switch so it is hidden as a plain param, yet it is a
    // genuine user setting that must propagate.
    bool isEnableToggle = false;
    if (auto* ecc = dynamic_cast<EnablingControllableContainer*>(src->parentContainer.get()))
        isEnableToggle = (ecc->enabled == src);

    if (!src->isSavable || !src->enabled) return;
    if (src->hideInEditor && !isEnableToggle) return;

    Cue* sourceCue = getCueAncestor(src);
    if (sourceCue == nullptr) return;

    const String relAddr = src->getControlAddress(sourceCue);
    if (relAddr.isEmpty()) return;

    const String topName = firstAddressSegment(relAddr);
    const String sourceType = sourceCue->getCueType();

    // Duration is Fade-only: never mirror it for other types (auto-computed per cue).
    // For an all-fade selection it is shared (scope all); in a mixed selection it mirrors
    // within the Fade group only. Other excluded names are dropped outright.
    bool isCommon;
    if (topName == "duration")
    {
        if (sourceType != "Fade") return;
        isCommon = allFade;
    }
    else
    {
        if (mirrorExcludedTopNames.contains(topName)) return;
        isCommon = commonNames.contains(topName);
    }

    const var newVal = src->getValue();

    Array<UndoableAction*> actions;
    for (auto& t : cues)
    {
        Cue* target = asCue(t);
        if (target == nullptr) continue;
        if (target == sourceCue) continue;
        if (!isCommon && target->getCueType() != sourceType) continue;

        Controllable* tc = target->getControllableForAddress(relAddr);
        Parameter* tp = dynamic_cast<Parameter*>(tc);
        if (tp == nullptr) continue;
        if (tp->getValue() == newVal) continue;

        if (UndoableAction* a = tp->setUndoableValue(tp->getValue(), newVal, true))
            actions.add(a);
    }

    if (actions.isEmpty()) return;

    isMirroring = true;
    UndoMaster::getInstance()->performActions("Edit " + String(cues.size()) + " cues", actions);
    isMirroring = false;
}

// ---------------------------------------------------------------------------------

String MultiCueEditor::firstAddressSegment(const String& address)
{
    StringArray split;
    split.addTokens(address, "/", "");
    split.removeEmptyStrings();
    return split.isEmpty() ? String() : split[0];
}

Cue* MultiCueEditor::getCueAncestor(Controllable* c)
{
    if (c == nullptr) return nullptr;
    ControllableContainer* cc = c->parentContainer;
    while (cc != nullptr)
    {
        if (Cue* cue = dynamic_cast<Cue*>(cc)) return cue;
        cc = cc->parentContainer;
    }
    return nullptr;
}

String MultiCueEditor::pluralizeCues(int count, const String& typeName)
{
    return String(count) + " " + typeName + (count > 1 ? " Cues" : " Cue");
}
