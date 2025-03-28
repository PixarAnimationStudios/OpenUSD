//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/extCompGpuPrimvarBufferSource.h"

#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/hash.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStExtCompGpuPrimvarBufferSource::HdStExtCompGpuPrimvarBufferSource(
        TfToken const &name,
        HdTupleType const &valueType,
        int numElements,
        SdfPath const& compId)
 : HdNullBufferSource()
 , _name(name)
 , _tupleType(valueType)
 , _numElements(numElements)
 , _compId(compId)
{
}

/* virtual */
TfToken const &
HdStExtCompGpuPrimvarBufferSource::GetName() const
{
    return _name;
}

/* virtual */
size_t
HdStExtCompGpuPrimvarBufferSource::ComputeHash() const
{
    // Simply return a hash based on the computation and primvar names, 
    // instead of hashing the contents of the inputs to the computation.
    // This effectively disables primvar sharing when using computed primvars.
    return TfHash::Combine(_compId, _name);
}

/* virtual */
bool
HdStExtCompGpuPrimvarBufferSource::Resolve()
{
    if (!_TryLock()) return false;
    _SetResolved();
    return true;
}

/* virtual */
bool
HdStExtCompGpuPrimvarBufferSource::_CheckValid() const
{
    return true;
}

/* virtual */
size_t
HdStExtCompGpuPrimvarBufferSource::GetNumElements() const
{
    return _numElements;
}

/* virtual */
HdTupleType
HdStExtCompGpuPrimvarBufferSource::GetTupleType() const
{
    return _tupleType;
}

/* virtual */
void
HdStExtCompGpuPrimvarBufferSource::GetBufferSpecs(
        HdBufferSpecVector *specs) const
{
    specs->emplace_back(_name, _tupleType);
}


PXR_NAMESPACE_CLOSE_SCOPE
