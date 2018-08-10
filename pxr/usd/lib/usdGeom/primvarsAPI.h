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
#ifndef USDGEOM_GENERATED_PRIMVARSAPI_H
#define USDGEOM_GENERATED_PRIMVARSAPI_H

/// \file usdGeom/primvarsAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usdGeom/primvar.h" 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// PRIMVARSAPI                                                                //
// -------------------------------------------------------------------------- //

/// \class UsdGeomPrimvarsAPI
///
/// UsdGeomPrimvarsAPI encodes geometric "primitive variables",
/// as UsdGeomPrimvar, which interpolate across a primitive's topology,
/// can override shader inputs, and inherit down namespace.
///
class UsdGeomPrimvarsAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::NonAppliedAPI;

    /// Construct a UsdGeomPrimvarsAPI on UsdPrim \p prim .
    /// Equivalent to UsdGeomPrimvarsAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomPrimvarsAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdGeomPrimvarsAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomPrimvarsAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomPrimvarsAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomPrimvarsAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomPrimvarsAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomPrimvarsAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomPrimvarsAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USDGEOM_API
    virtual UsdSchemaType _GetSchemaType() const;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDGEOM_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDGEOM_API
    virtual const TfType &_GetTfType() const;

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
    /// \name Primvar Creation and Introspection
    /// @{
    // --------------------------------------------------------------------- //
 
    /// Author scene description to create an attribute on this prim that
    /// will be recognized as Primvar (i.e. will present as a valid
    /// UsdGeomPrimvar).
    ///
    /// The name of the created attribute may or may not be the specified
    /// \p attrName, due to the possible need to apply property namespacing
    /// for primvars.  See \ref Usd_Creating_and_Accessing_Primvars
    /// for more information.  Creation may fail and return an invalid
    /// Primvar if \p attrName contains a reserved keyword, such as the 
    /// "indices" suffix we use for indexed primvars.
    ///
    /// The behavior with respect to the provided \p typeName
    /// is the same as for UsdAttributes::Create(), and
    /// \p interpolation and \p elementSize are as described in
    /// UsdGeomPrimvar::GetInterpolation() and UsdGeomPrimvar::GetElementSize().
    ///
    /// If \p interpolation and/or \p elementSize are left unspecified, we
    /// will author no opinions for them, which means any (strongest) opinion
    /// already authored in any contributing layer for these fields will
    /// become the Primvar's values, or the fallbacks if no opinions
    /// have been authored.
    ///
    /// \return an invalid UsdGeomPrimvar if we failed to create a valid
    /// attribute, a valid UsdGeomPrimvar otherwise.  It is not an
    /// error to create over an existing, compatible attribute.
    ///
    /// \sa UsdPrim::CreateAttribute(), UsdGeomPrimvar::IsPrimvar()
    USDGEOM_API
    UsdGeomPrimvar CreatePrimvar(const TfToken& attrName,
                                 const SdfValueTypeName &typeName,
                                 const TfToken& interpolation = TfToken(),
                                 int elementSize = -1) const;

    /// Return the Primvar attribute named by \p name, which will
    /// be valid if a Primvar attribute definition already exists.
    ///
    /// Name lookup will account for Primvar namespacing, which means
    /// that this method will succeed in some cases where
    /// \code
    /// UsdGeomPrimvar(prim->GetAttribute(name))
    /// \endcode
    /// will not, unless \p name is properly namespace prefixed.
    ///
    /// \sa HasPrimvar()
    USDGEOM_API
    UsdGeomPrimvar GetPrimvar(const TfToken &name) const;
    
    /// Return valid UsdGeomPrimvar objects for all defined Primvars on
    /// this prim.
    ///
    /// Although we hope eventually to make this faster, this is currently
    /// a fairly expensive operation.  If you know you'll need to process
    /// other attributes as well, you might do better by fetching all
    /// the attributes at once, and using the pattern described in 
    /// \ref UsdGeomPrimvar_Using_Primvar "Using Primvars" to test individual
    /// attributes.
    USDGEOM_API
    std::vector<UsdGeomPrimvar> GetPrimvars() const;

    /// Like GetPrimvars(), but exclude primvars that have no authored scene
    /// description.
    USDGEOM_API
    std::vector<UsdGeomPrimvar> GetAuthoredPrimvars() const;

    /// Like GetPrimvars(), but searches instead for authored
    /// primvars inherited from ancestor prims.  Primvars are only
    /// inherited if they do not exist on the prim itself.  The
    /// returned primvars will be bound to attributes on the corresponding
    /// ancestor prims.  Only primvars with authored values are inherited;
    /// fallback values are not inherited.   The order of the returned
    /// primvars is undefined.
    USDGEOM_API
    std::vector<UsdGeomPrimvar> FindInheritedPrimvars() const;

    /// Like GetPrimvar(), but searches instead for the named primvar
    /// inherited on ancestor prim.  Primvars are only inherited if
    /// they do not exist on the prim itself.  The returned primvar will
    /// be bound to the attribute on the corresponding ancestor prim.
    USDGEOM_API
    UsdGeomPrimvar FindInheritedPrimvar(const TfToken &name) const;

    /// Is there a defined Primvar \p name on this prim?
    ///
    /// Name lookup will account for Primvar namespacing.
    ///
    /// \sa GetPrimvar()
    USDGEOM_API
    bool HasPrimvar(const TfToken &name) const;

    /// Is there an inherited Primvar \p name on this prim?
    /// The name given is the primvar name, not its underlying attribute name.
    /// \sa FindInheritedPrimvar()
    USDGEOM_API
    bool HasInheritedPrimvar(const TfToken &name) const;

    /// @}

private:
    // Helper for Get(Authored)Primvars().
    std::vector<UsdGeomPrimvar>
    _MakePrimvars(std::vector<UsdProperty> const &props) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
