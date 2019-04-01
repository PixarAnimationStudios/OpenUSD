//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef USDGEOM_GENERATED_COORDSYSAPI_H
#define USDGEOM_GENERATED_COORDSYSAPI_H

/// \file usdGeom/coordSysAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

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

/// \class UsdGeomCoordSysAPI
///
/// UsdCoordSysAPI provides a way to designate, name,
/// and discover coordinate systems.
/// 
/// Coordinate systems are implicitly established by UsdGeomXformable
/// prims, using their local space.  That coordinate system may be
/// bound (i.e., named) from another prim.  The binding is encoded
/// as a single-target relationship in the "coordSys:" namespace.
/// Coordinate system bindings apply to descendants of the prim
/// where the binding is expressed, but names may be re-bound by
/// descendant prims.
/// 
/// Named coordinate systems are useful in shading workflows.
/// An example is projection paint, which projects a texture
/// from a certain view (the paint coordinate system).  Using
/// the paint coordinate frame avoids the need to assign a UV
/// set to the object, and can be a concise way to project
/// paint across a collection of objects with a single shared
/// paint coordinate system.
/// 
/// Conceptually, UsdGeomCoordSysAPI has similarities with
/// UsdGeomConstraintTarget, in that both publish named coordinate
/// systems.  The distinction is that UsdGeomCoordSysAPI designates
/// the coordinate system as an Xformable prim target, and
/// intends that binding to inherit to descendants; whereas
/// the Constraint API stores time-sampled transformation
/// matrices directly, and only on model prims.
/// 
/// This is a non-applied API schema.
/// 
///
class UsdGeomCoordSysAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::NonAppliedAPI;

    /// Construct a UsdGeomCoordSysAPI on UsdPrim \p prim .
    /// Equivalent to UsdGeomCoordSysAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomCoordSysAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdGeomCoordSysAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomCoordSysAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomCoordSysAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomCoordSysAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomCoordSysAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomCoordSysAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomCoordSysAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USDGEOM_API
    UsdSchemaType _GetSchemaType() const override;

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

    /// A name bound to a path indicating a coordinate system.
    typedef std::pair<TfToken, SdfPath> Binding;

    /// A name resolved to a UsdGeomXformable coordinate system.
    typedef std::pair<TfToken, UsdGeomXformable> CoordinateSystem;

    /// Find the list of named coordinate systems introduced
    /// by bindings at this pirm.
    ///
    /// To be valid, a binding must target an Xformable prim.
    /// Invalid bindings will be silently skipped.
    USDGEOM_API
    std::vector<UsdGeomCoordSysAPI::CoordinateSystem>
    GetCoordinateSystems() const;

    /// Find the list of named coordinate systems that apply at
    /// this prim, including inherited bindings.
    ///
    /// This computation examines this prim and ancestors for
    /// the strongest binding for each name.  A binding expressed by
    /// a child prim supercedes bindings on ancestors.
    ///
    /// To be valid, a binding must target an Xformable prim.
    /// Invalid bindings will be silently skipped.
    USDGEOM_API
    std::vector<UsdGeomCoordSysAPI::CoordinateSystem>
    FindCoordinateSystemsWithInheritance() const;

    /// Get the list of coordinate system bindings local to this prim.
    /// This does not process inherited bindings.  It does not
    /// validate that a prim exists at the indicated path.
    /// If the relationship has multiple targets, only the first
    /// is used.
    USDGEOM_API
    std::vector<Binding> GetLocalBindings() const;

    /// Bind the name to the given path.
    /// The prim at the given path is expected to be UsdGeomXformable,
    /// in order for the binding to be succesfully resolved.
    USDGEOM_API
    bool Bind(TfToken const &name, SdfPath const &path) const;

    /// Clear the indicated coordinate system binding on this prim
    /// from the current edit target.
    ///
    /// Only remove the spec if \p removeSpec is true (leave the spec to
    /// preserve meta-data we may have intentionally authored on the
    /// relationship)
    USDGEOM_API
    bool ClearBinding(TfToken const& name, bool removeSpec) const;

    /// Block the indicated coordinate system binding on this prim
    /// by blocking targets on the underlying relationship.
    USDGEOM_API
    bool BlockBinding(const TfToken &name) const;

    /// Returns the fully namespaced coordinate system relationship
    /// name, given the coordinate system name.
    USDGEOM_API
    static TfToken GetCoordSysRelationshipName(const std::string &coordSysName);

private:
    void
    _ResolveCoordinateSystems(
        std::vector<UsdGeomCoordSysAPI::CoordinateSystem> *result,
        bool includeInherited) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
