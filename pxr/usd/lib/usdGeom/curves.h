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
#ifndef USDGEOM_GENERATED_CURVES_H
#define USDGEOM_GENERATED_CURVES_H

/// \file usdGeom/curves.h

#include "pxr/pxr.h"
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

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// CURVES                                                                     //
// -------------------------------------------------------------------------- //

/// \class UsdGeomCurves
///
/// Base class for BasisCurves and NurbsCurves.  The BasisCurves
/// schema is designed to be analagous to offline renderers' notion
/// of batched curves (such as the classical RIB definition via
/// Basis and Curves statements), while the NurbsCurve schema
/// is designed to be analgous to the NURBS curves found in packages
/// like Maya and Houdini while retaining their consistency with the 
/// RenderMan specification for NURBS Patches.
///
class UsdGeomCurves : public UsdGeomPointBased
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::AbstractTyped;

    /// Construct a UsdGeomCurves on UsdPrim \p prim .
    /// Equivalent to UsdGeomCurves::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomCurves(const UsdPrim& prim=UsdPrim())
        : UsdGeomPointBased(prim)
    {
    }

    /// Construct a UsdGeomCurves on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomCurves(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomCurves(const UsdSchemaBase& schemaObj)
        : UsdGeomPointBased(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomCurves();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomCurves holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomCurves(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomCurves
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
    // --------------------------------------------------------------------- //
    // CURVEVERTEXCOUNTS 
    // --------------------------------------------------------------------- //
    /// Curves-derived primitives can represent multiple distinct,
    /// potentially disconnected curves.  The length of 'curveVertexCounts'
    /// gives the number of such curves, and each element describes the
    /// number of vertices in the corresponding curve
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetCurveVertexCountsAttr() const;

    /// See GetCurveVertexCountsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateCurveVertexCountsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // WIDTHS 
    // --------------------------------------------------------------------- //
    /// Provides width specification for the curves, whose application
    /// will depend on whether the curve is oriented (normals are defined for
    /// it), in which case widths are "ribbon width", or unoriented, in which
    /// case widths are cylinder width.  'widths' is not a generic Primvar,
    /// but the number of elements in this attribute will be determined by
    /// its 'interpolation'.  See \ref SetWidthsInterpolation() .  If 'widths'
    /// and 'primvars:widths' are both specified, the latter has precedence.
    ///
    /// \n  C++ Type: VtArray<float>
    /// \n  Usd Type: SdfValueTypeNames->FloatArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDGEOM_API
    UsdAttribute GetWidthsAttr() const;

    /// See GetWidthsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateWidthsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// Get the \ref Usd_InterpolationVals "interpolation" for the \em widths
    /// attribute.
    ///
    /// Although 'widths' is not classified as a generic UsdGeomPrimvar (and
    /// will not be included in the results of UsdGeomImageable::GetPrimvars() )
    /// it does require an interpolation specification.  The fallback
    /// interpolation, if left unspecified, is UsdGeomTokens->vertex , 
    /// which means a width value is specified at the end of each curve segment.
    USDGEOM_API
    TfToken GetWidthsInterpolation() const;

    /// Set the \ref Usd_InterpolationVals "interpolation" for the \em widths
    /// attribute.
    ///
    /// \return true upon success, false if \p interpolation is not a legal
    /// value as defined by UsdPrimvar::IsValidInterpolation(), or if there 
    /// was a problem setting the value.  No attempt is made to validate
    /// that the widths attr's value contains the right number of elements
    /// to match its interpolation to its prim's topology.
    ///
    /// \sa GetWidthsInterpolation()
    USDGEOM_API
    bool SetWidthsInterpolation(TfToken const &interpolation);

    /// Compute the extent for the curves defined by points and widths.
    ///
    /// \return true upon success, false if unable to calculate extent.
    ///
    /// On success, extent will contain an approximate axis-aligned bounding 
    /// box of the curve defined by points with the given widths.
    ///
    /// This function is to provide easy authoring of extent for usd authoring 
    /// tools, hence it is static and acts outside a specific prim (as in 
    /// attribute based methods).
    USDGEOM_API
    static bool ComputeExtent(const VtVec3fArray& points,
        const VtFloatArray& widths, VtVec3fArray* extent);

    /// \overload
    /// Computes the extent as if the matrix \p transform was first applied.
    USDGEOM_API
    static bool ComputeExtent(const VtVec3fArray& points,
        const VtFloatArray& widths, const GfMatrix4d& transform,
        VtVec3fArray* extent);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
