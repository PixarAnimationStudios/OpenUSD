//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLIGHTFIELD_GENERATED_GAUSSIANSAPI_H
#define USDLIGHTFIELD_GENERATED_GAUSSIANSAPI_H

/// \file usdLightField/gaussiansAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLightField/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLightField/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// GAUSSIANSAPI                                                               //
// -------------------------------------------------------------------------- //

/// \class UsdLightFieldGaussiansAPI
///
/// AppliedAPI schema to extend UsdGeomPoints to describe Gaussians.
/// 
/// Orientations and Scales are provided in both half and float precision data to allow flexibility
/// in storage. Renderers should prefer the float precision data if present, and they are capable.
/// 
/// UsdGeomPoints already provides a Widths attribute that represents the size of the point.  In order
/// to allow renderers that do not support rendering Gaussians we suggest setting Widths to the length
/// of the Scales attribute, and then normalizing the Scales attribute.  Renderers reconstruct the
/// original Scales attribute value by multiplying by the UsdGeomPoints Widths attribute.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdLightFieldTokens.
/// So to set an attribute to the value "rightHanded", use UsdLightFieldTokens->rightHanded
/// as the value.
///
class UsdLightFieldGaussiansAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdLightFieldGaussiansAPI on UsdPrim \p prim .
    /// Equivalent to UsdLightFieldGaussiansAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLightFieldGaussiansAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdLightFieldGaussiansAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdLightFieldGaussiansAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLightFieldGaussiansAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDLIGHTFIELD_API
    virtual ~UsdLightFieldGaussiansAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLIGHTFIELD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLightFieldGaussiansAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLightFieldGaussiansAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLIGHTFIELD_API
    static UsdLightFieldGaussiansAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Returns true if this <b>single-apply</b> API schema can be applied to 
    /// the given \p prim. If this schema can not be a applied to the prim, 
    /// this returns false and, if provided, populates \p whyNot with the 
    /// reason it can not be applied.
    /// 
    /// Note that if CanApply returns false, that does not necessarily imply
    /// that calling Apply will fail. Callers are expected to call CanApply
    /// before calling Apply if they want to ensure that it is valid to 
    /// apply a schema.
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDLIGHTFIELD_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "GaussiansAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdLightFieldGaussiansAPI object is returned upon success. 
    /// An invalid (or empty) UsdLightFieldGaussiansAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDLIGHTFIELD_API
    static UsdLightFieldGaussiansAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDLIGHTFIELD_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDLIGHTFIELD_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDLIGHTFIELD_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // GAUSSIANSHAPE 
    // --------------------------------------------------------------------- //
    /// The shape of the gaussian to render.
    /// Gaussians may be available in a range of shapes, therefore this list of allowedTokens is not strictly
    /// comprehensive. A renderer may support a subset or superset of the allowed tokens.
    /// 
    /// An elipsoid is what many people will colloquially refer to as a splat.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token gaussianShape = "elipsoid"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdLightFieldTokens "Allowed Values" | elipsoid, triangle, plane |
    USDLIGHTFIELD_API
    UsdAttribute GetGaussianShapeAttr() const;

    /// See GetGaussianShapeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLIGHTFIELD_API
    UsdAttribute CreateGaussianShapeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SORTINGMODEHINT 
    // --------------------------------------------------------------------- //
    /// A hint for the renderer on how to sort the gaussians while drawing.
    /// Renderers are free to ignore this, but the hint is often valuable to tune the rendering of the scene.
    /// 
    /// Rasterizers usually sort gaussians from back to front.
    /// This attribute is a hint for the metric used to sort the gaussians with respect to the camera.
    /// 
    /// 'zDepth' sorts the gaussians using the z component of the position of the gaussians once
    /// transformed in to camera space.
    /// 
    /// 'cameraDistance' sorts the gaussians using the euclidean distance between the gaussians
    /// and the camera.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token sortingModeHint = "zDepth"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdLightFieldTokens "Allowed Values" | zDepth, cameraDistance |
    USDLIGHTFIELD_API
    UsdAttribute GetSortingModeHintAttr() const;

    /// See GetSortingModeHintAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLIGHTFIELD_API
    UsdAttribute CreateSortingModeHintAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PROJECTIONMODEHINT 
    // --------------------------------------------------------------------- //
    /// A hint for the renderer on how to project the gaussian to achieve a perspective correct view.
    /// Renderers are free to ignore this, but the hint is often valuable to tune the rendering of the scene.
    /// 
    /// 'Perspective' projection is similar to standard object rendering from a camera view.
    /// Gaussians are projected with scaling and distortion based on depth.
    /// 
    /// 'Tangential' projection treats the image plane as a tangent to the viewing sphere.
    /// Gaussians are projected orthogonally, preserving shape and scale better, which helps reduce
    /// distortion for certain rendering applications like novel view synthesis
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token projectionModeHint = "perspective"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdLightFieldTokens "Allowed Values" | perspective, tangential |
    USDLIGHTFIELD_API
    UsdAttribute GetProjectionModeHintAttr() const;

    /// See GetProjectionModeHintAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLIGHTFIELD_API
    UsdAttribute CreateProjectionModeHintAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ORIENTATIONS 
    // --------------------------------------------------------------------- //
    /// Orientation of the gaussians coordinate frame for scale to be applied in (quaternion).
    /// Half precision data.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `quath[] orientations` |
    /// | C++ Type | VtArray<GfQuath> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->QuathArray |
    USDLIGHTFIELD_API
    UsdAttribute GetOrientationsAttr() const;

    /// See GetOrientationsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLIGHTFIELD_API
    UsdAttribute CreateOrientationsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ORIENTATIONSF 
    // --------------------------------------------------------------------- //
    /// Orientation of the gaussians coordinate frame for scale to be applied in (quaternion).
    /// Float precision data, renderers should prefer float data over half if present.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `quatf[] orientationsf` |
    /// | C++ Type | VtArray<GfQuatf> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->QuatfArray |
    USDLIGHTFIELD_API
    UsdAttribute GetOrientationsfAttr() const;

    /// See GetOrientationsfAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLIGHTFIELD_API
    UsdAttribute CreateOrientationsfAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SCALES 
    // --------------------------------------------------------------------- //
    /// Scale of each point in an XYZ coordinate frame defined by orientation.
    /// Half precision data.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `half3[] scales` |
    /// | C++ Type | VtArray<GfVec3h> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Half3Array |
    USDLIGHTFIELD_API
    UsdAttribute GetScalesAttr() const;

    /// See GetScalesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLIGHTFIELD_API
    UsdAttribute CreateScalesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SCALESF 
    // --------------------------------------------------------------------- //
    /// Scale of each point in an XYZ coordinate frame defined by orientation.
    /// Float precision data, renderers should prefer float data over half if present.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float3[] scalesf` |
    /// | C++ Type | VtArray<GfVec3f> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float3Array |
    USDLIGHTFIELD_API
    UsdAttribute GetScalesfAttr() const;

    /// See GetScalesfAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLIGHTFIELD_API
    UsdAttribute CreateScalesfAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
