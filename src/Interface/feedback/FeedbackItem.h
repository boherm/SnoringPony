/*
  ==============================================================================

    FeedbackItem.h
    Created: 20 Apr 2026
    Author:  boherm

    Abstract base class for cuelist state feedback items.
    Uses ContainerAsyncListener for safe async notifications.

  ==============================================================================
*/

#pragma once

#include "../../MainIncludes.h"

class Cuelist;

class FeedbackItem :
    public BaseItem,
    public ContainerAsyncListener
{
public:
    FeedbackItem(const String& name = "Feedback");
    virtual ~FeedbackItem();

    BoolParameter* useMainCuelist;
    TargetParameter* targetCuelist;

    Cuelist* resolvedCuelist = nullptr;

    Cuelist* resolveCuelist();
    void bindCuelist();
    void unbindCuelist();

    void onContainerParameterChangedInternal(Parameter* p) override;

    // ContainerAsyncListener
    void newMessage(const ContainerAsyncEvent& e) override;

    virtual void onCuelistBound() {}
    virtual void onCuelistUnbound() {}
    virtual void sendFeedback() {}

    // Which parameter name this feedback monitors (set by subclasses)
    String monitoredParameterName;
    bool refreshOnDescriptionChange = false;

    String getTypeString() const override { return "Feedback"; }
    static FeedbackItem* create(var params) { return nullptr; }
};
