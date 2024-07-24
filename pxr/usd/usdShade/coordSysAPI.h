//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDSHADE_GENERATED_COORDSYSAPI_H
#define USDSHADE_GENERATED_COORDSYSAPI_H

/// \file usdShade/coordSysAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdShade/tokens.h"

#include "pxr/usd/usdGeom/xformable.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// COORDSYSAPI                                                                //
// -------------------------------------------------------------------------- //

/// \class UsdShadeCoordSysAPI
///
/// UsdShadeCoordSysAPI provides a way to designate, name,
/// and discover coordinate systems.
/// 
/// Coordinate systems are implicitly established by UsdGeomXformable
/// prims, using their local space.  That coordinate system may be
/// bound (i.e., named) from another prim.  The binding is encoded
/// as a single-target relationship.
/// Coordinate system bindings apply to descendants of the prim
/// where the binding is expressed, but names may be re-bound by
/// descendant prims.
/// 
/// CoordSysAPI is a multi-apply API schema, where instance names 
/// signify the named coordinate systems. The instance names are
/// used with the "coordSys:" namespace to determine the binding
/// to the UsdGeomXformable prim.
/// 
/// Named coordinate systems are useful in shading (and other) workflows.
/// An example is projection paint, which projects a texture
/// from a certain view (the paint coordinate system), encoded as 
/// (e.g.) "rel coordSys:paint:binding".  Using the paint coordinate frame 
/// avoids the need to assign a UV set to the object, and can be a 
/// concise way to project paint across a collection of objects with 
/// a single shared paint coordinate system.
/// 
///
class UsdShadeCoordSysAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::MultipleApplyAPI;

    /// Construct a UsdShadeCoordSysAPI on UsdPrim \p prim with
    /// name \p name . Equivalent to
    /// UsdShadeCoordSysAPI::Get(
    ///    prim.GetStage(),
    ///    prim.GetPath().AppendProperty(
    ///        "coordSys:name"));
    ///
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdShadeCoordSysAPI(
        const UsdPrim& prim=UsdPrim(), const TfToken &name=TfToken())
        : UsdAPISchemaBase(prim, /*instanceName*/ name)
    { }

    /// Construct a UsdShadeCoordSysAPI on the prim held by \p schemaObj with
    /// name \p name.  Should be preferred over
    /// UsdShadeCoordSysAPI(schemaObj.GetPrim(), name), as it preserves
    /// SchemaBase state.
    explicit UsdShadeCoordSysAPI(
        const UsdSchemaBase& schemaObj, const TfToken &name)
        : UsdAPISchemaBase(schemaObj, /*instanceName*/ name)
    { }

    /// Destructor.
    USDSHADE_API
    virtual ~UsdShadeCoordSysAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDSHADE_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes for a given instance name.  Does not
    /// include attributes that may be authored by custom/extended methods of
    /// the schemas involved. The names returned will have the proper namespace
    /// prefix.
    USDSHADE_API
    static TfTokenVector
    GetSchemaAttributeNames(bool includeInherited, const TfToken &instanceName);

    /// Returns the name of this multiple-apply schema instance
    TfToken GetName() const {
        return _GetInstanceName();
    }

    /// Return a UsdShadeCoordSysAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  \p path must be of the format
    /// <path>.coordSys:name .
    ///
    /// This is shorthand for the following:
    ///
    /// \code
    /// TfToken name = SdfPath::StripNamespace(path.GetToken());
    /// UsdShadeCoordSysAPI(
    ///     stage->GetPrimAtPath(path.GetPrimPath()), name);
    /// \endcode
    ///
    USDSHADE_API
    static UsdShadeCoordSysAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Return a UsdShadeCoordSysAPI with name \p name holding the
    /// prim \p prim. Shorthand for UsdShadeCoordSysAPI(prim, name);
    USDSHADE_API
    static UsdShadeCoordSysAPI
    Get(const UsdPrim &prim, const TfToken &name);

    /// Return a vector of all named instances of UsdShadeCoordSysAPI on the 
    /// given \p prim.
    USDSHADE_API
    static std::vector<UsdShadeCoordSysAPI>
    GetAll(const UsdPrim &prim);

    /// Checks if the given name \p baseName is the base name of a property
    /// of CoordSysAPI.
    USDSHADE_API
    static bool
    IsSchemaPropertyBaseName(const TfToken &baseName);

    /// Checks if the given path \p path is of an API schema of type
    /// CoordSysAPI. If so, it stores the instance name of
    /// the schema in \p name and returns true. Otherwise, it returns false.
    USDSHADE_API
    static bool
    IsCoordSysAPIPath(const SdfPath &path, TfToken *name);

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
    USDSHADE_API
    static bool 
    CanApply(const UsdPrim &prim, const TfToken &name, 
             std::string *whyNot=nullptr);

    /// Applies this <b>multiple-apply</b> API schema to the given \p prim 
    /// along with the given instance name, \p name. 
    /// 
    /// This information is stored by adding "CoordSysAPI:<i>name</i>" 
    /// to the token-valued, listOp metadata \em apiSchemas on the prim.
    /// For example, if \p name is 'instance1', the token 
    /// 'CoordSysAPI:instance1' is added to 'apiSchemas'.
    /// 
    /// \return A valid UsdShadeCoordSysAPI object is returned upon success. 
    /// An invalid (or empty) UsdShadeCoordSysAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for 
    /// conditions resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDSHADE_API
    static UsdShadeCoordSysAPI 
    Apply(const UsdPrim &prim, const TfToken &name);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDSHADE_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDSHADE_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDSHADE_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // BINDING 
    // --------------------------------------------------------------------- //
    /// Prim binding expressing the appropriate coordinate systems.
    ///
    USDSHADE_API
    UsdRelationship GetBindingRel() const;

    /// See GetBindingRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDSHADE_API
    UsdRelationship CreateBindingRel() const;

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

    /// A coordinate system binding.
    /// Binds a name to a coordSysPrim for the bindingPrim (and its descendants,
    /// unless overriden).
    typedef struct {
        TfToken name;
        SdfPath bindingRelPath;
        SdfPath coordSysPrimPath;
    } Binding;

    /// Returns true if the prim has local coordinate system relationship exists. 
    ///
    /// \deprecated 
    /// This method is deprecated as it operates on the old non-applied
    /// UsdShadeCoordSysAPI
    /// If USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to True, if prim has
    /// appropriate API applied, that is conforming to the new behavior.
    /// If USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to Warn, try to see if
    /// multi-apply API compliant local bindings are present for the prim, if 
    /// not fallback to backward compatible deprecated behavior.
    ///
    USDSHADE_API
    bool HasLocalBindings() const;

    /// Returns true if the prim has UsdShadeCoordSysAPI applied. Which implies
    /// it has the appropriate binding relationship(s).
    ///
    USDSHADE_API
    static bool HasLocalBindingsForPrim(const UsdPrim &prim);

    /// Get the list of coordinate system bindings local to this prim. This 
    /// does not process inherited bindings.  It does not validate that a prim 
    /// exists at the indicated path. If the binding relationship has multiple 
    /// targets, only the first is used.
    /// 
    /// \deprecated 
    /// This method is deprecated as it operates on the old non-applied
    /// UsdShadeCoordSysAPI
    /// If USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to True, returns 
    /// bindings conforming to the new multi-apply UsdShadeCoordSysAPI schema.
    /// If USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to Warn, try to get 
    /// multi-apply API compliant local bindings for the prim, if none 
    /// fallback to backward compatible deprecated behavior.
    ///
    USDSHADE_API
    std::vector<Binding> GetLocalBindings() const;

    /// Get the list of coordinate system bindings local to this prim, across
    /// all multi-apply instanceNames. This does not process inherited bindings.
    /// It does not validate that a prim exists at the indicated path. If the 
    /// binding relationship has multiple targets, only the first is used.
    ///
    /// Note that this will always return empty vector of bindings if the 
    /// \p prim being queried does not have UsdShadeCoordSysAPI applied.
    ///
    USDSHADE_API
    static std::vector<Binding> GetLocalBindingsForPrim(const UsdPrim &prim);

    /// Get the coordinate system bindings local to this prim corresponding to
    /// this instance name. This does not process inherited bindings. It does 
    /// not validate that a prim exists at the indicated path. If the binding 
    /// relationship has multiple targets, only the first is used.
    /// 
    USDSHADE_API
    Binding GetLocalBinding() const;

    /// Find the list of coordinate system bindings that apply to this prim,
    /// including inherited bindings.
    ///
    /// This computation examines this prim and ancestors for the strongest
    /// binding for each name. A binding expressed by a child prim supercedes
    /// bindings on ancestors.
    ///
    /// Note that this API does not validate the prims at the target paths;
    /// they may be of incorrect type, or missing entirely.
    ///
    /// Binding relationships with no resolved targets are skipped.
    /// 
    /// \deprecated 
    /// This method is deprecated as it operates on the old non-applied
    /// UsdShadeCoordSysAPI
    /// If USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to True, returns 
    /// bindings conforming to the new multi-apply UsdShadeCoordSysAPI schema.
    /// If USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to Warn, try to get 
    /// multi-apply API compliant local bindings for the prim, if none 
    /// fallback to backward compatible deprecated behavior.
    ///
    USDSHADE_API
    std::vector<Binding> FindBindingsWithInheritance() const;

    /// Find the list of coordinate system bindings that apply to this prim,
    /// including inherited bindings.
    ///
    /// This computation examines this prim and ancestors for the strongest
    /// binding for each name. A binding expressed by a child prim supercedes
    /// bindings on ancestors. Only prims which have the UsdShadeCoordSysAPI
    /// applied are considered and queried for a binding.
    ///
    /// Note that this API does not validate the prims at the target paths;
    /// they may be of incorrect type, or missing entirely.
    ///
    /// Binding relationships with no resolved targets are skipped.
    USDSHADE_API
    static std::vector<Binding> FindBindingsWithInheritanceForPrim(
            const UsdPrim &prim);

    /// Find the coordinate system bindings that apply to this prim, including
    /// inherited bindings.
    ///
    /// This computation examines this prim and ancestors for the strongest
    /// binding for the specific instanceName. A binding expressed by a child 
    /// prim supercedes bindings on ancestors. Only ancestor prims which have 
    /// the UsdShadeCoordSysAPI:instanceName applied are considered.
    ///
    /// Note that this API does not validate the prims at the target paths;
    /// they may be of incorrect type, or missing entirely.
    ///
    /// Binding relationships with no resolved targets are skipped.
    USDSHADE_API
    Binding FindBindingWithInheritance() const;

    /// Bind the name to the given path. The prim at the given path is expected
    /// to be UsdGeomXformable, in order for the binding to be succesfully
    /// resolved.
    ///
    /// \deprecated 
    /// This method is deprecated as it operates on the old non-applied
    /// UsdShadeCoordSysAPI
    /// If USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to True, adds a binding
    /// conforming to the new multi-apply UsdShadeCoordSysAPI schema.
    /// If USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to Warn, try to also 
    /// bind to multi-apply API compliant relationship for the prim, along with
    /// backward compatible deprecated behavior.
    ///
    USDSHADE_API
    bool Bind(const TfToken &name, const SdfPath &path) const;

    /// A convinience API for clients to use to Apply schema in accordance with
    /// new UsdShadeCoordSysAPI schema constructs and appropriate Bind the
    /// target. Note that this is only for clients using old behavior.
    ///
    /// \deprecated
    USDSHADE_API
    bool ApplyAndBind(const TfToken &name, const SdfPath &path) const;

    /// Bind the name to the given path. The prim at the given path is expected
    /// to be UsdGeomXformable, in order for the binding to be succesfully
    /// resolved.
    ///
    USDSHADE_API
    bool Bind(const SdfPath &path) const;

    /// Clear the indicated coordinate system binding on this prim from the
    /// current edit target.
    ///
    /// Only remove the spec if \p removeSpec is true (leave the spec to 
    /// preserve meta-data we may have intentionally authored on the 
    /// relationship)
    ///
    /// \deprecated 
    /// This method is deprecated as it operates on the old non-applied
    /// UsdShadeCoordSysAPI
    /// If USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to True, clears a binding
    /// conforming to the new multi-apply UsdShadeCoordSysAPI schema.
    /// If USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to Warn, try to also 
    /// clear binding for multi-apply API compliant relationship for the prim, 
    /// along with backward compatible deprecated behavior.
    ///
    USDSHADE_API
    bool ClearBinding(const TfToken &name, bool removeSpec) const;

    /// Clear the coordinate system binding on the prim corresponding to the
    /// instanceName of this UsdShadeCoordSysAPI, from the current edit target.
    ///
    /// Only remove the spec if \p removeSpec is true (leave the spec to 
    /// preserve meta-data we may have intentionally authored on the 
    /// relationship)
    ///
    USDSHADE_API
    bool ClearBinding(bool removeSpec) const;

    /// Block the indicated coordinate system binding on this prim by blocking
    /// targets on the underlying relationship.
    ///
    /// \deprecated 
    /// This method is deprecated as it operates on the old non-applied
    /// UsdShadeCoordSysAPI
    /// If USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to True, blocks binding
    /// conforming to the new multi-apply UsdShadeCoordSysAPI schema.
    /// If USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to Warn, try to also 
    /// block binding for multi-apply API compliant relationship for the prim, 
    /// along with backward compatible deprecated behavior.
    ///
    USDSHADE_API
    bool BlockBinding(const TfToken &name) const;

    /// Block the indicated coordinate system binding on this prim by blocking
    /// targets on the underlying relationship.
    ///
    USDSHADE_API
    bool BlockBinding() const;

    /// Returns the fully namespaced coordinate system relationship
    /// name, given the coordinate system name.
    //
    /// \deprecated 
    /// This method is deprecated as it operates on the old non-applied
    /// UsdShadeCoordSysAPI
    //
    USDSHADE_API
    static TfToken GetCoordSysRelationshipName(const std::string &coordSysName);

    /// Test whether a given \p name contains the "coordSys:" prefix
    USDSHADE_API
    static bool CanContainPropertyName(const TfToken &name);

    /// Strips "coordSys:" from the relationship name and returns
    /// "<instanceName>:binding".
    USDSHADE_API
    static TfToken GetBindingBaseName(const TfToken &name);

    /// Strips "coordSys:" from the relationship name and returns
    /// "<instanceName>:binding".
    USDSHADE_API
    TfToken GetBindingBaseName() const;

private:
    USDSHADE_API
    static void _GetBindingsForPrim(const UsdPrim &prim, 
            std::vector<Binding> &result, bool checkExistingBindings=false);

};

PXR_NAMESPACE_CLOSE_SCOPE

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE
USDSHADE_API
extern TfEnvSetting<std::string> USD_SHADE_COORD_SYS_IS_MULTI_APPLY;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
