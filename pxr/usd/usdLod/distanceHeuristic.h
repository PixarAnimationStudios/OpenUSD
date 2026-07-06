//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLOD_GENERATED_DISTANCEHEURISTIC_H
#define USDLOD_GENERATED_DISTANCEHEURISTIC_H

/// \file usdLod/distanceHeuristic.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLod/api.h"
#include "pxr/usd/usdLod/heuristic.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLod/tokens.h"

#include "pxr/usd/usdLod/distanceHeuristicQuery.h"
#include "pxr/usd/usdGeom/boundable.h"


#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// LODDISTANCEHEURISTIC                                                       //
// -------------------------------------------------------------------------- //

/// \class UsdLodDistanceHeuristic
///
/// This LOD heuristic selects LOD children on the basis of distance
/// from the view point to the LOD root.
/// 
/// The LOD child selected is determined by the comparing the computed
/// distance with the values in thresholds and blendThresholds. Providing
/// values in blendThresholds can allow the renderer to blend between
/// different LOD children which can be useful to avoid flickering
/// between two LODs. Alternatively, hysteresis can be employed to avoid
/// changing the LOD when the distance changes by a small amount.
/// 
/// The center and boundingVolume properties describe geometric information for
/// the Root prim that informs the distance calculation. If boundingVolume is
/// authored and has a valid extent, the distance to the nearest point on the
/// extent box is used; otherwise the distance to the center is used. Both
/// center and boundingVolume coordinates are assumed to be in the local
/// coordinate system of the LOD root. (Note that this heuristic prim may have
/// a completely different coordinate system than that of the root.)
/// 
///
class UsdLodDistanceHeuristic : public UsdLodHeuristic
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdLodDistanceHeuristic on UsdPrim \p prim .
    /// Equivalent to UsdLodDistanceHeuristic::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLodDistanceHeuristic(const UsdPrim& prim=UsdPrim())
        : UsdLodHeuristic(prim)
    {
    }

    /// Construct a UsdLodDistanceHeuristic on the prim held by \p schemaObj .
    /// Should be preferred over UsdLodDistanceHeuristic(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLodDistanceHeuristic(const UsdSchemaBase& schemaObj)
        : UsdLodHeuristic(schemaObj)
    {
    }

    /// Destructor.
    USDLOD_API
    virtual ~UsdLodDistanceHeuristic();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLOD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLodDistanceHeuristic holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLodDistanceHeuristic(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLOD_API
    static UsdLodDistanceHeuristic
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
    USDLOD_API
    static UsdLodDistanceHeuristic
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDLOD_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDLOD_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDLOD_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // CENTER 
    // --------------------------------------------------------------------- //
    /// The center point of this LOD Root in local coordinates.
    /// 
    /// This value is used to calculate the distance from the view point if the
    /// boundingVolume relationship does not target a Boundable prim.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `point3f center = (0, 0, 0)` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Point3f |
    USDLOD_API
    UsdAttribute GetCenterAttr() const;

    /// See GetCenterAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLOD_API
    UsdAttribute CreateCenterAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // THRESHOLDS 
    // --------------------------------------------------------------------- //
    /// This defines the distance thresholds for LOD transitions in
    /// ascending order.
    /// 
    /// For example, [10.0, 50.0, 100.0] means:
    /// - Use LOD 0 when distance < 10.0
    /// - Use LOD 1 when 10.0 <= distance < 50.0
    /// - Use LOD 2 when 50.0 <= distance < 100.0
    /// - Use LOD 3 when 100.0 <= distance
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float[] thresholds = []` |
    /// | C++ Type | VtArray<float> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->FloatArray |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDLOD_API
    UsdAttribute GetThresholdsAttr() const;

    /// See GetThresholdsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLOD_API
    UsdAttribute CreateThresholdsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // BLENDTHRESHOLDS 
    // --------------------------------------------------------------------- //
    /// This defines distance thresholds for LOD transitions in
    /// ascending order. If the blend threshold values are defined, then the
    /// calculation can return a result between two levels of detail that can be
    /// used to blend or combine multiple LOD items.
    /// 
    /// For example if:
    /// - thresholds is [10.0, 50.0, 100.0]
    /// - blendThresholds is [11.0, 55.0, 110.0]
    /// 
    /// then:
    /// - Use LOD 0 when distance < 10.0
    /// - Blend LOD 0 and LOD 1 when 10.0 <= distance < 11.0
    /// - Use LOD 1 when 11.0 <= distance < 50.0
    /// - Blend LOD 1 and LOD 2 when 50 <= distance < 55.0
    /// - Use LOD 2 when 55.0 <= distance < 100.0
    /// - Blend LOD 2 and LOD 3 when 100 <= distance < 110.0
    /// - Use LOD 3 when 110.0 <= distance
    /// 
    /// The blend amount should be computed linearly between the corresponding
    /// thresholds and blendThresholds values. See thresholds for additional
    /// information. Note that blendThresholds values are clamped internally to
    /// the current and next thresholds values, ensuring that blending never
    /// occurs between more than 2 LOD children. Missing blendThresholds
    /// values result in no blending, just a discontinuous jump from one LOD
    /// child to the next.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float[] blendThresholds = []` |
    /// | C++ Type | VtArray<float> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->FloatArray |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDLOD_API
    UsdAttribute GetBlendThresholdsAttr() const;

    /// See GetBlendThresholdsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLOD_API
    UsdAttribute CreateBlendThresholdsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // BOUNDINGVOLUME 
    // --------------------------------------------------------------------- //
    /// Optional relationship to a Boundable prim that defines the
    /// bounding volume for this LOD Root, the prim's extent will be used for
    /// distance calculation if it is valid (not empty). The distance will be
    /// calculated from the viewpoint to the nearest point on the extent box.
    /// 
    ///
    USDLOD_API
    UsdRelationship GetBoundingVolumeRel() const;

    /// See GetBoundingVolumeRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDLOD_API
    UsdRelationship CreateBoundingVolumeRel() const;

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

    /// Return a UsdLodDistanceHeuristicQuery that matches the current attribute
    /// values of this UsdLodDistanceHeuristic.
    ///
    /// The UsdLodDistanceHeuristicQuery can be used to perform LOD calculations
    /// in a renderer without referencing back to the original USD data.
    USDLOD_API
    UsdLodDistanceHeuristicQuery CreateDistanceHeuristicQuery(
        UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Calculate a distance given a `viewpoint` and a `transform`.
    ///
    /// The matrix `transform` should convert from the root's local coordinate
    /// space to `viewpoint`'s coordinate space.  Typically, `viewpoint`
    /// will be specified in world coordinates and `transform` will convert
    /// from the root's object coordinates to world coordinates.
    ///
    /// If `transform` is close to singular then any extent is ignored and
    /// the distance is calculated from `viewpoint` to `center`.
    ///
    /// \see UsdLodDistanceHeuristic::ComputeLOD
    USDLOD_API
    double ComputeDistance(
        const GfVec3d& viewpoint,
        const GfMatrix4d& transform,
        UsdTimeCode time = UsdTimeCode::Default()) const
    {
        return CreateDistanceHeuristicQuery(time)
            .ComputeDistance(viewpoint, transform);
    }

    /// \overload
    /// Calculate a distance given a `viewpoint` and a `transform` with
    /// hysteresis.
    ///
    /// The arguments `prevDistance` and `hysteresis` should contain the most
    /// recently calculated distance and a hysteresis threshold respectively.
    /// If the actual distance is within `hysteresis` distance of `prevDistance`
    /// then `prevDistance` is returned. If the actual distance is outside the
    /// hysteresis window then the returned value lags behind the actual value
    /// by `hysteresis`. Specifically, if the actual distance is greater than
    /// `prevDistance + hysteresis` then the calculated distance - `hysteresis`
    /// is returned and if it is less than `prevDistance - hysteresis` then the
    /// calculated distance + `hysteresis` is returned. This helps prevent
    /// flickering between LOD levels when the computed size is very close to a
    /// threshold value.
    ///
    /// \see UsdLodDistanceHeuristicQuery::ComputeLOD
    USDLOD_API
    double ComputeDistance(
        const GfVec3d& viewpoint,
        const GfMatrix4d& transform,
        double prevDistance,
        double hysteresis,
        UsdTimeCode time = UsdTimeCode::Default()) const
    {
        return CreateDistanceHeuristicQuery(time)
            .ComputeDistance(viewpoint, transform, prevDistance, hysteresis);
    }

    /// Calculate an LOD index given a `distance`.
    ///
    /// Compare the distance against the values in `thresholds` to determine an
    /// LOD index. If `blendThresholds` has values and `distance` is between a
    /// `thresholds` value and its corresponding `blendThresholds` value, then
    /// the returned value will be non-integral. If the renderer can blend
    /// between 2 LOD item children then it should use the returned index as
    /// follows:
    ///
    /// \code
    ///    float index = heuristic.ComputeLOD(distance);
    ///    if (blending_between_LOD_levels_is_active) {
    ///        int lowIndex = int(std::floor(index));
    ///        int highIndex = int(std::ceil(index));
    ///        float alpha = index - lowIndex;
    ///
    ///        // Display a linear interpolation using:
    ///        //   (1-alpha) * item[lowIndex] + alpha * item[highIndex]
    ///    } else {
    ///        int index = int(std::round(index))
    ///
    ///        // Display item[index]
    ///    }
    /// \endcode
    ///
    /// If `blendThresholds` is not set then only integer valued results will
    /// ever be returned.
    ///
    /// \see UsdLodDistanceHeuristic::ComputeDistance
    USDLOD_API
    float ComputeLOD(
        double distance,
        UsdTimeCode time = UsdTimeCode::Default()) const
    {
        return CreateDistanceHeuristicQuery(time)
            .ComputeLOD(distance);
    }

    /// \overload
    /// Compute an LOD index given a `viewpoint` and a `transform`.
    ///
    /// Compute the distance given `viewpoint` and `transform`, then compute the
    /// LOD index from that distance.
    USDLOD_API
    float ComputeLOD(
        const GfVec3d& viewpoint,
        const GfMatrix4d& transform,
        UsdTimeCode time = UsdTimeCode::Default()) const
    {
        return CreateDistanceHeuristicQuery(time)
            .ComputeLOD(viewpoint, transform);
    }

    /// \overload

    /// Compute an LOD index given a `viewpoint`, a `transform`, a
    /// `prevDistance`, and a `hysteresis` value.
    ///
    /// Compute the distance given the input arguments, then compute the LOD
    /// index from that distance. The computed distance is stored in the double
    /// pointed to by distanceOut.
    ///
    /// The computed distance will be stored in `distanceOut` so it can be used
    /// in the next call to `ComputeLOD` with hysteresis.
    ///
    /// \see ComputeDistance for details on the hysteresis calculation.
    USDLOD_API
    float ComputeLOD(
        const GfVec3d& viewpoint,
        const GfMatrix4d& transform,
        double prevDistance,
        double hysteresis,
        double* distanceOut,
        UsdTimeCode time = UsdTimeCode::Default()) const
    {
        return CreateDistanceHeuristicQuery(time)
            .ComputeLOD(viewpoint, transform, prevDistance, hysteresis,
                        distanceOut);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
