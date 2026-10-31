//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/extGpuBufferSource.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/hash.h"

#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

HdStExtGpuBufferSource::HdStExtGpuBufferSource(
    TfToken const &name,
    HdStExtGpuBufferDesc const &desc)
    : _name(name)
    , _descriptor(desc)
{
    // Nothing to build. The descriptor already holds a strong reference to the
    // buffer and the buffer already owns its Hgi resource, so there is no
    // wrapper to construct here and no handle id to mint -- both used to be
    // this constructor's job.
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
    // HdBufferSource::ComputeHash hashes the bytes at GetData(), which is null
    // here, so hash the GPU-side identity instead: which buffer, and where in
    // it this stream sits.
    //
    // The buffer's address stands in for its identity. That is sound because
    // the descriptor holds a strong reference, so the object cannot be
    // destroyed and a new one land at the same address while this hash is
    // live.
    //
    // Deliberately stable across animation frames: HdStMesh's primvar
    // population moves the range to mutable on the second dirty frame and
    // resets the immutable share key, so this hash only gates first-time
    // cross-rprim sharing.
    return TfHash::Combine(
        _name,
        _descriptor.tupleType,
        _descriptor.numElements,
        _descriptor.byteOffset,
        _descriptor.byteStride,
        reinterpret_cast<uintptr_t>(_descriptor.externalBuffer.get()));
}

void const *
HdStExtGpuBufferSource::GetData() const
{
    TF_CODING_ERROR("GetData() called on HdStExtGpuBufferSource '%s'. A "
                    "GPU-backed source has no CPU data; check IsGpuBacked() "
                    "and take the GPU-to-GPU path.",
                    _name.GetText());
    return nullptr;
}

bool
HdStExtGpuBufferSource::IsGpuBacked() const
{
    return true;
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
    return _descriptor.externalBuffer
        && _descriptor.externalBuffer->GetBuffer()
        && _descriptor.numElements > 0
        && _descriptor.tupleType.type != HdTypeInvalid;
}

HdStExtGpuBufferSource const *
HdSt_GetExtGpuBufferSource(HdBufferSourceSharedPtr const &source)
{
    if (!source || !source->IsGpuBacked()) {
        return nullptr;
    }
    // IsGpuBacked() is Storm's own signal and this is the only Storm source
    // that sets it, so the type is known rather than guessed.
    return static_cast<HdStExtGpuBufferSource const *>(source.get());
}

PXR_NAMESPACE_CLOSE_SCOPE
