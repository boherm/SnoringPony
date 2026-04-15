/*
  ==============================================================================

    DCAMixingCuelist.h
    Created: 14 Apr 2026
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../Cuelist.h"

class DCAMixingCuelist :
    public Cuelist
{
public:
    DCAMixingCuelist(var params = var());
    virtual ~DCAMixingCuelist();

    void registerCueTypes(Factory<Cue>& f) override;

    InspectableEditor* getEditorInternal(bool isRoot, Array<Inspectable*> inspectables);

    void loadJSONDataInternal(var data) override;

    String getTypeString() const override { return "DCA Mixing Cuelist"; }
    static DCAMixingCuelist* create(var params) { return new DCAMixingCuelist(params); }

private:
    void ensureLineCheckCue();
};
