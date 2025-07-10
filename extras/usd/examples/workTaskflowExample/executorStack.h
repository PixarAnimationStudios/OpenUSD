//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_EXECUTOR_STACK_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_EXECUTOR_STACK_H

#include <taskflow/taskflow.hpp>

// Stack element that holds the executor
struct WorkTaskflow_Executor 
{
    tf::Executor *executor; 

    explicit WorkTaskflow_Executor(tf::Executor *e) : executor(e) { 
        Push();}

    ~WorkTaskflow_Executor(){
        Pop();
    }

    inline void Push();
    inline void Pop() const;
    inline bool IsHead();
    void *_localStack;
    WorkTaskflow_Executor * _prev;
};

// Stack that holds the executors.
struct _Stack
{    
    WorkTaskflow_Executor * head;
};

// A helper struct for thread_local that uses nullptr initialization as a
// sentinel to prevent guard variable use from being invoked after first
// initialization.
template <class T>
struct _FastThreadLocalBase
{
    static T &Get() {
        static thread_local T *theTPtr = nullptr;
        if (theTPtr) {
            return *theTPtr;
        }
        static thread_local T theT;
        theTPtr = &theT;

        return *theTPtr;
    }
};

struct WorkTaskflow_LocalStack : _FastThreadLocalBase<_Stack> {};

void
WorkTaskflow_Executor::Push()
{
    _Stack &stack = WorkTaskflow_LocalStack::Get();
    _prev = stack.head;
    _localStack = &stack;
    stack.head = this;
}

void
WorkTaskflow_Executor::Pop() const
{
    _Stack &stack = *static_cast<_Stack *>(_localStack);
    assert(stack.head == this);
    stack.head = _prev;
}

bool
WorkTaskflow_Executor::IsHead() 
{
    _Stack &stack = *static_cast<_Stack *>(_localStack);
    return stack.head == this;
}

#endif // PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_EXECUTOR_STACK_H