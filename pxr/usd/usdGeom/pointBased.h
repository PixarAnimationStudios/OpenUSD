//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDGEOM_GENERATED_POINTBASED_H
#define USDGEOM_GENERATED_POINTBASED_H

/// \file usdGeom/pointBased.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
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

PXR_NAMESPACE_OPEN_SCOPE

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
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::AbstractTyped;

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
    USDGEOM_API
    virtual ~UsdGeomPointBased();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
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
    USDGEOM_API
    static UsdGeomPointBased
    Get(const UsdStagePtr &stage, const SdfPath &path);


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
    // POINTS 
    // --------------------------------------------------------------------- //
    /// The primary geometry attribute for all PointBased
    /// primitives, describes points in (local) space.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `point3f[] points` |
    /// | C++ Type | VtArray<GfVec3f> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Point3fArray |
    USDGEOM_API
    UsdAttribute GetPointsAttr() const;

    /// See GetPointsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreatePointsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VELOCITIES 
    // --------------------------------------------------------------------- //
    /// If provided, 'velocities' should be used by renderers to 
    /// 
    /// compute positions between samples for the 'points' attribute, rather
    /// than interpolating between neighboring 'points' samples.  This is the
    /// only reasonable means of computing motion blur for topologically
    /// varying PointBased primitives.  It follows that the length of each
    /// 'velocities' sample must match the length of the corresponding
    /// 'points' sample.  Velocity is measured in position units per second,
    /// as per most simulation software. To convert to position units per
    /// UsdTimeCode, divide by UsdStage::GetTimeCodesPerSecond().
    /// 
    /// See also \ref UsdGeom_VelocityInterpolation .
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `vector3f[] velocities` |
    /// | C++ Type | VtArray<GfVec3f> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Vector3fArray |
    USDGEOM_API
    UsdAttribute GetVelocitiesAttr() const;

    /// See GetVelocitiesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateVelocitiesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ACCELERATIONS 
    // --------------------------------------------------------------------- //
    /// If provided, 'accelerations' should be used with
    /// velocities to compute positions between samples for the 'points'
    /// attribute rather than interpolating between neighboring 'points'
    /// samples. Acceleration is measured in position units per second-squared.
    /// To convert to position units per squared UsdTimeCode, divide by the
    /// square of UsdStage::GetTimeCodesPerSecond().
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `vector3f[] accelerations` |
    /// | C++ Type | VtArray<GfVec3f> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Vector3fArray |
    USDGEOM_API
    UsdAttribute GetAccelerationsAttr() const;

    /// See GetAccelerationsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateAccelerationsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // NORMALS 
    // --------------------------------------------------------------------- //
    /// Provide an object-space orientation for individual points, 
    /// which, depending on subclass, may define a surface, curve, or free 
    /// points.  Note that 'normals' should not be authored on any Mesh that
    /// is subdivided, since the subdivision algorithm will define its own
    /// normals. 'normals' is not a generic primvar, but the number of elements
    /// in this attribute will be determined by its 'interpolation'.  See
    /// \ref SetNormalsInterpolation() . If 'normals' and 'primvars:normals'
    /// are both specified, the latter has precedence.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `normal3f[] normals` |
    /// | C++ Type | VtArray<GfVec3f> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Normal3fArray |
    USDGEOM_API
    UsdAttribute GetNormalsAttr() const;

    /// See GetNormalsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDGEOM_API
    UsdAttribute CreateNormalsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
    
    /// Get the \ref Usd_InterpolationVals "interpolation" for the \em normals
    /// attribute.
    ///
    /// Although 'normals' is not classified as a generic UsdGeomPrimvar (and
    /// will not be included in the results of UsdGeomPrimvarsAPI::GetPrimvars() )
    /// it does require an interpolation specification.  The fallback
    /// interpolation, if left unspecified, is UsdGeomTokens->vertex , 
    /// which will generally produce smooth shading on a polygonal mesh.
    /// To achieve partial or fully faceted shading of a polygonal mesh
    /// with normals, one should use UsdGeomTokens->faceVarying or
    /// UsdGeomTokens->uniform interpolation.
    USDGEOM_API
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
    USDGEOM_API
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
    USDGEOM_API
    static bool ComputeExtent(const VtVec3fArray& points, VtVec3fArray* extent);

    /// \overload
    /// Computes the extent as if the matrix \p transform was first applied.
    USDGEOM_API
    static bool ComputeExtent(const VtVec3fArray& points,
        const GfMatrix4d& transform, VtVec3fArray* extent);

public:
    /// Compute points given the positions, velocities and accelerations
    /// at \p time. 
    ///
    /// This will return \c false and leave \p points untouched if:
    /// - \p points is NULL
    /// - one of \p time and \p baseTime is numeric and the other is
    ///   UsdTimeCode::Default() (they must either both be numeric or both be
    ///   default)
    /// - there is no authored points attribute
    ///
    /// If there is no error, we will return \c true and \p points will contain
    /// the computed points.
    /// 
    /// \param points - the out parameter for the new points.  Its size
    ///                 will depend on the authored data.
    /// \param time - UsdTimeCode at which we want to evaluate the transforms
    /// \param baseTime - required for correct interpolation between samples
    ///                   when \em velocities or \em accelerations are
    ///                   present. If there are samples for \em positions and
    ///                   \em velocities at t1 and t2, normal value resolution
    ///                   would attempt to interpolate between the two samples,
    ///                   and if they could not be interpolated because they
    ///                   differ in size (common in cases where velocity is
    ///                   authored), will choose the sample at t1.  When
    ///                   sampling for the purposes of motion-blur, for example,
    ///                   it is common, when rendering the frame at t2, to 
    ///                   sample at [ t2-shutter/2, t2+shutter/2 ] for a
    ///                   shutter interval of \em shutter.  The first sample
    ///                   falls between t1 and t2, but we must sample at t2
    ///                   and apply velocity-based interpolation based on those
    ///                   samples to get a correct result.  In such scenarios,
    ///                   one should provide a \p baseTime of t2 when querying
    ///                   \em both samples. If your application does not care
    ///                   about off-sample interpolation, it can supply the
    ///                   same value for \p baseTime that it does for \p time.
    ///                   When \p baseTime is less than or equal to \p time,
    ///                   we will choose the lower bracketing timeSample.
    USDGEOM_API
    bool
    ComputePointsAtTime(
        VtArray<GfVec3f>* points,
        const UsdTimeCode time,
        const UsdTimeCode baseTime) const;

    /// Compute points as in ComputePointsAtTime, but using multiple sample times. An
    /// array of vector arrays is returned where each vector array contains the
    /// points for the corresponding time in \p times .
    ///
    /// \param times - A vector containing the UsdTimeCodes at which we want to
    ///                sample.
    USDGEOM_API
    bool
    ComputePointsAtTimes(
        std::vector<VtArray<GfVec3f>>* pointsArray,
        const std::vector<UsdTimeCode>& times,
        const UsdTimeCode baseTime) const;

    /// \overload
    /// Perform the point computation. This does the same computation as
    /// the non-static ComputePointsAtTime method, but takes all
    /// data as parameters rather than accessing authored data.
    ///
    /// \param points - the out parameter for the computed points. Its size
    ///                 will depend on the given data.
    /// \param stage - the UsdStage
    /// \param time - time at which we want to evaluate the transforms
    /// \param positions - array containing all current points.
    /// \param velocities - array containing all velocities. This array
    ///                     must be either the same size as \p positions or
    ///                     empty. If it is empty, points are computed as if
    ///                     all velocities were zero in all dimensions.
    /// \param velocitiesSampleTime - time at which the samples from
    ///                               \p velocities were taken.
    /// \param accelerations - array containing all accelerations. 
    ///                     This array must be either the same size as 
    ///                     \p positions or empty. If it is empty, points
    ///                     are computed as if all accelerations were zero in 
    ///                     all dimensions.
    /// \param velocityScale - \deprecated
    USDGEOM_API
    static bool
    ComputePointsAtTime(
        VtArray<GfVec3f>* points,
        UsdStageWeakPtr& stage,
        UsdTimeCode time,
        const VtVec3fArray& positions,
        const VtVec3fArray& velocities,
        UsdTimeCode velocitiesSampleTime,
        const VtVec3fArray& accelerations,
        float velocityScale=1.0);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
