//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/volume.h"

PXR_NAMESPACE_OPEN_SCOPE

HdVolume::HdVolume(SdfPath const& id)
 : HdRprim(id)
{
}

HdVolume::~HdVolume() = default;

/* virtual */
TfTokenVector const &
HdVolume::GetBuiltinPrimvarNames() const
{
    static const TfTokenVector primvarNames;
    return primvarNames;
}

PXR_NAMESPACE_CLOSE_SCOPE
