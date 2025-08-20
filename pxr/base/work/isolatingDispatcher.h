//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_ISOLATING_DISPATCHER_H
#define PXR_BASE_WORK_ISOLATING_DISPATCHER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/impl.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class WorkIsolatingDispatcher
/// \extends Work_Dispatcher
/// 
/// The isolating work dispatcher is a specialization of WorkDispatcher,
/// mirroring its public API and documented behaviors.
/// 
/// However, the WorkIsolatingDispatcher imposes additional restrictions on
/// how it processes its concurrent tasks: Tasks added to the dispatcher will
/// only be run by this dispatcher, or nested instances of non-isolating
/// WorkDispatcher. Moreover, in its Wait() method, the WorkIsolatingDispatcher
/// will not run tasks that were not originally added to this dispatcher, or
/// one of its nested, non-isolating WorkDispatcher instances.
/// 
/// For work implementations that support work stealing (e.g., OneTBB) the
/// WorkIsolatingDispatcher effectively prevents unrelated tasks from being
/// stolen by this dispatcher, as well as dispatcher tasks being stolen by
/// unrelated parallel constructs (unless they are non-isolating and nested
/// within one of the tasks of this dispatcher).
/// 
/// \note
/// Enforcing the work stealing prevention behavior of the
/// WorkIsolatingDispatcher carries significant additional cost. Using the
/// WorkDispatcher is preferred in most cases, unless the isolation behavior is
/// required, and the scoping semantics of WorkWithScopedParallelism() are
/// insufficient for the particular use-case.
///
/// For more information, refer to the documentation for WorkDispatcher.
/// 
class WorkIsolatingDispatcher :
#if defined WORK_IMPL_HAS_ISOLATING_DISPATCHER
    public Work_Dispatcher<PXR_WORK_IMPL_NS::WorkImpl_IsolatingDispatcher>
#else
    public Work_Dispatcher<PXR_WORK_IMPL_NS::WorkImpl_Dispatcher>
#endif
{};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
