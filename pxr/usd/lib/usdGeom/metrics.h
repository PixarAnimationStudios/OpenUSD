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
#ifndef USDGEOM_METRICS_H
#define USDGEOM_METRICS_H




#include "pxr/usd/usdGeom/api.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usd/common.h"

/// \file metrics.h
/// \brief Schema and utilities for encoding various spatial and geometric
/// metrics of a UsdStage and its contents.

/// \defgroup UsdGeomUpAxis_group Encoding Stage UpAxis
/// 
/// Even if all UsdGeomCamera objects in a scene are encoded with a 
/// transformation that assumes the Y axis is up, that cannot always imply
/// that the geometry contained in the scene was modeled with the Y axis
/// pointing up, since some modeling applications assume Z is up, and others
/// allow you to configure whether Y or Z is up.  Rather than impose one axis
/// or the other on all UsdGeom scenes - which would require either
/// transposing point coordinates into and out of some applications, or making
/// assumptions about where a -90 degree rotation has been applied and 
/// effectively, mysteriously swapping the roles of Y and Z coordinates -
/// we allow each stage to carry, in its root layer, a declaration of the
/// up axis for all the geometry contained in the stage, and require 
/// applications to consult it if they wish to create new cameras to view
/// the scene in its intended orientation.
///
/// Note that because you can make only a Stage-level declaration of up axis,
/// it must hold true for all referenced geometry and assets.  A conscientious
/// set/environment-construction tool could examine the up axis of referenced
/// assets as they are being referenced, and apply a corrective rotation
/// on the referencing prim as it is being added to the scene, when the
/// referenced up axis differs from the set's.
///
/// We provide free functions for setting (UsdGeomSetStageUpAxis()),
/// retrieving (UsdGeomGetStageUpAxis()), and declaring a site-level
/// fallback up axis (UsdGeomGetFallbackUpAxis()) that can be configured
/// with a PlugPlugin plugInfo.json file.  See the individual methods for
/// details.
///
/// The stage up axis is encoded as stage metadatum \em upAxis, whose legal
/// values are "Y" and "Z", as represented by UsdGeomTokens->y and
/// UsdGeomTokens->z.  Of course, constructing a correct camera view of a
/// scene depends not only on the up axis, but also on the handedness of the
/// coordinate system.  Like OpenGL and the fallback for
/// UsdGeomGprim::GetOrientationAttr(), <b>UsdGeom stipulates a right-handed
/// coordinate system.</b> Therefore, when viewing a UsdStage with a "Y" up
/// axis, the stage's Z axis will be pointing out of the screen, and when
/// viewing a UsdStage with a "Z" up axis, the stage's Y axis will be
/// pointing into the screen.
/// @{

/// Fetch and return \p stage 's upAxis.  If unauthored, will return the
/// value provided by UsdGeomGetFallbackUpAxis().  Exporters, however, are
/// strongly encouraged to always set the upAxis for every USD file they create.
///
/// \return one of: UsdGeomTokens->y or UsdGeomTokens->z, unless there was
/// an error, in which case returns an empty TfToken
///
/// \sa UsdGeomUpAxis_group
USDGEOM_API
TfToken UsdGeomGetStageUpAxis(const UsdStageWeakPtr &stage);


/// Set \p stage 's upAxis to \p axis, which must be one of UsdGeomTokens->y
/// or UsdGeomTokens->z
/// 
/// UpAxis is stage-level metadata, therefore see UsdStage::SetMetadata().
///
/// \return true if upAxis was successfully set.
/// \sa UsdGeomUpAxis_group
USDGEOM_API
bool UsdGeomSetStageUpAxis(const UsdStageWeakPtr &stage, const TfToken &axis);


/// Return the site-level fallback up axis as a TfToken.
///
/// In a generic installation of USD, the fallback will be "Y".  This can be
/// changed to "Z" by adding, in a plugInfo.json file discoverable by USD's
/// PlugPlugin mechanism:
/// \code{json}
///    "UsdGeomMetrics": {
///        "upAxis": "Z"
///    }
/// \endcode
///
/// If more than one such entry is discovered and the values for upAxis differ,
/// we will issue a warning during the first call to this function, and ignore
/// all of them, so that we devolve to deterministic behavior of Y up axis
/// until the problem is rectified.
USDGEOM_API
TfToken UsdGeomGetFallbackUpAxis();

/// @}

#endif // USDGEOM_METRICS_H
