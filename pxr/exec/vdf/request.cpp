//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/request.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/output.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

VdfRequest::VdfRequest() : 
    _request(std::make_shared<const VdfMaskedOutputVector>())
{
    // Do nothing
}

VdfRequest::VdfRequest(const VdfMaskedOutput& output) :
    _request(std::make_shared<const VdfMaskedOutputVector>(1, output))
{
    // Do nothing
}

VdfRequest::VdfRequest(const VdfMaskedOutputVector& vector)
{
    std::shared_ptr<VdfMaskedOutputVector> request =
        std::make_shared<VdfMaskedOutputVector>(vector);
    VdfSortAndUniqueMaskedOutputVector(request.get());
    _request = std::move(request);
}

VdfRequest::VdfRequest(VdfMaskedOutputVector&& vector)
{
    std::shared_ptr<VdfMaskedOutputVector> request =
        std::make_shared<VdfMaskedOutputVector>(std::move(vector));
    VdfSortAndUniqueMaskedOutputVector(request.get());
    _request = std::move(request);
}

VdfRequest::~VdfRequest()
{
    // Do nothing
}

VdfRequest::const_iterator::const_iterator() :
    _mo(nullptr),
    _firstMo(nullptr),
    _bits(nullptr)
{
    // Do nothing
}

inline
VdfRequest::const_iterator::const_iterator(
    const VdfMaskedOutput* mo,
    const VdfMaskedOutput* first,
    const TfBits* bits) :
    _mo(mo),
    _firstMo(first),
    _bits(bits)
{
}

void
VdfRequest::const_iterator::_Increment()
{
    if (!_bits) {
        ++_mo;
    } else {
        const size_t index = _ComputeIndex();
        const size_t distance = _bits->FindNextSet(index+1) - index;
        _mo += distance;
    }
}

inline size_t
VdfRequest::const_iterator::_ComputeIndex() const
{
    return _mo - _firstMo;
}

VdfRequest::const_iterator
VdfRequest::begin() const
{
    if (IsEmpty()) {
        return end();
    }

    const bool isSubset = !_bits.IsEmpty();

    const VdfMaskedOutput *first = _request->data();
    const size_t index = isSubset ? _bits.GetFirstSet() : 0;
    const TfBits *bits = isSubset ? &_bits : nullptr;
    return const_iterator(first + index, first, bits);
}

VdfRequest::const_iterator
VdfRequest::end() const
{
    const bool isSubset = !_bits.IsEmpty();

    const VdfMaskedOutput *first = _request->data();
    const size_t index = _request->size();
    const TfBits *bits = isSubset ? &_bits : nullptr;
    return const_iterator(first + index, first, bits);
}

void
VdfRequest::_Add(size_t index) 
{
    // Early bail if all of the elements are already in the request.
    if (_bits.GetSize() == 0) {
        return;
    }

    // Verify that index is not out-of-range.
    if (!TF_VERIFY(index < _request->size())) {
        return;
    }

    _bits.Set(index);

    // If they are all reset, set to sentinel empty bits.
    if (_bits.AreAllSet()) {
        _bits.Resize(0);
    }
}

void
VdfRequest::Add(const const_iterator& iterator)
{
    if (!TF_VERIFY(iterator._firstMo == _request->data())) {
        return;
    }
    
    _Add(iterator._ComputeIndex());
}

void 
VdfRequest::_Remove(size_t index)
{
    // Early bail if index is out of range.
    if (!TF_VERIFY(index < _request->size())) {
        return;
    }

    // Resize bit set to length of vector if we are removing from a full 
    // request. Initialize the bit set to be all set.
    if (_bits.GetSize() == 0) {
        _bits.Resize(_request->size());
        _bits.SetAll();
    }

    _bits.Clear(index);
}

void
VdfRequest::Remove(const const_iterator& iterator)
{
    if (!TF_VERIFY(iterator._firstMo == _request->data())) {
        return;
    }

    _Remove(iterator._ComputeIndex());
}

void 
VdfRequest::AddAll()
{
    if (_bits.GetSize() != 0) {
        _bits.Resize(0);
    }
}

void
VdfRequest::RemoveAll()
{
    if (_bits.GetSize() == 0) {
        _bits.Resize(_request->size());
    }

    _bits.ClearAll();
}

bool
VdfRequest::IsEmpty() const
{
    return _request->empty() || 
        (_bits.AreAllUnset() && _bits.GetSize() == _request->size());
}

size_t 
VdfRequest::GetSize() const 
{
    return  _bits.GetSize() == 0 ? _request->size() : _bits.GetNumSet();
}

const VdfNetwork* 
VdfRequest::GetNetwork() const
{
    return !IsEmpty() ? 
        &_request->front().GetOutput()->GetNode().GetNetwork() :
        nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
