//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/extGpuBufferSource.h"
#include "pxr/imaging/hdSt/extGpuBuffer.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/hash.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

HdStExtGpuBufferSource::HdStExtGpuBufferSource(
    TfToken const &name,
    HdStExtGpuBufferDesc const &hdDesc)
    : _name(name)
{
    _descriptor = hdDesc;

    if (hdDesc.importedBuffer) {
        // Imported route: routing already resolved a real backend buffer
        // aliasing the producer's memory, so there is nothing to wrap. The
        // descriptor copied above shares ownership of it, which is what keeps
        // it alive until it reaches a buffer array range.
        _descriptor.cachedHgiHandle = hdDesc.GetImportedHandle();
        return;
    }

    size_t byteSize = hdDesc.rawHandleByteSize > 0
        ? hdDesc.rawHandleByteSize
        : hdDesc.numElements * HdDataSizeOfTupleType(hdDesc.tupleType);

    _ownedExternalGpuBuffer = std::make_unique<HdStExtGpuBuffer>(
        hdDesc.rawHandle, byteSize);

    _descriptor.cachedHgiHandle = HgiBufferHandle(
        _ownedExternalGpuBuffer.get(), HdSt_GetNextExtGpuBufferHandleId());
}

HdStExtGpuBufferSource::~HdStExtGpuBufferSource() = default;

TfToken const &
HdStExtGpuBufferSource::GetName() const
{
    return _name;
}

void
HdStExtGpuBufferSource::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    if (specs) {
        specs->push_back(HdBufferSpec(_name, _descriptor.tupleType));
    }
}

bool
HdStExtGpuBufferSource::Resolve()
{
    if (!_TryLock()) {
        return false;
    }
    _SetResolved();
    return true;
}

size_t
HdStExtGpuBufferSource::ComputeHash() const
{
    // The default HdBufferSource::ComputeHash hashes GetData() bytes, which
    // dereferences nullptr for external GPU sources (no CPU payload).  Hash
    // the GPU-side identity instead.  This is intentionally stable across
    // animation frames -- HdStMesh::_PopulateVertexPrimvars transitions the
    // BAR to mutable on the second dirty frame and resets the immutable
    // share key, so this hash only gates first-time cross-rprim sharing.
    return TfHash::Combine(
        _name,
        _descriptor.tupleType,
        _descriptor.numElements,
        _descriptor.byteOffset,
        _descriptor.byteStride,
        _descriptor.rawHandle,
        // An import-only producer leaves rawHandle at 0, so the memory identity
        // is what distinguishes its streams from each other.
        _descriptor.externalMemoryHandle,
        _descriptor.memoryOffset);
}

void const *
HdStExtGpuBufferSource::GetData() const
{
    // External GPU buffers have no CPU-side data.
    // CopyData callers must detect this source (dynamic_cast) first.
    TF_CODING_ERROR("GetData() called on HdStExtGpuBufferSource '%s'. "
                    "External GPU buffer sources have no CPU data; "
                    "CopyData must use the GPU-to-GPU path.",
                    _name.GetText());
    return nullptr;
}

HdTupleType
HdStExtGpuBufferSource::GetTupleType() const
{
    return _descriptor.tupleType;
}

size_t
HdStExtGpuBufferSource::GetNumElements() const
{
    return _descriptor.numElements;
}

bool
HdStExtGpuBufferSource::_CheckValid() const
{
    return (_descriptor.rawHandle != 0 || _descriptor.importedBuffer)
        && _descriptor.numElements > 0
        && _descriptor.tupleType.type != HdTypeInvalid;
}

PXR_NAMESPACE_CLOSE_SCOPE
