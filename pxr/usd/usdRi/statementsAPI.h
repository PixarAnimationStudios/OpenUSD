//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDRI_GENERATED_STATEMENTSAPI_H
#define USDRI_GENERATED_STATEMENTSAPI_H

/// \file usdRi/statementsAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdRi/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usdGeom/primvarsAPI.h"


#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// STATEMENTSAPI                                                              //
// -------------------------------------------------------------------------- //

/// \class UsdRiStatementsAPI
///
/// Container namespace schema for all renderman statements.
/// 
/// \note The longer term goal is for clients to go directly to primvar
/// or render-attribute API's, instead of using UsdRi StatementsAPI
/// for inherited attributes.  Anticpating this, StatementsAPI
/// can smooth the way via a few environment variables:
/// 
/// * USDRI_STATEMENTS_READ_OLD_ENCODING: Causes StatementsAPI to read
/// old-style attributes instead of primvars in the "ri:"
/// namespace.
/// 
///
class UsdRiStatementsAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdRiStatementsAPI on UsdPrim \p prim .
    /// Equivalent to UsdRiStatementsAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdRiStatementsAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdRiStatementsAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdRiStatementsAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdRiStatementsAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDRI_API
    virtual ~UsdRiStatementsAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDRI_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdRiStatementsAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdRiStatementsAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDRI_API
    static UsdRiStatementsAPI
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
    USDRI_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "StatementsAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdRiStatementsAPI object is returned upon success. 
    /// An invalid (or empty) UsdRiStatementsAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDRI_API
    static UsdRiStatementsAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDRI_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDRI_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDRI_API
    const TfType &_GetTfType() const override;

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

    // --------------------------------------------------------------------- //
    // CreateRiAttribute 
    // --------------------------------------------------------------------- //
    /// Create a rib attribute on the prim to which this schema is attached.
    /// A rib attribute consists of an attribute \em "nameSpace" and an
    /// attribute \em "name".  For example, the namespace "cull" may define
    /// attributes "backfacing" and "hidden", and user-defined attributes
    /// belong to the namespace "user".
    ///
    /// This method makes no attempt to validate that the given \p nameSpace
    /// and \em name are actually meaningful to prman or any other
    /// renderer.
    ///
    /// \param riType should be a known RenderMan type definition, which
    /// can be array-valued.  For instance, both "color" and "float[3]"
    /// are valid values for \p riType.
    USDRI_API
    UsdAttribute
    CreateRiAttribute(
        const TfToken &name, 
        const std::string &riType,
        const std::string &nameSpace = "user");

    /// Creates an attribute of the given \p tfType.
    /// \overload
    USDRI_API
    UsdAttribute
    CreateRiAttribute(
        const TfToken &name, 
        const TfType &tfType,
        const std::string &nameSpace = "user");

    /// Return a UsdAttribute representing the Ri attribute with the
    /// name \a name, in the namespace \a nameSpace.  The attribute
    /// returned may or may not \b actually exist so it must be
    /// checked for validity.
    USDRI_API
    UsdAttribute
    GetRiAttribute(
        const TfToken &name, 
        const std::string &nameSpace = "user");

    // --------------------------------------------------------------------- //
    // GetRiAttributes 
    // --------------------------------------------------------------------- //
    /// Return all rib attributes on this prim, or under a specific 
    /// namespace (e.g.\ "user").
    ///
    /// As noted above, rib attributes can be either UsdAttribute or 
    /// UsdRelationship, and like all UsdProperties, need not have a defined 
    /// value.
    USDRI_API
    std::vector<UsdProperty>
    GetRiAttributes(const std::string &nameSpace = "") const;
    // --------------------------------------------------------------------- //
    // GetRiAttributeName 
    // --------------------------------------------------------------------- //
    /// Return the base, most-specific name of the rib attribute.  For example,
    /// the \em name of the rib attribute "cull:backfacing" is "backfacing"
    inline static TfToken GetRiAttributeName(const UsdProperty &prop) {
        return prop.GetBaseName();
    }

    // --------------------------------------------------------------------- //
    // GetRiAttributeNameSpace
    // --------------------------------------------------------------------- //
    /// Return the containing namespace of the rib attribute (e.g.\ "user").
    ///
    USDRI_API
    static TfToken GetRiAttributeNameSpace(const UsdProperty &prop);

    // --------------------------------------------------------------------- //
    // IsRiAttribute
    // --------------------------------------------------------------------- //
    /// Return true if the property is in the "ri:attributes" namespace.
    ///
    USDRI_API
    static bool IsRiAttribute(const UsdProperty &prop);

    // --------------------------------------------------------------------- //
    // MakeRiAttributePropertyName
    // --------------------------------------------------------------------- //
    /// Returns the given \p attrName prefixed with the full Ri attribute
    /// namespace, creating a name suitable for an RiAttribute UsdProperty.
    /// This handles conversion of common separator characters used in
    /// other packages, such as periods and underscores.
    ///
    /// Will return empty string if attrName is not a valid property
    /// identifier; otherwise, will return a valid property name
    /// that identifies the property as an RiAttribute, according to the 
    /// following rules:
    /// \li If \p attrName is already a properly constructed RiAttribute 
    ///     property name, return it unchanged.  
    /// \li If \p attrName contains two or more tokens separated by a \em colon,
    ///     consider the first to be the namespace, and the rest the name, 
    ///     joined by underscores
    /// \li If \p attrName contains two or more tokens separated by a \em period,
    ///     consider the first to be the namespace, and the rest the name, 
    ///     joined by underscores
    /// \li If \p attrName contains two or more tokens separated by an,
    ///     \em underscore consider the first to be the namespace, and the
    ///     rest the name, joined by underscores
    /// \li else, assume \p attrName is the name, and "user" is the namespace
    USDRI_API
    static std::string MakeRiAttributePropertyName(const std::string &attrName);

    // --------------------------------------------------------------------- //
    // SetCoordinateSystem
    // --------------------------------------------------------------------- //
    /// \deprecated This API will be removed in a future release.  Clients
    /// should use UsdShadeCoordSysAPI instead.
    ///
    /// Sets the "ri:coordinateSystem" attribute to the given string value,
    /// creating the attribute if needed. That identifies this prim as providing
    /// a coordinate system, which can be retrieved via
    /// UsdGeomXformable::GetTransformAttr(). Also adds the owning prim to the
    /// ri:modelCoordinateSystems relationship targets on its parent leaf model
    /// prim, if it exists. If this prim is not under a leaf model, no
    /// relationship targets will be authored.
    ///
    USDRI_API
    void SetCoordinateSystem(const std::string &coordSysName);

    // --------------------------------------------------------------------- //
    // GetCoordinateSystem
    // --------------------------------------------------------------------- //
    /// \deprecated This API will be removed in a future release.  Clients
    /// should use UsdShadeCoordSysAPI instead.
    ///
    /// Returns the value in the "ri:coordinateSystem" attribute if it exists.
    ///
    USDRI_API
    std::string GetCoordinateSystem() const;

    // --------------------------------------------------------------------- //
    // HasCoordinateSystem
    // --------------------------------------------------------------------- //
    /// \deprecated This API will be removed in a future release.  Clients
    /// should use UsdShadeCoordSysAPI instead.
    ///
    /// Returns true if the underlying prim has a ri:coordinateSystem opinion.
    ///
    USDRI_API
    bool HasCoordinateSystem() const;

    // --------------------------------------------------------------------- //
    // SetScopedCoordinateSystem
    // --------------------------------------------------------------------- //
    /// \deprecated This API will be removed in a future release.  Clients
    /// should use UsdShadeCoordSysAPI instead.
    ///
    /// Sets the "ri:scopedCoordinateSystem" attribute to the given string
    /// value, creating the attribute if needed. That identifies this prim as
    /// providing a coordinate system, which can be retrieved via
    /// UsdGeomXformable::GetTransformAttr(). Such coordinate systems are
    /// local to the RI attribute stack state, but does get updated properly
    /// for instances when defined inside an object master.  Also adds the
    /// owning prim to the ri:modelScopedCoordinateSystems relationship
    /// targets on its parent leaf model prim, if it exists. If this prim is
    /// not under a leaf model, no relationship targets will be authored.
    ///
    USDRI_API
    void SetScopedCoordinateSystem(const std::string &coordSysName);

    // --------------------------------------------------------------------- //
    // GetScopedCoordinateSystem
    // --------------------------------------------------------------------- //
    /// \deprecated This API will be removed in a future release.  Clients
    /// should use UsdShadeCoordSysAPI instead.
    ///
    /// Returns the value in the "ri:scopedCoordinateSystem" attribute if it
    /// exists.
    ///
    USDRI_API
    std::string GetScopedCoordinateSystem() const;

    // --------------------------------------------------------------------- //
    // HasScopedCoordinateSystem
    // --------------------------------------------------------------------- //
    /// \deprecated This API will be removed in a future release.  Clients
    /// should use UsdShadeCoordSysAPI instead.
    ///
    /// Returns true if the underlying prim has a ri:scopedCoordinateSystem
    /// opinion.
    ///
    USDRI_API
    bool HasScopedCoordinateSystem() const;

    // --------------------------------------------------------------------- //
    // GetModelCoordinateSystems
    // --------------------------------------------------------------------- //
    /// \deprecated This API will be removed in a future release.  Clients
    /// should use UsdShadeCoordSysAPI instead.
    ///
    /// Populates the output \p targets with the authored
    /// ri:modelCoordinateSystems, if any. Returns true if the query was
    /// successful.
    ///
    USDRI_API
    bool GetModelCoordinateSystems(SdfPathVector *targets) const;

    // --------------------------------------------------------------------- //
    // GetModelScopedCoordinateSystems
    // --------------------------------------------------------------------- //
    /// \deprecated This API will be removed in a future release.  Clients
    /// should use UsdShadeCoordSysAPI instead.
    ///
    /// Populates the output \p targets with the authored
    /// ri:modelScopedCoordinateSystems, if any.  Returns true if the query was
    /// successful.
    ///
    USDRI_API
    bool GetModelScopedCoordinateSystems(SdfPathVector *targets) const;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
