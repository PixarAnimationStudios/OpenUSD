//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "workTaskflowExample/detachedTask.h"

#include <atomic>
#include <chrono>
#include <thread>

static std::atomic<std::thread *> detachedWaiter { nullptr };

WorkImpl_Dispatcher &
WorkTaskflow_GetDetachedDispatcher()
{
    // Deliberately leak this in case there are tasks still using it after we
    // exit from main().
    static WorkImpl_Dispatcher *theDispatcher = new WorkImpl_Dispatcher;
    return *theDispatcher;
}

void
WorkTaskflow_EnsureDetachedTaskProgress()
{
    // Check to see if there's a waiter thread already.  If not, try to create
    // one.
    std::thread *c = detachedWaiter.load();
    if (!c) {
        std::thread *newThread = new std::thread;
        if (detachedWaiter.compare_exchange_strong(c, newThread)) {
            // We won the race, so start the waiter thread.
            WorkImpl_Dispatcher &dispatcher = 
                WorkTaskflow_GetDetachedDispatcher();
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
