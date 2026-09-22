//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_SPHERE_H
#define PXR_IMAGING_HD_ST_SPHERE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hdSt/implicitSurface.h"

#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdStSphere
///
/// Represents a sphere prim that can be rendered natively by Storm.
///
class HdStSphere final : public HdStImplicitSurface
{
public:
    HF_MALLOC_TAG_NEW("new HdStSphere");

    HDST_API
    explicit HdStSphere(SdfPath const &id);

    HDST_API
    ~HdStSphere() override = default;

protected:
    HDST_API
    void _PopulateCustomConstantPrimvars(HdSceneDelegate *sceneDelegate,
                                         HdRenderParam *renderParam,
                                         HdStDrawItem *drawItem) override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_SPHERE_H
