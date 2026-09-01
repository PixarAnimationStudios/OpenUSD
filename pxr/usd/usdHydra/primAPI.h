//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDHYDRA_GENERATED_PRIMAPI_H
#define USDHYDRA_GENERATED_PRIMAPI_H

/// \file usdHydra/primAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdHydra/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdHydra/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// HYDRAPRIMAPI                                                               //
// -------------------------------------------------------------------------- //

/// \class UsdHydraPrimAPI
///
/// Provides Hydra-specific prim settings.
///
class UsdHydraPrimAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdHydraPrimAPI on UsdPrim \p prim .
    /// Equivalent to UsdHydraPrimAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdHydraPrimAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdHydraPrimAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdHydraPrimAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdHydraPrimAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDHYDRA_API
    virtual ~UsdHydraPrimAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDHYDRA_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdHydraPrimAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdHydraPrimAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDHYDRA_API
    static UsdHydraPrimAPI
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
    USDHYDRA_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "HydraPrimAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdHydraPrimAPI object is returned upon success. 
    /// An invalid (or empty) UsdHydraPrimAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDHYDRA_API
    static UsdHydraPrimAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDHYDRA_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDHYDRA_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDHYDRA_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // HYDRAEXPANDINSTANCES 
    // --------------------------------------------------------------------- //
    /// This property can be used to de-instance the Hydra scene
    /// index representation of prims that were otherwise instanceable
    /// in the USD scenegraph.  If the USD prim was not instanceable,
    /// this has no effect.
    /// 
    /// If a prim has hydra:expandInstances=1, it and all its namespace
    /// descendants will be unrolled in the scene index using the USD
    /// proxy prim API.
    /// 
    /// This can be used to facilitate Hydra scene index filtering
    /// transformations which cannot be easily expressed when the
    /// structure uses instancing.  For example, some operations
    /// require unique modifications to an instance of what would
    /// otherwise be a shareable prototype.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform bool hydra:expandInstances = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDHYDRA_API
    UsdAttribute GetHydraExpandInstancesAttr() const;

    /// See GetHydraExpandInstancesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDHYDRA_API
    UsdAttribute CreateHydraExpandInstancesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    // Returns true if the prim (or any ancestor) has UsdHydraPrimAPI's
    // hydra:expandInstances attribute authored to true.  By default
    // this recursively checks ancestors, but this can be skipped
    // using includeAncestors=false.
    USDHYDRA_API
    static bool ShouldExpandInstancesForPrim(
        UsdPrim const& prim,
        bool includeAncestors=true);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
