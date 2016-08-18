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
#include "pxr/imaging/cameraUtil/conformWindow.h"

#include "pxr/base/gf/camera.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/range2d.h"
#include "pxr/base/gf/frustum.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

// Overview of ConformWindowPolicy:
//
//                 Original window:
//
//                        w
//                 |<----------->|
//
//                 ***************  ---
//                 *   O     o   *   A
//                 * --|-- --|-- *   | h
//                 *   |     |   *   |
//                 *  / \   / \  *   V
//                 ***************  ---
//
//
//
// The result of applying the given ConformWindowPolicy when
//
//     target aspect                target aspect
//           >                            <
//    original aspect              original aspect
// 
//
//                 Match Vertically:
//
//
//  ******************* ---           ********* ---
//  *     O     o     *  A            *O     O*  A
//  *   --|-- --|--   *  | h          *|-- --|*  | h
//  *     |     |     *  |            *|     |*  |
//  *    / \   / \    *  V            * \   / *  V
//  ******************* ---           ********* ---
//
//                Match Horizontally:
//                                        w
//                                 |<----------->|
//           w 
//    |<----------->|              ***************
//                                 *             *
//    ***************              *   O     O   *
//    * --|-- --|-- *              * --|-- --|-- *
//    *   |     |   *              *   |     |   *
//    ***************              *  / \   / \  *
//                                 *             *
//                                 ***************
//
//                       Fit:
//
//                                       w
//                                 |<----------->|
//
//                                 ***************                             
//  ******************* ---        *             *
//  *     O     o     *  A         *   O     O   * 
//  *   --|-- --|--   *  | h       * --|-- --|-- * 
//  *     |     |     *  |         *   |     |   *
//  *    / \   / \    *  V         *  / \   / \  *
//  ******************* ---        *             *
//                                 ***************
//
//                      Crop:
//
//           w
//    |<----------->|            
//                                    ********* ---
//    ***************                 *O     O*  A
//    * --|-- --|-- *                 *|-- --|*  | h        
//    *   |     |   *                 *|     |*  |
//    ***************                 * \   / *  V
//                                    ********* ---  

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(CameraUtilMatchVertically,   "MatchVertically");
    TF_ADD_ENUM_NAME(CameraUtilMatchHorizontally, "MatchHotizontally");
    TF_ADD_ENUM_NAME(CameraUtilFit,               "Fit");
    TF_ADD_ENUM_NAME(CameraUtilCrop,              "Crop");
}

static
CameraUtilConformWindowPolicy
_ResolveConformWindowPolicy(const GfVec2d &size,
                            CameraUtilConformWindowPolicy policy,
                            double targetAspect)
{
    if ((policy == CameraUtilMatchVertically) or 
        (policy == CameraUtilMatchHorizontally)) {
        return policy;
    }

    const double aspect =
        (size[1] != 0.0) ? size[0] / size[1] : 1.0;

    if ((policy == CameraUtilFit) xor (aspect > targetAspect)) {
        return CameraUtilMatchVertically;
    }
    return CameraUtilMatchHorizontally;
}

/* static */
GfVec2d
CameraUtilConformedWindow(
    const GfVec2d &window,
    CameraUtilConformWindowPolicy policy,
    double targetAspect)
{
    const CameraUtilConformWindowPolicy resolvedPolicy =
        _ResolveConformWindowPolicy(window, policy, targetAspect);

    if (resolvedPolicy == CameraUtilMatchHorizontally) {
        return GfVec2d(window[0],
                       window[0] / (targetAspect != 0.0 ? targetAspect : 1.0));
    } else {
        return GfVec2d(window[1] * targetAspect, window[1]);
    }
}

/* static */
GfRange2d
CameraUtilConformedWindow(
    const GfRange2d &window,
    CameraUtilConformWindowPolicy policy,
    double targetAspect)
{
    const GfVec2d &size = window.GetSize();
    const GfVec2d center = (window.GetMin() + window.GetMax()) / 2.0;

    const CameraUtilConformWindowPolicy resolvedPolicy =
        _ResolveConformWindowPolicy(size, policy, targetAspect);
    
    
    if (resolvedPolicy == CameraUtilMatchHorizontally) {
        const double height =
            size[0] / (targetAspect != 0.0 ? targetAspect : 1.0);

        return GfRange2d(
            GfVec2d(window.GetMin()[0], center[1] - height / 2.0),
            GfVec2d(window.GetMax()[0], center[1] + height / 2.0));
    } else {
        const double width = size[1] * targetAspect;
        
        return GfRange2d(
            GfVec2d(center[0] - width / 2.0, window.GetMin()[1]),
            GfVec2d(center[0] + width / 2.0, window.GetMax()[1]));
    }
}

/* static */
GfVec4d
CameraUtilConformedWindow(
    const GfVec4d &window,
    CameraUtilConformWindowPolicy policy,
    double targetAspect)
{
    const GfRange2d original(GfVec2d(window[0], window[2]),
                             GfVec2d(window[1], window[3]));

    const GfRange2d conformed =
        CameraUtilConformedWindow(original, policy, targetAspect);

    return GfVec4d(conformed.GetMin()[0], conformed.GetMax()[0],
                   conformed.GetMin()[1], conformed.GetMax()[1]);
}

/* static */
void
CameraUtilConformWindow(
    GfCamera *camera,
    CameraUtilConformWindowPolicy policy,
    double targetAspect)
{
    const GfVec2d original(camera->GetHorizontalAperture(),
                           camera->GetVerticalAperture());
    const GfVec2d conformed = 
        CameraUtilConformedWindow(original, policy, targetAspect);

    camera->SetHorizontalAperture(conformed[0]);
    camera->SetVerticalAperture(conformed[1]);
}


void
CameraUtilConformWindow(
    GfFrustum *frustum,
    CameraUtilConformWindowPolicy policy, double targetAspect)
{
    GfRange2d screenWindowFitted = CameraUtilConformedWindow(
        frustum->GetWindow(), policy, targetAspect);
    frustum->SetWindow(screenWindowFitted);
}
