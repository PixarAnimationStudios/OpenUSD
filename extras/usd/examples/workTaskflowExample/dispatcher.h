//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_DISPATCHER_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_DISPATCHER_H

#include "workTaskflowExample/api.h"
#include "workTaskflowExample/executorStack.h"
#include "workTaskflowExample/threadLimits.h"

#include <taskflow/taskflow.hpp>

#include <atomic>
#include <functional>
#include <type_traits>
#include <utility>

class WorkImpl_Dispatcher
{
public:
    /// Construct a new dispatcher.
    WORK_TASKFLOW_EXAMPLE_API
    WorkImpl_Dispatcher();

    /// Wait() for any pending tasks to complete, then destroy the dispatcher.
    WORK_TASKFLOW_EXAMPLE_API
    ~WorkImpl_Dispatcher() noexcept;

    WorkImpl_Dispatcher(WorkImpl_Dispatcher const &) = delete;
    WorkImpl_Dispatcher &operator=(WorkImpl_Dispatcher const &) = delete;

    template <class Callable>
    inline void Run(Callable &&c) {
        // If executing serially, the task cannot run immediately.
        // There are deep nestings of dispatcher calls which
        // could lead to stack overflows if called recursively.
        if( WorkImpl_GetConcurrencyLimit() == 1) {
            _serialTasks.emplace(
                _InvokerTaskWrapper(std::forward<Callable>(c), 
                                    nullptr, &_jobCount));
        } else {
             _jobCount++;
            tf::Executor *e;
            tf::Executor *forwardedExecutor = nullptr;
            // Grab the top most executor from the stack to run the task if one
            // exists. If not, use the global executor.
            WorkTaskflow_Executor *head = WorkTaskflow_LocalStack::Get().head;
            if (head == nullptr) {
                e = &_GetGlobalExecutor();
            } else if (head->executor == nullptr){
                // if head is valid but it's executor isn't then, the code has 
                // left the scope of withScopedParallelism. We can safely delete
                // head from the stack.
                WorkTaskflow_Executor *head = 
                    WorkTaskflow_LocalStack::Get().head;
                delete head;
                e = &_GetGlobalExecutor();
            } else {
                e = head->executor;
                forwardedExecutor = head->executor;
            }

            e->silent_async(
                _InvokerTaskWrapper<std::remove_reference_t<Callable>>
                (std::forward<Callable>(c), forwardedExecutor, &_jobCount));
        }
    }

    template <class Callable, class A0, class ... Args>
    inline void Run(Callable &&c, A0 &&a0, Args&&... args) {
        Run(std::bind(std::forward<Callable>(c),
                      std::forward<A0>(a0),
                      std::forward<Args>(args)...));
    }

    // Run a task from the global executor
    template <class Callable>
    inline void RunWithGlobal(Callable &&c) {
        tf::Executor *e;
        WorkTaskflow_Executor *head = WorkTaskflow_LocalStack::Get().head;
        e = &_GetGlobalExecutor();
        e->silent_async(
        _InvokerTaskWrapper(std::forward<Callable>(c), nullptr, &_jobCount));
    }

    // Resets relevant contexts if needed
    WORK_TASKFLOW_EXAMPLE_API
    void Reset();

    // Block until the work started by Run() completes.
    WORK_TASKFLOW_EXAMPLE_API
    void Wait();

    // Implements WorkDispatcher::Cancel()
    WORK_TASKFLOW_EXAMPLE_API
    void Cancel();

private:
    WORK_TASKFLOW_EXAMPLE_API
    static tf::Executor &_GetGlobalExecutor();

    WORK_TASKFLOW_EXAMPLE_API
    static tf::Executor &_GetGlobalSerialExecutor();

    tf::Taskflow _serialTasks;

    template <class Fn>
    struct _InvokerTaskWrapper {
        explicit _InvokerTaskWrapper(Fn &&fn, 
                                     tf::Executor *e,
                                     std::atomic<uint64_t> *jobCount) 
        {
            _fn = std::make_shared<Fn>(std::forward<Fn>(fn));
            _executor = e;
            _count = jobCount;
        }

        explicit _InvokerTaskWrapper(Fn const &fn, 
                                     tf::Executor *e, 
                                     std::atomic<uint64_t> *jobCount) 
        {
            _fn = std::make_shared<Fn>(std::forward<Fn>(fn));
            _executor = e;
            _count = jobCount;
        }

        _InvokerTaskWrapper(_InvokerTaskWrapper &&other) = default;
        _InvokerTaskWrapper(const _InvokerTaskWrapper &other) = default;

        void operator()() {
            // if the function is executed in a different thread than spawned
            // pass along the executor to the new thread such that any ongoing 
            // work isolation is maintained.
            if (_executor != nullptr) {
                WorkTaskflow_Executor *head = 
                    WorkTaskflow_LocalStack::Get().head;
                if ((head == nullptr) || (head->executor != _executor)) {
                    new WorkTaskflow_Executor(_executor);
                }
            }
            (*_fn)();
            (*_count)--;
        }
    private:
        tf::Executor * _executor;
        std::shared_ptr<Fn> _fn;
        std::atomic<uint64_t> * _count;
    };

    std::atomic<uint64_t> _jobCount;
};

#endif // PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_DISPATCHER_H
