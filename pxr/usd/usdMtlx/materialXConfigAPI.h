//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDMTLX_GENERATED_MATERIALXCONFIGAPI_H
#define USDMTLX_GENERATED_MATERIALXCONFIGAPI_H

/// \file usdMtlx/materialXConfigAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdMtlx/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdMtlx/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// MATERIALXCONFIGAPI                                                         //
// -------------------------------------------------------------------------- //

/// \class UsdMtlxMaterialXConfigAPI
///
/// MaterialXConfigAPI is an API schema that provides an interface for
/// storing information about the MaterialX environment.
/// 
/// Initially, it only exposes an interface to record the MaterialX library
/// version that data was authored against. The intention is to use this
/// information to allow the MaterialX library to perform upgrades on data
/// from prior MaterialX versions.
/// 
///
class UsdMtlxMaterialXConfigAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdMtlxMaterialXConfigAPI on UsdPrim \p prim .
    /// Equivalent to UsdMtlxMaterialXConfigAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdMtlxMaterialXConfigAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdMtlxMaterialXConfigAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdMtlxMaterialXConfigAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdMtlxMaterialXConfigAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDMTLX_API
    virtual ~UsdMtlxMaterialXConfigAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDMTLX_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdMtlxMaterialXConfigAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdMtlxMaterialXConfigAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDMTLX_API
    static UsdMtlxMaterialXConfigAPI
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
    USDMTLX_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "MaterialXConfigAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdMtlxMaterialXConfigAPI object is returned upon success. 
    /// An invalid (or empty) UsdMtlxMaterialXConfigAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDMTLX_API
    static UsdMtlxMaterialXConfigAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDMTLX_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDMTLX_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDMTLX_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // CONFIGMTLXVERSION 
    // --------------------------------------------------------------------- //
    /// MaterialX library version that the data has been authored
    /// against. Defaults to 1.38 to allow correct verisoning of old files.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `string config:mtlx:version = "1.38"` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    USDMTLX_API
    UsdAttribute GetConfigMtlxVersionAttr() const;

    /// See GetConfigMtlxVersionAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDMTLX_API
    UsdAttribute CreateConfigMtlxVersionAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
