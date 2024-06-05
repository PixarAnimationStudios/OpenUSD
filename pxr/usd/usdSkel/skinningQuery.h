//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_SKEL_SKINNING_QUERY_H
#define PXR_USD_USD_SKEL_SKINNING_QUERY_H

/// \file usdSkel/skinningQuery.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/usdGeom/primvar.h"

#include "pxr/usd/usdSkel/animMapper.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE


class UsdGeomBoundable;


/// \class UsdSkelSkinningQuery
///
/// Object used for querying resolved bindings for skinning.
class UsdSkelSkinningQuery
{
public:
    USDSKEL_API
    UsdSkelSkinningQuery();

    /// Construct a new skining query for the resolved properties
    /// set through the UsdSkelBindingAPI, as inherited on \p prim.
    /// The resulting query will be marked valid only if the inherited
    /// properties provide proper valid joint influences.
    USDSKEL_API
    UsdSkelSkinningQuery(const UsdPrim& prim,
                         const VtTokenArray& skelJointOrder,
                         const VtTokenArray& blendShapeOrder,
                         const UsdAttribute& jointIndices,
                         const UsdAttribute& jointWeights,
                         const UsdAttribute& skinningMethod,
                         const UsdAttribute& geomBindTransform,
                         const UsdAttribute& joints,
                         const UsdAttribute& blendShapes,
                         const UsdRelationship& blendShapeTargets);

    /// Returns true if this query is valid.
    bool IsValid() const { return bool(_prim); }
    
    /// Boolean conversion operator. Equivalent to IsValid().
    explicit operator bool() const { return IsValid(); }

    const UsdPrim& GetPrim() const { return _prim; }

    /// Returns true if there are blend shapes associated with this prim.    
    USDSKEL_API
    bool HasBlendShapes() const;

    /// Returns true if joint influence data is associated with this prim.
    USDSKEL_API
    bool HasJointInfluences() const;

    /// Returns the number of influences encoded for each component.
    /// If the prim defines rigid joint influences, then this returns
    /// the number of influences that map to every point. Otherwise,
    /// this provides the number of influences per point.
    /// \sa IsRigidlyDeformed
    int GetNumInfluencesPerComponent() const {
        return _numInfluencesPerComponent;
    }

    const TfToken& GetInterpolation() const { return _interpolation; }

    /// Returns true if the held prim has the same joint influences
    /// across all points, or false otherwise.
    USDSKEL_API
    bool IsRigidlyDeformed() const;

    const UsdAttribute& GetSkinningMethodAttr() const {
        return _skinningMethodAttr;
    }

    const UsdAttribute& GetGeomBindTransformAttr() const {
        return _geomBindTransformAttr;
    }

    const UsdGeomPrimvar& GetJointIndicesPrimvar() const {
        return _jointIndicesPrimvar;
    }

    const UsdGeomPrimvar& GetJointWeightsPrimvar() const {
        return _jointWeightsPrimvar;
    }

    const UsdAttribute& GetBlendShapesAttr() const {
        return _blendShapes;
    }

    const UsdRelationship& GetBlendShapeTargetsRel() const {
        return _blendShapeTargets;
    }

    /// Return a mapper for remapping from the joint order of the skeleton
    /// to the local joint order of this prim, if any. Returns a null
    /// pointer if the prim has no custom joint orer.
    /// The mapper maps data from the order given by the \em joints order
    /// on the Skeleton to the order given by the \em skel:joints property,
    /// as optionally set through the UsdSkelBindingAPI.
    const UsdSkelAnimMapperRefPtr& GetJointMapper() const {
        return _jointMapper;
    }

    /// \deprecated Use GetJointMapper.
    const UsdSkelAnimMapperRefPtr& GetMapper() const { return _jointMapper; }


    /// Return the mapper for remapping blend shapes from the order of the
    /// bound SkelAnimation to the local blend shape order of this prim.
    /// Returns a null reference if the underlying prim has no blend shapes.
    /// The mapper maps data from the order given by the \em blendShapes order
    /// on the SkelAnimation to the order given by the \em skel:blendShapes
    /// property, as set through the UsdSkelBindingAPI.
    const UsdSkelAnimMapperRefPtr& GetBlendShapeMapper() const {
        return _blendShapeMapper;
    }

    /// Get the custom joint order for this skinning site, if any.
    USDSKEL_API
    bool GetJointOrder(VtTokenArray* jointOrder) const;

    /// Get the blend shapes for this skinning site, if any.
    USDSKEL_API
    bool GetBlendShapeOrder(VtTokenArray* blendShapes) const;

    /// Populate \p times with the union of time samples for all properties
    /// that affect skinning, independent of joint transforms and any
    /// other prim-specific properties (such as points).
    ///
    /// \sa UsdAttribute::GetTimeSamples
    USDSKEL_API
    bool GetTimeSamples(std::vector<double>* times) const;

    /// Populate \p times with the union of time samples within \p interval,
    /// for all properties that affect skinning, independent of joint   
    /// transforms and any other prim-specific properties (such as points).
    ///
    /// \sa UsdAttribute::GetTimeSamplesInInterval
    USDSKEL_API
    bool GetTimeSamplesInInterval(const GfInterval& interval,
                                  std::vector<double>* times) const;

    /// Convenience method for computing joint influences.
    /// In addition to querying influences, this will also perform
    /// validation of the basic form of the weight data -- although
    /// the array contents is not validated.
    USDSKEL_API
    bool ComputeJointInfluences(VtIntArray* indices,
                                VtFloatArray* weights,
                                UsdTimeCode time=UsdTimeCode::Default()) const;

    /// Convenience method for computing joint influence, where constant
    /// influences are expanded to hold values per point.
    /// In addition to querying influences, this will also perform
    /// validation of the basic form of the weight data -- although
    /// the array contents is not validated.
    USDSKEL_API
    bool ComputeVaryingJointInfluences(
             size_t numPoints,
             VtIntArray* indices,
             VtFloatArray* weights,
             UsdTimeCode time=UsdTimeCode::Default()) const;

    /// Compute skinned points using specified skinning method attr
    /// (fallback to linear blend skinning if not specified)
    /// Both \p xforms and \p points are given in _skeleton space_,
    /// using the joint order of the bound skeleton.
    /// Joint influences and the (optional) binding transform are computed
    /// at time \p time (which will typically be unvarying).
    ///
    /// \sa UsdSkelSkeletonQuery::ComputeSkinningTransforms
    template <typename Matrix4>
    USDSKEL_API
    bool ComputeSkinnedPoints(const VtArray<Matrix4>& xforms,
                              VtVec3fArray* points,
                              UsdTimeCode time=UsdTimeCode::Default()) const;

    /// Compute skinned normals using specified skinning method attr
    /// (fallback to linear blend skinning if not specified)
    /// Both \p xforms and \p points are given in _skeleton space_,
    /// using the joint order of the bound skeleton.
    /// Joint influences and the (optional) binding transform are computed
    /// at time \p time (which will typically be unvarying).
    ///
    /// \sa UsdSkelSkeletonQuery::ComputeSkinningTransforms
    template <typename Matrix4>
    USDSKEL_API
    bool ComputeSkinnedNormals(const VtArray<Matrix4>& xforms,
                              VtVec3fArray* points,
                              UsdTimeCode time=UsdTimeCode::Default()) const;

    /// Compute a skinning transform using specified skinning method attr
    /// (fallback to linear blend skinning if not specified)
    /// The \p xforms are given in _skeleton space_, using the joint order of
    /// the bound skeleton.
    /// Joint influences and the (optional) binding transform are computed
    /// at time \p time (which will typically be unvarying).
    /// If this skinning query holds non-constant joint influences,
    /// no transform will be computed, and the function will return false.
    ///
    /// \sa UsdSkelSkeletonQuery::ComputeSkinningTransforms
    template <typename Matrix4>
    USDSKEL_API
    bool ComputeSkinnedTransform(const VtArray<Matrix4>& xforms,
                                 Matrix4* xform,
                                 UsdTimeCode time=UsdTimeCode::Default()) const;

    /// Helper for computing an *approximate* padding for use in extents
    /// computations. The padding is computed as the difference between the
    /// pivots of the \p skelRestXforms -- _skeleton space_ joint transforms
    /// at rest -- and the extents of the skinned primitive.
    /// This is intended to provide a suitable, constant metric for padding
    /// joint extents as computed by UsdSkelComputeJointsExtent.
    template <typename Matrix4>
    USDSKEL_API
    float ComputeExtentsPadding(const VtArray<Matrix4>& skelRestXforms,
                                const UsdGeomBoundable& boundable) const;

    USDSKEL_API
    TfToken GetSkinningMethod() const;

    USDSKEL_API
    GfMatrix4d
    GetGeomBindTransform(UsdTimeCode time=UsdTimeCode::Default()) const;

    USDSKEL_API
    std::string GetDescription() const;

private:

    void _InitializeJointInfluenceBindings(
             const UsdAttribute& jointIndices,
             const UsdAttribute& jointWeights);

    void _InitializeBlendShapeBindings(
             const UsdAttribute& blendShapes,
             const UsdRelationship& blendShapeTargets);

    UsdPrim _prim;
    int _numInfluencesPerComponent = 1;
    int _flags = 0;
    TfToken _interpolation;

    UsdGeomPrimvar _jointIndicesPrimvar;
    UsdGeomPrimvar _jointWeightsPrimvar;
    UsdAttribute _skinningMethodAttr;
    UsdAttribute _geomBindTransformAttr;
    UsdAttribute _blendShapes;
    UsdRelationship _blendShapeTargets;
    UsdSkelAnimMapperRefPtr _jointMapper;
    UsdSkelAnimMapperRefPtr _blendShapeMapper;
    std::optional<VtTokenArray> _jointOrder;
    std::optional<VtTokenArray> _blendShapeOrder;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_SKEL_SKINNING_QUERY_H
