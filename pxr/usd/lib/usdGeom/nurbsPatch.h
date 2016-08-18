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
#ifndef USDGEOM_GENERATED_NURBSPATCH_H
#define USDGEOM_GENERATED_NURBSPATCH_H

/// \file usdGeom/nurbsPatch.h

#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usdGeom/pointBased.h"
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
// NURBSPATCH                                                                 //
// -------------------------------------------------------------------------- //

/// \class UsdGeomNurbsPatch
///
/// Encodes a rational or polynomial non-uniform B-spline
/// surface, with optional trim curves.
/// 
/// The encoding mostly follows that of RiNuPatch and RiTrimCurve: 
/// https://renderman.pixar.com/resources/current/RenderMan/geometricPrimitives.html#rinupatch , with some minor renaming and coalescing for clarity.
/// 
/// The layout of control vertices in the \em points attribute inherited
/// from UsdGeomPointBased is row-major with U considered rows, and V columns.
/// 
/// \anchor UsdGeom_NurbsPatch_Form
/// <b>NurbsPatch Form</b>
/// 
/// The authored points, orders, knots, weights, and ranges are all that is
/// required to render the nurbs patch.  However, the only way to model closed
/// surfaces with nurbs is to ensure that the first and last control points
/// along the given axis are coincident.  Similarly, to ensure the surface is
/// not only closed but also C2 continuous, the last \em order - 1 control
/// points must be (correspondingly) coincident with the first \em order - 1
/// control points, and also the spacing of the last corresponding knots
/// must be the same as the first corresponding knots.
/// 
/// <b>Form</b> is provided as an aid to interchange between modeling and
/// animation applications so that they can robustly identify the intent with
/// which the surface was modelled, and take measures (if they are able) to
/// preserve the continuity/concidence constraints as the surface may be rigged
/// or deformed.  
/// \li An \em open-form NurbsPatch has no continuity constraints.
/// \li A \em closed-form NurbsPatch expects the first and last control points
/// to overlap
/// \li A \em periodic-form NurbsPatch expects the first and last
/// \em order - 1 control points to overlap.
/// 
/// <b>Nurbs vs Subdivision Surfaces</b>
/// 
/// Nurbs are an important modeling primitive in CAD/CAM tools and early
/// computer graphics DCC's.  Because they have a natural UV parameterization
/// they easily support "trim curves", which allow smooth shapes to be
/// carved out of the surface.
/// 
/// However, the topology of the patch is always rectangular, and joining two 
/// nurbs patches together (especially when they have differing numbers of
/// spans) is difficult to do smoothly.  Also, nurbs are not supported by
/// the Ptex texturing technology (http://ptex.us).
/// 
/// Neither of these limitations are shared by subdivision surfaces; therefore,
/// although they do not subscribe to trim-curve-based shaping, subdivs are
/// often considered a more flexible modeling primitive.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdGeomTokens.
/// So to set an attribute to the value "rightHanded", use UsdGeomTokens->rightHanded
/// as the value.
///
class UsdGeomNurbsPatch : public UsdGeomPointBased
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = true;

    /// Construct a UsdGeomNurbsPatch on UsdPrim \p prim .
    /// Equivalent to UsdGeomNurbsPatch::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomNurbsPatch(const UsdPrim& prim=UsdPrim())
        : UsdGeomPointBased(prim)
    {
    }

    /// Construct a UsdGeomNurbsPatch on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomNurbsPatch(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomNurbsPatch(const UsdSchemaBase& schemaObj)
        : UsdGeomPointBased(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomNurbsPatch();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomNurbsPatch holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomNurbsPatch(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomNurbsPatch
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
    static UsdGeomNurbsPatch
    Define(const UsdStagePtr &stage, const SdfPath &path);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDGEOM_API
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // UVERTEXCOUNT 
    // --------------------------------------------------------------------- //
    /// Number of vertices in the U direction.  Should be at least as
    /// large as uOrder.
    ///
    /// \n  C++ Type: int
    /// \n  Usd Type: SdfValueTypeNames->Int
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetUVertexCountAttr() const;

    /// See GetUVertexCountAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateUVertexCountAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VVERTEXCOUNT 
    // --------------------------------------------------------------------- //
    /// Number of vertices in the V direction.  Should be at least as
    /// large as vOrder.
    ///
    /// \n  C++ Type: int
    /// \n  Usd Type: SdfValueTypeNames->Int
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetVVertexCountAttr() const;

    /// See GetVVertexCountAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateVVertexCountAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // UORDER 
    // --------------------------------------------------------------------- //
    /// Order in the U direction.  Order must be positive and is
    /// equal to the degree of the polynomial basis to be evaluated, plus 1.
    ///
    /// \n  C++ Type: int
    /// \n  Usd Type: SdfValueTypeNames->Int
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetUOrderAttr() const;

    /// See GetUOrderAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateUOrderAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VORDER 
    // --------------------------------------------------------------------- //
    /// Order in the V direction.  Order must be positive and is
    /// equal to the degree of the polynomial basis to be evaluated, plus 1.
    ///
    /// \n  C++ Type: int
    /// \n  Usd Type: SdfValueTypeNames->Int
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetVOrderAttr() const;

    /// See GetVOrderAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateVOrderAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // UKNOTS 
    // --------------------------------------------------------------------- //
    /// Knot vector for U direction providing U parameterization.
    /// The length of this array must be ( uVertexCount + uOrder ), and its
    /// entries must take on monotonically increasing values.
    ///
    /// \n  C++ Type: VtArray<double>
    /// \n  Usd Type: SdfValueTypeNames->DoubleArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetUKnotsAttr() const;

    /// See GetUKnotsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateUKnotsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VKNOTS 
    // --------------------------------------------------------------------- //
    /// Knot vector for V direction providing U parameterization.
    /// The length of this array must be ( vVertexCount + vOrder ), and its
    /// entries must take on monotonically increasing values.
    ///
    /// \n  C++ Type: VtArray<double>
    /// \n  Usd Type: SdfValueTypeNames->DoubleArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetVKnotsAttr() const;

    /// See GetVKnotsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateVKnotsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // UFORM 
    // --------------------------------------------------------------------- //
    /// Interpret the control grid and knot vectors as representing
    /// an open, geometrically closed, or geometrically closed and C2 continuous
    /// surface along the U dimension.
    /// \sa \ref UsdGeom_NurbsPatch_Form "NurbsPatch Form" 
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: open
    /// \n  \ref UsdGeomTokens "Allowed Values": [open, closed, periodic]
    USDGEOM_API
    UsdAttribute GetUFormAttr() const;

    /// See GetUFormAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateUFormAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VFORM 
    // --------------------------------------------------------------------- //
    /// Interpret the control grid and knot vectors as representing
    /// an open, geometrically closed, or geometrically closed and C2 continuous
    /// surface along the V dimension.
    /// \sa \ref UsdGeom_NurbsPatch_Form "NurbsPatch Form" 
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: open
    /// \n  \ref UsdGeomTokens "Allowed Values": [open, closed, periodic]
    USDGEOM_API
    UsdAttribute GetVFormAttr() const;

    /// See GetVFormAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateVFormAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // URANGE 
    // --------------------------------------------------------------------- //
    /// Provides the minimum and maximum parametric values (as defined
    /// by uKnots) over which the surface is actually defined.  The minimum
    /// must be less than the maximum, and greater than or equal to the
    /// value of uKnots[uOrder-1].  The maxium must be less than or equal
    /// to the last element's value in uKnots.
    ///
    /// \n  C++ Type: GfVec2d
    /// \n  Usd Type: SdfValueTypeNames->Double2
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetURangeAttr() const;

    /// See GetURangeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateURangeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VRANGE 
    // --------------------------------------------------------------------- //
    /// Provides the minimum and maximum parametric values (as defined
    /// by vKnots) over which the surface is actually defined.  The minimum
    /// must be less than the maximum, and greater than or equal to the
    /// value of vKnots[vOrder-1].  The maxium must be less than or equal
    /// to the last element's value in vKnots.
    ///
    /// \n  C++ Type: GfVec2d
    /// \n  Usd Type: SdfValueTypeNames->Double2
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetVRangeAttr() const;

    /// See GetVRangeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateVRangeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // POINTWEIGHTS 
    // --------------------------------------------------------------------- //
    /// Optionally provides "w" components for each control point,
    /// thus must be the same length as the points attribute.  If authored,
    /// the patch will be rational.  If unauthored, the patch will be
    /// polynomial, i.e. weight for all points is 1.0.
    /// \note Some DCC's pre-weight the \em points, but in this schema, 
    /// \em points are not pre-weighted.
    ///
    /// \n  C++ Type: VtArray<double>
    /// \n  Usd Type: SdfValueTypeNames->DoubleArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetPointWeightsAttr() const;

    /// See GetPointWeightsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreatePointWeightsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TRIMCURVECOUNTS 
    // --------------------------------------------------------------------- //
    /// Each element specifies how many curves are present in each
    /// "loop" of the trimCurve, and the length of the array determines how
    /// many loops the trimCurve contains.  The sum of all elements is the
    /// total nuber of curves in the trim, to which we will refer as 
    /// \em nCurves in describing the other trim attributes.
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetTrimCurveCountsAttr() const;

    /// See GetTrimCurveCountsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateTrimCurveCountsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TRIMCURVEORDERS 
    // --------------------------------------------------------------------- //
    /// Flat list of orders for each of the \em nCurves curves.
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetTrimCurveOrdersAttr() const;

    /// See GetTrimCurveOrdersAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateTrimCurveOrdersAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TRIMCURVEVERTEXCOUNTS 
    // --------------------------------------------------------------------- //
    /// Flat list of number of vertices for each of the
    /// \em nCurves curves.
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetTrimCurveVertexCountsAttr() const;

    /// See GetTrimCurveVertexCountsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateTrimCurveVertexCountsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TRIMCURVEKNOTS 
    // --------------------------------------------------------------------- //
    /// Flat list of parametric values for each of the
    /// \em nCurves curves.  There will be as many knots as the sum over
    /// all elements of \em vertexCounts plus the sum over all elements of
    /// \em orders.
    ///
    /// \n  C++ Type: VtArray<double>
    /// \n  Usd Type: SdfValueTypeNames->DoubleArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetTrimCurveKnotsAttr() const;

    /// See GetTrimCurveKnotsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateTrimCurveKnotsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TRIMCURVERANGES 
    // --------------------------------------------------------------------- //
    /// Flat list of minimum and maximum parametric values 
    /// (as defined by \em knots) for each of the \em nCurves curves.
    ///
    /// \n  C++ Type: VtArray<GfVec2d>
    /// \n  Usd Type: SdfValueTypeNames->Double2Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetTrimCurveRangesAttr() const;

    /// See GetTrimCurveRangesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateTrimCurveRangesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TRIMCURVEPOINTS 
    // --------------------------------------------------------------------- //
    /// Flat list of homogeneous 2D points (u, v, w) that comprise
    /// the \em nCurves curves.  The number of points should be equal to the
    /// um over all elements of \em vertexCounts.
    ///
    /// \n  C++ Type: VtArray<GfVec3d>
    /// \n  Usd Type: SdfValueTypeNames->Double3Array
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetTrimCurvePointsAttr() const;

    /// See GetTrimCurvePointsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateTrimCurvePointsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to close the class delcaration with }; and complete the
    // include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--
};

#endif
