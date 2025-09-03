//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/dynamicTopologicalSorter.h"

PXR_NAMESPACE_OPEN_SCOPE

Vdf_TopologicalPriorityAllocator::Vdf_TopologicalPriorityAllocator()
    : _next(0)
{}

Vdf_TopologicalPriorityAllocator::~Vdf_TopologicalPriorityAllocator()
{}

int
Vdf_TopologicalPriorityAllocator::Allocate()
{
    if (!_reusablePriorities.empty()) {
        int priority = _reusablePriorities.back();
        _reusablePriorities.pop_back();
        return priority;
    }

    return _next++;
}

void
Vdf_TopologicalPriorityAllocator::Free(int priority)
{
    _reusablePriorities.push_back(priority);
}

void
Vdf_TopologicalPriorityAllocator::Clear()
{
    _next = 0;
    TfReset(_reusablePriorities);
}

PXR_NAMESPACE_CLOSE_SCOPE
