//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_TBB_IMPL_H
#define PXR_BASE_WORK_TBB_IMPL_H

/// This file pulls in the entire work implementation

#include "pxr/base/work/workTBB/detachedTask_impl.h"
#include "pxr/base/work/workTBB/dispatcher_impl.h"
#include "pxr/base/work/workTBB/isolatingDispatcher_impl.h"
#include "pxr/base/work/workTBB/loops_impl.h"
#include "pxr/base/work/workTBB/reduce_impl.h"
#include "pxr/base/work/workTBB/sort_impl.h"
#include "pxr/base/work/workTBB/taskGraph_impl.h"
#include "pxr/base/work/workTBB/threadLimits_impl.h"
#include "pxr/base/work/workTBB/withScopedParallelism_impl.h"

#include "pxr/pxr.h"
#if PXR_USE_NAMESPACES
#define WORK_IMPL_NS PXR_NS
#endif

#endif // PXR_BASE_WORK_TBB_IMPL_H
