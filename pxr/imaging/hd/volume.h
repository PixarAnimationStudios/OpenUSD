//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_VOLUME_H
#define PXR_IMAGING_HD_VOLUME_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprim.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using HdVolumePtrConstVector = std::vector<class HdVolume const *>;

/// \class HdVolume
///
/// Hd schema for a renderable volume primitive.
///
class HdVolume : public HdRprim
{
public:
    HD_API
    HdVolume(SdfPath const& id);

    HD_API
    ~HdVolume() override;

    HD_API
    TfTokenVector const & GetBuiltinPrimvarNames() const override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_VOLUME_H
