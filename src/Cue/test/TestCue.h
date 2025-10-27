/*
  ==============================================================================

    TestCue.h
    Created: 27 Oct 2025 09:32:20pm
    Author:  boherm

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"
#include "../Cue.h"

class TestCue :
    public Cue
{
public:
    TestCue(var params = var());
    virtual ~TestCue();

    String getTypeString() const override { return "Test"; }
    String getCueType() const override { return "Test"; }
    static TestCue* create(var params) { return new TestCue(params); }

    void play();
};
