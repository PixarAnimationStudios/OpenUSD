//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_GEOM_BBOX_CACHE_H
#define PXR_USD_USD_GEOM_BBOX_CACHE_H

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdGeom/pointInstancer.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/work/dispatcher.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE


class UsdGeomModelAPI;

/// \class UsdGeomBBoxCache
///
/// Caches bounds by recursively computing and aggregating bounds of children in
/// world space and aggregating the result back into local space.
///
/// The cache is configured for a specific time and
/// \ref UsdGeomImageable::GetPurposeAttr() set of purposes. When querying a
/// bound, transforms and extents are read either from the time specified or
/// UsdTimeCode::Default(), following \ref Usd_ValueResolution standard
/// time-sample value resolution.  As noted in SetIncludedPurposes(), changing
/// the included purposes does not invalidate the cache, because we cache
/// purpose along with the geometric data.
///
/// Child prims that are invisible at the requested time are excluded when
/// computing a prim's bounds.  However, if a bound is requested directly for an
/// excluded prim, it will be computed. Additionally, only prims deriving from
/// UsdGeomImageable are included in child bounds computations.
///
/// Unlike standard UsdStage traversals, the traversal performed by the
/// UsdGeomBBoxCache includes prims that are unloaded (see UsdPrim::IsLoaded()).
/// This makes it possible to fetch bounds for a UsdStage that has been opened
/// without \em forcePopulate , provided the unloaded model prims have authored
/// extent hints (see UsdGeomModelAPI::GetExtentsHint()).
///
/// This class is optimized for computing tight <b>untransformed "object"
/// space</b> bounds for component-models.  In the absence of component models,
/// bounds are optimized for world-space, since there is no other easily
/// identifiable space for which to optimize, and we cannot optimize for every
/// prim's local space without performing quadratic work.
///
/// The TfDebug flag, USDGEOM_BBOX, is provided for debugging.
///
/// Warnings:
///  * This class should only be used with valid UsdPrim objects.
///
///  * This cache does not listen for change notifications; the user is
///    responsible for clearing the cache when changes occur.
///
///  * Thread safety: instances of this class may not be used concurrently.
///
///  * Plugins may be loaded in order to compute extents for prim types provided
///    by that plugin.  See UsdGeomBoundable::ComputeExtentFromPlugins
///
class UsdGeomBBoxCache
{
public:
    /// Construct a new BBoxCache for a specific \p time and set of
    /// \p includedPurposes.
    ///
    /// Only prims with a purpose that matches the \p includedPurposes will be
    /// considered when accumulating child bounds. See UsdGeomImageable for
    /// allowed purpose values.
    ///
    /// If \p useExtentsHint is true, then when computing the bounds for any
    /// model-root prim, if the prim is visible at \p time, we will fetch its
    /// extents hint (via UsdGeomModelAPI::GetExtentsHint()). If it is authored,
    /// we use it to compute the bounding box for the selected combination of
    /// includedPurposes by combining bounding box hints that have been cached
    /// for various values of purposes.
    ///
    /// If \p ignoreVisibility is true invisible prims will be included during
    /// bounds computations.
    ///
    USDGEOM_API
    UsdGeomBBoxCache(UsdTimeCode time, TfTokenVector includedPurposes,
                     bool useExtentsHint=false, bool ignoreVisibility=false);

    /// Copy constructor.
    USDGEOM_API
    UsdGeomBBoxCache(UsdGeomBBoxCache const &other);
     
    /// Copy assignment.
    USDGEOM_API
    UsdGeomBBoxCache &operator=(UsdGeomBBoxCache const &other);

    /// Compute the bound of the given prim in world space, leveraging any
    /// pre-existing, cached bounds.
    ///
    /// The bound of the prim is computed, including the transform (if any)
    /// authored on the node itself, and then transformed to world space.
    ///
    /// Error handling note: No checking of \p prim validity is performed.  If
    /// \p prim is invalid, this method will abort the program; therefore it is
    /// the client's responsibility to ensure \p prim is valid.
    USDGEOM_API
    GfBBox3d ComputeWorldBound(const UsdPrim& prim);

    /// Computes the bound of the prim's descendents in world space while
    /// excluding the subtrees rooted at the paths in \p pathsToSkip.
    ///
    /// Additionally, the parameter \p primOverride overrides the local-to-world
    /// transform of the prim and \p ctmOverrides is used to specify overrides
    /// the local-to-world transforms of certain paths underneath the prim.
    ///
    /// This leverages any pre-existing, cached bounds, but does not include the
    /// transform (if any) authored on the prim itself.
    ///
    /// See ComputeWorldBound() for notes on performance and error handling.
    USDGEOM_API
    GfBBox3d ComputeWorldBoundWithOverrides(
        const UsdPrim &prim,
        const SdfPathSet &pathsToSkip,
        const GfMatrix4d &primOverride,
        const TfHashMap<SdfPath, GfMatrix4d, SdfPath::Hash> &ctmOverrides);


    /// Compute the bound of the given prim in the space of an ancestor prim,
    /// \p relativeToAncestorPrim, leveraging any pre-existing cached bounds.
    ///
    /// The computed bound excludes the local transform at
    /// \p relativeToAncestorPrim. The computed bound may be incorrect if
    /// \p relativeToAncestorPrim is not an ancestor of \p prim.
    ///
    USDGEOM_API
    GfBBox3d ComputeRelativeBound(const UsdPrim &prim,
                                  const UsdPrim &relativeToAncestorPrim);

    /// Computes the oriented bounding box of the given prim, leveraging any
    /// pre-existing, cached bounds.
    ///
    /// The computed bound includes the transform authored on the prim itself,
    /// but does not include any ancestor transforms (it does not include the
    /// local-to-world transform).
    ///
    /// See ComputeWorldBound() for notes on performance and error handling.
    USDGEOM_API
    GfBBox3d ComputeLocalBound(const UsdPrim& prim);

    /// Computes the bound of the prim's children leveraging any pre-existing,
    /// cached bounds, but does not include the transform (if any) authored on
    /// the prim itself.
    ///
    /// \b IMPORTANT: while the BBox does not contain the local transformation,
    /// in general it may still contain a non-identity transformation matrix to
    /// put the bounds in the correct space. Therefore, to obtain the correct
    /// axis-aligned bounding box, the client must call ComputeAlignedRange().
    ///
    /// See ComputeWorldBound() for notes on performance and error handling.
    USDGEOM_API
    GfBBox3d ComputeUntransformedBound(const UsdPrim& prim);

    /// \overload
    /// Computes the bound of the prim's descendents while excluding the
    /// subtrees rooted at the paths in \p pathsToSkip. Additionally, the
    /// parameter \p ctmOverrides is used to specify overrides to the CTM values
    /// of certain paths underneath the prim. The CTM values in the
    /// \p ctmOverrides map are in the space of the given prim, \p prim.
    ///
    /// This leverages any pre-existing, cached bounds, but does not include the
    /// transform (if any) authored on the prim itself.
    ///
    /// \b IMPORTANT: while the BBox does not contain the local transformation,
    /// in general it may still contain a non-identity transformation matrix to
    /// put the bounds in the correct space. Therefore, to obtain the correct
    /// axis-aligned bounding box, the client must call ComputeAlignedRange().
    ///
    /// See ComputeWorldBound() for notes on performance and error handling.
    USDGEOM_API
    GfBBox3d ComputeUntransformedBound(
        const UsdPrim &prim,
        const SdfPathSet &pathsToSkip,
        const TfHashMap<SdfPath, GfMatrix4d, SdfPath::Hash> &ctmOverrides);

    /// Compute the bound of the given point instances in world space.
    ///
    /// The bounds of each instance is computed and then transformed to world
    /// space.  The \p result pointer must point to \p numIds GfBBox3d instances
    /// to be filled.
    USDGEOM_API
    bool
    ComputePointInstanceWorldBounds(
        const UsdGeomPointInstancer& instancer,
        int64_t const *instanceIdBegin,
        size_t numIds,
        GfBBox3d *result);

    /// Compute the bound of the given point instance in world space.
    ///
    GfBBox3d
    ComputePointInstanceWorldBound(
        const UsdGeomPointInstancer& instancer, int64_t instanceId) {
        GfBBox3d ret;
        ComputePointInstanceWorldBounds(instancer, &instanceId, 1, &ret);
        return ret;
    }

    /// Compute the bounds of the given point instances in the space of an
    /// ancestor prim \p relativeToAncestorPrim.  Write the results to
    /// \p result.
    ///
    /// The computed bound excludes the local transform at
    /// \p relativeToAncestorPrim. The computed bound may be incorrect if
    /// \p relativeToAncestorPrim is not an ancestor of \p prim.
    ///
    /// The \p result pointer must point to \p numIds GfBBox3d instances to be
    /// filled.
    USDGEOM_API
    bool
    ComputePointInstanceRelativeBounds(
        const UsdGeomPointInstancer &instancer,
        int64_t const *instanceIdBegin,
        size_t numIds,
        const UsdPrim &relativeToAncestorPrim,
        GfBBox3d *result);

    /// Compute the bound of the given point instance in the space of an
    /// ancestor prim \p relativeToAncestorPrim.
    GfBBox3d
    ComputePointInstanceRelativeBound(
        const UsdGeomPointInstancer &instancer,
        int64_t instanceId,
        const UsdPrim &relativeToAncestorPrim) {
        GfBBox3d ret;
        ComputePointInstanceRelativeBounds(
            instancer, &instanceId, 1, relativeToAncestorPrim, &ret);
        return ret;
    }

    /// Compute the oriented bounding boxes of the given point instances.
    ///
    /// The computed bounds include the transform authored on the instancer
    /// itself, but does not include any ancestor transforms (it does not
    /// include the local-to-world transform).
    ///
    /// The \p result pointer must point to \p numIds GfBBox3d instances to be
    /// filled.
    USDGEOM_API
    bool
    ComputePointInstanceLocalBounds(
        const UsdGeomPointInstancer& instancer,
        int64_t const *instanceIdBegin,
        size_t numIds,
        GfBBox3d *result);

    /// Compute the oriented bounding boxes of the given point instances.
    GfBBox3d
    ComputePointInstanceLocalBound(
        const UsdGeomPointInstancer& instancer,
        int64_t instanceId) {
        GfBBox3d ret;
        ComputePointInstanceLocalBounds(instancer, &instanceId, 1, &ret);
        return ret;
    }
            

    /// Computes the bound of the given point instances, but does not include
    /// the transform (if any) authored on the instancer itself.
    ///
    /// \b IMPORTANT: while the BBox does not contain the local transformation,
    /// in general it may still contain a non-identity transformation matrix to
    /// put the bounds in the correct space. Therefore, to obtain the correct
    /// axis-aligned bounding box, the client must call ComputeAlignedRange().
    ///
    /// The \p result pointer must point to \p numIds GfBBox3d instances to be
    /// filled.
    USDGEOM_API
    bool
    ComputePointInstanceUntransformedBounds(
        const UsdGeomPointInstancer& instancer,
        int64_t const *instanceIdBegin,
        size_t numIds,
        GfBBox3d *result);

    /// Computes the bound of the given point instances, but does not include
    /// the instancer's transform.
    GfBBox3d
    ComputePointInstanceUntransformedBound(
        const UsdGeomPointInstancer& instancer,
        int64_t instanceId) {
        GfBBox3d ret;
        ComputePointInstanceUntransformedBounds(
            instancer, &instanceId, 1, &ret);
        return ret;
    }
    
    /// Clears all pre-cached values.
    USDGEOM_API
    void Clear();

    /// Indicate the set of \p includedPurposes to use when resolving child
    /// bounds. Each child's purpose must match one of the elements of this set
    /// to be included in the computation; if it does not, child is excluded.
    ///
    /// Note the use of *child* in the docs above, purpose is ignored for the
    /// prim for whose bounds are directly queried.
    ///
    /// Changing this value <b>does not invalidate existing caches</b>.
    USDGEOM_API
    void SetIncludedPurposes(const TfTokenVector& includedPurposes);

    /// Get the current set of included purposes.
    const TfTokenVector& GetIncludedPurposes() { return _includedPurposes; }

    /// Returns whether authored extent hints are used to compute
    /// bounding boxes.
    bool GetUseExtentsHint() const {
        return _useExtentsHint;
    }

    /// Returns whether prim visibility should be ignored when computing
    /// bounding boxes.
    bool GetIgnoreVisibility() const {
        return _ignoreVisibility;
    }    

    /// Use the new \p time when computing values and may clear any existing
    /// values cached for the previous time. Setting \p time to the current time
    /// is a no-op.
    USDGEOM_API
    void SetTime(UsdTimeCode time);

    /// Get the current time from which this cache is reading values.
    UsdTimeCode GetTime() const { return _time; }

    /// Set the base time value for this bbox cache.  This value is used only
    /// when computing bboxes for point instancer instances (see
    /// ComputePointInstanceWorldBounds(), for example).  See
    /// UsdGeomPointInstancer::ComputeExtentAtTime() for more information.  If
    /// unset, the bbox cache uses its time (GetTime() / SetTime()) for this
    /// value.
    ///
    /// Note that setting the base time does not invalidate any cache entries.
    void SetBaseTime(UsdTimeCode baseTime) {
        _baseTime = baseTime;
    }

    /// Return the base time if set, otherwise GetTime().  Use HasBaseTime() to
    /// observe if a base time has been set.
    UsdTimeCode GetBaseTime() const {
        return _baseTime.value_or(GetTime());
    }

    /// Clear this cache's baseTime if one has been set.  After calling this,
    /// the cache will use its time as the baseTime value.
    void ClearBaseTime() {
        _baseTime = std::nullopt;
    }

    /// Return true if this cache has a baseTime that's been explicitly set,
    /// false otherwise.
    bool HasBaseTime() const {
        return static_cast<bool>(_baseTime);
    }

private:
    // Worker task.
    class _BBoxTask;

    // Helper object for computing bounding boxes for instance prototypes.
    class _PrototypeBBoxResolver;

    // Map of purpose tokens to associated bboxes.
    typedef std::map<TfToken, GfBBox3d,  TfTokenFastArbitraryLessThan>
        _PurposeToBBoxMap;

    // Each individual prim will have it's own entry in the bbox cache.  When
    // instancing is involved we store the prototype prims and their children in
    // the cache for use by each prim that instances each prototype.  However,
    // because of the way we compute and inherit purpose, we may end up needed
    // to compute multitple different bboxes for prototypes and their children
    // if the prims that instance them would cause these prototypes to inherit a
    // different purpose value when the prims under the prototype don't have an
    // authored purpose of their own.
    //
    // This struct is here to represent a prim and the purpose that it would
    // inherit from the prim that instances it. It is used as the key for the
    // map of prim's to the cached entries, allowing prims in prototypes to have
    // more than one bbox cache entry for each distinct context needed to
    // appropriately compute for all instances. instanceInheritablePurpose will
    // always be empty for prims that aren't prototypes or children of
    // prototypes, meaning that prims not in prototypes will only have one
    // context each.
    struct _PrimContext {
        // The prim itself
        UsdPrim prim;

        // The purpose that would be inherited from the instancing prim if this
        // prim does not have an explicit purpose.
        TfToken instanceInheritablePurpose;

        _PrimContext() = default;
        explicit _PrimContext(const UsdPrim &prim_, 
                               const TfToken &purpose = TfToken()) 
            : prim(prim_), instanceInheritablePurpose(purpose) {};

        bool operator==(const _PrimContext &rhs) const {
            return prim == rhs.prim && 
                instanceInheritablePurpose == rhs.instanceInheritablePurpose;
        }

        // Convenience stringify for debugging.
        std::string ToString() const;
    };

    template<typename TransformType>
    GfBBox3d _ComputeBoundWithOverridesHelper(
        const UsdPrim &prim,
        const SdfPathSet &pathsToSkip,
        const TransformType &primOverride,
        const TfHashMap<SdfPath, GfMatrix4d, SdfPath::Hash> &ctmOverrides);

    bool
    _ComputePointInstanceBoundsHelper(
        const UsdGeomPointInstancer &instancer,
        int64_t const *instanceIdBegin,
        size_t numIds,
        GfMatrix4d const &xform,
        GfBBox3d *result);

    // Returns true if the \p prim should be included during child bounds
    // accumulation.
    bool _ShouldIncludePrim(const UsdPrim& prim);

    // True if \p attr or \p query may return different values given different
    // time queries. Note that a true result implies the attribute may have no
    // value, a default value or a single time sample value.
    bool _IsVarying(const UsdAttribute& attr);
    bool _IsVarying(const UsdAttributeQuery& query);

    // Populate the local bbox for the requested prim, without the
    // local-to-world transform or local transform applied. Return true when
    // bbox volume > 0.
    bool _Resolve(const UsdPrim& prim, _PurposeToBBoxMap *bboxes);

    // Resolves a single prim. This method must be thread safe. Assumes the
    // cache entry has been created for \p prim.
    //
    // \p inverseComponentCtm is used to combine all the child bboxes in
    // component-relative space.
    void _ResolvePrim(const _BBoxTask* task,
                      const _PrimContext& prim,
                      const GfMatrix4d &inverseComponentCtm);

    struct _Entry {
        _Entry()
            : isComplete(false)
            , isVarying(false)
            , isIncluded(false)
        { }

        // The cached bboxes for the various values of purpose token.
        _PurposeToBBoxMap bboxes;

        // Queries for attributes that need to be re-computed at each
        // time for this entry. This will be invalid for non-varying entries.
        std::shared_ptr<UsdAttributeQuery[]> queries;

        // Computed purpose info of the prim that's associated with the entry.
        // This data includes the prim's actual computed purpose as well as
        // whether this purpose is inheritable by child prims.
        UsdGeomImageable::PurposeInfo purposeInfo;

        // True when data in the entry is valid.
        bool isComplete;

        // True when the entry varies over time.
        bool isVarying;

        // True when the entry is visible.
        bool isIncluded;
    };

    // Returns the cache entry for the given \p prim if one already exists.
    // If no entry exists, creates (but does not resolve) entries for
    // \p prim and all of its descendents. In this case, the prototype prims
    // whose bounding boxes need to be resolved in order to resolve \p prim
    // will be returned in \p prototypePrimContexts.
    _Entry* _FindOrCreateEntriesForPrim(
        const _PrimContext& prim,
        std::vector<_PrimContext> *prototypePrimContexts);

    // Returns the combined bounding box for the currently included set of
    // purposes given a _PurposeToBBoxMap.
    GfBBox3d _GetCombinedBBoxForIncludedPurposes(
        const _PurposeToBBoxMap &bboxes);

    // Populates \p bbox with the bounding box computed from the authored
    // extents hint. Based on the included purposes, the extents in the
    // extentsHint attribute are combined together to compute the bounding box.
    bool _GetBBoxFromExtentsHint(
        const UsdGeomModelAPI &geomModel,
        const UsdAttributeQuery &extentsHintQuery,
        _PurposeToBBoxMap *bboxes);

    // Returns whether the children of the given prim can be pruned
    // from the traversal to pre-populate entries.
    bool _ShouldPruneChildren(const UsdPrim &prim, _Entry *entry);

    // Helper function for computing a prim's purpose info efficiently by 
    // using the parent entry's cached computed purpose info and caching it
    // its cache entry.
    // Optionally this can recursively compute and cache the purposes for any 
    // existing parent entries in the cache that haven't had their purposes 
    // computed yet.
    template <bool IsRecursive>
    void _ComputePurposeInfo(_Entry *entry, const _PrimContext &prim);

    // Helper to determine if we should use extents hints for \p prim.
    inline bool _UseExtentsHintForPrim(UsdPrim const &prim) const;

    // Specialize TfHashAppend for TfHash
    template <typename HashState>
    friend void TfHashAppend(HashState& h, const _PrimContext &key)
    {
        h.Append(key.prim);
        h.Append(key.instanceInheritablePurpose);
    }

    // Need hash_value for boost to key cache entries by prim context.
    friend size_t hash_value(const _PrimContext &key) { return TfHash{}(key); }

    typedef TfHash _PrimContextHash;
    typedef TfHashMap<_PrimContext, _Entry, _PrimContextHash> _PrimBBoxHashMap;

    // Finds the cache entry for the prim context if it exists.
    _Entry *_FindEntry(const _PrimContext &primContext)
    {
        return TfMapLookupPtr(_bboxCache, primContext);
    }

    // Returns the cache entry for the prim context, adding it if doesn't 
    // exist.
    _Entry *_InsertEntry(const _PrimContext &primContext)
    {
        return &(_bboxCache[primContext]);
    }

    WorkDispatcher _dispatcher;
    UsdTimeCode _time;
    std::optional<UsdTimeCode> _baseTime;
    TfTokenVector _includedPurposes;
    UsdGeomXformCache _ctmCache;
    _PrimBBoxHashMap _bboxCache;
    bool _useExtentsHint;
    bool _ignoreVisibility;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_GEOM_BBOX_CACHE_H
