//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/work/workTBB/isolatingDispatcher_impl.h"

#include "pxr/base/work/workTBB/threadLimits_impl.h"

#include <tbb/concurrent_queue.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// We do this at Intel's suggestion, since creating and destroying arenas is
// pretty expensive and rather concurrency unfriendly.  We have code that,
// depending on usage patterns, may have concurrent transient arenas so here we
// are.  The other suggestion was to try to not have too many arenas since
// apparently the tbb internals wind up walking arena lists when doling out
// tasks, so this can be a slowdown point as well.
class _ArenaManager
{
    static constexpr size_t FreeLimit = 64; // a guess...

public:
    tbb::task_arena * CheckOut() {
        // Try to pop an arena from _freeArenas.
        tbb::task_arena *ret;
        if (_freeArenas.try_pop(ret)) {
            return ret;
        }
        // Otherwise create a new arena.
        return new tbb::task_arena(WorkImpl_GetConcurrencyLimit());
    }

    void Return(tbb::task_arena *arena) {
        // Racy size check -- if we already have too many free arenas just
        // delete to avoid having too many arenas total.  Note that we can
        // definitely have more than FreeLimit free arenas, due to the racy size
        // check.  That's okay.
        if (_freeArenas.unsafe_size() >= FreeLimit) {
            delete arena;
        }
        // Otherwise return to free list.
        else {
            _freeArenas.push(arena);
        }        
    }

private:
    tbb::concurrent_queue<tbb::task_arena *> _freeArenas;
};

static _ArenaManager &
GetTheArenaManager()
{
    // We heap allocate the manager so we don't try to run the dtor at static
    // destruction time, to avoid any potential issues with task_arena dtors
    // accessing destroyed parts of tbb internals.
    static _ArenaManager *theManager = new _ArenaManager;
    return *theManager;
}

}

WorkImpl_IsolatingDispatcher::WorkImpl_IsolatingDispatcher()
    : _arena(GetTheArenaManager().CheckOut())
{}

WorkImpl_IsolatingDispatcher::~WorkImpl_IsolatingDispatcher()
{
    Wait();
    GetTheArenaManager().Return(_arena);
}

void
WorkImpl_IsolatingDispatcher::Wait()
{
    // We call Wait() inside the arena, to only wait for the completion of tasks
    // submitted to that arena. This will also give the calling thread a chance
    // to join the arena (if it can) and thus "help" complete any pending tasks.
    //
    // Note that it is not harmful to call Wait() without executing it in the
    // arena. That would just mean that the calling thread cannot migrate into
    // the arena, and can therefore not do any work from that arena, while it
    // is waiting.

    _arena->execute([&dispatcher = _dispatcher](){
        dispatcher.Wait();
    });
}

PXR_NAMESPACE_CLOSE_SCOPE
