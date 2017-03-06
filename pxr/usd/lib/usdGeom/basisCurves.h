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
/// Basis curves are analagous to RiCurves.  A 'basis' matrix and 
/// \em vstep are used to uniformly interpolate the curves.  These curves are 
/// often used to render dense aggregate geometry like hair.
/// 
/// A curves prim may have many curves, determined implicitly by the length of
/// the 'curveVertexCounts' vector.  An individual curve is composed of one 
/// or more curve segments, the smoothly interpolated part between vertices.
/// 
/// Curves may have a m'type' of either linear or cubic.  Linear curve 
/// segments are interpolated between two vertices, and cubic curve segments are 
/// interpolated between 4 vertices.  The segment count of a cubic curve  
/// is determined by the vertex count, the 'wrap' (periodicity), and 
/// the vstep of the basis.
/// 
/// cubic basis   | vstep
/// ------------- | -------------
/// bezier        | 3
/// catmullRom    | 1
/// bspline       | 1
/// hermite       | 2
/// power         | 4
/// 
/// The first segment of a cubic curve is always defined by its first 4 points. 
/// The vstep is the increment used to determine what cv determines the 
/// next segment.  For a two segment bspline basis curve (vstep = 1), the first 
/// segment will be defined by interpolating vertices [0, 1, 2, 3] and the 
/// second  segment will be defined by [1, 2, 3, 4].  For a two segment bezier 
/// basis curve (vstep = 3), the first segment will be defined by interpolating 
/// vertices [0, 1, 2, 3] and the second segment will be defined by 
/// [3, 4, 5, 6].  If the vstep is not one, then you must take special care
/// to make sure that the number of cvs properly divides by your vstep.  If 
/// the type of a curve is linear, the basis matrix and vstep are unused.
/// 
/// When validating curve topology, each entry in the curveVertexCounts vector
/// must pass this check.
/// 
/// wrap           | cubic vertex count validity
/// -------------- | ---------------------------------------
/// nonperiodic    | (curveVertexCounts[i] - 4) % vstep == 0
/// periodic       | (curveVertexCounts[i]) % vstep == 0
/// 
/// 
/// To convert an entry in the curveVertexCounts vector into a segment count 
/// for an individual curve, apply these rules.  Sum up all the results in
/// order to compute how many total segments all curves have.
/// 
/// wrap          | segment count [linear curves]
/// ------------- | ----------------------------------------
/// nonperiodic   | curveVertexCounts[i] - 1
/// periodic      | curveVertexCounts[i]
/// 
/// wrap          | segment count [cubic curves]
/// ------------- | ----------------------------------------
/// nonperiodic   | (curveVertexCounts[i] - 4) / vstep + 1
/// periodic      | curveVertexCounts[i] / vstep 
/// 
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
/// wrap          | expected linear (varying) data size
/// ------------- | ----------------------------------------
/// nonperiodic   | segmentCount + 1
/// periodic      | segmentCount
/// 
/// Both curve types additionally define 'constant'
/// interpolation for the entire prim and 'uniform' interpolation as per curve 
/// data.
/// 
/// \image html USDCurvePrimvars.png
/// 
/// While not technically UsdGeomPrimvars, the widths and optional normals
/// also have interpolation metadata.  It's common for authored widths to have
/// constant, varying, or vertex interpolation
/// (see UsdGeomCurves::GetWidthsInterpolation()).  It's common for
/// authored normals to have varying interpolation
/// (see UsdGeomPointBased::GetNormalsInterpolation()).
/// 
/// This prim represents two different entries in the RI spec:  RiBasis
/// and RiCurves, hence the name "BasisCurves."  If we are interested in
/// specifying a custom basis as RenderMan allows you to do, the basis
/// enum could be extended with a new "custom" token and with additional 
/// attributes vstep and matrix, but for compatability with AbcGeom and the
/// rarity of this use case, it is omitted for now.
/// 
/// Example of deriving per curve segment and varying primvar data counts from
/// the wrap, type, basis, and curveVertexCount.
/// 
/// wrap          | type    | basis   | curveVertexCount  | curveSegmentCount  | varyingDataCount
/// ------------- | ------- | ------- | ----------------- | ------------------ | -------------------------
/// nonperiodic   | linear  | N/A     | [2 3 2 5]         | [1 2 1 4]          | [2 3 2 5]
/// nonperiodic   | cubic   | bezier  | [4 7 10 4 7]      | [1 2 3 1 2]        | [2 3 4 2 3]
/// nonperiodic   | cubic   | bspline | [5 4 6 7]         | [2 1 3 4]          | [3 2 4 5]
/// periodic      | cubic   | bezier  | [6 9 6]           | [2 3 2]            | [2 3 2]
/// periodic      | linear  | N/A     | [3 7]             | [3 7]              | [3 7]
/// 
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
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = true;

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
    // --------------------------------------------------------------------- //
    // TYPE 
    // --------------------------------------------------------------------- //
    /// Linear curves interpolate linearly between cvs.  
    /// Cubic curves use a basis matrix with 4 cvs to interpolate a segment.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: cubic
    /// \n  \ref UsdGeomTokens "Allowed Values": [linear, cubic]
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
    /// the basis specifies the vstep and matrix used for interpolation.
    /// a custom basis could be supported with the addition of a custom token
    /// and an additional set of matrix/vstep parameters.  For simplicity and
    /// consistency with AbcGeom, we have omitted this.  The order of basis
    /// and default value is intentionally the same as AbcGeom.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: bezier
    /// \n  \ref UsdGeomTokens "Allowed Values": [bezier, bspline, catmullRom, hermite, power]
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
    /// if wrap is set to periodic, the curve when rendered will 
    /// repeat the initial vertices (dependent on the vstep) to connect the
    /// end points.
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: nonperiodic
    /// \n  \ref UsdGeomTokens "Allowed Values": [nonperiodic, periodic]
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

    /// \}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
