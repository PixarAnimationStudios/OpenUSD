//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/binding.h"

PXR_NAMESPACE_OPEN_SCOPE

bool
HdStBindingRequest::operator==(HdStBindingRequest const &other) const
{
    return
        _bindingType == other._bindingType &&
        _dataType == other._dataType &&
        _name == other._name &&
        _resource == other._resource &&
        _bar == other._bar &&
        _isInterleaved == other._isInterleaved &&
        _isWritable == other._isWritable &&
        _arraySize == other._arraySize &&
        _concatenateNames == other._concatenateNames;
}

bool
HdStBindingRequest::operator!=(HdStBindingRequest const &other) const
{
    return !(*this == other);
}

size_t
HdStBindingRequest::ComputeHash() const
{
    return TfHash()(*this);
}

PXR_NAMESPACE_CLOSE_SCOPE

