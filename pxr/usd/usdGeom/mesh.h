//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDGEOM_GENERATED_MESH_H
#define USDGEOM_GENERATED_MESH_H

/// \file usdGeom/mesh.h

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
// MESH                                                                       //
// -------------------------------------------------------------------------- //

/// \class UsdGeomMesh
///
/// Encodes a mesh with optional subdivision properties and features.
/// 
/// As a point-based primitive, meshes are defined in terms of points that 
/// are connected into edges and faces. Many references to meshes use the
/// term 'vertex' in place of or interchangeably with 'points', while some
/// use 'vertex' to refer to the 'face-vertices' that define a face.  To
/// avoid confusion, the term 'vertex' is intentionally avoided in favor of
/// 'points' or 'face-vertices'.
/// 
/// The connectivity between points, edges and faces is encoded using a
/// common minimal topological description of the faces of the mesh.  Each
/// face is defined by a set of face-vertices using indices into the Mesh's
/// _points_ array (inherited from UsdGeomPointBased) and laid out in a
/// single linear _faceVertexIndices_ array for efficiency.  A companion
/// _faceVertexCounts_ array provides, for each face, the number of
/// consecutive face-vertices in _faceVertexIndices_ that define the face.
/// No additional connectivity information is required or constructed, so
/// no adjacency or neighborhood queries are available.
/// 
/// A key property of this mesh schema is that it encodes both subdivision
/// surfaces and simpler polygonal meshes. This is achieved by varying the
/// _subdivisionScheme_ attribute, which is set to specify Catmull-Clark
/// subdivision by default, so polygonal meshes must always be explicitly
/// declared. The available subdivision schemes and additional subdivision
/// features encoded in optional attributes conform to the feature set of
/// OpenSubdiv
/// (https://graphics.pixar.com/opensubdiv/docs/subdivision_surfaces.html).
/// 
/// \anchor UsdGeom_Mesh_Primvars
/// __A Note About Primvars__
/// 
/// The following list clarifies the number of elements for and the
/// interpolation behavior of the different primvar interpolation types
/// for meshes:
/// 
/// - __constant__: One element for the entire mesh; no interpolation.
/// - __uniform__: One element for each face of the mesh; elements are
/// typically not interpolated but are inherited by other faces derived
/// from a given face (via subdivision, tessellation, etc.).
/// - __varying__: One element for each point of the mesh;
/// interpolation of point data is always linear.
/// - __vertex__: One element for each point of the mesh;
/// interpolation of point data is applied according to the
/// _subdivisionScheme_ attribute.
/// - __faceVarying__: One element for each of the face-vertices that
/// define the mesh topology; interpolation of face-vertex data may
/// be smooth or linear, according to the _subdivisionScheme_ and
/// _faceVaryingLinearInterpolation_ attributes.
/// 
/// Primvar interpolation types and related utilities are described more
/// generally in \ref Usd_InterpolationVals.
/// 
/// \anchor UsdGeom_Mesh_Normals
/// __A Note About Normals__
/// 
/// Normals should not be authored on a subdivision mesh, since subdivision
/// algorithms define their own normals. They should only be authored for
/// polygonal meshes (_subdivisionScheme_ = "none").
/// 
/// The _normals_ attribute inherited from UsdGeomPointBased is not a generic
/// primvar, but the number of elements in this attribute will be determined by
/// its _interpolation_.  See \ref UsdGeomPointBased::GetNormalsInterpolation() .
/// If _normals_ and _primvars:normals_ are both specified, the latter has
/// precedence.  If a polygonal mesh specifies __neither__ _normals_ nor
/// _primvars:normals_, then it should be treated and rendered as faceted,
/// with no attempt to compute smooth normals.
/// 
/// The normals generated for smooth subdivision schemes, e.g. Catmull-Clark
/// and Loop, will likewise be smooth, but others, e.g. Bilinear, may be
/// discontinuous between faces and/or within non-planar irregular faces.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdGeomTokens.
/// So to set an attribute to the value "rightHanded", use UsdGeomTokens->rightHanded
/// as the value.
///
class UsdGeomMesh : public UsdGeomPointBased
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

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
    USDGEOM_API
    virtual ~UsdGeomMesh();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
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
    USDGEOM_API
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
    USDGEOM_API
    static UsdGeomMesh
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
    // FACEVERTEXINDICES 
    // --------------------------------------------------------------------- //
    /// Flat list of the index (into the _points_ attribute) of each
    /// vertex of each face in the mesh.  If this attribute has more than
    /// one timeSample, the mesh is considered to be topologically varying.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int[] faceVertexIndices` |
    /// | C++ Type | VtArray<int> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->IntArray |
    USDGEOM_API
    UsdAttribute GetFaceVertexIndicesAttr() const;

    /// See GetFaceVertexIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateFaceVertexIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FACEVERTEXCOUNTS 
    // --------------------------------------------------------------------- //
    /// Provides the number of vertices in each face of the mesh, 
    /// which is also the number of consecutive indices in _faceVertexIndices_
    /// that define the face.  The length of this attribute is the number of
    /// faces in the mesh.  If this attribute has more than
    /// one timeSample, the mesh is considered to be topologically varying.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int[] faceVertexCounts` |
    /// | C++ Type | VtArray<int> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->IntArray |
    USDGEOM_API
    UsdAttribute GetFaceVertexCountsAttr() const;

    /// See GetFaceVertexCountsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateFaceVertexCountsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SUBDIVISIONSCHEME 
    // --------------------------------------------------------------------- //
    /// The subdivision scheme to be applied to the surface.
    /// Valid values are:
    /// 
    /// - __catmullClark__: The default, Catmull-Clark subdivision; preferred
    /// for quad-dominant meshes (generalizes B-splines); interpolation
    /// of point data is smooth (non-linear)
    /// - __loop__: Loop subdivision; preferred for purely triangular meshes;
    /// interpolation of point data is smooth (non-linear)
    /// - __bilinear__: Subdivision reduces all faces to quads (topologically
    /// similar to "catmullClark"); interpolation of point data is bilinear
    /// - __none__: No subdivision, i.e. a simple polygonal mesh; interpolation
    /// of point data is linear
    /// 
    /// Polygonal meshes are typically lighter weight and faster to render,
    /// depending on renderer and render mode.  Use of "bilinear" will produce
    /// a similar shape to a polygonal mesh and may offer additional guarantees
    /// of watertightness and additional subdivision features (e.g. holes) but
    /// may also not respect authored normals.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token subdivisionScheme = "catmullClark"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdGeomTokens "Allowed Values" | catmullClark, loop, bilinear, none |
    USDGEOM_API
    UsdAttribute GetSubdivisionSchemeAttr() const;

    /// See GetSubdivisionSchemeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateSubdivisionSchemeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INTERPOLATEBOUNDARY 
    // --------------------------------------------------------------------- //
    /// Specifies how subdivision is applied for faces adjacent to
    /// boundary edges and boundary points. Valid values correspond to choices
    /// available in OpenSubdiv:
    /// 
    /// - __none__: No boundary interpolation is applied and boundary faces are
    /// effectively treated as holes
    /// - __edgeOnly__: A sequence of boundary edges defines a smooth curve to
    /// which the edges of subdivided boundary faces converge
    /// - __edgeAndCorner__: The default, similar to "edgeOnly" but the smooth
    /// boundary curve is made sharp at corner points
    /// 
    /// These are illustrated and described in more detail in the OpenSubdiv
    /// documentation:
    /// https://graphics.pixar.com/opensubdiv/docs/subdivision_surfaces.html#boundary-interpolation-rules
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token interpolateBoundary = "edgeAndCorner"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref UsdGeomTokens "Allowed Values" | none, edgeOnly, edgeAndCorner |
    USDGEOM_API
    UsdAttribute GetInterpolateBoundaryAttr() const;

    /// See GetInterpolateBoundaryAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateInterpolateBoundaryAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // FACEVARYINGLINEARINTERPOLATION 
    // --------------------------------------------------------------------- //
    /// Specifies how elements of a primvar of interpolation type
    /// "faceVarying" are interpolated for subdivision surfaces. Interpolation
    /// can be as smooth as a "vertex" primvar or constrained to be linear at
    /// features specified by several options.  Valid values correspond to
    /// choices available in OpenSubdiv:
    /// 
    /// - __none__: No linear constraints or sharpening, smooth everywhere
    /// - __cornersOnly__: Sharpen corners of discontinuous boundaries only,
    /// smooth everywhere else
    /// - __cornersPlus1__: The default, same as "cornersOnly" plus additional
    /// sharpening at points where three or more distinct face-varying
    /// values occur
    /// - __cornersPlus2__: Same as "cornersPlus1" plus additional sharpening
    /// at points with at least one discontinuous boundary corner or
    /// only one discontinuous boundary edge (a dart)
    /// - __boundaries__: Piecewise linear along discontinuous boundaries,
    /// smooth interior
    /// - __all__: Piecewise linear everywhere
    /// 
    /// These are illustrated and described in more detail in the OpenSubdiv
    /// documentation:
    /// https://graphics.pixar.com/opensubdiv/docs/subdivision_surfaces.html#face-varying-interpolation-rules
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token faceVaryingLinearInterpolation = "cornersPlus1"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref UsdGeomTokens "Allowed Values" | none, cornersOnly, cornersPlus1, cornersPlus2, boundaries, all |
    USDGEOM_API
    UsdAttribute GetFaceVaryingLinearInterpolationAttr() const;

    /// See GetFaceVaryingLinearInterpolationAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateFaceVaryingLinearInterpolationAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TRIANGLESUBDIVISIONRULE 
    // --------------------------------------------------------------------- //
    /// Specifies an option to the subdivision rules for the
    /// Catmull-Clark scheme to try and improve undesirable artifacts when
    /// subdividing triangles.  Valid values are "catmullClark" for the
    /// standard rules (the default) and "smooth" for the improvement.
    /// 
    /// See https://graphics.pixar.com/opensubdiv/docs/subdivision_surfaces.html#triangle-subdivision-rule
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token triangleSubdivisionRule = "catmullClark"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref UsdGeomTokens "Allowed Values" | catmullClark, smooth |
    USDGEOM_API
    UsdAttribute GetTriangleSubdivisionRuleAttr() const;

    /// See GetTriangleSubdivisionRuleAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateTriangleSubdivisionRuleAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // HOLEINDICES 
    // --------------------------------------------------------------------- //
    /// The indices of all faces that should be treated as holes,
    /// i.e. made invisible. This is traditionally a feature of subdivision
    /// surfaces and not generally applied to polygonal meshes.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int[] holeIndices = []` |
    /// | C++ Type | VtArray<int> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->IntArray |
    USDGEOM_API
    UsdAttribute GetHoleIndicesAttr() const;

    /// See GetHoleIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateHoleIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CORNERINDICES 
    // --------------------------------------------------------------------- //
    /// The indices of points for which a corresponding sharpness
    /// value is specified in _cornerSharpnesses_ (so the size of this array
    /// must match that of _cornerSharpnesses_).
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int[] cornerIndices = []` |
    /// | C++ Type | VtArray<int> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->IntArray |
    USDGEOM_API
    UsdAttribute GetCornerIndicesAttr() const;

    /// See GetCornerIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateCornerIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CORNERSHARPNESSES 
    // --------------------------------------------------------------------- //
    /// The sharpness values associated with a corresponding set of
    /// points specified in _cornerIndices_ (so the size of this array must
    /// match that of _cornerIndices_). Use the constant `SHARPNESS_INFINITE`
    /// for a perfectly sharp corner.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float[] cornerSharpnesses = []` |
    /// | C++ Type | VtArray<float> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->FloatArray |
    USDGEOM_API
    UsdAttribute GetCornerSharpnessesAttr() const;

    /// See GetCornerSharpnessesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateCornerSharpnessesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CREASEINDICES 
    // --------------------------------------------------------------------- //
    /// The indices of points grouped into sets of successive pairs
    /// that identify edges to be creased. The size of this array must be
    /// equal to the sum of all elements of the _creaseLengths_ attribute.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int[] creaseIndices = []` |
    /// | C++ Type | VtArray<int> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->IntArray |
    USDGEOM_API
    UsdAttribute GetCreaseIndicesAttr() const;

    /// See GetCreaseIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateCreaseIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CREASELENGTHS 
    // --------------------------------------------------------------------- //
    /// The length of this array specifies the number of creases
    /// (sets of adjacent sharpened edges) on the mesh. Each element gives
    /// the number of points of each crease, whose indices are successively
    /// laid out in the _creaseIndices_ attribute. Since each crease must
    /// be at least one edge long, each element of this array must be at
    /// least two.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int[] creaseLengths = []` |
    /// | C++ Type | VtArray<int> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->IntArray |
    USDGEOM_API
    UsdAttribute GetCreaseLengthsAttr() const;

    /// See GetCreaseLengthsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateCreaseLengthsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CREASESHARPNESSES 
    // --------------------------------------------------------------------- //
    /// The per-crease or per-edge sharpness values for all creases.
    /// Since _creaseLengths_ encodes the number of points in each crease,
    /// the number of elements in this array will be either len(creaseLengths)
    /// or the sum over all X of (creaseLengths[X] - 1). Note that while
    /// the RI spec allows each crease to have either a single sharpness
    /// or a value per-edge, USD will encode either a single sharpness
    /// per crease on a mesh, or sharpnesses for all edges making up
    /// the creases on a mesh.  Use the constant `SHARPNESS_INFINITE` for a
    /// perfectly sharp crease.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float[] creaseSharpnesses = []` |
    /// | C++ Type | VtArray<float> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->FloatArray |
    USDGEOM_API
    UsdAttribute GetCreaseSharpnessesAttr() const;

    /// See GetCreaseSharpnessesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateCreaseSharpnessesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// Validate the topology of a mesh.
    /// This validates that the sum of \p faceVertexCounts is equal to the size
    /// of the \p faceVertexIndices array, and that all face vertex indices in
    /// the \p faceVertexIndices array are in the range [0, numPoints).
    /// Returns true if the topology is valid, or false otherwise.
    /// If the topology is invalid and \p reason is non-null, an error message
    /// describing the validation error will be set.
    USDGEOM_API
    static bool ValidateTopology(const VtIntArray& faceVertexIndices,
                                 const VtIntArray& faceVertexCounts,
                                 size_t numPoints,
                                 std::string* reason=nullptr);

public:

    /// Returns whether or not \p sharpness is considered infinite.
    ///
    /// The \p sharpness value is usually intended for 'creaseSharpness' or
    /// 'cornerSharpness' arrays and a return value of \c true indicates that
    /// the crease or corner is perfectly sharp.
    USDGEOM_API
    static bool IsSharpnessInfinite(const float sharpness);

    /// \var const float SHARPNESS_INFINITE
    /// As an element of a 'creaseSharpness' or 'cornerSharpness' array,
    /// indicates that the crease or corner is perfectly sharp.
    USDGEOM_API
    static const float SHARPNESS_INFINITE;

    /// Returns the number of faces as defined by the size of the
    /// _faceVertexCounts_ array at _timeCode_.
    ///
    /// \snippetdoc snippets.dox GetCount
    /// \sa GetFaceVertexCountsAttr()
    USDGEOM_API
    size_t GetFaceCount(UsdTimeCode timeCode = UsdTimeCode::Default()) const;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
