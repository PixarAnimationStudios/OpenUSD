//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "dispatcher.h"

WorkImpl_Dispatcher::WorkImpl_Dispatcher()
{
    _jobCount.store(0);
}

WorkImpl_Dispatcher::~WorkImpl_Dispatcher() noexcept
{
    Wait();
}

void
WorkImpl_Dispatcher::Wait()
{
    if ( WorkImpl_GetConcurrencyLimit() == 1){
        _GetGlobalSerialExecutor().run(_serialTasks).wait();
    } else {
        tf::Executor *e;
        WorkTaskflow_Executor * s = WorkTaskflow_LocalStack::Get().head;
        if (s == nullptr ||  s->executor == nullptr) {
            e = &_GetGlobalExecutor();
        } else {
            e = s->executor;
        }

        // Each dispatcher needs to have its own task grouping, but each 
        // executor can have multiple tasks from each dispatcher. We cannot call
        // tf:::Executor::wait_for_all() since that wouid wait on all the tasks 
        // assigned to that executor. Instead we manually keep track of jobs and
        // spin until they are complete. 
        tf::Taskflow taskflow;
        taskflow.emplace([&](){      
                while(_jobCount.load() != 0){
                    std::this_thread::yield();
                }
        });
        
        e->run(taskflow).wait();
    }
}

void
WorkImpl_Dispatcher::Reset() 
{
    return;
}

void
WorkImpl_Dispatcher::Cancel()
{
    return;
}

tf::Executor &
WorkImpl_Dispatcher::_GetGlobalExecutor()
{
    // Creating Executors has a non-negible overhead. Limiting to 4 threads
    // so we don't try to spawn 31 threads per executor.
    static tf::Executor globalExecutor(4);
    return globalExecutor;
}

tf::Executor &
WorkImpl_Dispatcher::_GetGlobalSerialExecutor()
{
    static tf::Executor globalSerialExecutor(1);
    return globalSerialExecutor;
}
