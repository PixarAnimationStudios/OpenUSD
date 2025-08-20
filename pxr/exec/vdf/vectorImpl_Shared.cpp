//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/vectorImpl_Shared.h"

#include "pxr/base/tf/mallocTag.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

Vdf_VectorImplShared::_SharedSource::_SharedSource(DataHolder* srcData):
    Vt_ArrayForeignDataSource(_Detached)
{
    // Move the data into the local DataHolder. Must create a new empty impl
    // first because MoveInto expects a valid destination.
    srcData->Get()->NewEmpty(0, &_data);
    srcData->Get()->MoveInto(&_data);
}

Vdf_VectorImplShared::_SharedSource::~_SharedSource()
{
    _data.Destroy();
}

void Vdf_VectorImplShared::_SharedSource::_Detached(
    Vt_ArrayForeignDataSource* selfBase) 
{
    _SharedSource* self = static_cast<_SharedSource*>(selfBase);
    TF_DEV_AXIOM(self->_refCount.load() == 0);
    delete self;
}

Vdf_VectorImplShared::Vdf_VectorImplShared(DataHolder* srcData):
    // _SharedSource is constructed with ref-count 0, so increment to 1.
    _source(TfDelegatedCountIncrementTag, new _SharedSource(srcData))
{
}

inline
Vdf_VectorImplShared::Vdf_VectorImplShared(const Vdf_VectorImplShared &o)
    : _source(o._source)
{
}

inline
Vdf_VectorImplShared::Vdf_VectorImplShared(Vdf_VectorImplShared &&o) noexcept
    : _source(std::move(o._source))
{
}

Vdf_VectorImplShared::~Vdf_VectorImplShared() = default;

const std::type_info& Vdf_VectorImplShared::GetTypeInfo() const
{
    return _source->GetHolder()->Get()->GetTypeInfo();
}

void Vdf_VectorImplShared::NewEmpty(size_t size, DataHolder* destData) const
{
    _source->GetHolder()->Get()->NewEmpty(size, destData);
}

void Vdf_VectorImplShared::NewSingle(DataHolder* destData) const 
{
    _source->GetHolder()->Get()->NewSingle(destData);
}

void Vdf_VectorImplShared::NewSparse(
    size_t size, size_t first, size_t last, DataHolder* destData) const 
{
    _source->GetHolder()->Get()->NewSparse(size, first, last, destData);
}

void Vdf_VectorImplShared::NewDense(size_t size, DataHolder* destData) const 
{
    _source->GetHolder()->Get()->NewDense(size, destData);
}

void Vdf_VectorImplShared::MoveInto(Vdf_VectorData::DataHolder* destData)
{
    TfAutoMallocTag tag("Vdf", __ARCH_PRETTY_FUNCTION__);
    destData->Destroy();
    destData->New< Vdf_VectorImplShared >(std::move(*this));
}

void Vdf_VectorImplShared::Clone(Vdf_VectorData::DataHolder* destData) const
{
    TfAutoMallocTag tag("Vdf", __ARCH_PRETTY_FUNCTION__);
    destData->Destroy();
    destData->New< Vdf_VectorImplShared >(*this);
}

void Vdf_VectorImplShared::CloneSubset(const VdfMask& mask, 
    Vdf_VectorData::DataHolder* destData) const
{
    _source->GetHolder()->Get()->CloneSubset(mask, destData);
}

void Vdf_VectorImplShared::Box(
    const VdfMask::Bits& bits, Vdf_VectorData::DataHolder* destData) const
{
    _source->GetHolder()->Get()->Box(bits, destData);
}

void Vdf_VectorImplShared::Merge(const VdfMask::Bits& bits,
    Vdf_VectorData::DataHolder* destData) const
{
    _source->GetHolder()->Get()->Merge(bits, destData);
}

void Vdf_VectorImplShared::Expand(size_t first, size_t last)
{
    TF_CODING_ERROR("Cannot mutate shared data without detaching.");
}

size_t Vdf_VectorImplShared::GetSize() const
{
    return _source->GetHolder()->Get()->GetSize();
}

size_t Vdf_VectorImplShared::GetNumStoredElements() const 
{
    return _source->GetHolder()->Get()->GetNumStoredElements();
}

Vt_ArrayForeignDataSource* Vdf_VectorImplShared::GetSharedSource() const 
{
    return _source.get();
}

Vdf_VectorData::Info Vdf_VectorImplShared::GetInfo() 
{
    const Vdf_VectorData::Info sourceInfo = 
        _source->GetHolder()->Get()->GetInfo();
    return Vdf_VectorData::Info(
        sourceInfo.data,
        sourceInfo.size,
        sourceInfo.first,
        sourceInfo.last,
        sourceInfo.compressedIndexMapping,
        sourceInfo.layout,
        Vdf_VectorData::Info::Ownership::Shared);
}

size_t Vdf_VectorImplShared::EstimateElementMemory() const
{
    return _source->GetHolder()->Get()->EstimateElementMemory();
}

void Vdf_VectorImplShared::Detach(Vdf_VectorData::DataHolder* data)
{
    TF_DEV_AXIOM(data && data->Get()->GetInfo().ownership ==
                 Vdf_VectorData::Info::Ownership::Shared);

    Vdf_VectorImplShared* const impl =
        static_cast<Vdf_VectorImplShared*>(data->Get());

    // Note that data is holding the source Vdf_VectorImplShared instance and
    // is the destination of the new detached impl. We cannot simply write
    // into data as that would destruct our source instance. Instead, transfer
    // ownership of the shared source to the stack.
    const TfDelegatedCountPtr<_SharedSource> source = std::move(impl->_source);
    if (source->IsUnique()) {
        // We are detaching with the last ref count, have data take ownership
        // directly rather than copying.
        source->GetHolder()->Get()->MoveInto(data);
    }
    else {
        source->GetHolder()->Get()->Clone(data);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
