//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLIGHTFIELD_GENERATED_SPHERICALHARMONICSAPI_H
#define USDLIGHTFIELD_GENERATED_SPHERICALHARMONICSAPI_H

/// \file usdLightField/sphericalHarmonicsAPI.h

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
// SPHERICALHARMONICSAPI                                                      //
// -------------------------------------------------------------------------- //

/// \class UsdLightFieldSphericalHarmonicsAPI
///
/// AppliedAPI schema to extend UsdGeomPoints to describe Spherical Harmonic data.
/// Spherical harmonics data must be interpreted in context of the point types it is applied to, for example
/// with ellipsoid gaussians using the GaussianAPI above.
/// 
/// The spherical harmonic coefficients can be provided as either half or float based data.
/// If floats are present, then renderers should prefer the higher precision data if they are able.
/// 
/// The spherical harmonic data is provided as an array of half3 or float3 data.  They are represented by 3 component values
/// as they are constructed to be the eigenfunctions of the angular part of the Laplacian in three dimensions.
/// 
/// Each point must have the same number of spherical harmonic coefficients, and therefore the array size must be equal
/// to the number of points multiplied by the number of spherical harmonic coefficients.
/// This allows the number of coefficients present for each point can be inferred by the array size.
/// 
/// Spherical Harmoics contain all but the 0th degree spherical harmonic, which is already encoded in the UsdGeomPoints.displayColor attribute.
/// Each point must have the same number of spherical harmonic coefficients, and therefore the array size must be
/// equal to the number of points multiplied by the number of spherical harmonic coefficients (minus one due to the 0th degree being used for color).
/// This allows the number of coefficients present for each point can be inferred by the array size.
/// 
/// Authors are responsible for authoring the correct color space metadata for their spherical harmonics data.
/// 
///
class UsdLightFieldSphericalHarmonicsAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdLightFieldSphericalHarmonicsAPI on UsdPrim \p prim .
    /// Equivalent to UsdLightFieldSphericalHarmonicsAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLightFieldSphericalHarmonicsAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdLightFieldSphericalHarmonicsAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdLightFieldSphericalHarmonicsAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLightFieldSphericalHarmonicsAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDLIGHTFIELD_API
    virtual ~UsdLightFieldSphericalHarmonicsAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLIGHTFIELD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLightFieldSphericalHarmonicsAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLightFieldSphericalHarmonicsAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLIGHTFIELD_API
    static UsdLightFieldSphericalHarmonicsAPI
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
    /// This information is stored by adding "SphericalHarmonicsAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdLightFieldSphericalHarmonicsAPI object is returned upon success. 
    /// An invalid (or empty) UsdLightFieldSphericalHarmonicsAPI object is returned upon 
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
    static UsdLightFieldSphericalHarmonicsAPI 
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
    // PRIMVARSSPHERICALHARMONICS 
    // --------------------------------------------------------------------- //
    /// Half buffer containing spherical harmonics data.
    /// See the description of the API for more information on interpreting the data.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `half3[] primvars:sphericalHarmonics` |
    /// | C++ Type | VtArray<GfVec3h> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Half3Array |
    USDLIGHTFIELD_API
    UsdAttribute GetPrimvarsSphericalHarmonicsAttr() const;

    /// See GetPrimvarsSphericalHarmonicsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLIGHTFIELD_API
    UsdAttribute CreatePrimvarsSphericalHarmonicsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PRIMVARSSPHERICALHARMONICSF 
    // --------------------------------------------------------------------- //
    /// Float buffer containing SH data.
    /// See the description of the API for more information on interpreting the data.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float3[] primvars:sphericalHarmonicsf` |
    /// | C++ Type | VtArray<GfVec3f> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float3Array |
    USDLIGHTFIELD_API
    UsdAttribute GetPrimvarsSphericalHarmonicsfAttr() const;

    /// See GetPrimvarsSphericalHarmonicsfAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLIGHTFIELD_API
    UsdAttribute CreatePrimvarsSphericalHarmonicsfAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
