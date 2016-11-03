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
/// down the -Z axis, with Y as the up axis.
/// 
/// \sa \ref usdGeom_linAlgBasics "UsdGeom Linear Algebra Basic Assumptions"
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
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = true;

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

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDGEOM_API
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // PROJECTION 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: perspective
    /// \n  \ref UsdGeomTokens "Allowed Values": [perspective, orthographic]
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
    /// Horizontal aperture in millimeters (or, more general, tenths
    /// of a world unit).
    /// Defaults to the standard 35mm spherical projector aperture.
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 20.9549999237
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
    /// Vertical aperture in millimeters (or, more general, tenths of
    /// a world unit).
    /// Defaults to the standard 35mm spherical projector aperture.
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 15.2908000946
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
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 0.0
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
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 0.0
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
    /// Perspective focal length in millimeters (or, more general,
    /// tenths of a world unit).
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 50.0
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
    /// Near and far clipping distances in centimeters (or, more
    /// general, world units).
    ///
    /// \n  C++ Type: GfVec2f
    /// \n  Usd Type: SdfValueTypeNames->Float2
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (1, 1e+06)
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
    /// \n  C++ Type: VtArray<GfVec4f>
    /// \n  Usd Type: SdfValueTypeNames->Float4Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: []
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
    /// Lens aperture. Defaults to 0.0, which turns off focusing.
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 0.0
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
    /// Distance from the camera to the focus plane in centimeters (or
    /// more general, world units).
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 0.0
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
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: mono
    /// \n  \ref UsdGeomTokens "Allowed Values": [mono, left, right]
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
    /// value indicates that the shutter opens before the current's
    /// frame time). Used for motion blur.
    ///
    /// \n  C++ Type: double
    /// \n  Usd Type: SdfValueTypeNames->Double
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 0.0
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
    /// renderer should produce a black image.
    ///
    /// \n  C++ Type: double
    /// \n  Usd Type: SdfValueTypeNames->Double
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 0.0
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
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to close the class delcaration with }; and complete the
    // include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

    /// Creates a GfCamera object from the attribute values at \p time.
    ///
    /// Outside of Pixar, never touch \p isZup, it is deprecated !
    ///
    /// Set \p isZup to UsdUtilsGetCamerasAreZup(prim.GetStage()). Eventually,
    /// z-Up encoding of cameras will be dropped and this parameter will go
    /// away.
    USDGEOM_API
    GfCamera GetCamera(const UsdTimeCode &time, const bool isZup = false) const;

    /// Write attribute values from \p camera for \p time.
    ///
    USDGEOM_API
    void SetFromCamera(const GfCamera &camera, const UsdTimeCode &time);
};

#endif
