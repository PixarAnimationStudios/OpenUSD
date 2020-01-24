//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/pxr.h"
#include "pxr/base/work/arenaDispatcher.h"

#include <tbb/concurrent_queue.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
// We do this at Intel's suggestion, since creating and destroying arenas is
// pretty expensive and rather concurrency unfriendly.  We have code that,
// depending on usage patterns, may have concurrent transient arenas so here we
// are.  The other suggestion was to try to not have too many arenas since
// apparently the tbb internals wind up walking arena lists when doling out
// tasks, so this can be a slowdown point as well.
struct _ArenaManager
{
    static constexpr size_t FreeLimit = 64; // a guess...
    
    inline tbb::task_arena *
    CheckOut() {
        // Try to pop one from freeArenas.
        tbb::task_arena *ret;
        if (freeArenas.try_pop(ret)) {
            return ret;
        }
        // Otherwise create a new one.
        return new tbb::task_arena(WorkGetConcurrencyLimit());
    }

    inline void
    Return(tbb::task_arena *arena) {
        // Racy size check -- if too many free already just delete to avoid
        // having too many arenas total.  Note that we can definitely have more
        // than FreeLimit free arenas, due to the racy size check.  That's okay.
        if (freeArenas.unsafe_size() >= FreeLimit) {
            delete arena;
        }
        // Otherwise return to free list.
        else {
            freeArenas.push(arena);
        }        
    }

    tbb::concurrent_queue<tbb::task_arena *> freeArenas;
};

_ArenaManager &
GetTheArenaManager()
{
    // We heap allocate the manager so we don't try to run the dtor at static
    // destruction time, to avoid any potential issues with task_arena dtors
    // accessing destroyed parts of tbb internals.
    static _ArenaManager *theManager = new _ArenaManager;
    return *theManager;
}

} // anon

WorkArenaDispatcher::~WorkArenaDispatcher()
{
    Wait();
    GetTheArenaManager().Return(_arena);
}

void
WorkArenaDispatcher::Wait()
{
    // We call Wait() inside the arena, to only wait for the completion of tasks
    // submitted to that arena. This will also give the calling thread a chance
    // to join the arena (if it can) and thus "help" complete any pending tasks.
    //
    // Note that it is not harmful to call Wait() without executing it in the
    // arena. That would just mean that the calling thread cannot migrate into
    // the arena, and can therefore not do any work from that arena, while it
    // is waiting.
    _arena->execute(std::bind(&WorkDispatcher::Wait, &_dispatcher));
}

void
WorkArenaDispatcher::Cancel()
{
    // Note that we do not execute Cancel() in the arena. We do not need to
    // enter the arena to issue the cancellation signal. We could, but doing so
    // would mean that the calling thread would have to migrate into the arena
    // or worse, if it cannot do that, we would have to synchronize on a new
    // task in the arena to execute the Cancel() call.
    _dispatcher.Cancel();
}

tbb::task_arena *
WorkArenaDispatcher::_GetArena() const
{
    return GetTheArenaManager().CheckOut();
}

PXR_NAMESPACE_CLOSE_SCOPE
