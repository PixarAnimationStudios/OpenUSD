//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDSEMANTICS_GENERATED_LABELSAPI_H
#define USDSEMANTICS_GENERATED_LABELSAPI_H

/// \file usdSemantics/labelsAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSemantics/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdSemantics/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// SEMANTICSLABELSAPI                                                         //
// -------------------------------------------------------------------------- //

/// \class UsdSemanticsLabelsAPI
///
/// Application of labels for a prim for a taxonomy specified by the
/// schema's instance name.
/// 
/// See `UsdSemanticsLabelsQuery` for more information about computations and
/// inheritance of semantics.
///
class UsdSemanticsLabelsAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::MultipleApplyAPI;

    /// Construct a UsdSemanticsLabelsAPI on UsdPrim \p prim with
    /// name \p name . Equivalent to
    /// UsdSemanticsLabelsAPI::Get(
    ///    prim.GetStage(),
    ///    prim.GetPath().AppendProperty(
    ///        "semantics:labels:name"));
    ///
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdSemanticsLabelsAPI(
        const UsdPrim& prim=UsdPrim(), const TfToken &name=TfToken())
        : UsdAPISchemaBase(prim, /*instanceName*/ name)
    { }

    /// Construct a UsdSemanticsLabelsAPI on the prim held by \p schemaObj with
    /// name \p name.  Should be preferred over
    /// UsdSemanticsLabelsAPI(schemaObj.GetPrim(), name), as it preserves
    /// SchemaBase state.
    explicit UsdSemanticsLabelsAPI(
        const UsdSchemaBase& schemaObj, const TfToken &name)
        : UsdAPISchemaBase(schemaObj, /*instanceName*/ name)
    { }

    /// Destructor.
    USDSEMANTICS_API
    virtual ~UsdSemanticsLabelsAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDSEMANTICS_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes for a given instance name.  Does not
    /// include attributes that may be authored by custom/extended methods of
    /// the schemas involved. The names returned will have the proper namespace
    /// prefix.
    USDSEMANTICS_API
    static TfTokenVector
    GetSchemaAttributeNames(bool includeInherited, const TfToken &instanceName);

    /// Returns the name of this multiple-apply schema instance
    TfToken GetName() const {
        return _GetInstanceName();
    }

    /// Return a UsdSemanticsLabelsAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  \p path must be of the format
    /// <path>.semantics:labels:name .
    ///
    /// This is shorthand for the following:
    ///
    /// \code
    /// TfToken name = SdfPath::StripNamespace(path.GetToken());
    /// UsdSemanticsLabelsAPI(
    ///     stage->GetPrimAtPath(path.GetPrimPath()), name);
    /// \endcode
    ///
    USDSEMANTICS_API
    static UsdSemanticsLabelsAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Return a UsdSemanticsLabelsAPI with name \p name holding the
    /// prim \p prim. Shorthand for UsdSemanticsLabelsAPI(prim, name);
    USDSEMANTICS_API
    static UsdSemanticsLabelsAPI
    Get(const UsdPrim &prim, const TfToken &name);

    /// Return a vector of all named instances of UsdSemanticsLabelsAPI on the 
    /// given \p prim.
    USDSEMANTICS_API
    static std::vector<UsdSemanticsLabelsAPI>
    GetAll(const UsdPrim &prim);

    /// Checks if the given name \p baseName is the base name of a property
    /// of SemanticsLabelsAPI.
    USDSEMANTICS_API
    static bool
    IsSchemaPropertyBaseName(const TfToken &baseName);

    /// Checks if the given path \p path is of an API schema of type
    /// SemanticsLabelsAPI. If so, it stores the instance name of
    /// the schema in \p name and returns true. Otherwise, it returns false.
    USDSEMANTICS_API
    static bool
    IsSemanticsLabelsAPIPath(const SdfPath &path, TfToken *name);

    /// Returns true if this <b>multiple-apply</b> API schema can be applied,
    /// with the given instance name, \p name, to the given \p prim. If this 
    /// schema can not be a applied the prim, this returns false and, if 
    /// provided, populates \p whyNot with the reason it can not be applied.
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
    USDSEMANTICS_API
    static bool 
    CanApply(const UsdPrim &prim, const TfToken &name, 
             std::string *whyNot=nullptr);

    /// Applies this <b>multiple-apply</b> API schema to the given \p prim 
    /// along with the given instance name, \p name. 
    /// 
    /// This information is stored by adding "SemanticsLabelsAPI:<i>name</i>" 
    /// to the token-valued, listOp metadata \em apiSchemas on the prim.
    /// For example, if \p name is 'instance1', the token 
    /// 'SemanticsLabelsAPI:instance1' is added to 'apiSchemas'.
    /// 
    /// \return A valid UsdSemanticsLabelsAPI object is returned upon success. 
    /// An invalid (or empty) UsdSemanticsLabelsAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for 
    /// conditions resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDSEMANTICS_API
    static UsdSemanticsLabelsAPI 
    Apply(const UsdPrim &prim, const TfToken &name);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDSEMANTICS_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDSEMANTICS_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDSEMANTICS_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // LABELS 
    // --------------------------------------------------------------------- //
    /// Array of labels specified directly at this prim.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token[] __INSTANCE_NAME__ = []` |
    /// | C++ Type | VtArray<TfToken> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->TokenArray |
    USDSEMANTICS_API
    UsdAttribute GetLabelsAttr() const;

    /// See GetLabelsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSEMANTICS_API
    UsdAttribute CreateLabelsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
    /// Get the instance names (taxonomies) of all applications of
    /// UsdSemanticsLabelsAPI for the specified prim.
    ///
    /// Prefer `UsdSemanticsLabelsAPI::GetAll(prim)` if the instances of
    /// the schema may be needed.
    ///
    /// Prefer `prim.HasAPI<UsdSemanticsLabelsAPI>(taxonomy)` for checking
    /// if a taxonomy has been directly applied to the prim.
    ///
    /// \sa ComputeInheritedTaxonomies
    /// \sa GetAll
    USDSEMANTICS_API
    static std::vector<TfToken> GetDirectTaxonomies(const UsdPrim& prim);

    /// Get the unique instance names (taxonomies) of all applications of
    /// UsdSemanticsLabelsAPI for the specified prim and its ancestors.
    ///
    /// \sa GetDirectTaxonomies
    USDSEMANTICS_API
    static std::vector<TfToken> ComputeInheritedTaxonomies(
        const UsdPrim& prim);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
