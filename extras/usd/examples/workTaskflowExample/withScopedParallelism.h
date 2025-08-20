//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_SCOPED_PARALLELISM_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_SCOPED_PARALLELISM_H

#include "workTaskflowExample/executorStack.h"

#include <taskflow/taskflow.hpp> 
#include <type_traits>

/// Implements WorkWithScopedParallelism
template <class Fn>
auto
WorkImpl_WithScopedParallelism(Fn &&fn)
{
    tf::Taskflow taskflow;
    tf::Executor executor(4);
    // tf::Executor's cannot steal work from each other so in order to
    // implement ScopedParallelism we create a new Executor and push it ontop
    // of a thread_local executor stack. All dispatched tasks will be spawned
    // from the top most executor, ensuring that nested scopings are respected. 
    // Right before we leave the scope, the executor is popped from the stack
    // and deleted.
    WorkTaskflow_Executor *w = new WorkTaskflow_Executor(&executor);
    if constexpr (!std::is_same<void,std::result_of_t<Fn(void)>>::value) {
        std::result_of_t<Fn(void)>  result;
        try{
            result = fn();
        }
        catch( std::exception& e) {
            std::cout << "Exception raised: " << e.what() << std::endl;
        }            
        executor.wait_for_all();
        delete w;
        return result;
    } else {
        try {
            fn();
        } catch( std::exception& e) {
            std::cout << "Exception raised: " << e.what() << std::endl;
        }
        executor.wait_for_all();
        delete w;
        return;
    }
}

#endif // PXR_EXTRAS_USD_EXAMPLES_WORK_TASKFLOW_EXAMPLE_SCOPED_PARALLELISM_H
