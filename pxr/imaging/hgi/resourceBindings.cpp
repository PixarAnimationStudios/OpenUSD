//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/resourceBindings.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiResourceBindings::HgiResourceBindings(HgiResourceBindingsDesc const& desc)
    : _descriptor(desc)
{
}

HgiResourceBindings::~HgiResourceBindings() = default;

HgiResourceBindingsDesc const&
HgiResourceBindings::GetDescriptor() const
{
    return _descriptor;
}

HgiBufferBindDesc::HgiBufferBindDesc()
    : bindingIndex(0)
    , stageUsage(HgiShaderStageVertex | HgiShaderStagePostTessellationVertex)
    , writable(false)
{
}

bool operator==(
    const HgiBufferBindDesc& lhs,
    const HgiBufferBindDesc& rhs)
{
    return lhs.buffers == rhs.buffers &&
           lhs.resourceType == rhs.resourceType &&
           lhs.offsets == rhs.offsets &&
           lhs.sizes == rhs.sizes &&
           lhs.bindingIndex == rhs.bindingIndex &&
           lhs.stageUsage == rhs.stageUsage &&
           lhs.writable == rhs.writable;
}

bool operator!=(
    const HgiBufferBindDesc& lhs,
    const HgiBufferBindDesc& rhs)
{
    return !(lhs == rhs);
}

HgiTextureBindDesc::HgiTextureBindDesc()
    : resourceType(HgiBindResourceTypeCombinedSamplerImage)
    , bindingIndex(0)
    , stageUsage(HgiShaderStageFragment)
    , writable(false)
{
}

bool operator==(
    const HgiTextureBindDesc& lhs,
    const HgiTextureBindDesc& rhs)
{
    return lhs.textures == rhs.textures &&
           lhs.resourceType == rhs.resourceType &&
           lhs.bindingIndex == rhs.bindingIndex &&
           lhs.stageUsage == rhs.stageUsage &&
           lhs.samplers == rhs.samplers &&
           lhs.writable == rhs.writable;
}

bool operator!=(
    const HgiTextureBindDesc& lhs,
    const HgiTextureBindDesc& rhs)
{
    return !(lhs == rhs);
}

HgiResourceBindingsDesc::HgiResourceBindingsDesc() = default;

bool operator==(
    const HgiResourceBindingsDesc& lhs,
    const HgiResourceBindingsDesc& rhs)
{
    return lhs.debugName == rhs.debugName &&
           lhs.buffers == rhs.buffers &&
           lhs.textures == rhs.textures;
}

bool operator!=(
    const HgiResourceBindingsDesc& lhs,
    const HgiResourceBindingsDesc& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
