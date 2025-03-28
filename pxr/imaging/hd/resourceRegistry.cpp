//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE


HdResourceRegistry::HdResourceRegistry()
{
}

HdResourceRegistry::~HdResourceRegistry()
{
}

void
HdResourceRegistry::GarbageCollect()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HD_PERF_COUNTER_INCR(HdPerfTokens->garbageCollected);

    // Prompt derived registries to collect their garbage.
    _GarbageCollect();
}

void
HdResourceRegistry::Commit()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Prompt derived registries to commit in-flight data.
    _Commit();
}

void HdResourceRegistry::InvalidateShaderRegistry()
{
    // Derived classes that hold shaders will override this,
    // but the base registry has nothing to do.
}

void
HdResourceRegistry::ReloadResource(
    TfToken const& resourceType,
    std::string const& path)
{
}

VtDictionary
HdResourceRegistry::GetResourceAllocation() const
{
    return VtDictionary();
}

void
HdResourceRegistry::_Commit()
{
}

void
HdResourceRegistry::_GarbageCollect()
{
}


PXR_NAMESPACE_CLOSE_SCOPE

