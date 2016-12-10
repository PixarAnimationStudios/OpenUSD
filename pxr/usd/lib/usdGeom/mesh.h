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
#ifndef USDGEOM_GENERATED_MESH_H
#define USDGEOM_GENERATED_MESH_H

/// \file usdGeom/mesh.h

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

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// MESH                                                                       //
// -------------------------------------------------------------------------- //

/// \class UsdGeomMesh
///
/// Encodes a mesh surface whose definition and feature-set
/// will converge with that of OpenSubdiv, http://graphics.pixar.com/opensubdiv/docs/subdivision_surfaces.html. Current exceptions/divergences include:
/// 
/// 1. Certain interpolation ("tag") parameters not yet supported
/// 
/// 2. Does not (as of 9/2014) yet support hierarchical edits.  We do intend
/// to provide some encoding in a future version of the schema.
/// 
/// A key property of this mesh schema is that it encodes both subdivision
/// surfaces, and non-subdived "polygonal meshes", by varying the
/// \em subdivisionScheme attribute.
/// 
/// \section UsdGeom_Mesh_Normals A Note About Normals
/// 
/// Although the \em normals attribute inherited from PointBased can be authored
/// on any mesh, they are almost never needed for subdivided meshes, and only
/// add rendering cost.  You may consider only authoring them for polygonal
/// meshes.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdGeomTokens.
/// So to set an attribute to the value "rightHanded", use UsdGeomTokens->rightHanded
/// as the value.
///
class UsdGeomMesh : public UsdGeomPointBased
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = true;

    /// Construct a UsdGeomMesh on UsdPrim \p prim .
    /// Equivalent to UsdGeomMesh::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomMesh(const UsdPrim& prim=UsdPrim())
        : UsdGeomPointBased(prim)
    {
    }

    /// Construct a UsdGeomMesh on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomMesh(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomMesh(const UsdSchemaBase& schemaObj)
        : UsdGeomPointBased(schemaObj)
    {
    }

    /// Destructor.
    virtual ~UsdGeomMesh();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomMesh holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomMesh(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    static UsdGeomMesh
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
    static UsdGeomMesh
    Define(const UsdStagePtr &stage, const SdfPath &path);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // FACEVERTEXINDICES 
    // --------------------------------------------------------------------- //
    /// Flat list of the index (into the 'points' attribute) of each
    /// vertex of each face in the mesh.  If this attribute has more than
    /// one timeSample, the mesh is considered to be topologically varying.
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetFaceVertexIndicesAttr() const;

    /// See GetFaceVertexIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateFaceVertexIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FACEVERTEXCOUNTS 
    // --------------------------------------------------------------------- //
    /// Provides the number of vertices in each face of the mesh, 
    /// which is also the number of consecutive indices in 'faceVertexIndices'
    /// that define the face.  The length of this attribute is the number of
    /// faces in the mesh.  If this attribute has more than
    /// one timeSample, the mesh is considered to be topologically varying.
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetFaceVertexCountsAttr() const;

    /// See GetFaceVertexCountsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateFaceVertexCountsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SUBDIVISIONSCHEME 
    // --------------------------------------------------------------------- //
    /// The subdivision scheme to be applied to the surface.  
    /// Valid values are "catmullClark" (the default), "loop", "bilinear", and
    /// "none" (i.e. a polymesh with no subdivision - the primary difference
    /// between schemes "bilinear" and "none" is that bilinearly subdivided
    /// meshes can be considered watertight, whereas there is no such guarantee
    /// for un-subdivided polymeshes, and more mesh features (e.g. holes) may
    /// apply to bilinear meshes but not polymeshes.  Polymeshes \em may be
    /// lighterweight and faster to render, depending on renderer and render
    /// mode.)
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: catmullClark
    /// \n  \ref UsdGeomTokens "Allowed Values": [catmullClark, loop, bilinear, none]
    UsdAttribute GetSubdivisionSchemeAttr() const;

    /// See GetSubdivisionSchemeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateSubdivisionSchemeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INTERPOLATEBOUNDARY 
    // --------------------------------------------------------------------- //
    /// Specifies how interpolation boundary face edges are
    /// interpolated. Valid values are "none", 
    /// "edgeAndCorner" (the default), or "edgeOnly".
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: edgeAndCorner
    /// \n  \ref UsdGeomTokens "Allowed Values": [none, edgeAndCorner, edgeOnly]
    UsdAttribute GetInterpolateBoundaryAttr() const;

    /// See GetInterpolateBoundaryAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateInterpolateBoundaryAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FACEVARYINGLINEARINTERPOLATION 
    // --------------------------------------------------------------------- //
    /// Specifies how face varying data is interpolated.  Valid values
    /// are "all" (no smoothing), "cornersPlus1" (the default, Smooth UV),
    /// "none" (Same as "cornersPlus1" but does not infer the presence 
    /// of corners where two faceVarying edges meet at a single face), or 
    /// "boundaries" (smooth only near vertices that are not at a
    /// discontinuous boundary).
    /// 
    /// See http://graphics.pixar.com/opensubdiv/docs/subdivision_surfaces.html#face-varying-interpolation-rules
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: cornersPlus1
    /// \n  \ref UsdGeomTokens "Allowed Values": [all, none, boundaries, cornersOnly, cornersPlus1, cornersPlus2]
    UsdAttribute GetFaceVaryingLinearInterpolationAttr() const;

    /// See GetFaceVaryingLinearInterpolationAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateFaceVaryingLinearInterpolationAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // HOLEINDICES 
    // --------------------------------------------------------------------- //
    /// The face indices (indexing into the 'faceVertexCounts'
    /// attribute) of all faces that should be made invisible.
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: []
    UsdAttribute GetHoleIndicesAttr() const;

    /// See GetHoleIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateHoleIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CORNERINDICES 
    // --------------------------------------------------------------------- //
    /// The vertex indices of all vertices that are sharp corners.
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: []
    UsdAttribute GetCornerIndicesAttr() const;

    /// See GetCornerIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateCornerIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CORNERSHARPNESSES 
    // --------------------------------------------------------------------- //
    /// The sharpness values for corners: each corner gets a single
    /// sharpness value (Usd.Mesh.SHARPNESS_INFINITE for a perfectly sharp
    /// corner), so the size of this array must match that of
    /// 'cornerIndices'
    ///
    /// \n  C++ Type: VtArray<float>
    /// \n  Usd Type: SdfValueTypeNames->FloatArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: []
    UsdAttribute GetCornerSharpnessesAttr() const;

    /// See GetCornerSharpnessesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateCornerSharpnessesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CREASEINDICES 
    // --------------------------------------------------------------------- //
    /// The indices of all vertices forming creased edges.  The size of 
    /// this array must be equal to the sum of all elements of the 
    /// 'creaseLengths' attribute.
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: []
    UsdAttribute GetCreaseIndicesAttr() const;

    /// See GetCreaseIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateCreaseIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CREASELENGTHS 
    // --------------------------------------------------------------------- //
    /// The length of this array specifies the number of creases on the
    /// surface. Each element gives the number of (must be adjacent) vertices in
    /// each crease, whose indices are linearly laid out in the 'creaseIndices'
    /// attribute. Since each crease must be at least one edge long, each
    /// element of this array should be greater than one.
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: []
    UsdAttribute GetCreaseLengthsAttr() const;

    /// See GetCreaseLengthsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateCreaseLengthsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CREASESHARPNESSES 
    // --------------------------------------------------------------------- //
    /// The per-crease or per-edge sharpness for all creases
    /// (Usd.Mesh.SHARPNESS_INFINITE for a perfectly sharp crease).  Since
    /// 'creaseLengths' encodes the number of vertices in each crease, the
    /// number of elements in this array will be either len(creaseLengths) or
    /// the sum over all X of (creaseLengths[X] - 1). Note that while
    /// the RI spec allows each crease to have either a single sharpness
    /// or a value per-edge, USD will encode either a single sharpness
    /// per crease on a mesh, or sharpnesses for all edges making up
    /// the creases on a mesh.
    ///
    /// \n  C++ Type: VtArray<float>
    /// \n  Usd Type: SdfValueTypeNames->FloatArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: []
    UsdAttribute GetCreaseSharpnessesAttr() const;

    /// See GetCreaseSharpnessesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateCreaseSharpnessesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to close the class delcaration with }; and complete the
    // include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

public:
    /// \var const float SHARPNESS_INFINITE
    /// As an element of a 'creaseSharpness' or 'cornerSharpness' array,
    /// indicates that the crease or corner is perfectly sharp.
    static const float SHARPNESS_INFINITE;

    // A transition API which can read both the new (faceVaryingLinearInterpolation)
    // and old(faceVaryingInterpolateBoundary) attributes, but only returns values
    // in the new form. This aims to limit the number of consumers which need to 
    // handle both sets of values.
    TfToken GetFaceVaryingLinearInterpolation(
        UsdTimeCode time=UsdTimeCode::Default()) const; 
};

#endif
