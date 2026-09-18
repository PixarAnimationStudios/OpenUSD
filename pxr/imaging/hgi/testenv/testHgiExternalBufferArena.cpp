//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Verifies HgiExternalBufferArena's reclaim ordering: that a buffer nobody
// references any more is released only after the GPU work that could still
// name it has retired.
//
// This needs no GPU and no Hgi. That is deliberate -- the property cannot be
// tested against a real device, because on an idle GPU the stamp taken in the
// first stage of GarbageCollect() has already retired by the second stage of
// the SAME pass, so the pending state is never observable and any assertion
// about it would be an assertion about a race. Stubbing the arena's two
// backend hooks puts the retire clock under the test's control instead, which
// makes the boundary exact.
//
// What the ordering buys, and therefore what is worth pinning down here: the
// moment a weak reference expires is the moment the buffer is genuinely safe
// to recycle. A producer can rely on that; it could not rely on a reference
// count reaching zero, which says only that the CPU let go.

#include "pxr/imaging/hgi/externalBuffer.h"
#include "pxr/imaging/hgi/externalBufferArena.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

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

// An arena whose notion of "the GPU has caught up" is a counter the test
// moves. Stamps are handed out in order, so `retiredUpTo` is a watermark:
// everything stamped at or below it is done.
class _StubArena final : public HgiExternalBufferArena
{
public:
    _StubArena()
        // No Hgi: nothing below reaches through it. In particular this arena
        // never calls _GetNextBufferHandleId(), which would.
        : HgiExternalBufferArena(nullptr, /*rawSourceDevice*/ 0)
    {
    }

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

} // anonymous namespace

int
main(int /*argc*/, char ** /*argv*/)
{
    TfErrorMark mark;

    bool success = _TestReleaseWaitsForRetire();
    success = success && _TestInUseBufferIsUntouched();
    success = success && _TestOnePassTakesOneStamp();
    success = success && _TestDestructionReleasesPending();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    }
    std::cout << "FAILED" << std::endl;
    return EXIT_FAILURE;
}
