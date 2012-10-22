// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "CCThreadedTest.h"

#include "CCActiveAnimation.h"
#include "CCInputHandler.h"
#include "CCLayerAnimationController.h"
#include "CCLayerImpl.h"
#include "base/stl_util.h"
#include "cc/content_layer.h"
#include "cc/layer.h"
#include "cc/layer_tree_host_impl.h"
#include "cc/scoped_thread_proxy.h"
#include "cc/settings.h"
#include "cc/single_thread_proxy.h"
#include "cc/test/animation_test_common.h"
#include "cc/test/fake_web_compositor_output_surface.h"
#include "cc/test/fake_web_graphics_context_3d.h"
#include "cc/test/occlusion_tracker_test_common.h"
#include "cc/test/test_common.h"
#include "cc/test/tiled_layer_test_common.h"
#include "cc/thread_task.h"
#include "cc/timing_function.h"
#include "testing/gmock/include/gmock/gmock.h"
#include <public/Platform.h>
#include <public/WebCompositorSupport.h>
#include <public/WebFilterOperation.h>
#include <public/WebFilterOperations.h>
#include <public/WebThread.h>

using namespace cc;
using namespace WebKit;

namespace WebKitTests {

scoped_ptr<CompositorFakeWebGraphicsContext3DWithTextureTracking> CompositorFakeWebGraphicsContext3DWithTextureTracking::create(Attributes attrs)
{
    return make_scoped_ptr(new CompositorFakeWebGraphicsContext3DWithTextureTracking(attrs));
}

WebGLId CompositorFakeWebGraphicsContext3DWithTextureTracking::createTexture()
{
    WebGLId texture = m_textures.size() + 1;
    m_textures.append(texture);
    return texture;
}

void CompositorFakeWebGraphicsContext3DWithTextureTracking::deleteTexture(WebGLId texture)
{
    for (size_t i = 0; i < m_textures.size(); i++) {
        if (m_textures[i] == texture) {
            m_textures.remove(i);
            break;
        }
    }
}

void CompositorFakeWebGraphicsContext3DWithTextureTracking::bindTexture(WGC3Denum /* target */, WebGLId texture)
{
    m_usedTextures.insert(texture);
}

int CompositorFakeWebGraphicsContext3DWithTextureTracking::numTextures() const { return static_cast<int>(m_textures.size()); }
int CompositorFakeWebGraphicsContext3DWithTextureTracking::texture(int i) const { return m_textures[i]; }
void CompositorFakeWebGraphicsContext3DWithTextureTracking::resetTextures() { m_textures.clear(); }

int CompositorFakeWebGraphicsContext3DWithTextureTracking::numUsedTextures() const { return static_cast<int>(m_usedTextures.size()); }
bool CompositorFakeWebGraphicsContext3DWithTextureTracking::usedTexture(int texture) const
{
  return ContainsKey(m_usedTextures, texture);
}

void CompositorFakeWebGraphicsContext3DWithTextureTracking::resetUsedTextures() { m_usedTextures.clear(); }

CompositorFakeWebGraphicsContext3DWithTextureTracking::CompositorFakeWebGraphicsContext3DWithTextureTracking(Attributes attrs) : CompositorFakeWebGraphicsContext3D(attrs)
{
}

CompositorFakeWebGraphicsContext3DWithTextureTracking::~CompositorFakeWebGraphicsContext3DWithTextureTracking()
{
}

bool TestHooks::prepareToDrawOnCCThread(cc::CCLayerTreeHostImpl*)
{
    return true;
}

scoped_ptr<WebCompositorOutputSurface> TestHooks::createOutputSurface()
{
    return FakeWebCompositorOutputSurface::create(CompositorFakeWebGraphicsContext3DWithTextureTracking::create(WebGraphicsContext3D::Attributes()).PassAs<WebKit::WebGraphicsContext3D>()).PassAs<WebKit::WebCompositorOutputSurface>();
}

scoped_ptr<MockLayerTreeHostImpl> MockLayerTreeHostImpl::create(TestHooks* testHooks, const CCLayerTreeSettings& settings, CCLayerTreeHostImplClient* client)
{
    return make_scoped_ptr(new MockLayerTreeHostImpl(testHooks, settings, client));
}

void MockLayerTreeHostImpl::beginCommit()
{
    CCLayerTreeHostImpl::beginCommit();
    m_testHooks->beginCommitOnCCThread(this);
}

void MockLayerTreeHostImpl::commitComplete()
{
    CCLayerTreeHostImpl::commitComplete();
    m_testHooks->commitCompleteOnCCThread(this);
}

bool MockLayerTreeHostImpl::prepareToDraw(FrameData& frame)
{
    bool result = CCLayerTreeHostImpl::prepareToDraw(frame);
    if (!m_testHooks->prepareToDrawOnCCThread(this))
        result = false;
    return result;
}

void MockLayerTreeHostImpl::drawLayers(const FrameData& frame)
{
    CCLayerTreeHostImpl::drawLayers(frame);
    m_testHooks->drawLayersOnCCThread(this);
}

void MockLayerTreeHostImpl::animateLayers(double monotonicTime, double wallClockTime)
{
    m_testHooks->willAnimateLayers(this, monotonicTime);
    CCLayerTreeHostImpl::animateLayers(monotonicTime, wallClockTime);
    m_testHooks->animateLayers(this, monotonicTime);
}

base::TimeDelta MockLayerTreeHostImpl::lowFrequencyAnimationInterval() const
{
    return base::TimeDelta::FromMilliseconds(16);
}

MockLayerTreeHostImpl::MockLayerTreeHostImpl(TestHooks* testHooks, const CCLayerTreeSettings& settings, CCLayerTreeHostImplClient* client)
    : CCLayerTreeHostImpl(settings, client)
    , m_testHooks(testHooks)
{
}

// Adapts CCLayerTreeHost for test. Injects MockLayerTreeHostImpl.
class MockLayerTreeHost : public cc::CCLayerTreeHost {
public:
    static scoped_ptr<MockLayerTreeHost> create(TestHooks* testHooks, cc::CCLayerTreeHostClient* client, scoped_refptr<cc::LayerChromium> rootLayer, const cc::CCLayerTreeSettings& settings)
    {
        scoped_ptr<MockLayerTreeHost> layerTreeHost(new MockLayerTreeHost(testHooks, client, settings));
        bool success = layerTreeHost->initialize();
        EXPECT_TRUE(success);
        layerTreeHost->setRootLayer(rootLayer);

        // LayerTreeHostImpl won't draw if it has 1x1 viewport.
        layerTreeHost->setViewportSize(IntSize(1, 1), IntSize(1, 1));

        layerTreeHost->rootLayer()->setLayerAnimationDelegate(testHooks);

        return layerTreeHost.Pass();
    }

    virtual scoped_ptr<cc::CCLayerTreeHostImpl> createLayerTreeHostImpl(cc::CCLayerTreeHostImplClient* client)
    {
        return MockLayerTreeHostImpl::create(m_testHooks, settings(), client).PassAs<cc::CCLayerTreeHostImpl>();
    }

    virtual void didAddAnimation() OVERRIDE
    {
        CCLayerTreeHost::didAddAnimation();
        m_testHooks->didAddAnimation();
    }

    virtual void setNeedsCommit() OVERRIDE
    {
        if (!m_testStarted)
            return;
        CCLayerTreeHost::setNeedsCommit();
    }

    void setTestStarted(bool started) { m_testStarted = started; }

private:
    MockLayerTreeHost(TestHooks* testHooks, cc::CCLayerTreeHostClient* client, const cc::CCLayerTreeSettings& settings)
        : CCLayerTreeHost(client, settings)
        , m_testHooks(testHooks)
        , m_testStarted(false)
    {
    }

    TestHooks* m_testHooks;
    bool m_testStarted;
};

// Implementation of CCLayerTreeHost callback interface.
class MockLayerTreeHostClient : public MockCCLayerTreeHostClient {
public:
    static scoped_ptr<MockLayerTreeHostClient> create(TestHooks* testHooks)
    {
        return make_scoped_ptr(new MockLayerTreeHostClient(testHooks));
    }

    virtual void willBeginFrame() OVERRIDE
    {
    }

    virtual void didBeginFrame() OVERRIDE
    {
    }

    virtual void animate(double monotonicTime) OVERRIDE
    {
        m_testHooks->animate(monotonicTime);
    }

    virtual void layout() OVERRIDE
    {
        m_testHooks->layout();
    }

    virtual void applyScrollAndScale(const IntSize& scrollDelta, float scale) OVERRIDE
    {
        m_testHooks->applyScrollAndScale(scrollDelta, scale);
    }

    virtual scoped_ptr<WebCompositorOutputSurface> createOutputSurface() OVERRIDE
    {
        return m_testHooks->createOutputSurface();
    }

    virtual void didRecreateOutputSurface(bool succeeded) OVERRIDE
    {
        m_testHooks->didRecreateOutputSurface(succeeded);
    }

    virtual scoped_ptr<CCInputHandler> createInputHandler() OVERRIDE
    {
        return scoped_ptr<CCInputHandler>();
    }

    virtual void willCommit() OVERRIDE
    {
    }

    virtual void didCommit() OVERRIDE
    {
        m_testHooks->didCommit();
    }

    virtual void didCommitAndDrawFrame() OVERRIDE
    {
        m_testHooks->didCommitAndDrawFrame();
    }

    virtual void didCompleteSwapBuffers() OVERRIDE
    {
    }

    virtual void scheduleComposite() OVERRIDE
    {
        m_testHooks->scheduleComposite();
    }

private:
    explicit MockLayerTreeHostClient(TestHooks* testHooks) : m_testHooks(testHooks) { }

    TestHooks* m_testHooks;
};

class TimeoutTask : public WebThread::Task {
public:
    explicit TimeoutTask(CCThreadedTest* test)
        : m_test(test)
    {
    }

    void clearTest()
    {
        m_test = 0;
    }

    virtual ~TimeoutTask()
    {
        if (m_test)
            m_test->clearTimeout();
    }

    virtual void run()
    {
        if (m_test)
            m_test->timeout();
    }

private:
    CCThreadedTest* m_test;
};

class BeginTask : public WebThread::Task {
public:
    explicit BeginTask(CCThreadedTest* test)
        : m_test(test)
    {
    }

    virtual ~BeginTask() { }
    virtual void run()
    {
        m_test->doBeginTest();
    }
private:
    CCThreadedTest* m_test;
};

CCThreadedTest::CCThreadedTest()
    : m_beginning(false)
    , m_endWhenBeginReturns(false)
    , m_timedOut(false)
    , m_finished(false)
    , m_scheduled(false)
    , m_started(false)
{
}

CCThreadedTest::~CCThreadedTest()
{
}

void CCThreadedTest::endTest()
{
    m_finished = true;

    // For the case where we endTest during beginTest(), set a flag to indicate that
    // the test should end the second beginTest regains control.
    if (m_beginning)
        m_endWhenBeginReturns = true;
    else
        m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadedTest::realEndTest));
}

void CCThreadedTest::endTestAfterDelay(int delayMilliseconds)
{
    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadedTest::endTest));
}

void CCThreadedTest::postSetNeedsAnimateToMainThread()
{
    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadedTest::dispatchSetNeedsAnimate));
}

void CCThreadedTest::postAddAnimationToMainThread()
{
    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadedTest::dispatchAddAnimation));
}

void CCThreadedTest::postAddInstantAnimationToMainThread()
{
    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadedTest::dispatchAddInstantAnimation));
}

void CCThreadedTest::postSetNeedsCommitToMainThread()
{
    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadedTest::dispatchSetNeedsCommit));
}

void CCThreadedTest::postAcquireLayerTextures()
{
    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadedTest::dispatchAcquireLayerTextures));
}

void CCThreadedTest::postSetNeedsRedrawToMainThread()
{
    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadedTest::dispatchSetNeedsRedraw));
}

void CCThreadedTest::postSetNeedsAnimateAndCommitToMainThread()
{
    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadedTest::dispatchSetNeedsAnimateAndCommit));
}

void CCThreadedTest::postSetVisibleToMainThread(bool visible)
{
    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadedTest::dispatchSetVisible, visible));
}

void CCThreadedTest::postDidAddAnimationToMainThread()
{
    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadedTest::dispatchDidAddAnimation));
}

void CCThreadedTest::doBeginTest()
{
    DCHECK(CCProxy::isMainThread());
    m_client = MockLayerTreeHostClient::create(this);

    scoped_refptr<LayerChromium> rootLayer = LayerChromium::create();
    m_layerTreeHost = MockLayerTreeHost::create(this, m_client.get(), rootLayer, m_settings);
    ASSERT_TRUE(m_layerTreeHost.get());
    rootLayer->setLayerTreeHost(m_layerTreeHost.get());
    m_layerTreeHost->setSurfaceReady();

    m_started = true;
    m_beginning = true;
    beginTest();
    m_beginning = false;
    if (m_endWhenBeginReturns)
        realEndTest();

    // Allow commits to happen once beginTest() has had a chance to post tasks
    // so that those tasks will happen before the first commit.
    if (m_layerTreeHost)
        static_cast<MockLayerTreeHost*>(m_layerTreeHost.get())->setTestStarted(true);
}

void CCThreadedTest::timeout()
{
    m_timedOut = true;
    endTest();
}

void CCThreadedTest::scheduleComposite()
{
    if (!m_started || m_scheduled || m_finished)
        return;
    m_scheduled = true;
    m_mainThreadProxy->postTask(createCCThreadTask(this, &CCThreadedTest::dispatchComposite));
}

void CCThreadedTest::realEndTest()
{
    DCHECK(CCProxy::isMainThread());
    WebKit::Platform::current()->currentThread()->exitRunLoop();
}

void CCThreadedTest::dispatchSetNeedsAnimate()
{
    DCHECK(CCProxy::isMainThread());

    if (m_finished)
        return;

    if (m_layerTreeHost.get())
        m_layerTreeHost->setNeedsAnimate();
}

void CCThreadedTest::dispatchAddInstantAnimation()
{
    DCHECK(CCProxy::isMainThread());

    if (m_finished)
        return;

    if (m_layerTreeHost.get() && m_layerTreeHost->rootLayer())
        addOpacityTransitionToLayer(*m_layerTreeHost->rootLayer(), 0, 0, 0.5, false);
}

void CCThreadedTest::dispatchAddAnimation()
{
    DCHECK(CCProxy::isMainThread());

    if (m_finished)
        return;

    if (m_layerTreeHost.get() && m_layerTreeHost->rootLayer())
        addOpacityTransitionToLayer(*m_layerTreeHost->rootLayer(), 10, 0, 0.5, true);
}

void CCThreadedTest::dispatchSetNeedsAnimateAndCommit()
{
    DCHECK(CCProxy::isMainThread());

    if (m_finished)
        return;

    if (m_layerTreeHost.get()) {
        m_layerTreeHost->setNeedsAnimate();
        m_layerTreeHost->setNeedsCommit();
    }
}

void CCThreadedTest::dispatchSetNeedsCommit()
{
    DCHECK(CCProxy::isMainThread());

    if (m_finished)
        return;

    if (m_layerTreeHost.get())
        m_layerTreeHost->setNeedsCommit();
}

void CCThreadedTest::dispatchAcquireLayerTextures()
{
    DCHECK(CCProxy::isMainThread());

    if (m_finished)
        return;

    if (m_layerTreeHost.get())
        m_layerTreeHost->acquireLayerTextures();
}

void CCThreadedTest::dispatchSetNeedsRedraw()
{
    DCHECK(CCProxy::isMainThread());

    if (m_finished)
        return;

    if (m_layerTreeHost.get())
        m_layerTreeHost->setNeedsRedraw();
}

void CCThreadedTest::dispatchSetVisible(bool visible)
{
    DCHECK(CCProxy::isMainThread());

    if (m_finished)
        return;

    if (m_layerTreeHost.get())
        m_layerTreeHost->setVisible(visible);
}

void CCThreadedTest::dispatchComposite()
{
    m_scheduled = false;
    if (m_layerTreeHost.get() && !m_finished)
        m_layerTreeHost->composite();
}

void CCThreadedTest::dispatchDidAddAnimation()
{
    DCHECK(CCProxy::isMainThread());

    if (m_finished)
        return;

    if (m_layerTreeHost.get())
        m_layerTreeHost->didAddAnimation();
}

void CCThreadedTest::runTest(bool threaded)
{
    // For these tests, we will enable threaded animations.
    CCScopedSettings scopedSettings;
    Settings::setAcceleratedAnimationEnabled(true);

    if (threaded) {
        m_webThread.reset(WebKit::Platform::current()->createThread("CCThreadedTest"));
        Platform::current()->compositorSupport()->initialize(m_webThread.get());
    } else
        Platform::current()->compositorSupport()->initialize(0);

    DCHECK(CCProxy::isMainThread());
    m_mainThreadProxy = CCScopedThreadProxy::create(CCProxy::mainThread());

    initializeSettings(m_settings);

    m_beginTask = new BeginTask(this);
    WebKit::Platform::current()->currentThread()->postDelayedTask(m_beginTask, 0); // postDelayedTask takes ownership of the task
    m_timeoutTask = new TimeoutTask(this);
    WebKit::Platform::current()->currentThread()->postDelayedTask(m_timeoutTask, 5000);
    WebKit::Platform::current()->currentThread()->enterRunLoop();

    if (m_layerTreeHost.get() && m_layerTreeHost->rootLayer())
        m_layerTreeHost->rootLayer()->setLayerTreeHost(0);
    m_layerTreeHost.reset();

    if (m_timeoutTask)
        m_timeoutTask->clearTest();

    ASSERT_FALSE(m_layerTreeHost.get());
    m_client.reset();
    if (m_timedOut) {
        FAIL() << "Test timed out";
        Platform::current()->compositorSupport()->shutdown();
        return;
    }
    afterTest();
    Platform::current()->compositorSupport()->shutdown();
}

}  // namespace WebKitTests
