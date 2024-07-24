//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/work/detachedTask.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/threadLimits.h"

#include <atomic>
#include <chrono>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

WorkDispatcher &
Work_GetDetachedDispatcher()
{
    // Deliberately leak this in case there are tasks still using it after we
    // exit from main().
    static WorkDispatcher *theDispatcher = new WorkDispatcher;
    return *theDispatcher;
}

static std::atomic<std::thread *> detachedWaiter { nullptr };

void
Work_EnsureDetachedTaskProgress()
{
    // Check to see if there's a waiter thread already.  If not, try to create
    // one.
    std::thread *c = detachedWaiter.load();
    if (ARCH_UNLIKELY(!c)) {
        std::thread *newThread = new std::thread;
        if (detachedWaiter.compare_exchange_strong(c, newThread)) {
            // We won the race, so start the waiter thread.
            WorkDispatcher &dispatcher = Work_GetDetachedDispatcher();
            *newThread =
                std::thread([&dispatcher]() {
                        while (true) {
                            // Process detached tasks.
                            dispatcher.Wait();
                            // Now sleep for a bit, and try again.
                            using namespace std::chrono_literals;
                            std::this_thread::sleep_for(50ms);
                        }
                    });
            newThread->detach();
        }
        else {
            // We lost the race, so delete our temporary thread.
            delete newThread;
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
