//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDHYDRA_GENERATED_GENERATIVEPROCEDURALAPI_H
#define USDHYDRA_GENERATED_GENERATIVEPROCEDURALAPI_H

/// \file usdHydra/generativeProceduralAPI.h

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
// HYDRAGENERATIVEPROCEDURALAPI                                               //
// -------------------------------------------------------------------------- //

/// \class UsdHydraGenerativeProceduralAPI
///
/// 
/// This API extends and configures the core UsdProcGenerativeProcedural schema
/// defined within usdProc for use with hydra generative procedurals as defined
/// within hdGp.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdHydraTokens.
/// So to set an attribute to the value "rightHanded", use UsdHydraTokens->rightHanded
/// as the value.
///
class UsdHydraGenerativeProceduralAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdHydraGenerativeProceduralAPI on UsdPrim \p prim .
    /// Equivalent to UsdHydraGenerativeProceduralAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdHydraGenerativeProceduralAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdHydraGenerativeProceduralAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdHydraGenerativeProceduralAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdHydraGenerativeProceduralAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDHYDRA_API
    virtual ~UsdHydraGenerativeProceduralAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDHYDRA_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdHydraGenerativeProceduralAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdHydraGenerativeProceduralAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDHYDRA_API
    static UsdHydraGenerativeProceduralAPI
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
    /// This information is stored by adding "HydraGenerativeProceduralAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdHydraGenerativeProceduralAPI object is returned upon success. 
    /// An invalid (or empty) UsdHydraGenerativeProceduralAPI object is returned upon 
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
    static UsdHydraGenerativeProceduralAPI 
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
    // PROCEDURALTYPE 
    // --------------------------------------------------------------------- //
    /// The registered name of a HdGpGenerativeProceduralPlugin to
    /// be executed.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token primvars:hdGp:proceduralType` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    USDHYDRA_API
    UsdAttribute GetProceduralTypeAttr() const;

    /// See GetProceduralTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDHYDRA_API
    UsdAttribute CreateProceduralTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PROCEDURALSYSTEM 
    // --------------------------------------------------------------------- //
    /// 
    /// This value should correspond to a configured instance of
    /// HdGpGenerativeProceduralResolvingSceneIndex which will evaluate the
    /// procedural. The default value of "hydraGenerativeProcedural" matches
    /// the equivalent default of HdGpGenerativeProceduralResolvingSceneIndex.
    /// Multiple instances of the scene index can be used to determine where
    /// within a scene index chain a given procedural will be evaluated.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token proceduralSystem = "hydraGenerativeProcedural"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    USDHYDRA_API
    UsdAttribute GetProceduralSystemAttr() const;

    /// See GetProceduralSystemAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDHYDRA_API
    UsdAttribute CreateProceduralSystemAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
