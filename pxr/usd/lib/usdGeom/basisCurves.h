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

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// BASISCURVES                                                                //
// -------------------------------------------------------------------------- //

/// Basis curves are analagous to RiCurves.  A basis matrix and vstep 
/// are used to uniformly interpolate the curves.  These curves are often used 
/// to render dense aggregate geometry like hair.
/// 
/// A segment is the smoothly interpolated part between vertices.  A curves prim 
/// may have many curves, determined implicitly by the length of the 
/// curveVertexCount vector.  The number of segments an individual curve has is 
/// determined by the number of vertices it has, the wrap (periodicity), and the 
/// vstep of the basis.
/// 
/// Basis to vstep mapping:
/// [bezier - 3, catmullRom - 1, bspline - 1, hermite - 2, power - 4]
/// 
/// The first segment of a cubic curve is always defined by its first 4 points. 
/// The vstep is the increment used to determine what cv determines the 
/// next segment.  For a two segment bspline basis curve (vstep=1), the first 
/// segment will be defined by interpolating vertices [0, 1, 2, 3] and the 
/// second  segment will be defined by [1, 2, 3, 4].  For a two segment bezier 
/// basis curve (vstep = 3), the first segment will be defined by interpolating 
/// vertices [0, 1, 2, 3] and the second segment will be defined by 
/// [3, 4, 5, 6].  If the vstep is not one, then you must take special care
/// to make sure that the number of cvs properly divides by your vstep.
/// 
/// For a given cubic curve, it holds that:
/// (curveVertexCount - 4) % vstep == 0 [nonperiodic]
/// (curveVertexCount) % vstep == 0 [periodic]
/// 
/// For a given linear curve: 
/// segmentCount = curveVertexCount - 1 [nonperiodic]
/// segmentCount = curveVertexCount [periodic]
/// For a given cubic curve:
/// segmentCount = (curveVertexCount - 4) / vstep + 1 [nonperiodic]
/// segmentCount = curveVertexCount / vstep [periodic]
/// 
/// For cubic curves, data can be either interpolated between vertices or 
/// across segments.  Per vertex data should be the same size as the number
/// of vertices in your curve.  Segment varying data is dependent on the 
/// wrap (periodicity) and number of segments in your curve.
/// 
/// For a given curve:
/// segmentVaryingDataSize = segmentCount + 1 [nonperiodic]
/// segmentVaryingDataSize = segmentCount [periodic]    
/// 
/// This prim represents two different entries in the RI spec:  RiBasis
/// and RiCurves, hence the name "BasisCurves."  If we are interested in
/// specifying a custom basis as RenderMan allows you to do, the basis
/// enum could be extended with a new "custom" token and with additional 
/// attributes vstep and matrix, but for compatability with AbcGeom and the
/// rarity of this use case, it is omitted for now.
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
    virtual ~UsdGeomBasisCurves();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// \brief Return a UsdGeomBasisCurves holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomBasisCurves(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    static UsdGeomBasisCurves
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// \brief Attempt to ensure a \a UsdPrim adhering to this schema at \p path
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
    static UsdGeomBasisCurves
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
    UsdAttribute GetTypeAttr() const;

    /// See GetTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
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
    UsdAttribute GetBasisAttr() const;

    /// See GetBasisAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
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
    UsdAttribute GetWrapAttr() const;

    /// See GetWrapAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateWrapAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to close the class delcaration with }; and complete the
    // include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

    /// \name Helper functions for working with UsdGeomCurves
    /// \{

    typedef std::vector< std::pair<TfToken, size_t> > ComputeInterpolationInfo;
    /// \brief Computes interpolation token for \p n.  
    /// if this returns an empty token and \p info was non-NULL, it'll contain
    /// the expected value for each token.
    ///
    /// The topology is determined using \p timeCode.
    TfToken ComputeInterpolationForSize(size_t n, 
            const UsdTimeCode& timeCode,
            ComputeInterpolationInfo* info=NULL) const;

    /// \brief Computes the expected size for data with "uniform" interpolation
    ///
    /// If you're trying to determine what interpolation to use, it is more
    /// efficient to use \c ComputeInterpolationForSize
    size_t ComputeUniformDataSize(const UsdTimeCode& timeCode) const;

    /// \brief Computes the expected size for data with "varying" interpolation
    ///
    /// If you're trying to determine what interpolation to use, it is more
    /// efficient to use \c ComputeInterpolationForSize
    size_t ComputeVaryingDataSize(const UsdTimeCode& timeCode) const;

    /// \brief Computes the expected size for data with "vertex" interpolation
    ///
    /// If you're trying to determine what interpolation to use, it is more
    /// efficient to use \c ComputeInterpolationForSize
    size_t ComputeVertexDataSize(const UsdTimeCode& timeCode) const;

    /// \}
};

#endif
