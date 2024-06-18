//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDGEOM_GENERATED_BASISCURVES_H
#define USDGEOM_GENERATED_BASISCURVES_H

/// \file usdGeom/basisCurves.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usdGeom/curves.h"
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
// BASISCURVES                                                                //
// -------------------------------------------------------------------------- //

/// \class UsdGeomBasisCurves
///
/// BasisCurves are a batched curve representation analogous to the
/// classic RIB definition via Basis and Curves statements. BasisCurves are
/// often used to render dense aggregate geometry like hair or grass.
/// 
/// A 'matrix' and 'vstep' associated with the \em basis are used to
/// interpolate the vertices of a cubic BasisCurves. (The basis attribute
/// is unused for linear BasisCurves.)
/// 
/// A single prim may have many curves whose count is determined implicitly by
/// the length of the \em curveVertexCounts vector.  Each individual curve is
/// composed of one or more segments. Each segment is defined by four vertices
/// for cubic curves and two vertices for linear curves. See the next section
/// for more information on how to map curve vertex counts to segment counts.
/// 
/// \section UsdGeomBasisCurves_Segment Segment Indexing
/// Interpolating a curve requires knowing how to decompose it into its 
/// individual segments.
/// 
/// The segments of a cubic curve are determined by the vertex count,
/// the \em wrap (periodicity), and the vstep of the basis. For linear
/// curves, the basis token is ignored and only the vertex count and
/// wrap are needed.
/// 
/// cubic basis   | vstep
/// ------------- | ------
/// bezier        | 3
/// catmullRom    | 1
/// bspline       | 1
/// 
/// The first segment of a cubic (nonperiodic) curve is always defined by its
/// first four points. The vstep is the increment used to determine what
/// vertex indices define the next segment.  For a two segment (nonperiodic)
/// bspline basis curve (vstep = 1), the first segment will be defined by
/// interpolating vertices [0, 1, 2, 3] and the second segment will be defined
/// by [1, 2, 3, 4].  For a two segment bezier basis curve (vstep = 3), the
/// first segment will be defined by interpolating vertices [0, 1, 2, 3] and
/// the second segment will be defined by [3, 4, 5, 6].  If the vstep is not
/// one, then you must take special care to make sure that the number of cvs
/// properly divides by your vstep. (The indices described are relative to
/// the initial vertex index for a batched curve.)
/// 
/// For periodic curves, at least one of the curve's initial vertices are
/// repeated to close the curve. For cubic curves, the number of vertices
/// repeated is '4 - vstep'. For linear curves, only one vertex is repeated
/// to close the loop.
/// 
/// Pinned curves are a special case of nonperiodic curves that only affects
/// the behavior of cubic Bspline and Catmull-Rom curves. To evaluate or render
/// pinned curves, a client must effectively add 'phantom points' at the 
/// beginning and end of every curve in a batch.  These phantom points
/// are injected to ensure that the interpolated curve begins at P[0] and
/// ends at P[n-1].
/// 
/// For a curve with initial point P[0] and last point P[n-1], the phantom
/// points are defined as.
/// P[-1]  = 2 * P[0] - P[1]
/// P[n] = 2 * P[n-1] - P[n-2]
/// 
/// Pinned cubic curves will (usually) have to be unpacked into the standard
/// nonperiodic representation before rendering. This unpacking can add some 
/// additional overhead. However, using pinned curves reduces the amount of
/// data recorded in a scene and (more importantly) better records the
/// authors' intent for interchange.
/// 
/// \note The additional phantom points mean that the minimum curve vertex
/// count for cubic bspline and catmullRom curves is 2.
/// 
/// Linear curve segments are defined by two vertices.
/// A two segment linear curve's first segment would be defined by
/// interpolating vertices [0, 1]. The second segment would be defined by 
/// vertices [1, 2]. (Again, for a batched curve, indices are relative to
/// the initial vertex index.)
/// 
/// When validating curve topology, each renderable entry in the
/// curveVertexCounts vector must pass this check.
/// 
/// type    | wrap                        | validitity
/// ------- | --------------------------- | ----------------
/// linear  | nonperiodic                 | curveVertexCounts[i] > 2
/// linear  | periodic                    | curveVertexCounts[i] > 3
/// cubic   | nonperiodic                 | (curveVertexCounts[i] - 4) % vstep == 0
/// cubic   | periodic                    | (curveVertexCounts[i]) % vstep == 0
/// cubic   | pinned (catmullRom/bspline) | (curveVertexCounts[i] - 2) >= 0
/// 
/// \section UsdGeomBasisCurves_BasisMatrix Cubic Vertex Interpolation
/// 
/// \image html USDCurveBasisMatrix.png width=750
/// 
/// \section UsdGeomBasisCurves_Linear Linear Vertex Interpolation
/// 
/// Linear interpolation is always used on curves of type linear.
/// 't' with domain [0, 1], the curve is defined by the equation 
/// P0 * (1-t) + P1 * t. t at 0 describes the first point and t at 1 describes
/// the end point.
/// 
/// \section UsdGeomBasisCurves_PrimvarInterpolation Primvar Interpolation
/// 
/// For cubic curves, primvar data can be either interpolated cubically between 
/// vertices or linearly across segments.  The corresponding token
/// for cubic interpolation is 'vertex' and for linear interpolation is
/// 'varying'.  Per vertex data should be the same size as the number
/// of vertices in your curve.  Segment varying data is dependent on the 
/// wrap (periodicity) and number of segments in your curve.  For linear curves,
/// varying and vertex data would be interpolated the same way.  By convention 
/// varying is the preferred interpolation because of the association of 
/// varying with linear interpolation.
/// 
/// \image html USDCurvePrimvars.png 
/// 
/// To convert an entry in the curveVertexCounts vector into a segment count 
/// for an individual curve, apply these rules.  Sum up all the results in
/// order to compute how many total segments all curves have.
/// 
/// The following tables describe the expected segment count for the 'i'th
/// curve in a curve batch as well as the entire batch. Python syntax
/// like '[:]' (to describe all members of an array) and 'len(...)' 
/// (to describe the length of an array) are used.
/// 
/// type    | wrap                        | curve segment count                    | batch segment count                                                       
/// ------- | --------------------------- | -------------------------------------- | --------------------------
/// linear  | nonperiodic                 | curveVertexCounts[i] - 1               | sum(curveVertexCounts[:]) - len(curveVertexCounts)
/// linear  | periodic                    | curveVertexCounts[i]                   | sum(curveVertexCounts[:])
/// cubic   | nonperiodic                 | (curveVertexCounts[i] - 4) / vstep + 1 | sum(curveVertexCounts[:] - 4) / vstep + len(curveVertexCounts)
/// cubic   | periodic                    | curveVertexCounts[i] / vstep           | sum(curveVertexCounts[:]) / vstep
/// cubic   | pinned (catmullRom/bspline) | (curveVertexCounts[i] - 2) + 1         | sum(curveVertexCounts[:] - 2) + len(curveVertexCounts)
/// 
/// The following table descrives the expected size of varying
/// (linearly interpolated) data, derived from the segment counts computed
/// above.
/// 
/// wrap                | curve varying count          | batch varying count
/// ------------------- | ---------------------------- | ------------------------------------------------
/// nonperiodic/pinned  | segmentCounts[i] + 1         | sum(segmentCounts[:]) + len(curveVertexCounts)
/// periodic            | segmentCounts[i]             | sum(segmentCounts[:])
/// 
/// Both curve types additionally define 'constant' interpolation for the
/// entire prim and 'uniform' interpolation as per curve data.
/// 
/// 
/// \note Take care when providing support for linearly interpolated data for
/// cubic curves. Its shape doesn't provide a one to one mapping with either
/// the number of curves (like 'uniform') or the number of vertices (like
/// 'vertex') and so it is often overlooked. This is the only primitive in
/// UsdGeom (as of this writing) where this is true. For meshes, while they
/// use different interpolation methods, 'varying' and 'vertex' are both
/// specified per point. It's common to assume that curves follow a similar
/// pattern and build in structures and language for per primitive, per
/// element, and per point data only to come upon these arrays that don't 
/// quite fit into either of those categories. It is
/// also common to conflate 'varying' with being per segment data and use the
/// segmentCount rules table instead of its neighboring varying data table
/// rules. We suspect that this is because for the common case of
/// nonperiodic cubic curves, both the provided segment count and varying data
/// size formula end with '+ 1'. While debugging, users may look at the double
/// '+ 1' as a mistake and try to remove it.  We take this time to enumerate
/// these issues because we've fallen into them before and hope that we save
/// others time in their own implementations.
/// 
/// As an example of deriving per curve segment and varying primvar data counts from
/// the wrap, type, basis, and curveVertexCount, the following table is provided.
/// 
/// wrap          | type    | basis   | curveVertexCount  | curveSegmentCount  | varyingDataCount
/// ------------- | ------- | ------- | ----------------- | ------------------ | -------------------------
/// nonperiodic   | linear  | N/A     | [2 3 2 5]         | [1 2 1 4]          | [2 3 2 5]
/// nonperiodic   | cubic   | bezier  | [4 7 10 4 7]      | [1 2 3 1 2]        | [2 3 4 2 3]
/// nonperiodic   | cubic   | bspline | [5 4 6 7]         | [2 1 3 4]          | [3 2 4 5]
/// periodic      | cubic   | bezier  | [6 9 6]           | [2 3 2]            | [2 3 2]
/// periodic      | linear  | N/A     | [3 7]             | [3 7]              | [3 7]
/// 
/// \section UsdGeomBasisCurves_TubesAndRibbons Tubes and Ribbons
/// 
/// The strictest definition of a curve as an infinitely thin wire is not 
/// particularly useful for describing production scenes. The additional
/// \em widths and \em normals attributes can be used to describe cylindrical
/// tubes and or flat oriented ribbons.
/// 
/// Curves with only widths defined are imaged as tubes with radius
/// 'width / 2'. Curves with both widths and normals are imaged as ribbons
/// oriented in the direction of the interpolated normal vectors.
/// 
/// While not technically UsdGeomPrimvars, widths and normals
/// also have interpolation metadata. It's common for authored widths to have
/// constant, varying, or vertex interpolation 
/// (see UsdGeomCurves::GetWidthsInterpolation()).  It's common for
/// authored normals to have varying interpolation 
/// (see UsdGeomPointBased::GetNormalsInterpolation()).
/// 
/// \image html USDCurveHydra.png
/// 
/// The file used to generate these curves can be found in
/// extras/usd/examples/usdGeomExamples/basisCurves.usda.  It's provided
/// as a reference on how to properly image both tubes and ribbons. The first
/// row of curves are linear; the second are cubic bezier. (We aim in future
/// releases of HdSt to fix the discontinuity seen with broken tangents to
/// better match offline renderers like RenderMan.) The yellow and violet
/// cubic curves represent cubic vertex width interpolation for which there is
/// no equivalent for linear curves.
/// 
/// \note How did this prim type get its name?  This prim is a portmanteau of
/// two different statements in the original RenderMan specification:
/// 'Basis' and 'Curves'.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdGeomTokens.
/// So to set an attribute to the value "rightHanded", use UsdGeomTokens->rightHanded
/// as the value.
///
class UsdGeomBasisCurves : public UsdGeomCurves
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdGeomBasisCurves on UsdPrim \p prim .
    /// Equivalent to UsdGeomBasisCurves::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomBasisCurves(const UsdPrim& prim=UsdPrim())
        : UsdGeomCurves(prim)
    {
    }

    /// Construct a UsdGeomBasisCurves on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomBasisCurves(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomBasisCurves(const UsdSchemaBase& schemaObj)
        : UsdGeomCurves(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomBasisCurves();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomBasisCurves holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomBasisCurves(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomBasisCurves
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
    static UsdGeomBasisCurves
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
    // TYPE 
    // --------------------------------------------------------------------- //
    /// Linear curves interpolate linearly between two vertices.  
    /// Cubic curves use a basis matrix with four vertices to interpolate a segment.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token type = "cubic"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdGeomTokens "Allowed Values" | linear, cubic |
    USDGEOM_API
    UsdAttribute GetTypeAttr() const;

    /// See GetTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // BASIS 
    // --------------------------------------------------------------------- //
    /// The basis specifies the vstep and matrix used for cubic 
    /// interpolation.  \note The 'hermite' and 'power' tokens have been
    /// removed. We've provided UsdGeomHermiteCurves
    /// as an alternative for the 'hermite' basis.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token basis = "bezier"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdGeomTokens "Allowed Values" | bezier, bspline, catmullRom |
    USDGEOM_API
    UsdAttribute GetBasisAttr() const;

    /// See GetBasisAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateBasisAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // WRAP 
    // --------------------------------------------------------------------- //
    /// If wrap is set to periodic, the curve when rendered will 
    /// repeat the initial vertices (dependent on the vstep) to close the
    /// curve. If wrap is set to 'pinned', phantom points may be created
    /// to ensure that the curve interpolation starts at P[0] and ends at P[n-1].
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token wrap = "nonperiodic"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdGeomTokens "Allowed Values" | nonperiodic, periodic, pinned |
    USDGEOM_API
    UsdAttribute GetWrapAttr() const;

    /// See GetWrapAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateWrapAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// \name Helper functions for working with UsdGeomCurves
    /// \{

    typedef std::vector< std::pair<TfToken, size_t> > ComputeInterpolationInfo;

    /// Computes interpolation token for \p n.
    ///
    /// If this returns an empty token and \p info was non-NULL, it'll contain
    /// the expected value for each token.
    ///
    /// The topology is determined using \p timeCode.
    USDGEOM_API
    TfToken ComputeInterpolationForSize(size_t n, 
            const UsdTimeCode& timeCode,
            ComputeInterpolationInfo* info=NULL) const;

    /// Computes the expected size for data with "uniform" interpolation.
    ///
    /// If you're trying to determine what interpolation to use, it is more
    /// efficient to use \c ComputeInterpolationForSize
    USDGEOM_API
    size_t ComputeUniformDataSize(const UsdTimeCode& timeCode) const;

    /// Computes the expected size for data with "varying" interpolation.
    ///
    /// If you're trying to determine what interpolation to use, it is more
    /// efficient to use \c ComputeInterpolationForSize
    USDGEOM_API
    size_t ComputeVaryingDataSize(const UsdTimeCode& timeCode) const;

    /// Computes the expected size for data with "vertex" interpolation.
    ///
    /// If you're trying to determine what interpolation to use, it is more
    /// efficient to use \c ComputeInterpolationForSize
    USDGEOM_API
    size_t ComputeVertexDataSize(const UsdTimeCode& timeCode) const;

    /// Computes the segment counts of the curves based on their vertex counts
    /// from the \c curveVertexCounts attribute.
    USDGEOM_API
    VtIntArray ComputeSegmentCounts(const UsdTimeCode& timeCode) const;

    /// \}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
