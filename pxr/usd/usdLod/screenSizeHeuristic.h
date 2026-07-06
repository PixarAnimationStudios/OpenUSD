//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLOD_GENERATED_SCREENSIZEHEURISTIC_H
#define USDLOD_GENERATED_SCREENSIZEHEURISTIC_H

/// \file usdLod/screenSizeHeuristic.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLod/api.h"
#include "pxr/usd/usdLod/heuristic.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLod/tokens.h"

#include "pxr/usd/usdLod/screenSizeHeuristicQuery.h"
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
// LODSCREENSIZEHEURISTIC                                                     //
// -------------------------------------------------------------------------- //

/// \class UsdLodScreenSizeHeuristic
///
/// This LOD heuristic selects LOD children on the basis of the
/// fraction of the viewplane would be covered by the extent.
/// 
/// The LOD child selected is determined by the comparing the computed screen
/// size with the values in thresholds and blendThresholds. Providing values in
/// blendThresholds can allow the renderer to blend between different LOD
/// children which can be useful to avoid flickering between two LODs.
/// Alternatively, hysteresis can be employed to avoid changing the LOD when the
/// distance changes by just a small amount.
/// 
/// The extent and boundingVolume properties describe geometric information for
/// the Root prim that informs the distance calculation. If boundingVolume is
/// authored and targets a prim with a valid extent, the targeted prim's extent
/// should govern the heuristic; otherwise, the extent holds. Both extent and
/// boundingVolume coordinates are assumed to be in the local coordinate system
/// of the LOD root. (Note that this heuristic prim may have a completely
/// different coordinate system than the root does.)
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdLodTokens.
/// So to set an attribute to the value "rightHanded", use UsdLodTokens->rightHanded
/// as the value.
///
class UsdLodScreenSizeHeuristic : public UsdLodHeuristic
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdLodScreenSizeHeuristic on UsdPrim \p prim .
    /// Equivalent to UsdLodScreenSizeHeuristic::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLodScreenSizeHeuristic(const UsdPrim& prim=UsdPrim())
        : UsdLodHeuristic(prim)
    {
    }

    /// Construct a UsdLodScreenSizeHeuristic on the prim held by \p schemaObj .
    /// Should be preferred over UsdLodScreenSizeHeuristic(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLodScreenSizeHeuristic(const UsdSchemaBase& schemaObj)
        : UsdLodHeuristic(schemaObj)
    {
    }

    /// Destructor.
    USDLOD_API
    virtual ~UsdLodScreenSizeHeuristic();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLOD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLodScreenSizeHeuristic holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLodScreenSizeHeuristic(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLOD_API
    static UsdLodScreenSizeHeuristic
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
    static UsdLodScreenSizeHeuristic
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
    // EXTENT 
    // --------------------------------------------------------------------- //
    /// The extent of this LOD Root in local coordinates.
    /// 
    /// This value is used to calculate the screen size from the camera if the
    /// boundingVolume relationship does not target a Boundable prim with a
    /// valid extent.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float3[] extent = [(-1, -1, -1), (1, 1, 1)]` |
    /// | C++ Type | VtArray<GfVec3f> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float3Array |
    USDLOD_API
    UsdAttribute GetExtentAttr() const;

    /// See GetExtentAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLOD_API
    UsdAttribute CreateExtentAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PROJECTIONMETHOD 
    // --------------------------------------------------------------------- //
    /// Defines how screen size is calculated.
    /// 
    /// - projectedExtent: Project the extent to the view plane
    /// - projectedSphere: Project the extent bounding sphere to the view plane
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token projectionMethod = "projectedSphere"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdLodTokens "Allowed Values" | projectedExtent, projectedSphere |
    USDLOD_API
    UsdAttribute GetProjectionMethodAttr() const;

    /// See GetProjectionMethodAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLOD_API
    UsdAttribute CreateProjectionMethodAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // THRESHOLDS 
    // --------------------------------------------------------------------- //
    /// The screen size thresholds for LOD transitions in descending
    /// order as a fraction of the viewport size.
    /// 
    /// For example, [0.25, 0.10, 0.025] means:
    /// - Use LOD 0 when the bounds cover more than 25% of the screen
    /// - Use LOD 1 when the bounds cover 25% or less but more than 10%
    /// - Use LOD 2 when the bounds cover 10% or less but more than 2.5%
    /// - Use LOD 3 when the bounds cover 2.5% or less of the screen
    /// 
    /// This field is advisory and not strongly interoperable.
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
    /// This defines screen size thresholds for LOD transitions in
    /// descending order. The blend threshold is consulted for transitions that
    /// blend or combine multiple LOD items.
    /// 
    /// For example if:
    /// - thresholds is [0.25, 0.10, 0.025]
    /// - blendThresholds is [0.20, 0.08, 0.020]
    /// 
    /// then:
    /// - Use LOD 0 when size > 25%
    /// - Blend LOD 0 and LOD 1 when 25% >= size > 20%
    /// - Use LOD 1 when 20% >= size > 10%
    /// - Blend LOD 1 and LOD 2 when 10% >= size > 8%
    /// - Use LOD 2 when 8% >= size > 2.5%
    /// - Blend LOD 2 and LOD 3 when 2.5 >= size > 2%
    /// - Use LOD 3 when 2% >= size
    /// 
    /// The blend amount should be computed linearly between the corresponding
    /// thresholds and blendThresholds values. See thresholds for complementary
    /// information.
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
    /// bounding volume for this LOD Root, used for size calculation if
    /// the referenced prim has a valid extent.
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

    /// Return a UsdLodScreenSizeHeuristicQuery that matches the current
    /// attribute values of this UsdLodScreenSizeHeuristic.
    ///
    /// The UsdLodScreenSizeHeuristicQuery can be used to perform LOD
    /// calculations in a renderer without referencing back to the original
    /// USD data.
    USDLOD_API
    UsdLodScreenSizeHeuristicQuery CreateScreenSizeHeuristicQuery(
        UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Calculate the screen size given a `frustum` and a `transform`.
    ///
    /// The matrix `transform` should convert from the root's local coordinate
    /// system to the viewpoint's coordinate system. Typically, the viewpoint
    /// will be specified in world coordinates and `transform` will convert
    /// from the root's coordinate system to world coordinates.
    ///
    /// The returned screen size is measured as the projection of the extent (or
    /// its bounding sphere) onto the viewplane and converted to a fraction of
    /// the visible window. The projected shape is not clipped by the viewing
    /// frustum as we do not want the computed size of an extent to change
    /// simply because it is moving off the side of the screen.
    ///
    /// If the extent crosses the near plane, then the calculated size will be
    /// 1.0 (i.e., 100%).  If the extent is entirely in front of the near plane
    /// or entirely behind the far plane (would be clipped by near or far plane
    /// clipping) then the calculated size will be 0.0.
    USDLOD_API
    double ComputeScreenSize(
        const GfFrustum& frustum,
        const GfMatrix4d& transform,
        UsdTimeCode time = UsdTimeCode::Default()) const
    {
        return CreateScreenSizeHeuristicQuery(time)
            .ComputeScreenSize(frustum, transform);
    }

    /// \overload
    /// Calculate the screen size given a `frustum` and a `transform` with
    /// hysteresis.
    ///
    /// The arguments `prevSize` and `hysteresis` should contain the most
    /// recently calculated screen size and a hysteresis threshold respectively.
    /// If the computed screen size differs from `prevSize` by less than
    /// `hysteresis`, then `prevSize` is returned, otherwise the computed
    /// screen size is returned. This helps prevent flickering between LOD
    /// levels when the computed size is very close to a threshold value.
    USDLOD_API
    double ComputeScreenSize(
        const GfFrustum& frustum,
        const GfMatrix4d& transform,
        double prevSize,
        double hysteresis,
        UsdTimeCode time = UsdTimeCode::Default()) const
    {
        return CreateScreenSizeHeuristicQuery(time)
            .ComputeScreenSize(frustum, transform, prevSize, hysteresis);
    }

    /// Calculate an LOD index given a screen size.
    ///
    /// Compare the size against the values in `thresholds` to determine an
    /// LOD index. If `blendThresholds` has values and `size` is between a
    /// `thresholds` value and its corresponding `blendThresholds` value,
    /// then the returned value will be non-integral. If the renderer can blend
    /// between 2 LOD item children then it should use the returned index as
    /// follows:
    ///
    /// \code
    ///    float index = heuristic.ComputeLOD(size);
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
    /// If `blendThresholds` is not set then only integral valued LOD indices
    /// will ever be returned.
    USDLOD_API
    float ComputeLOD(
        double size,
        UsdTimeCode time = UsdTimeCode::Default()) const
    {
        return CreateScreenSizeHeuristicQuery(time)
            .ComputeLOD(size);
    }

    /// \overload
    /// Calculate an LOD index given a `frustum` and a `transform`.
    ///
    /// See \ref ComputeScreenSize
    USDLOD_API
    float ComputeLOD(
        const GfFrustum& frustum,
        const GfMatrix4d& transform,
        UsdTimeCode time = UsdTimeCode::Default()) const
    {
        return CreateScreenSizeHeuristicQuery(time)
            .ComputeLOD(frustum, transform);
    }

    /// \overload
    /// Calculate an LOD index given a `frustum` and a `transform` with
    /// hysteresis.
    ///
    /// The arguments `prevSize` and `hysteresis` should contain the most
    /// recently calculated screen size and a hysteresis threshold respectively.
    /// If the computed screen size differs from `prevSize` by less than
    /// `hysteresis`, then `prevSize` is used, otherwise the computed
    /// screen size is used. This helps prevent flickering between LOD
    /// levels when the computed size is very close to a threshold value.
    /// The size used (either `prevSize` or the newly computed size) will be
    /// returned in `screenSizeOut` so it can be used in next call to
    /// `ComputeLOD`.
    USDLOD_API
    float ComputeLOD(
        const GfFrustum& frustum,
        const GfMatrix4d& transform,
        double prevSize,
        double hysteresis,
        double* screenSizeOut,
        UsdTimeCode time = UsdTimeCode::Default()) const
    {
        return CreateScreenSizeHeuristicQuery(time)
            .ComputeLOD(frustum, transform, prevSize, hysteresis,
                        screenSizeOut);
    }

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
