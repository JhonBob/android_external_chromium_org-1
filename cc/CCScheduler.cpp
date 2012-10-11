// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "CCScheduler.h"

#include "TraceEvent.h"
#include <base/auto_reset.h>

namespace cc {

CCScheduler::CCScheduler(CCSchedulerClient* client, scoped_ptr<CCFrameRateController> frameRateController)
    : m_client(client)
    , m_frameRateController(frameRateController.Pass())
    , m_insideProcessScheduledActions(false)
{
    ASSERT(m_client);
    m_frameRateController->setClient(this);
    ASSERT(!m_stateMachine.vsyncCallbackNeeded());
}

CCScheduler::~CCScheduler()
{
    m_frameRateController->setActive(false);
}

void CCScheduler::setCanBeginFrame(bool can)
{
    m_stateMachine.setCanBeginFrame(can);
    processScheduledActions();
}

void CCScheduler::setVisible(bool visible)
{
    m_stateMachine.setVisible(visible);
    processScheduledActions();
}

void CCScheduler::setCanDraw(bool canDraw)
{
    m_stateMachine.setCanDraw(canDraw);
    processScheduledActions();
}

void CCScheduler::setNeedsCommit()
{
    m_stateMachine.setNeedsCommit();
    processScheduledActions();
}

void CCScheduler::setNeedsForcedCommit()
{
    m_stateMachine.setNeedsForcedCommit();
    processScheduledActions();
}

void CCScheduler::setNeedsRedraw()
{
    m_stateMachine.setNeedsRedraw();
    processScheduledActions();
}

void CCScheduler::setNeedsForcedRedraw()
{
    m_stateMachine.setNeedsForcedRedraw();
    processScheduledActions();
}

void CCScheduler::setMainThreadNeedsLayerTextures()
{
    m_stateMachine.setMainThreadNeedsLayerTextures();
    processScheduledActions();
}

void CCScheduler::beginFrameComplete()
{
    TRACE_EVENT0("cc", "CCScheduler::beginFrameComplete");
    m_stateMachine.beginFrameComplete();
    processScheduledActions();
}

void CCScheduler::beginFrameAborted()
{
    TRACE_EVENT0("cc", "CCScheduler::beginFrameAborted");
    m_stateMachine.beginFrameAborted();
    processScheduledActions();
}

void CCScheduler::setMaxFramesPending(int maxFramesPending)
{
    m_frameRateController->setMaxFramesPending(maxFramesPending);
}

void CCScheduler::setSwapBuffersCompleteSupported(bool supported)
{
    m_frameRateController->setSwapBuffersCompleteSupported(supported);
}

void CCScheduler::didSwapBuffersComplete()
{
    TRACE_EVENT0("cc", "CCScheduler::didSwapBuffersComplete");
    m_frameRateController->didFinishFrame();
}

void CCScheduler::didLoseContext()
{
    TRACE_EVENT0("cc", "CCScheduler::didLoseContext");
    m_frameRateController->didAbortAllPendingFrames();
    m_stateMachine.didLoseContext();
    processScheduledActions();
}

void CCScheduler::didRecreateContext()
{
    TRACE_EVENT0("cc", "CCScheduler::didRecreateContext");
    m_stateMachine.didRecreateContext();
    processScheduledActions();
}

void CCScheduler::setTimebaseAndInterval(base::TimeTicks timebase, base::TimeDelta interval)
{
    m_frameRateController->setTimebaseAndInterval(timebase, interval);
}

base::TimeTicks CCScheduler::anticipatedDrawTime()
{
    return m_frameRateController->nextTickTime();
}

void CCScheduler::vsyncTick(bool throttled)
{
    TRACE_EVENT1("cc", "CCScheduler::vsyncTick", "throttled", throttled);
    if (!throttled)
        m_stateMachine.didEnterVSync();
    processScheduledActions();
    if (!throttled)
        m_stateMachine.didLeaveVSync();
}

void CCScheduler::processScheduledActions()
{
    // We do not allow processScheduledActions to be recursive.
    // The top-level call will iteratively execute the next action for us anyway.
    if (m_insideProcessScheduledActions)
        return;

    AutoReset<bool> markInside(&m_insideProcessScheduledActions, true);

    CCSchedulerStateMachine::Action action = m_stateMachine.nextAction();
    while (action != CCSchedulerStateMachine::ACTION_NONE) {
        m_stateMachine.updateState(action);
        TRACE_EVENT1("cc", "CCScheduler::processScheduledActions()", "action", action);

        switch (action) {
        case CCSchedulerStateMachine::ACTION_NONE:
            break;
        case CCSchedulerStateMachine::ACTION_BEGIN_FRAME:
            m_client->scheduledActionBeginFrame();
            break;
        case CCSchedulerStateMachine::ACTION_COMMIT:
            m_client->scheduledActionCommit();
            break;
        case CCSchedulerStateMachine::ACTION_DRAW_IF_POSSIBLE: {
            CCScheduledActionDrawAndSwapResult result = m_client->scheduledActionDrawAndSwapIfPossible();
            m_stateMachine.didDrawIfPossibleCompleted(result.didDraw);
            if (result.didSwap)
                m_frameRateController->didBeginFrame();
            break;
        }
        case CCSchedulerStateMachine::ACTION_DRAW_FORCED: {
            CCScheduledActionDrawAndSwapResult result = m_client->scheduledActionDrawAndSwapForced();
            if (result.didSwap)
                m_frameRateController->didBeginFrame();
            break;
        } case CCSchedulerStateMachine::ACTION_BEGIN_CONTEXT_RECREATION:
            m_client->scheduledActionBeginContextRecreation();
            break;
        case CCSchedulerStateMachine::ACTION_ACQUIRE_LAYER_TEXTURES_FOR_MAIN_THREAD:
            m_client->scheduledActionAcquireLayerTexturesForMainThread();
            break;
        }
        action = m_stateMachine.nextAction();
    }

    // Activate or deactivate the frame rate controller.
    m_frameRateController->setActive(m_stateMachine.vsyncCallbackNeeded());
    m_client->didAnticipatedDrawTimeChange(m_frameRateController->nextTickTime());
}

}  // namespace cc
