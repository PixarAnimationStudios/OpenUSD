//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLOD_GENERATED_ROOTAPI_H
#define USDLOD_GENERATED_ROOTAPI_H

/// \file usdLod/rootAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLod/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLod/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// LODROOTAPI                                                                 //
// -------------------------------------------------------------------------- //

/// \class UsdLodRootAPI
///
/// API for configuring Level of Detail (LOD) facilities on a prim.
/// 
/// This API schema marks a prim as the root of an LOD selection hierarchy. The
/// immediate namespace children of this prim are the LOD items that can be
/// selected during rendering. The order of LOD item child prims in the USD stage
/// determines the LOD index, from 0 to the number_of_children - 1.
/// 
/// Selection of appropriate LOD items is entirely up to the renderer that is
/// processing the scene. Upon encountering an LOD root, the renderer should
/// check for an LOD override (see UsdLodOverrideAPI), then check the targets of
/// the lod:heuristics relationship for an appropriate heuristic using the
/// heuristic's type and lod:domain value to select one, falling back to the
/// lod:default:index value if all else fails. Each heuristic type will likely
/// have a specific, unique API to query that heuristic for its LOD
/// recommendation. It is then up to the renderer to display the selected LOD
/// item child and not display LOD item children that are not selected.
/// 
/// LOD heuristics are specified by heuristic prims targeted by the
/// lod:heuristics relationship. The targeted heuristics are intended to be
/// queried and used by the renderer.
/// 
/// LOD items are generally mutually exclusive selections, though renderers may
/// blend between multiple children or even display all of them when given an
/// appropriate override.
/// 
/// The schema is singleApply, but multiple systems may independently specify
/// level of detail criteria via heuristics specified by the lod:heuristics
/// relationship. For example, a single prim may have both "imaging" and
/// "physics" domain heuristics; the choice of the domains to consider and the
/// heuristic(s) to use is up to the renderer.
/// 
/// A child or descendent of an LODRootAPI prim may itself have LODRootAPI
/// applied to create nested hierarchical LOD structures.
/// 
///
class UsdLodRootAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdLodRootAPI on UsdPrim \p prim .
    /// Equivalent to UsdLodRootAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLodRootAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdLodRootAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdLodRootAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLodRootAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDLOD_API
    virtual ~UsdLodRootAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLOD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLodRootAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLodRootAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLOD_API
    static UsdLodRootAPI
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
    USDLOD_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "LODRootAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdLodRootAPI object is returned upon success. 
    /// An invalid (or empty) UsdLodRootAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDLOD_API
    static UsdLodRootAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDLOD_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDLOD_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDLOD_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // LODDEFAULTINDEX 
    // --------------------------------------------------------------------- //
    /// The LOD item child to use if none of the heuristics are
    /// understood by the renderer or if none return a usable LOD.
    /// 
    /// Consider an "audio" renderer that does not understand "imaging" domain
    /// heuristics. When it encounters a prim with LODRootAPI applied and only
    /// "imaging" domain heuristics, it should select and process the child
    /// indicated by lod:default:index and ignore the other children.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int lod:default:index = 0` |
    /// | C++ Type | int |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int |
    USDLOD_API
    UsdAttribute GetLodDefaultIndexAttr() const;

    /// See GetLodDefaultIndexAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLOD_API
    UsdAttribute CreateLodDefaultIndexAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LODHEURISTICS 
    // --------------------------------------------------------------------- //
    /// Relationship targeting all the heuristic prims that could be
    /// used for this particular LOD root. The choice of which heuristic(s) will
    /// be used is up to the renderer.
    /// 
    ///
    USDLOD_API
    UsdRelationship GetLodHeuristicsRel() const;

    /// See GetLodHeuristicsRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDLOD_API
    UsdRelationship CreateLodHeuristicsRel() const;

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

    /// Return the prim's LOD item children.
    //
    /// Equivalent to GetPrim().GetChildren();
    inline UsdPrimSiblingRange GetLODItems() const
    {
        return GetPrim().GetChildren();
    }
    
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
