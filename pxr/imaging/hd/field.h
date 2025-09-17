//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_FIELD_H
#define PXR_IMAGING_HD_FIELD_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bprim.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

#define HD_FIELD_TOKENS                                    \
    (filePath)                                             \
    (fieldName)

TF_DECLARE_PUBLIC_TOKENS(HdFieldTokens, HD_API, HD_FIELD_TOKENS);

class HdSceneDelegate;
using HdFieldPtrConstVector = std::vector<class HdField const *>;

/// \class HdField
///
/// Hydra schema for a USD field primitive. Acts like a texture, combined
/// with other fields to make up a renderable volume.
///
class HdField : public HdBprim
{
public:
    HD_API
    HdField(SdfPath const & id);
    HD_API
    ~HdField() override;

    // Change tracking for HdField
    enum DirtyBits : HdDirtyBits {
        Clean                 = 0,
        DirtyTransform        = 1 << 0,
        DirtyParams           = 1 << 1,
        AllDirty              = (DirtyTransform
                                 |DirtyParams)
    };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_FIELD_H
