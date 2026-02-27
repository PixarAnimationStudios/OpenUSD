//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDVOL_GENERATED_PARTICLEFIELDSCALEATTRIBUTEAPI_H
#define USDVOL_GENERATED_PARTICLEFIELDSCALEATTRIBUTEAPI_H

/// \file usdVol/particleFieldScaleAttributeAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdVol/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdVol/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// PARTICLEFIELDSCALEATTRIBUTEAPI                                             //
// -------------------------------------------------------------------------- //

/// \class UsdVolParticleFieldScaleAttributeAPI
///
/// A ParticleField related applied schema that provides a
/// scales attribute to define the linear scale factor applied to the
/// particles.
/// 
/// The scales here are linear scales, in line with scales provided
/// elsewhere in USD, and not specified in log-format as is sometimes
/// seen in PLY files associated with gaussian splats.
/// 
/// Attributes are provided in both `float` and `half` types for some
/// easy data footprint affordance, data consumers should prefer
/// `float` version if available.
/// 
/// The length of this attribute is expected to match the length of
/// the provided position data. If it is too long it will be truncated
/// to the number of particles define by the position data. If it is
/// too short it will be ignored.
/// 
/// If the attribute is ignored or not provided, then a default unit
/// scale should be applied to the kernel.
///
class UsdVolParticleFieldScaleAttributeAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdVolParticleFieldScaleAttributeAPI on UsdPrim \p prim .
    /// Equivalent to UsdVolParticleFieldScaleAttributeAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdVolParticleFieldScaleAttributeAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdVolParticleFieldScaleAttributeAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdVolParticleFieldScaleAttributeAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdVolParticleFieldScaleAttributeAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDVOL_API
    virtual ~UsdVolParticleFieldScaleAttributeAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDVOL_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdVolParticleFieldScaleAttributeAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdVolParticleFieldScaleAttributeAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDVOL_API
    static UsdVolParticleFieldScaleAttributeAPI
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
    USDVOL_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "ParticleFieldScaleAttributeAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdVolParticleFieldScaleAttributeAPI object is returned upon success. 
    /// An invalid (or empty) UsdVolParticleFieldScaleAttributeAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDVOL_API
    static UsdVolParticleFieldScaleAttributeAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDVOL_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDVOL_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDVOL_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // SCALES 
    // --------------------------------------------------------------------- //
    /// Affine linear scale factor applied to the kernel that is
    /// instantiated at each particle.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float3[] scales` |
    /// | C++ Type | VtArray<GfVec3f> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float3Array |
    USDVOL_API
    UsdAttribute GetScalesAttr() const;

    /// See GetScalesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDVOL_API
    UsdAttribute CreateScalesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SCALESH 
    // --------------------------------------------------------------------- //
    /// Affine linear scale factor applied to the kernel that is
    /// instantiated at each particle. If the float precision version is
    /// defined it should be preferred.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `half3[] scalesh` |
    /// | C++ Type | VtArray<GfVec3h> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Half3Array |
    USDVOL_API
    UsdAttribute GetScaleshAttr() const;

    /// See GetScaleshAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDVOL_API
    UsdAttribute CreateScaleshAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
