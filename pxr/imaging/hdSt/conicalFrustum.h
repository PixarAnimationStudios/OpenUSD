//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_CONICAL_FRUSTUM_H
#define PXR_IMAGING_HD_ST_CONICAL_FRUSTUM_H

#include "pxr/imaging/hdSt/implicitSurface.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdStConicalFrustum
///
/// Represents a conical frustum (including cylinders and cones) that
/// can be rendered natively by Storm.
///
template <bool IsCone>
class HdStConicalFrustum final : public HdStImplicitSurface
{
public:
    HF_MALLOC_TAG_NEW("new HdStConicalFrustum");

    HDST_API
    explicit HdStConicalFrustum(SdfPath const &id);

    HDST_API
    ~HdStConicalFrustum() override = default;

protected:
    HDST_API
    void _PopulateCustomConstantPrimvars(HdSceneDelegate *sceneDelegate,
                                         HdRenderParam *renderParam,
                                         HdStDrawItem *drawItem) override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_CONICAL_FRUSTUM_H
