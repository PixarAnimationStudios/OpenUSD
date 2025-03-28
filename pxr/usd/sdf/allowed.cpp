//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file Allowed.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/allowed.h"

PXR_NAMESPACE_OPEN_SCOPE

const std::string&
SdfAllowed::GetWhyNot() const
{
    static std::string empty;
    return _state ? *_state : empty;
}

PXR_NAMESPACE_CLOSE_SCOPE
