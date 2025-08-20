//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDUI_GENERATED_ACCESSIBILITYAPI_H
#define USDUI_GENERATED_ACCESSIBILITYAPI_H

/// \file usdUI/accessibilityAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdUI/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdUI/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// ACCESSIBILITYAPI                                                           //
// -------------------------------------------------------------------------- //

/// \class UsdUIAccessibilityAPI
///
/// 
/// This API describes \em Accessibility information on a Prim that may be
/// surfaced to a given runtime's accessibility frameworks.
/// This information may be used by assistive tooling such as voice controls
/// or screen readers.
/// Accessibility information is provided as a standard triplet of label,
/// description and priority.
/// 
/// OpenUSD does not provide a accessibility runtime itself, but endeavours
/// to provide the information needed for compatible runtimes to extract and
/// present this information.
/// 
/// This is a multiple apply schema, and so may have multiple namespaced
/// accessibility triplets, where a instance name may reflect a given purpose
/// for that triplet. For example, you may desire to express different
/// information for different aspects of the prim, such as size or color.
/// 
/// \note The API will use \em "default" as an instance name if no
/// namespace is specified.
/// 
/// There are several best practices for using this schema.:
/// 
/// \li Most accessibility runtimes support a single accessibility description.
/// Therefore we recommend placing any critical information in the default
/// instance.
/// 
/// \li A default value should be authored if using time sampled accessibility
/// information. This helps accessibility runtimes that do not currently
/// support time sampled information.
/// 
/// \li Provide accessibility information of your scene on the default prim
/// of the layer, and any top level prims. This allows accessibility systems to
/// provide concise scene descriptions to a user, but also allows supporting
/// accessibility systems that either do not support hierarchy information or
/// when a user has turned off that level of granularity. Accessibility
/// information may still be provided on other prims in the hierarchy.
/// 
/// \note The use of the default prim and top level prims for scene
/// accessibility descriptions is a recommended convention. Outside of that,
/// accessibility information is not implicitly inherited through a prim
/// hierarchy. The inheritance should be left to the accessibility runtime to
/// decide how best to surface information to users.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdUITokens.
/// So to set an attribute to the value "rightHanded", use UsdUITokens->rightHanded
/// as the value.
///
class UsdUIAccessibilityAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::MultipleApplyAPI;

    /// Construct a UsdUIAccessibilityAPI on UsdPrim \p prim with
    /// name \p name . Equivalent to
    /// UsdUIAccessibilityAPI::Get(
    ///    prim.GetStage(),
    ///    prim.GetPath().AppendProperty(
    ///        "accessibility:name"));
    ///
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdUIAccessibilityAPI(
        const UsdPrim& prim=UsdPrim(), const TfToken &name=TfToken())
        : UsdAPISchemaBase(prim, /*instanceName*/ name)
    { }

    /// Construct a UsdUIAccessibilityAPI on the prim held by \p schemaObj with
    /// name \p name.  Should be preferred over
    /// UsdUIAccessibilityAPI(schemaObj.GetPrim(), name), as it preserves
    /// SchemaBase state.
    explicit UsdUIAccessibilityAPI(
        const UsdSchemaBase& schemaObj, const TfToken &name)
        : UsdAPISchemaBase(schemaObj, /*instanceName*/ name)
    { }

    /// Destructor.
    USDUI_API
    virtual ~UsdUIAccessibilityAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDUI_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes for a given instance name.  Does not
    /// include attributes that may be authored by custom/extended methods of
    /// the schemas involved. The names returned will have the proper namespace
    /// prefix.
    USDUI_API
    static TfTokenVector
    GetSchemaAttributeNames(bool includeInherited, const TfToken &instanceName);

    /// Returns the name of this multiple-apply schema instance
    TfToken GetName() const {
        return _GetInstanceName();
    }

    /// Return a UsdUIAccessibilityAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  \p path must be of the format
    /// <path>.accessibility:name .
    ///
    /// This is shorthand for the following:
    ///
    /// \code
    /// TfToken name = SdfPath::StripNamespace(path.GetToken());
    /// UsdUIAccessibilityAPI(
    ///     stage->GetPrimAtPath(path.GetPrimPath()), name);
    /// \endcode
    ///
    USDUI_API
    static UsdUIAccessibilityAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Return a UsdUIAccessibilityAPI with name \p name holding the
    /// prim \p prim. Shorthand for UsdUIAccessibilityAPI(prim, name);
    USDUI_API
    static UsdUIAccessibilityAPI
    Get(const UsdPrim &prim, const TfToken &name);

    /// Return a vector of all named instances of UsdUIAccessibilityAPI on the 
    /// given \p prim.
    USDUI_API
    static std::vector<UsdUIAccessibilityAPI>
    GetAll(const UsdPrim &prim);

    /// Checks if the given name \p baseName is the base name of a property
    /// of AccessibilityAPI.
    USDUI_API
    static bool
    IsSchemaPropertyBaseName(const TfToken &baseName);

    /// Checks if the given path \p path is of an API schema of type
    /// AccessibilityAPI. If so, it stores the instance name of
    /// the schema in \p name and returns true. Otherwise, it returns false.
    USDUI_API
    static bool
    IsAccessibilityAPIPath(const SdfPath &path, TfToken *name);

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
    USDUI_API
    static bool 
    CanApply(const UsdPrim &prim, const TfToken &name, 
             std::string *whyNot=nullptr);

    /// Applies this <b>multiple-apply</b> API schema to the given \p prim 
    /// along with the given instance name, \p name. 
    /// 
    /// This information is stored by adding "AccessibilityAPI:<i>name</i>" 
    /// to the token-valued, listOp metadata \em apiSchemas on the prim.
    /// For example, if \p name is 'instance1', the token 
    /// 'AccessibilityAPI:instance1' is added to 'apiSchemas'.
    /// 
    /// \return A valid UsdUIAccessibilityAPI object is returned upon success. 
    /// An invalid (or empty) UsdUIAccessibilityAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for 
    /// conditions resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDUI_API
    static UsdUIAccessibilityAPI 
    Apply(const UsdPrim &prim, const TfToken &name);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDUI_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDUI_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDUI_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // LABEL 
    // --------------------------------------------------------------------- //
    /// A short label to concisely describe the prim.
    /// It is not recommended to time vary the label unless the concise
    /// description changes substantially.
    /// 
    /// There is no specific suggested length for the label, but it is
    /// recommended to keep it succinct.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `string label` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    USDUI_API
    UsdAttribute GetLabelAttr() const;

    /// See GetLabelAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDUI_API
    UsdAttribute CreateLabelAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DESCRIPTION 
    // --------------------------------------------------------------------- //
    /// An extended description of the prim to provide more details.
    /// If a label attribute is not authored in a given instance name,
    /// the description attribute should not be used in it its place. A
    /// description is an optional attribute, and some accessibility systems
    /// may only use the label.
    /// 
    /// Descriptions may be time varying for runtimes that support it. For
    /// example, you may describe what a character is doing at a given time.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `string description` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    USDUI_API
    UsdAttribute GetDescriptionAttr() const;

    /// See GetDescriptionAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDUI_API
    UsdAttribute CreateDescriptionAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PRIORITY 
    // --------------------------------------------------------------------- //
    /// A hint to the accessibility runtime of how to prioritize this
    /// instance's label and description, relative to others.
    /// 
    /// This attribute is optional and is considered a hint that runtimes may
    /// ignore, if they feel there are other necessities that take precedence
    /// over the prioritization values.
    /// 
    /// Priority may not be time varying.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token priority = "standard"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdUITokens "Allowed Values" | low, standard, high |
    USDUI_API
    UsdAttribute GetPriorityAttr() const;

    /// See GetPriorityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDUI_API
    UsdAttribute CreatePriorityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// Creates an instance of the API with the default instance name.
    /// /sa UsdUIAccessibilityAPI
    USDUI_API
    static UsdUIAccessibilityAPI CreateDefaultAPI(const UsdPrim& prim);

    /// Creates an instance of the API with a schema object using the default instance name.
    /// /sa UsdUIAccessibilityAPI
    USDUI_API
    static UsdUIAccessibilityAPI CreateDefaultAPI(const UsdSchemaBase& schemaObj);

    /// Applies an instance of the API with the default instance name.
    /// /sa Apply
    USDUI_API
    static UsdUIAccessibilityAPI ApplyDefaultAPI(const UsdPrim& prim);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
