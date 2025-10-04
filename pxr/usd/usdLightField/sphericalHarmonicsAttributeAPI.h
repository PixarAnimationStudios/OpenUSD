//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLIGHTFIELD_GENERATED_SPHERICALHARMONICSATTRIBUTEAPI_H
#define USDLIGHTFIELD_GENERATED_SPHERICALHARMONICSATTRIBUTEAPI_H

/// \file usdLightField/sphericalHarmonicsAttributeAPI.h

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
// SPHERICALHARMONICSATTRIBUTEAPI                                             //
// -------------------------------------------------------------------------- //

/// \class UsdLightFieldSphericalHarmonicsAttributeAPI
///
/// A ParticleField related appliedAPI schema that provides spherical
/// harmonics attributes to define the radiance of the particles.
/// 
/// Attributes are provided in both `float` and `half` types for some
/// easy data footprint affordance, data consumers should prefer
/// `float` version if available.
///
class UsdLightFieldSphericalHarmonicsAttributeAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdLightFieldSphericalHarmonicsAttributeAPI on UsdPrim \p prim .
    /// Equivalent to UsdLightFieldSphericalHarmonicsAttributeAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLightFieldSphericalHarmonicsAttributeAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdLightFieldSphericalHarmonicsAttributeAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdLightFieldSphericalHarmonicsAttributeAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLightFieldSphericalHarmonicsAttributeAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDLIGHTFIELD_API
    virtual ~UsdLightFieldSphericalHarmonicsAttributeAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLIGHTFIELD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLightFieldSphericalHarmonicsAttributeAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLightFieldSphericalHarmonicsAttributeAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLIGHTFIELD_API
    static UsdLightFieldSphericalHarmonicsAttributeAPI
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
    /// This information is stored by adding "SphericalHarmonicsAttributeAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdLightFieldSphericalHarmonicsAttributeAPI object is returned upon success. 
    /// An invalid (or empty) UsdLightFieldSphericalHarmonicsAttributeAPI object is returned upon 
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
    static UsdLightFieldSphericalHarmonicsAttributeAPI 
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
    // RADIANCESPHERICALHARMONICSDEGREE 
    // --------------------------------------------------------------------- //
    /// The maximum degree of the spherical harmonics.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform int radiance:sphericalHarmonicsDegree = 3` |
    /// | C++ Type | int |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDLIGHTFIELD_API
    UsdAttribute GetRadianceSphericalHarmonicsDegreeAttr() const;

    /// See GetRadianceSphericalHarmonicsDegreeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLIGHTFIELD_API
    UsdAttribute CreateRadianceSphericalHarmonicsDegreeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PRIMVARSRADIANCESPHERICALHARMONICSCOEFFICIENTS 
    // --------------------------------------------------------------------- //
    /// Flattened array of SH coefficients.
    /// The each of the different SH coefficients are not interleaved. The SH
    /// coefficents for a given particle are encoded in the array sequentially,
    /// with the all of the SH coefficients for the first particle being defined
    /// first, followed by all the SH coefficients for the second particle and
    /// so on. This means the elementSize metadata can be authored, but is
    /// dependent on the degree of the spherical harmonic data being applied.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float3[] primvars:radiance:sphericalHarmonicsCoefficients` |
    /// | C++ Type | VtArray<GfVec3f> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float3Array |
    USDLIGHTFIELD_API
    UsdAttribute GetPrimvarsRadianceSphericalHarmonicsCoefficientsAttr() const;

    /// See GetPrimvarsRadianceSphericalHarmonicsCoefficientsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLIGHTFIELD_API
    UsdAttribute CreatePrimvarsRadianceSphericalHarmonicsCoefficientsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PRIMVARSRADIANCESPHERICALHARMONICSCOEFFICIENTSH 
    // --------------------------------------------------------------------- //
    /// Flattened array of SH coefficients.
    /// The each of the different SH coefficients are not interleaved. The SH
    /// coefficents for a given particle are encoded in the array sequentially,
    /// with the all of the SH coefficients for the first particle being defined
    /// first, followed by all the SH coefficients for the second particle and
    /// so on. This means the elementSize metadata can be authored, but is
    /// dependent on the degree of the spherical harmonic data being applied.
    /// 
    /// If the float precision version is available it should be preferred.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `half3[] primvars:radiance:sphericalHarmonicsCoefficientsh` |
    /// | C++ Type | VtArray<GfVec3h> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Half3Array |
    USDLIGHTFIELD_API
    UsdAttribute GetPrimvarsRadianceSphericalHarmonicsCoefficientshAttr() const;

    /// See GetPrimvarsRadianceSphericalHarmonicsCoefficientshAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLIGHTFIELD_API
    UsdAttribute CreatePrimvarsRadianceSphericalHarmonicsCoefficientshAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
