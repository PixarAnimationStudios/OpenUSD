//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Verifies the two pieces of HgiExternalBufferArena that are pure CPU
// bookkeeping around a GPU contract:
//
//  1. RECLAIM ORDERING -- a buffer nobody references any more is released only
//     after the GPU work that could still name it has retired.
//
//  2. THE SYNCHRONIZATION CONTRACT -- the epoch guard that decides whether a
//     given call actually encodes a wait or a signal. Getting this wrong does
//     not corrupt one buffer; it either hangs the frame on a semaphore nobody
//     will signal, or lets the application overwrite memory a draw has not
//     read yet.
//
// Neither needs a GPU or an Hgi, and for the same underlying reason: what is
// being tested is a decision, not its effect. For reclaim the decision cannot
// even be observed on a real device -- on an idle GPU the stamp taken in the
// first stage of GarbageCollect() has already retired by the second stage of
// the SAME pass, so the pending state never exists and any assertion about it
// would be an assertion about a race. Stubbing the arena's two backend hooks
// puts the retire clock under the test's control instead, and stubbing the
// semaphores turns "what would have been encoded" into something countable.
//
// What the reclaim ordering buys, and therefore what is worth pinning down
// here: the moment a weak reference expires is the moment the buffer is
// genuinely safe to recycle. A producer can rely on that; it could not rely on
// a reference count reaching zero, which says only that the CPU let go.
//
// What the sync contract buys is that repetition is the arena's problem rather
// than the application's. Hgi calls the wait from StartFrame and the signal
// from EndFrame over every arena it owns, unconditionally, and the guard is
// what makes an unconditional call correct when the application published
// nothing, or published once and is drawn several times.

#include "pxr/imaging/hgi/externalBuffer.h"
#include "pxr/imaging/hgi/externalBufferArena.h"
#include "pxr/imaging/hgi/semaphore.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

class _StubArena;

// The minimum an HgiExternalBuffer can be: it exists to be reference counted.
// GetBuffer() returns an empty handle because nothing here binds anything.
class _StubBuffer final : public HgiExternalBuffer
{
public:
    _StubBuffer(_StubArena *arena, size_t byteSize);

    HgiBufferHandle GetBuffer() const override {
        return HgiBufferHandle();
    }
};

// A semaphore that records what the arena asked it to encode instead of
// encoding it. The arena's contract is expressed entirely in these calls --
// whether one happened, with which value, and covering which buffers -- so
// counting them is the whole assertion.
class _StubSemaphore final : public HgiSemaphore
{
public:
    explicit _StubSemaphore(HgiSemaphoreKind kind = HgiSemaphoreKindBinary)
        : HgiSemaphore(kind)
    {
    }

    void EncodeWait(
        uint64_t value,
        std::vector<HgiExternalBuffer *> const &buffers) override
    {
        ++numWaits;
        lastValue = value;
        lastNumBuffers = buffers.size();
    }

    void EncodeSignal(
        uint64_t value,
        std::vector<HgiExternalBuffer *> const &buffers) override
    {
        ++numSignals;
        lastValue = value;
        lastNumBuffers = buffers.size();
    }

    int numWaits = 0;
    int numSignals = 0;
    uint64_t lastValue = 0;
    size_t lastNumBuffers = 0;
};

// An arena whose notion of "the GPU has caught up" is a counter the test
// moves. Stamps are handed out in order, so `retiredUpTo` is a watermark:
// everything stamped at or below it is done.
class _StubArena final : public HgiExternalBufferArena
{
public:
    _StubArena()
        // No Hgi: nothing below reaches through it. In particular this arena
        // never calls _GetNextBufferHandleId(), which would.
        : HgiExternalBufferArena(nullptr)
    {
    }

    // _SetSemaphores is protected, and in production is reached through a
    // backend's ImportSemaphores / CreateSemaphores. Exposed here for the same
    // reason the retire clock is: to put the backend's side of the contract
    // under the test's control.
    void SetSemaphoresForTest(
        HgiSemaphoreSharedPtr appDone,
        HgiSemaphoreSharedPtr hgiDone)
    {
        _SetSemaphores(std::move(appDone), std::move(hgiDone));
    }

    // Protected on the base, because Hgi is the only thing that may encode the
    // bracket -- it is a friend of the arena, and an application deliberately
    // is not. A test has to stand in for Hgi, and a using-declaration is the
    // narrowest way to do it: it re-widens exactly these two, on this subclass
    // only, and introduces no second name that could drift from the one
    // production calls.
    using HgiExternalBufferArena::EncodeAppDoneWait;
    using HgiExternalBufferArena::EncodeHgiDoneSignal;

    HgiExternalBufferSharedPtr AllocateBuffer(
        size_t byteSize,
        HgiBufferUsage /*usage*/,
        std::string const & /*debugName*/) override
    {
        return _Register(
            HgiExternalBufferSharedPtr(new _StubBuffer(this, byteSize)));
    }

    // Everything stamped at or below this is retired.
    uint64_t retiredUpTo = 0;

    // How many stamps have been handed out. One per GarbageCollect() pass that
    // retired anything -- never one per buffer.
    uint64_t numStamps = 0;

protected:
    uint64_t _CaptureSubmissionStamp() override {
        return ++numStamps;
    }

    bool _IsSubmissionRetired(uint64_t stamp) override {
        return stamp <= retiredUpTo;
    }
};

_StubBuffer::_StubBuffer(_StubArena *arena, size_t byteSize)
    : HgiExternalBuffer(arena, byteSize)
{
}

// A buffer nobody else references is not destroyed when the last outside
// reference drops -- only GarbageCollect() releases the arena's own, and only
// once the work that could name it has retired.
bool
_TestReleaseWaitsForRetire()
{
    _StubArena arena;

    HgiExternalBufferSharedPtr buffer = arena.AllocateBuffer(
        1024, /*usage*/ 0, "test");
    if (!TF_VERIFY(buffer)) {
        return false;
    }
    const HgiExternalBufferWeakPtr weak = buffer;

    TF_AXIOM(arena.GetUsage().numBuffers == 1);
    TF_AXIOM(arena.GetUsage().totalByteSize == 1024);

    // The consumer and the producer both let go. Nothing has been collected,
    // so the arena's own reference is all that is left.
    buffer.reset();
    TF_AXIOM(!weak.expired());
    TF_AXIOM(arena.GetUsage().numBuffers == 1);

    // First pass: the arena notices and stamps it, but the GPU has not caught
    // up, so it stays alive. It now counts as pending rather than live.
    arena.GarbageCollect();
    TF_AXIOM(!weak.expired());
    TF_AXIOM(arena.GetUsage().numBuffers == 0);
    TF_AXIOM(arena.GetUsage().numPendingDestroy == 1);
    TF_AXIOM(arena.GetUsage().totalByteSize == 1024);
    TF_AXIOM(arena.numStamps == 1);

    // However many passes run, an unretired buffer is pinned. A premature
    // release here is exactly the use-after-free the deferral exists to stop.
    for (int i = 0; i < 16; ++i) {
        arena.GarbageCollect();
    }
    TF_AXIOM(!weak.expired());
    TF_AXIOM(arena.GetUsage().numPendingDestroy == 1);
    // Idle passes retire nothing, so they take no new stamps either.
    TF_AXIOM(arena.numStamps == 1);

    // The work retires. The next pass, and only then, releases it.
    arena.retiredUpTo = arena.numStamps;
    arena.GarbageCollect();
    TF_AXIOM(weak.expired());
    TF_AXIOM(arena.GetUsage().numPendingDestroy == 0);
    TF_AXIOM(arena.GetUsage().numBuffers == 0);
    TF_AXIOM(arena.GetUsage().totalByteSize == 0);

    return true;
}

// A buffer somebody is still using is not a candidate at all: it is never
// stamped and never becomes pending, however often collection runs.
bool
_TestInUseBufferIsUntouched()
{
    _StubArena arena;

    HgiExternalBufferSharedPtr buffer = arena.AllocateBuffer(
        512, /*usage*/ 0, "in use");
    if (!TF_VERIFY(buffer)) {
        return false;
    }

    // Retirement is not the gate here -- being referenced is. Say the GPU is
    // fully caught up, so that only the reference can be keeping it.
    arena.retiredUpTo = 1000;
    for (int i = 0; i < 8; ++i) {
        arena.GarbageCollect();
    }

    TF_AXIOM(arena.GetUsage().numBuffers == 1);
    TF_AXIOM(arena.GetUsage().numPendingDestroy == 0);
    TF_AXIOM(arena.numStamps == 0);

    // Once it is released it follows the ordinary path, and the GPU is already
    // caught up, so one pass both stamps and releases it.
    const HgiExternalBufferWeakPtr weak = buffer;
    buffer.reset();
    arena.GarbageCollect();
    TF_AXIOM(weak.expired());
    TF_AXIOM(arena.numStamps == 1);

    return true;
}

// Everything retired in one pass shares a single stamp. Per-buffer stamping
// would be correct but wasteful: on OpenGL each stamp is a glFenceSync.
bool
_TestOnePassTakesOneStamp()
{
    _StubArena arena;

    HgiExternalBufferSharedPtr a = arena.AllocateBuffer(16, 0, "a");
    HgiExternalBufferSharedPtr b = arena.AllocateBuffer(32, 0, "b");
    HgiExternalBufferSharedPtr c = arena.AllocateBuffer(64, 0, "c");
    if (!TF_VERIFY(a && b && c)) {
        return false;
    }
    const HgiExternalBufferWeakPtr weakA = a;
    const HgiExternalBufferWeakPtr weakC = c;

    // Two of the three are dropped together.
    a.reset();
    c.reset();
    arena.GarbageCollect();
    TF_AXIOM(arena.numStamps == 1);
    TF_AXIOM(arena.GetUsage().numPendingDestroy == 2);
    TF_AXIOM(arena.GetUsage().numBuffers == 1);      // b is still held
    TF_AXIOM(!weakA.expired());
    TF_AXIOM(!weakC.expired());

    // The third goes later, so it belongs to a later batch with its own stamp.
    const HgiExternalBufferWeakPtr weakB = b;
    b.reset();
    arena.GarbageCollect();
    TF_AXIOM(arena.numStamps == 2);
    TF_AXIOM(arena.GetUsage().numPendingDestroy == 3);

    // Retiring the first batch releases exactly that batch.
    arena.retiredUpTo = 1;
    arena.GarbageCollect();
    TF_AXIOM(weakA.expired());
    TF_AXIOM(weakC.expired());
    TF_AXIOM(!weakB.expired());
    TF_AXIOM(arena.GetUsage().numPendingDestroy == 1);

    arena.retiredUpTo = 2;
    arena.GarbageCollect();
    TF_AXIOM(weakB.expired());
    TF_AXIOM(arena.GetUsage().numPendingDestroy == 0);

    return true;
}

// An arena that goes away while buffers are still pending must not leak them,
// even though they never retired. Teardown is the one point where waiting is
// not an option.
bool
_TestDestructionReleasesPending()
{
    HgiExternalBufferWeakPtr weak;
    {
        _StubArena arena;
        HgiExternalBufferSharedPtr buffer =
            arena.AllocateBuffer(128, 0, "pending at teardown");
        if (!TF_VERIFY(buffer)) {
            return false;
        }
        weak = buffer;
        buffer.reset();
        arena.GarbageCollect();        // stamped, not retired
        TF_AXIOM(!weak.expired());
    }
    TF_AXIOM(weak.expired());
    return true;
}

// A keepalive is released when the buffer is destroyed -- which, because the
// arena drops its own reference only after the retire test, means after the
// GPU has finished with the buffer.
//
// That ordering is the whole point of the mechanism, and the reason it is
// worth a test of its own. A producer hands over a reference to its own
// buffer object precisely so that object outlives Hgi's use of it; a release
// at reference-count zero would be too early, because the count falling only
// says the CPU let go. Nothing else here would notice if that regressed.
bool
_TestKeepaliveReleasedAfterRetire()
{
    _StubArena arena;

    bool released = false;
    {
        HgiExternalBufferSharedPtr buffer = arena.AllocateBuffer(
            256, /*usage*/ 0, "keepalive");
        if (!TF_VERIFY(buffer)) {
            return false;
        }

        // Stands in for the producer's own reference-counted buffer object.
        // Hgi never looks inside a keepalive -- it is shared_ptr<void> -- so a
        // payload whose deleter records its own destruction is faithful.
        std::shared_ptr<int> payload(new int(0), [&released](int *p) {
            released = true;
            delete p;
        });
        buffer->SetKeepalive(payload);
        TF_AXIOM(buffer->GetKeepalive() != nullptr);

        // The producer lets go of both the buffer and its own handle on the
        // payload, so from here the buffer is the only thing holding it.
        payload.reset();
        buffer.reset();
        TF_AXIOM(!released);

        // Stamped but not retired: the buffer is still alive, so what it holds
        // must still be alive too.
        arena.GarbageCollect();
        TF_AXIOM(!released);

        // And stays so for as long as the work has not retired, however many
        // collection passes run in the meantime.
        for (int i = 0; i < 8; ++i) {
            arena.GarbageCollect();
        }
        TF_AXIOM(!released);

        // The work retires. The next pass destroys the buffer, which drops the
        // last reference to the payload -- and only now may the producer's
        // object go.
        arena.retiredUpTo = arena.numStamps;
        arena.GarbageCollect();
        TF_AXIOM(released);
    }

    return true;
}

// ---------------------------------------------------------------------------
// The synchronization contract.
// ---------------------------------------------------------------------------

// An arena with no semaphore pair encodes nothing and signals nothing, however
// the application behaves.
//
// This is not a degenerate case, it is the common one: a producer sharing the
// consumer's own context or queue needs no semaphores, because command order
// already sequences the accesses. Two things have to hold. 
// The wait must be a no-op rather than a wait on a null semaphore, 
// and the signal must report that it did nothing --
// Hgi skips its queue flush on that false, so an application sharing without
// semaphores does not pay a submission per frame for a call Hgi makes
// unconditionally.
bool
_TestNoSemaphoresEncodesNothing()
{
    _StubArena arena;

    TF_AXIOM(arena.GetAppDoneHgiSemaphore() == nullptr);
    TF_AXIOM(arena.GetHgiDoneHgiSemaphore() == nullptr);

    // Even having published: there is nothing to wait on and nothing to tell.
    arena.NotifyAppDone();
    arena.EncodeAppDoneWait();
    TF_AXIOM(!arena.EncodeHgiDoneSignal());

    // And repeatedly, since Hgi will call both every frame regardless.
    for (int i = 0; i < 4; ++i) {
        arena.EncodeAppDoneWait();
        TF_AXIOM(!arena.EncodeHgiDoneSignal());
    }

    return true;
}

// An application that published nothing this frame is waited for by nobody.
//
// This is the assertion that keeps an unconditional call in StartFrame safe.
// A binary semaphore has no "already signalled" state to observe, so a wait
// encoded when the application never signalled does not resolve late -- it
// does not resolve at all, and the frame hangs. An idle viewport is the normal
// case, not an edge case, so this path runs far more often than the busy one.
bool
_TestIdleFrameNeitherWaitsNorSignals()
{
    _StubArena arena;

    auto appDone = std::make_shared<_StubSemaphore>();
    auto hgiDone = std::make_shared<_StubSemaphore>();
    arena.SetSemaphoresForTest(appDone, hgiDone);

    // Several frames of a client dutifully calling both hooks while the
    // application publishes nothing.
    for (int i = 0; i < 4; ++i) {
        arena.EncodeAppDoneWait();
        TF_AXIOM(!arena.EncodeHgiDoneSignal());
    }

    TF_AXIOM(appDone->numWaits == 0);
    TF_AXIOM(hgiDone->numSignals == 0);

    return true;
}

// One publish yields exactly one wait and exactly one signal, no matter how
// many times either is called.
//
// This is what the epoch guard is for. A client runs several render passes per
// frame -- shadow, main, pick -- and may run several HdEngine::Execute() calls
// per refresh, so both halves get called more than once for a single publish.
// The second and later calls must be silent: a repeated wait blocks on a
// signal that is not coming, and a repeated signal on a binary semaphore
// leaves it signalled with nobody waiting, which the next frame then consumes
// as if it meant something.
bool
_TestOnePublishCollapsesToOneWaitAndOneSignal()
{
    _StubArena arena;

    auto appDone = std::make_shared<_StubSemaphore>();
    auto hgiDone = std::make_shared<_StubSemaphore>();
    arena.SetSemaphoresForTest(appDone, hgiDone);

    // Two live buffers, so the barrier list the arena passes down is
    // observable. GL needs that list: glWaitSemaphoreEXT and
    // glSignalSemaphoreEXT establish coherency for the objects named in it,
    // and an imported buffer left out of it is not made visible.
    HgiExternalBufferSharedPtr a = arena.AllocateBuffer(64, 0, "a");
    HgiExternalBufferSharedPtr b = arena.AllocateBuffer(64, 0, "b");
    if (!TF_VERIFY(a && b)) {
        return false;
    }

    arena.NotifyAppDone();

    for (int i = 0; i < 5; ++i) {
        arena.EncodeAppDoneWait();
    }
    TF_AXIOM(appDone->numWaits == 1);
    TF_AXIOM(appDone->lastNumBuffers == 2);

    // First signal reports that it did something; the rest report that they
    // did not, and encode nothing.
    TF_AXIOM(arena.EncodeHgiDoneSignal());
    TF_AXIOM(hgiDone->numSignals == 1);
    TF_AXIOM(hgiDone->lastNumBuffers == 2);

    for (int i = 0; i < 3; ++i) {
        TF_AXIOM(!arena.EncodeHgiDoneSignal());
    }
    TF_AXIOM(hgiDone->numSignals == 1);

    // A signal consumes the epoch, so later frames with no new publish are
    // back to the idle case above.
    for (int i = 0; i < 3; ++i) {
        arena.EncodeAppDoneWait();
    }
    TF_AXIOM(appDone->numWaits == 1);

    return true;
}

// A signal is only ever encoded for an epoch that was actually waited for.
//
// The pairing is what makes the count come out right: the application waits
// for one hgi-done per publish, so a signal without a preceding wait is one
// signal too many. It is also the mechanism behind the documented requirement
// that a producer publish BEFORE StartFrame -- see the slip test below. Worth
// asserting on its own, because the guard reads like a redundant safety check
// and removing it would look like a simplification.
bool
_TestSignalRequiresAPrecedingWait()
{
    _StubArena arena;

    auto appDone = std::make_shared<_StubSemaphore>();
    auto hgiDone = std::make_shared<_StubSemaphore>();
    arena.SetSemaphoresForTest(appDone, hgiDone);

    arena.NotifyAppDone();

    // Published, but nothing has read it yet. Claiming the reads are finished
    // here would be a lie in the dangerous direction.
    TF_AXIOM(!arena.EncodeHgiDoneSignal());
    TF_AXIOM(hgiDone->numSignals == 0);

    arena.EncodeAppDoneWait();
    TF_AXIOM(appDone->numWaits == 1);

    TF_AXIOM(arena.EncodeHgiDoneSignal());
    TF_AXIOM(hgiDone->numSignals == 1);

    return true;
}

// A publish that lands after the frame's wait has been encoded is picked up by
// the NEXT frame, not this one.
//
// This is the cost of encoding the wait in StartFrame, and it is asserted here
// rather than left to be discovered. A producer that calls NotifyAppDone()
// after StartFrame -- an adapter registering a buffer during Sync, say -- gets
// no wait this frame, and because the signal only fires for an epoch that was
// waited for, no signal at the end of it either. Its next wait for hgi-done
// therefore blocks until the following frame's EndFrame.
//
// With frames continuing that is added latency. With a bounded wait it is a
// hard failure, which is why the requirement is stated in the API docs rather
// than left implicit. What must NOT happen is the epoch being dropped
// altogether, and that is the last third of this test.
bool
_TestPublishAfterWaitSlipsOneFrame()
{
    _StubArena arena;

    auto appDone = std::make_shared<_StubSemaphore>();
    auto hgiDone = std::make_shared<_StubSemaphore>();
    arena.SetSemaphoresForTest(appDone, hgiDone);

    // Frame 1, done correctly: publish, then the frame.
    arena.NotifyAppDone();
    arena.EncodeAppDoneWait();
    TF_AXIOM(arena.EncodeHgiDoneSignal());
    TF_AXIOM(appDone->numWaits == 1);
    TF_AXIOM(hgiDone->numSignals == 1);

    // Frame 2, done late. StartFrame finds nothing new to wait for...
    arena.EncodeAppDoneWait();
    TF_AXIOM(appDone->numWaits == 1);

    // ...the producer publishes in the middle of the frame...
    arena.NotifyAppDone();

    // ...and EndFrame cannot honestly signal, because nothing waited for this
    // epoch and so nothing in this frame was ordered against it.
    TF_AXIOM(!arena.EncodeHgiDoneSignal());
    TF_AXIOM(hgiDone->numSignals == 1);

    // Frame 3 picks the epoch up. The publish is delayed, never lost.
    arena.EncodeAppDoneWait();
    TF_AXIOM(appDone->numWaits == 2);
    TF_AXIOM(arena.EncodeHgiDoneSignal());
    TF_AXIOM(hgiDone->numSignals == 2);

    return true;
}

// The published value survives the round trip: the value the application
// passed to NotifyAppDone is the value the wait is encoded with, and each
// signal carries a distinct increasing one.
//
// NOTE: no backend consumes these values today, and this test is the only
// thing holding the plumbing correct. Binary semaphores ignore the value by
// definition, and binary is all any backend currently accepts --
// HgiVulkanSemaphore encodes through the command queue's pending wait and
// signal lists, which carry no values, and GL_EXT_semaphore has no timeline
// form at all. Both Vulkan creation paths now refuse a timeline kind outright.
//
// So this guards an interface contract ahead of its implementation, which is
// worth doing deliberately rather than by accident: the arena's half of
// timeline support is finished and cheap to keep working, and if the queue
// ever learns to carry values it should not also need the arena rewritten.
// The stubs below declare themselves timeline purely to say that values are
// the point of them -- SetSemaphoresForTest bypasses creation, so nothing here
// depends on a kind production can currently produce.
bool
_TestPublishedValuesAreCarried()
{
    _StubArena arena;

    auto appDone = std::make_shared<_StubSemaphore>(HgiSemaphoreKindTimeline);
    auto hgiDone = std::make_shared<_StubSemaphore>(HgiSemaphoreKindTimeline);
    arena.SetSemaphoresForTest(appDone, hgiDone);

    TF_AXIOM(arena.GetAppDoneHgiSemaphore()->GetKind() ==
             HgiSemaphoreKindTimeline);
    TF_AXIOM(arena.GetHgiDoneHgiSemaphore()->GetKind() ==
             HgiSemaphoreKindTimeline);

    // The application's value is passed through untouched -- the arena carries
    // it from NotifyAppDone to the wait and does not interpret it.
    arena.NotifyAppDone(42);
    arena.EncodeAppDoneWait();
    TF_AXIOM(appDone->lastValue == 42);

    // Hgi's own values are the arena's to choose, and must increase so a
    // timeline waiter can tell one frame's completion from another's.
    TF_AXIOM(arena.EncodeHgiDoneSignal());
    TF_AXIOM(hgiDone->lastValue == 1);

    arena.NotifyAppDone(43);
    arena.EncodeAppDoneWait();
    TF_AXIOM(appDone->lastValue == 43);
    TF_AXIOM(arena.EncodeHgiDoneSignal());
    TF_AXIOM(hgiDone->lastValue == 2);

    // Non-monotonic application values are the application's business, not
    // the arena's: it reports what it was given.
    arena.NotifyAppDone(7);
    arena.EncodeAppDoneWait();
    TF_AXIOM(appDone->lastValue == 7);
    TF_AXIOM(arena.EncodeHgiDoneSignal());
    TF_AXIOM(hgiDone->lastValue == 3);

    TF_AXIOM(appDone->numWaits == 3);
    TF_AXIOM(hgiDone->numSignals == 3);

    return true;
}

} // anonymous namespace

int
main(int /*argc*/, char ** /*argv*/)
{
    TfErrorMark mark;

    // Reclaim ordering.
    bool success = _TestReleaseWaitsForRetire();
    success = success && _TestInUseBufferIsUntouched();
    success = success && _TestOnePassTakesOneStamp();
    success = success && _TestDestructionReleasesPending();
    success = success && _TestKeepaliveReleasedAfterRetire();

    // The synchronization contract.
    success = success && _TestNoSemaphoresEncodesNothing();
    success = success && _TestIdleFrameNeitherWaitsNorSignals();
    success = success && _TestOnePublishCollapsesToOneWaitAndOneSignal();
    success = success && _TestSignalRequiresAPrecedingWait();
    success = success && _TestPublishAfterWaitSlipsOneFrame();
    success = success && _TestPublishedValuesAreCarried();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    }
    std::cout << "FAILED" << std::endl;
    return EXIT_FAILURE;
}
