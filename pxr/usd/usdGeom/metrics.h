//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_GEOM_METRICS_H
#define PXR_USD_USD_GEOM_METRICS_H

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usd/common.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \file usdGeom/metrics.h
///
/// Schema and utilities for encoding various spatial and geometric metrics of
/// a UsdStage and its contents.
///
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
///
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
/// \return true if upAxis was successfully set.  The stage's UsdEditTarget
/// must be either its root layer or session layer.
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

/// \defgroup UsdGeomLinearUnits_group Encoding Stage Linear Units
/// 
/// As with \ref UsdGeomUpAxis_group, we restrict the encoding of linear
/// units to be stage-wide; if assembling assets of different metrics, it is
/// the assembler's responsibility to apply suitable correctives to the 
/// referenced data to bring it into the referencing stage's metric.
///
/// We encode linear measure as **meters per unit** (stage-level metadata
/// *metersPerUnit*) rather than units-per-meter because it makes it easier
/// to encode precise imperial units (one imperial foot = 0.3048 meters exactly).
///
/// If a UsdStage has no authored *metersPerUnit* we fall back to the value
/// \ref UsdGeomLinearUnits "UsdGeomLinearUnits::centimeters", i.e. 0.01.
///
/// The specified *metersPerUnit* metric should only be applied to 
/// **world space** (i.e. fully transformed) attributes of types:
/// - point3{h,f,d} (and Arrays thereof)
/// - vector3{h,f,d} (and Arrays thereof)
/// - schema attributes that define a length, such as Sphere.radius
///   and Boundable.extent
///
/// It may be tempting to interpret raw attribute values, and even 
/// xformOp:translate values in the *metersPerUnit* metric, but because any
/// xformOp:scale transformations will change the metric of the authored 
/// attribute, and in fact we *require* this sort of scale change when
/// referencing assets of differing metrics, it is unreliable to do so.

/// @{

/// Return *stage*'s authored *metersPerUnit*, or 0.01 if unauthored.
/// \sa UsdGeomLinearUnits_group
USDGEOM_API
double UsdGeomGetStageMetersPerUnit(const UsdStageWeakPtr &stage);

/// Return whether *stage* has an authored *metersPerUnit*.
/// \sa UsdGeomLinearUnits_group
USDGEOM_API
bool UsdGeomStageHasAuthoredMetersPerUnit(const UsdStageWeakPtr &stage);

/// Author *stage*'s *metersPerUnit*.
///
/// \return true if metersPerUnit was successfully set.  The stage's
/// UsdEditTarget must be either its root layer or session layer.
/// \sa UsdGeomLinearUnits_group
USDGEOM_API
bool UsdGeomSetStageMetersPerUnit(const UsdStageWeakPtr &stage, 
                                  double metersPerUnit);

/// Return *true* if the two given metrics are within the provided
/// relative *epsilon* of each other, when you need to know an absolute
/// metric rather than a scaling factor.  
///
/// Use like so:
/// \code
/// double stageUnits = UsdGeomGetStageMetersPerUnit(stage);
///
/// if (UsdGeomLinearUnitsAre(stageUnits, UsdGeomLinearUnits::meters))
///     // do something for meters
/// else if (UsdGeomLinearUnitsAre(stageUnits, UsdGeomLinearUnits::feet))
///     // do something for feet
/// \endcode
///
/// \return *false* if either input is zero or negative, otherwise relative
/// floating-point comparison between the two inputs.
/// \sa UsdGeomLinearUnits_group
USDGEOM_API
bool UsdGeomLinearUnitsAre(double authoredUnits, double standardUnits,
                           double epsilon = 1e-5);

/// \class UsdGeomLinearUnits
/// Container class for static double-precision symbols representing common
/// units of measure expressed in meters.
/// \sa UsdGeomLinearUnits_group
class UsdGeomLinearUnits {
public:
    static constexpr double nanometers  = 1e-9;
    static constexpr double micrometers = 1e-6;
    static constexpr double millimeters = 0.001;
    static constexpr double centimeters = 0.01;
    static constexpr double meters = 1.0;
    static constexpr double kilometers = 1000;

    /// Measured for one year = 365.25 days
    static constexpr double lightYears = 9.4607304725808e15;

    static constexpr double inches = 0.0254;
    static constexpr double feet   = 0.3048;
    static constexpr double yards  = 0.9144;
    static constexpr double miles  = 1609.344;
};

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_GEOM_METRICS_H
