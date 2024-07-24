//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_SHADOW_MATRIX_COMPUTATION_H
#define PXR_IMAGING_HDX_SHADOW_MATRIX_COMPUTATION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/cameraUtil/conformWindow.h"
#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

class CameraUtilFraming;

// Interface class for computing the shadow matrix
// for a given viewport.
class HdxShadowMatrixComputation
{
public:
    // For legacy clients using viewport, will be removed eventually.
    virtual std::vector<GfMatrix4d> Compute(const GfVec4f &viewport, CameraUtilConformWindowPolicy policy) = 0;

    // For modern clients using camera framing API.
    virtual std::vector<GfMatrix4d> Compute(const CameraUtilFraming &framing, CameraUtilConformWindowPolicy policy) = 0;

protected:
    HdxShadowMatrixComputation()          = default;
    virtual ~HdxShadowMatrixComputation() = default;

private:
    HdxShadowMatrixComputation(const HdxShadowMatrixComputation &)             = delete;
    HdxShadowMatrixComputation &operator =(const HdxShadowMatrixComputation &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_SHADOW_MATRIX_COMPUTATION_H
