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
#ifndef USDGEOM_GENERATED_POINTBASED_H
#define USDGEOM_GENERATED_POINTBASED_H

/// \file usdGeom/pointBased.h

#include "pxr/usd/usdGeom/gprim.h"
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
// POINTBASED                                                                 //
// -------------------------------------------------------------------------- //

/// \class UsdGeomPointBased
///
/// Base class for all UsdGeomGprims that possess points,
/// providing common attributes such as normals and velocities.
///
class UsdGeomPointBased : public UsdGeomGprim
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Construct a UsdGeomPointBased on UsdPrim \p prim .
    /// Equivalent to UsdGeomPointBased::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomPointBased(const UsdPrim& prim=UsdPrim())
        : UsdGeomGprim(prim)
    {
    }

    /// Construct a UsdGeomPointBased on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomPointBased(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomPointBased(const UsdSchemaBase& schemaObj)
        : UsdGeomGprim(schemaObj)
    {
    }

    /// Destructor.
    virtual ~UsdGeomPointBased();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomPointBased holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomPointBased(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    static UsdGeomPointBased
    Get(const UsdStagePtr &stage, const SdfPath &path);


private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // POINTS 
    // --------------------------------------------------------------------- //
    /// The primary geometry attribute for all PointBased
    /// primitives, describes points in (local) space.
    ///
    /// \n  C++ Type: VtArray<GfVec3f>
    /// \n  Usd Type: SdfValueTypeNames->Point3fArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetPointsAttr() const;

    /// See GetPointsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreatePointsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VELOCITIES 
    // --------------------------------------------------------------------- //
    /// If provided, 'velocities' should be used by renderers to 
    /// compute motion blur for a given 'points' sample, rather than 
    /// interpolating to a neighboring 'points' sample.  This is the only
    /// reasonable means of specifying motion blur for topologically
    /// varying PointBased primitives.  It follows that the length of each
    /// 'velocities' sample must match the length of the corresponding
    /// 'points' sample.
    ///
    /// \n  C++ Type: VtArray<GfVec3f>
    /// \n  Usd Type: SdfValueTypeNames->Vector3fArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetVelocitiesAttr() const;

    /// See GetVelocitiesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateVelocitiesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // NORMALS 
    // --------------------------------------------------------------------- //
    /// Provide orientation for individual points, which, depending on
    /// subclass, may define a surface, curve, or free points.  Note that in
    /// general you should not need or want to provide 'normals' for any
    /// Mesh that is subdivided, as the subdivision scheme will provide smooth
    /// normals.  'normals' is not a generic Primvar,
    /// but the number of elements in this attribute will be determined by
    /// its 'interpolation'.  See \ref SetNormalsInterpolation()
    ///
    /// \n  C++ Type: VtArray<GfVec3f>
    /// \n  Usd Type: SdfValueTypeNames->Normal3fArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetNormalsAttr() const;

    /// See GetNormalsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateNormalsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to close the class declaration with }; and complete the
    // include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--
    
    /// Get the \ref Usd_InterpolationVals "interpolation" for the \em normals
    /// attribute.
    ///
    /// Although 'normals' is not classified as a generic UsdGeomPrimvar (and
    /// will not be included in the results of UsdGeomImageable::GetPrimvars() )
    /// it does require an interpolation specification.  The fallback
    /// interpolation, if left unspecified, is UsdGeomTokens->varying , 
    /// which will generally produce smooth shading on a polygonal mesh.
    /// To achieve partial or fully faceted shading of a polygonal mesh
    /// with normals, one should use UsdGeomTokens->faceVarying or
    /// UsdGeomTokens->uniform interpolation.
    TfToken GetNormalsInterpolation() const;

    /// Set the \ref Usd_InterpolationVals "interpolation" for the \em normals
    /// attribute.
    ///
    /// \return true upon success, false if \p interpolation is not a legal
    /// value as defined by UsdGeomPrimvar::IsValidInterpolation(), or if there 
    /// was a problem setting the value.  No attempt is made to validate
    /// that the normals attr's value contains the right number of elements
    /// to match its interpolation to its prim's topology.
    ///
    /// \sa GetNormalsInterpolation()
    bool SetNormalsInterpolation(TfToken const &interpolation);

    /// Compute the extent for the point cloud defined by points. 
    ///
    /// \return true on success, false if extents was unable to be calculated.
    /// 
    /// On success, extent will contain the axis-aligned bounding box of the
    /// point cloud defined by points.
    ///
    /// This function is to provide easy authoring of extent for usd authoring
    /// tools, hence it is static and acts outside a specific prim (as in 
    /// attribute based methods). 
    static bool ComputeExtent(const VtVec3fArray& points, VtVec3fArray* extent);
};

#endif
