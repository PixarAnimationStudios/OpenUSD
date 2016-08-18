//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef HDX_COMPUTE_SHADOW_MATRIX_H
#define HDX_COMPUTE_SHADOW_MATRIX_H

#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/cameraUtil/conformWindow.h"
#include "pxr/base/gf/matrix4d.h"

// Interface class for computing the shadow matrix
// for a given viewport.
class HdxShadowMatrixComputation
{
public:
    virtual GfMatrix4d Compute(const GfVec4f &viewport, CameraUtilConformWindowPolicy policy) = 0;

protected:
    HdxShadowMatrixComputation()          = default;
    virtual ~HdxShadowMatrixComputation() = default;

private:
    HdxShadowMatrixComputation(const HdxShadowMatrixComputation &)             = delete;
    HdxShadowMatrixComputation &operator =(const HdxShadowMatrixComputation &) = delete;
};

#endif // HDX_COMPUTE_SHADOW_MATRIX_H
