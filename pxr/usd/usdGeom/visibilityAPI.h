//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDGEOM_GENERATED_VISIBILITYAPI_H
#define USDGEOM_GENERATED_VISIBILITYAPI_H

/// \file usdGeom/visibilityAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// VISIBILITYAPI                                                              //
// -------------------------------------------------------------------------- //

/// \class UsdGeomVisibilityAPI
///
/// 
/// UsdGeomVisibilityAPI introduces properties that can be used to author
/// visibility opinions.
/// 
/// \note
/// Currently, this schema only introduces the attributes that are used to
/// control purpose visibility. Later, this schema will define _all_
/// visibility-related properties and UsdGeomImageable will no longer define
/// those properties.
/// 
/// The purpose visibility attributes added by this schema,
/// _guideVisibility_, _proxyVisibility_, and _renderVisibility_ can each be
/// used to control visibility for geometry of the corresponding purpose
/// values, with the overall _visibility_ attribute acting as an
/// override. I.e., if _visibility_ evaluates to "invisible", purpose
/// visibility is invisible; otherwise, purpose visibility is determined by
/// the corresponding purpose visibility attribute.
/// 
/// Note that the behavior of _guideVisibility_ is subtly different from the
/// _proxyVisibility_ and _renderVisibility_ attributes, in that "guide"
/// purpose visibility always evaluates to either "invisible" or "visible",
/// whereas the other attributes may yield computed values of "inherited" if
/// there is no authored opinion on the attribute or inherited from an
/// ancestor. This is motivated by the fact that, in Pixar"s user workflows,
/// we have never found a need to have all guides visible in a scene by
/// default, whereas we do find that flexibility useful for "proxy" and
/// "render" geometry.
/// 
/// This schema can only be applied to UsdGeomImageable prims. The
/// UseGeomImageable schema provides API for computing the purpose visibility
/// values that result from the attributes introduced by this schema.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdGeomTokens.
/// So to set an attribute to the value "rightHanded", use UsdGeomTokens->rightHanded
/// as the value.
///
class UsdGeomVisibilityAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdGeomVisibilityAPI on UsdPrim \p prim .
    /// Equivalent to UsdGeomVisibilityAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomVisibilityAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdGeomVisibilityAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomVisibilityAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomVisibilityAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomVisibilityAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomVisibilityAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomVisibilityAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomVisibilityAPI
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
    USDGEOM_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "VisibilityAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdGeomVisibilityAPI object is returned upon success. 
    /// An invalid (or empty) UsdGeomVisibilityAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDGEOM_API
    static UsdGeomVisibilityAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDGEOM_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDGEOM_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDGEOM_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // GUIDEVISIBILITY 
    // --------------------------------------------------------------------- //
    /// 
    /// This attribute controls visibility for geometry with purpose "guide".
    /// 
    /// Unlike overall _visibility_, _guideVisibility_ is uniform, and
    /// therefore cannot be animated.
    /// 
    /// Also unlike overall _visibility_, _guideVisibility_ is tri-state, in
    /// that a descendant with an opinion of "visible" overrides an ancestor
    /// opinion of "invisible".
    /// 
    /// The _guideVisibility_ attribute works in concert with the overall
    /// _visibility_ attribute: The visibility of a prim with purpose "guide"
    /// is determined by the inherited values it receives for the _visibility_
    /// and _guideVisibility_ attributes. If _visibility_ evaluates to
    /// "invisible", the prim is invisible. If _visibility_ evaluates to
    /// "inherited" and _guideVisibility_ evaluates to "visible", then the
    /// prim is visible. __Otherwise, it is invisible.__
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token guideVisibility = "invisible"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdGeomTokens "Allowed Values" | inherited, invisible, visible |
    USDGEOM_API
    UsdAttribute GetGuideVisibilityAttr() const;

    /// See GetGuideVisibilityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateGuideVisibilityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PROXYVISIBILITY 
    // --------------------------------------------------------------------- //
    /// 
    /// This attribute controls visibility for geometry with purpose "proxy".
    /// 
    /// Unlike overall _visibility_, _proxyVisibility_ is uniform, and
    /// therefore cannot be animated.
    /// 
    /// Also unlike overall _visibility_, _proxyVisibility_ is tri-state, in
    /// that a descendant with an opinion of "visible" overrides an ancestor
    /// opinion of "invisible".
    /// 
    /// The _proxyVisibility_ attribute works in concert with the overall
    /// _visibility_ attribute: The visibility of a prim with purpose "proxy"
    /// is determined by the inherited values it receives for the _visibility_
    /// and _proxyVisibility_ attributes. If _visibility_ evaluates to
    /// "invisible", the prim is invisible. If _visibility_ evaluates to
    /// "inherited" then: If _proxyVisibility_ evaluates to "visible", then
    /// the prim is visible; if _proxyVisibility_ evaluates to "invisible",
    /// then the prim is invisible; if _proxyVisibility_ evaluates to
    /// "inherited", then the prim may either be visible or invisible,
    /// depending on a fallback value determined by the calling context.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token proxyVisibility = "inherited"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdGeomTokens "Allowed Values" | inherited, invisible, visible |
    USDGEOM_API
    UsdAttribute GetProxyVisibilityAttr() const;

    /// See GetProxyVisibilityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateProxyVisibilityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // RENDERVISIBILITY 
    // --------------------------------------------------------------------- //
    /// 
    /// This attribute controls visibility for geometry with purpose
    /// "render".
    /// 
    /// Unlike overall _visibility_, _renderVisibility_ is uniform, and
    /// therefore cannot be animated.
    /// 
    /// Also unlike overall _visibility_, _renderVisibility_ is tri-state, in
    /// that a descendant with an opinion of "visible" overrides an ancestor
    /// opinion of "invisible".
    /// 
    /// The _renderVisibility_ attribute works in concert with the overall
    /// _visibility_ attribute: The visibility of a prim with purpose "render"
    /// is determined by the inherited values it receives for the _visibility_
    /// and _renderVisibility_ attributes. If _visibility_ evaluates to
    /// "invisible", the prim is invisible. If _visibility_ evaluates to
    /// "inherited" then: If _renderVisibility_ evaluates to "visible", then
    /// the prim is visible; if _renderVisibility_ evaluates to "invisible",
    /// then the prim is invisible; if _renderVisibility_ evaluates to
    /// "inherited", then the prim may either be visible or invisible,
    /// depending on a fallback value determined by the calling context.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token renderVisibility = "inherited"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdGeomTokens "Allowed Values" | inherited, invisible, visible |
    USDGEOM_API
    UsdAttribute GetRenderVisibilityAttr() const;

    /// See GetRenderVisibilityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateRenderVisibilityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
    
    /// Return the attribute that is used for expressing visibility opinions
    /// for the given \p purpose.
    ///
    /// The valid purpose tokens are "guide", "proxy", and "render" which
    /// return the attributes *guideVisibility*, *proxyVisibility*, and 
    /// *renderVisibility* respectively.
    ///
    /// Note that while "default" is a valid purpose token for 
    /// UsdGeomImageable::GetPurposeVisibilityAttr, it is not a valid purpose
    /// for this function, as UsdGeomVisibilityAPI itself does not have a 
    /// default visibility attribute. Calling this function with "default
    /// will result in a coding error.
    ///
    USDGEOM_API
    UsdAttribute GetPurposeVisibilityAttr(const TfToken &purpose) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
