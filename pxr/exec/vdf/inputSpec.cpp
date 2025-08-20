//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/inputSpec.h"
#include "pxr/exec/vdf/outputSpec.h"

#include "pxr/base/tf/hash.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////

VdfInputSpec::~VdfInputSpec()
{
}

std::string
VdfInputSpec::GetTypeName() const
{
    return _type.GetTypeName();
}

size_t
VdfInputSpec::GetHash() const
{
    return TfHash::Combine(
        _name,
        _type,
        _associatedOutputName,
        _access,
        _prerequisite);
}

bool
VdfInputSpec::TypeMatches(const VdfOutputSpec &other) const
{
    return GetType() == other.GetType();
}

PXR_NAMESPACE_CLOSE_SCOPE
