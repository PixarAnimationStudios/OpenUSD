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
#ifndef USDGEOM_GENERATED_CAMERA_H
#define USDGEOM_GENERATED_CAMERA_H

/// \file usdGeom/camera.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/base/gf/camera.h" 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// CAMERA                                                                     //
// -------------------------------------------------------------------------- //

/// \class UsdGeomCamera
///
/// Transformable camera.
/// 
/// Describes optical properties of a camera via a common set of attributes
/// that provide control over the camera's frustum as well as its depth of
/// field. For stereo, the left and right camera are individual prims tagged
/// through the \ref UsdGeomCamera::GetStereoRoleAttr() "stereoRole attribute".
/// 
/// There is a corresponding class GfCamera, which can hold the state of a
/// camera (at a particular time). \ref UsdGeomCamera::GetCamera() and
/// \ref UsdGeomCamera::SetFromCamera() convert between a USD camera prim and
/// a GfCamera.
/// 
/// To obtain the camera's location in world space, call the following on a
/// UsdGeomCamera 'camera':
/// \code
/// GfMatrix4d camXform = camera.ComputeLocalToWorldTransform(time);
/// \endcode
/// \note
/// <b>Cameras in USD are always "Y up", regardless of the stage's orientation
/// (i.e. UsdGeomGetStageUpAxis()).</b>  This means that the inverse of 
/// 'camXform' (the VIEW half of the <A HREF="http://www.glprogramming.com/red/chapter03.html#name2">MODELVIEW transform in OpenGL parlance</A>) 
/// will transform the world such that the camera is at the origin, looking 
/// down the -Z axis, with +Y as the up axis, and +X pointing to the right.
/// This describes a __right handed coordinate system__. 
/// 
/// \section UsdGeom_CameraUnits Units of Measure for Camera Properties
/// 
/// Despite the familiarity of millimeters for specifying some physical
/// camera properties, UsdGeomCamera opts for greater consistency with all
/// other UsdGeom schemas, which measure geometric properties in scene units,
/// as determined by UsdGeomGetStageMetersPerUnit().  We do make a
/// concession, however, in that lens and filmback properties are measured in
/// __tenths of a scene unit__ rather than "raw" scene units.  This means
/// that with the fallback value of .01 for _metersPerUnit_ - i.e. scene unit
/// of centimeters - then these "tenth of scene unit" properties are
/// effectively millimeters.
/// 
/// \note If one adds a Camera prim to a UsdStage whose scene unit is not
/// centimeters, the fallback values for filmback properties will be
/// incorrect (or at the least, unexpected) in an absolute sense; however,
/// proper imaging through a "default camera" with focusing disabled depends
/// only on ratios of the other properties, so the camera is still usable.
/// However, it follows that if even one property is authored in the correct
/// scene units, then they all must be.
/// 
/// \section UsdGeom_CameraExposure Camera Exposure Model
/// 
/// UsdGeomCamera models exposure by a camera in terms of exposure time, ISO,
/// f-number, and exposure compensation, mirroring the controls on a real camera.
/// These parameters are provided by \ref UsdGeomCamera::GetExposureTimeAttr(),
/// \ref UsdGeomCamera::GetExposureIsoAttr(), \ref UsdGeomCamera::GetExposureFNumberAttr(), 
/// and \ref UsdGeomCamera::GetExposureAttr(), respectively. 
/// \ref UsdGeomCamera::GetExposureResponsivityAttr() provides an additional scaling
/// factor to model the overall responsivity of the system, including response of 
/// the sensor and loss by the lens.
/// 
/// The combination of these parameters gives an exposure scaling factor (also 
/// referred to as the __imaging ratio__) that converts from incident luminance 
/// passing through the lens system and reaching the sensor, in __nits__, to an 
/// output signal proportional to photometric exposure in __lux-seconds__. 
/// 
/// The calculated scaling factor can be obtained from 
/// \ref UsdGeomCamera::GetExposureScale(), which is also the value provided by 
/// \ref HdCamera::GetExposure(). It is computed as:
/// \code
/// exposureScale = 
/// (exposureTime * (exposureIso/100) * pow(2, exposure) * exposureResponsivity) 
/// / (exposureFStop * exposureFStop)
/// \endcode
/// 
/// Renderers should simply multiply the brightness of the image by the exposure 
/// scale. The default values for the exposure-related attributes combine to give
/// a multiplier of 1.0.
/// 
/// 
/// \sa \ref UsdGeom_LinAlgBasics
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdGeomTokens.
/// So to set an attribute to the value "rightHanded", use UsdGeomTokens->rightHanded
/// as the value.
///
class UsdGeomCamera : public UsdGeomXformable
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdGeomCamera on UsdPrim \p prim .
    /// Equivalent to UsdGeomCamera::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomCamera(const UsdPrim& prim=UsdPrim())
        : UsdGeomXformable(prim)
    {
    }

    /// Construct a UsdGeomCamera on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomCamera(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomCamera(const UsdSchemaBase& schemaObj)
        : UsdGeomXformable(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomCamera();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomCamera holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomCamera(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomCamera
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Attempt to ensure a \a UsdPrim adhering to this schema at \p path
    /// is defined (according to UsdPrim::IsDefined()) on this stage.
    ///
    /// If a prim adhering to this schema at \p path is already defined on this
    /// stage, return that prim.  Otherwise author an \a SdfPrimSpec with
    /// \a specifier == \a SdfSpecifierDef and this schema's prim type name for
    /// the prim at \p path at the current EditTarget.  Author \a SdfPrimSpec s
    /// with \p specifier == \a SdfSpecifierDef and empty typeName at the
    /// current EditTarget for any nonexistent, or existing but not \a Defined
    /// ancestors.
    ///
    /// The given \a path must be an absolute prim path that does not contain
    /// any variant selections.
    ///
    /// If it is impossible to author any of the necessary PrimSpecs, (for
    /// example, in case \a path cannot map to the current UsdEditTarget's
    /// namespace) issue an error and return an invalid \a UsdPrim.
    ///
    /// Note that this method may return a defined prim whose typeName does not
    /// specify this schema class, in case a stronger typeName opinion overrides
    /// the opinion at the current EditTarget.
    ///
    USDGEOM_API
    static UsdGeomCamera
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDGEOM_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDGEOM_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDGEOM_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // PROJECTION 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token projection = "perspective"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref UsdGeomTokens "Allowed Values" | perspective, orthographic |
    USDGEOM_API
    UsdAttribute GetProjectionAttr() const;

    /// See GetProjectionAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateProjectionAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // HORIZONTALAPERTURE 
    // --------------------------------------------------------------------- //
    /// Horizontal aperture in tenths of a scene unit; see 
    /// \ref UsdGeom_CameraUnits . Default is the equivalent of 
    /// the standard 35mm spherical projector aperture.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float horizontalAperture = 20.955` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetHorizontalApertureAttr() const;

    /// See GetHorizontalApertureAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateHorizontalApertureAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VERTICALAPERTURE 
    // --------------------------------------------------------------------- //
    /// Vertical aperture in tenths of a scene unit; see 
    /// \ref UsdGeom_CameraUnits . Default is the equivalent of 
    /// the standard 35mm spherical projector aperture.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float verticalAperture = 15.2908` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetVerticalApertureAttr() const;

    /// See GetVerticalApertureAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateVerticalApertureAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // HORIZONTALAPERTUREOFFSET 
    // --------------------------------------------------------------------- //
    /// Horizontal aperture offset in the same units as
    /// horizontalAperture. Defaults to 0.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float horizontalApertureOffset = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetHorizontalApertureOffsetAttr() const;

    /// See GetHorizontalApertureOffsetAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateHorizontalApertureOffsetAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VERTICALAPERTUREOFFSET 
    // --------------------------------------------------------------------- //
    /// Vertical aperture offset in the same units as
    /// verticalAperture. Defaults to 0.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float verticalApertureOffset = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetVerticalApertureOffsetAttr() const;

    /// See GetVerticalApertureOffsetAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateVerticalApertureOffsetAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FOCALLENGTH 
    // --------------------------------------------------------------------- //
    /// Perspective focal length in tenths of a scene unit; see 
    /// \ref UsdGeom_CameraUnits .
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float focalLength = 50` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetFocalLengthAttr() const;

    /// See GetFocalLengthAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateFocalLengthAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CLIPPINGRANGE 
    // --------------------------------------------------------------------- //
    /// Near and far clipping distances in scene units; see 
    /// \ref UsdGeom_CameraUnits .
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float2 clippingRange = (1, 1000000)` |
    /// | C++ Type | GfVec2f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float2 |
    USDGEOM_API
    UsdAttribute GetClippingRangeAttr() const;

    /// See GetClippingRangeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateClippingRangeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CLIPPINGPLANES 
    // --------------------------------------------------------------------- //
    /// Additional, arbitrarily oriented clipping planes.
    /// A vector (a,b,c,d) encodes a clipping plane that cuts off
    /// (x,y,z) with a * x + b * y + c * z + d * 1 < 0 where (x,y,z)
    /// are the coordinates in the camera's space.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float4[] clippingPlanes = []` |
    /// | C++ Type | VtArray<GfVec4f> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float4Array |
    USDGEOM_API
    UsdAttribute GetClippingPlanesAttr() const;

    /// See GetClippingPlanesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateClippingPlanesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FSTOP 
    // --------------------------------------------------------------------- //
    /// Lens aperture. Defaults to 0.0, which turns off depth of field effects.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float fStop = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetFStopAttr() const;

    /// See GetFStopAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateFStopAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FOCUSDISTANCE 
    // --------------------------------------------------------------------- //
    /// Distance from the camera to the focus plane in scene units; see 
    /// \ref UsdGeom_CameraUnits .
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float focusDistance = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetFocusDistanceAttr() const;

    /// See GetFocusDistanceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateFocusDistanceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // STEREOROLE 
    // --------------------------------------------------------------------- //
    /// If different from mono, the camera is intended to be the left
    /// or right camera of a stereo setup.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token stereoRole = "mono"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdGeomTokens "Allowed Values" | mono, left, right |
    USDGEOM_API
    UsdAttribute GetStereoRoleAttr() const;

    /// See GetStereoRoleAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateStereoRoleAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SHUTTEROPEN 
    // --------------------------------------------------------------------- //
    /// Frame relative shutter open time in UsdTimeCode units (negative
    /// value indicates that the shutter opens before the current
    /// frame time). Used for motion blur.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `double shutter:open = 0` |
    /// | C++ Type | double |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Double |
    USDGEOM_API
    UsdAttribute GetShutterOpenAttr() const;

    /// See GetShutterOpenAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateShutterOpenAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SHUTTERCLOSE 
    // --------------------------------------------------------------------- //
    /// Frame relative shutter close time, analogous comments from
    /// shutter:open apply. A value greater or equal to shutter:open
    /// should be authored, otherwise there is no exposure and a
    /// renderer should produce a black image. Used for motion blur.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `double shutter:close = 0` |
    /// | C++ Type | double |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Double |
    USDGEOM_API
    UsdAttribute GetShutterCloseAttr() const;

    /// See GetShutterCloseAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateShutterCloseAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EXPOSURE 
    // --------------------------------------------------------------------- //
    /// Exposure compensation, as a log base-2 value.  The default
    /// of 0.0 has no effect.  A value of 1.0 will double the
    /// image-plane intensities in a rendered image; a value of
    /// -1.0 will halve them.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float exposure = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetExposureAttr() const;

    /// See GetExposureAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateExposureAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EXPOSUREISO 
    // --------------------------------------------------------------------- //
    /// The speed rating of the sensor or film when calculating exposure.
    /// Higher numbers give a brighter image, lower numbers darker.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float exposure:iso = 100` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetExposureIsoAttr() const;

    /// See GetExposureIsoAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateExposureIsoAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EXPOSURETIME 
    // --------------------------------------------------------------------- //
    /// Time in seconds that the sensor is exposed to light when calculating exposure.
    /// Longer exposure times create a brighter image, shorter times darker.
    /// Note that shutter:open and shutter:close model essentially the 
    /// same property of a physical camera, but are for specifying the 
    /// size of the motion blur streak which is for practical purposes
    /// useful to keep separate.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float exposure:time = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetExposureTimeAttr() const;

    /// See GetExposureTimeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateExposureTimeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EXPOSUREFSTOP 
    // --------------------------------------------------------------------- //
    /// f-stop of the aperture when calculating exposure. Smaller numbers
    /// create a brighter image, larger numbers darker.
    /// Note that the fStop attribute also models the diameter of the camera
    /// aperture, but for specifying depth of field.  For practical 
    /// purposes it is useful to keep the exposure and the depth of field
    /// controls separate.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float exposure:fStop = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetExposureFStopAttr() const;

    /// See GetExposureFStopAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateExposureFStopAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // EXPOSURERESPONSIVITY 
    // --------------------------------------------------------------------- //
    /// Scalar multiplier representing overall responsivity of the 
    /// sensor system to light when calculating exposure. Intended to be
    /// used as a per camera/lens system measured scaling value.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float exposure:responsivity = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDGEOM_API
    UsdAttribute GetExposureResponsivityAttr() const;

    /// See GetExposureResponsivityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateExposureResponsivityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with PXR_NAMESPACE_CLOSE_SCOPE
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

    /// Creates a GfCamera object from the attribute values at \p time.
    ///
    USDGEOM_API
    GfCamera GetCamera(const UsdTimeCode &time) const;

    /// Write attribute values from \p camera for \p time.
    /// These attributes will be updated:
    ///  - projection
    ///  - horizontalAperture
    ///  - horizontalApertureOffset
    ///  - verticalAperture
    ///  - verticalApertureOffset
    ///  - focalLength
    ///  - clippingRange
    ///  - clippingPlanes
    ///  - fStop
    ///  - focalDistance
    ///  - xformOpOrder and xformOp:transform
    /// 
    /// \note This will clear any existing xformOpOrder and replace
    /// it with a single xformOp:transform entry. The xformOp:transform
    /// property is created or updated here to match the transform
    /// on \p camera . This operation will fail if there are stronger xform op
    /// opinions in the composed layer stack that are stronger than that of
    /// the current edit target.
    ///
    USDGEOM_API
    void SetFromCamera(const GfCamera &camera, const UsdTimeCode &time);

    /// Computes the ratio between incident luminance and photometric exposure 
    /// (in lux-seconds), given the <tt>exposure</tt>, <tt>exposure:iso</tt>, 
    /// <tt>exposure:fnumber</tt>, <tt>exposure:time</tt> and <tt>exposure:responsivity</tt> 
    /// attributes.
    ///
    /// This is expected to be applied as a multiplier to the brightness of the 
    /// image generated by the renderer, and given physically meaningful lighting
    /// values in the scene, allows the exposure controls on UsdGeomCamera to behave
    /// like those of a real camera.
    ///
    USDGEOM_API
    float GetExposureScale(UsdTimeCode time=UsdTimeCode::Default()) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
