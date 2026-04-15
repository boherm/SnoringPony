/*
  ==============================================================================

    DCAAssignment.cpp
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#include "DCAAssignment.h"
#include "DCACue.h"
#include "ui/DCAAssignmentEditor.h"
#include "../../Interface/mixer/Character.h"

DCAAssignment::DCAAssignment(var params) :
    BaseItem(params.getProperty("name", "DCA"))
{
    itemDataType = "DCA Assignment";
    saveAndLoadRecursiveData = true;
    canBeDisabled = false;
    userCanDuplicate = false;

    dcaNumber = addIntParameter("DCA Number", "Which DCA this assignment targets", params.getProperty("dca", 1), 1, 16);

    displayName = addStringParameter("Display Name", "Custom name shown on the console and cuelist. Leave empty to use the character name (single character) or a comma-separated list.", "");

    characters.reset(new BaseManager<CharacterRef>("Characters"));
    characters->userCanAddItemsManually = false;
    addChildControllableContainer(characters.get());
}

DCACue* DCAAssignment::getParentCue() const
{
    ControllableContainer* mgr = parentContainer.get();
    if (mgr == nullptr) return nullptr;
    return dynamic_cast<DCACue*>(mgr->parentContainer.get());
}

InspectableEditor* DCAAssignment::getEditorInternal(bool isRoot, Array<Inspectable*> /*inspectables*/)
{
    return new DCAAssignmentEditor(this, isRoot);
}

DCAAssignment::~DCAAssignment()
{
}

String DCAAssignment::getEffectiveDisplayName() const
{
    if (displayName->stringValue().isNotEmpty())
        return displayName->stringValue();

    if (characters->items.size() == 1)
    {
        if (auto* c = characters->items[0]->getCharacter())
            return c->characterName->stringValue();
    }

    StringArray parts;
    for (auto* r : characters->items)
        if (auto* c = r->getCharacter())
            parts.add(c->characterName->stringValue());
    return parts.joinIntoString(", ");
}

bool DCAAssignment::hasCharacter(Character* c) const
{
    for (auto* r : characters->items)
        if (r->getCharacter() == c) return true;
    return false;
}

void DCAAssignment::addCharacter(Character* c)
{
    if (c == nullptr || hasCharacter(c)) return;
    CharacterRef* r = new CharacterRef();
    r->characterRef->setValueFromTarget(c);
    characters->addItem(r);
}

void DCAAssignment::removeCharacter(Character* c)
{
    for (int i = characters->items.size() - 1; i >= 0; --i)
    {
        if (characters->items[i]->getCharacter() == c)
            characters->removeItem(characters->items[i]);
    }
}
