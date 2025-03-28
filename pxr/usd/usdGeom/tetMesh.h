//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDGEOM_GENERATED_TETMESH_H
#define USDGEOM_GENERATED_TETMESH_H

/// \file usdGeom/tetMesh.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usd/usd/timeCode.h" 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// TETMESH                                                                    //
// -------------------------------------------------------------------------- //

/// \class UsdGeomTetMesh
///
/// Encodes a tetrahedral mesh. A tetrahedral mesh is defined as a set of 
/// tetrahedra. Each tetrahedron is defined by a set of 4 points, with the 
/// triangles of the tetrahedron determined from these 4 points as described in 
/// the <b>tetVertexIndices</b> attribute description. The mesh surface faces 
/// are encoded as triangles. Surface faces must be provided for consumers 
/// that need to do surface calculations, such as renderers or consumers using 
/// physics attachments. Both tetrahedra and surface face definitions use 
/// indices into the TetMesh's <b>points</b> attribute, inherited from 
/// UsdGeomPointBased.
///
class UsdGeomTetMesh : public UsdGeomPointBased
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdGeomTetMesh on UsdPrim \p prim .
    /// Equivalent to UsdGeomTetMesh::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomTetMesh(const UsdPrim& prim=UsdPrim())
        : UsdGeomPointBased(prim)
    {
    }

    /// Construct a UsdGeomTetMesh on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomTetMesh(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomTetMesh(const UsdSchemaBase& schemaObj)
        : UsdGeomPointBased(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomTetMesh();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomTetMesh holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomTetMesh(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomTetMesh
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Attempt to ensure a \a UsdPrim adhering to this schema at \p path
    /// is defined (according to UsdPrim::IsDefined()) on this stage.
    ///
    /// If a prim adhering to this schema at \p path is already defined on this
    /// stage, return that prim.  Otherwise author an \a SdfPrimSpec with
    /// \a specifier == \a SdfSpecifierDef and this schema's prim type name for
    /// the prim at \p path at the current EditTarget.  Author \a SdfPrimSpec s
    /// with \p specifier == \a SdfSpecifierDef and empty typeName at the
    /// current EditTarget for any nonexistent, or existing but not \a Defined
    /// ancestors.
    ///
    /// The given \a path must be an absolute prim path that does not contain
    /// any variant selections.
    ///
    /// If it is impossible to author any of the necessary PrimSpecs, (for
    /// example, in case \a path cannot map to the current UsdEditTarget's
    /// namespace) issue an error and return an invalid \a UsdPrim.
    ///
    /// Note that this method may return a defined prim whose typeName does not
    /// specify this schema class, in case a stronger typeName opinion overrides
    /// the opinion at the current EditTarget.
    ///
    USDGEOM_API
    static UsdGeomTetMesh
    Define(const UsdStagePtr &stage, const SdfPath &path);

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
    // TETVERTEXINDICES 
    // --------------------------------------------------------------------- //
    /// Flat list of the index (into the <b>points</b> attribute) of 
    /// each vertex of each tetrahedron in the mesh. Each int4 corresponds to the
    /// indices of a single tetrahedron. Users should set the <b>orientation</b>
    /// attribute of UsdGeomPrim accordingly. That is if the <b>orientation</b> 
    /// is "rightHanded", the CCW face ordering of a tetrahedron is
    /// [123],[032],[013],[021] with respect to the int4. This results in the
    /// normals facing outward from the center of the tetrahedron. The following
    /// diagram shows the face ordering of an unwrapped tetrahedron with 
    /// "rightHanded" orientation.
    /// 
    /// \image html USDTetMeshRightHanded.svg
    /// 
    /// If the <b>orientation</b> attribute is set to "leftHanded" the face 
    /// ordering of the tetrahedron is [321],[230],[310],[120] and the 
    /// leftHanded CW face normals point outward from the center of the 
    /// tetrahedron. The following diagram shows the face ordering of an 
    /// unwrapped tetrahedron with "leftHanded" orientation.
    /// 
    /// \image html USDTetMeshLeftHanded.svg
    /// 
    /// Setting the <b>orientation</b> attribute to align with the 
    /// ordering of the int4 for the tetrahedrons is the responsibility of the 
    /// user.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int4[] tetVertexIndices` |
    /// | C++ Type | VtArray<GfVec4i> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int4Array |
    USDGEOM_API
    UsdAttribute GetTetVertexIndicesAttr() const;

    /// See GetTetVertexIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateTetVertexIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SURFACEFACEVERTEXINDICES 
    // --------------------------------------------------------------------- //
    /// <b>surfaceFaceVertexIndices</b> defines the triangle
    /// surface faces indices wrt. <b>points</b> of the tetmesh surface. Again 
    /// the <b>orientation</b> attribute inherited from UsdGeomPrim should be 
    /// set accordingly. The <b>orientation</b> for faces of tetrahedra and  
    /// surface faces must match.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int3[] surfaceFaceVertexIndices` |
    /// | C++ Type | VtArray<GfVec3i> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int3Array |
    USDGEOM_API
    UsdAttribute GetSurfaceFaceVertexIndicesAttr() const;

    /// See GetSurfaceFaceVertexIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateSurfaceFaceVertexIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// ComputeSurfaceFaces determines the vertex indices of the surface faces 
    /// from tetVertexIndices. The surface faces are the set of faces that occur 
    /// only once when traversing the faces of all the tetrahedra. The algorithm 
    /// is O(nlogn) in the number of tetrahedra. Method returns false if 
    /// surfaceFaceIndices argument is nullptr and returns true otherwise.
    /// The algorithm can't be O(n) because we need to sort the resulting
    /// surface faces for deterministic behavior across different compilers 
    /// and OS. 
    USDGEOM_API    
    static bool ComputeSurfaceFaces(const UsdGeomTetMesh& tetMesh,
                                    VtVec3iArray* surfaceFaceIndices,
                                    const UsdTimeCode timeCode = UsdTimeCode::Default()); 

    /// FindInvertedElements is used to determine if the tetMesh has inverted 
    /// tetrahedral elements at the given time code. Inverted elements are 
    /// determined wrt. the "orientation" attribute of the UsdGeomTetMesh and
    /// are stored in the invertedElements arg. Method returns true if it 
    /// succeeds and if invertedElements is empty then all the tetrahedra have  
    /// the correct orientation.
    USDGEOM_API    
    static bool FindInvertedElements(const UsdGeomTetMesh& tetMesh,
                                     VtIntArray* invertedElements,
                                     const UsdTimeCode timeCode = UsdTimeCode::Default());
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
