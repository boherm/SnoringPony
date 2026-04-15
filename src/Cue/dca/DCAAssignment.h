/*
  ==============================================================================

    DCAAssignment.h
    Created: 14 Apr 2026
    Author:  boherm

    A DCAAssignment represents the membership of one DCA in a DCACue:
    - its DCA number
    - an optional custom display name (pushed to the console's /dca/<n>/name)
    - the list of characters assigned to it

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "CharacterRef.h"

class DCACue;
class Character;

class DCAAssignment :
    public BaseItem
{
public:
    DCAAssignment(var params = var());
    virtual ~DCAAssignment();

    IntParameter* dcaNumber;
    StringParameter* displayName;

    std::unique_ptr<BaseManager<CharacterRef>> characters;

    // Walks up the container hierarchy to find the owning DCACue.
    DCACue* getParentCue() const;

    // Name actually pushed to the console AND shown in the cuelist table cell
    String getEffectiveDisplayName() const;

    bool hasCharacter(Character* c) const;
    void addCharacter(Character* c);
    void removeCharacter(Character* c);

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables) override;

    String getTypeString() const override { return "DCA Assignment"; }
    static DCAAssignment* create(var params) { return new DCAAssignment(params); }
};
