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
#ifndef USDGEOM_BBOXCACHE_H
#define USDGEOM_BBOXCACHE_H

#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/base/gf/bbox3d.h"

#include <boost/shared_array.hpp>
#include "pxr/base/tf/hashmap.h"

class UsdGeomModelAPI;
class UsdGeomPointBased;

/// \class UsdGeomBBoxCache
/// Caches bounds by recursively computing and aggregating bounds of children
/// in world space and aggregating the result back into local space.
///
/// The cache is configured for a specific time and  
/// \ref UsdGeomImageable::GetPurposeAttr() set of purposes. When
/// querying a bound, transforms and extents are read either from the time
/// specified or UsdTimeCode::Default(), following \ref Usd_ValueResolution standard
/// time-sample value resolution.  As noted in SetIncludedPurposes(), changing
/// the included purposes does not invalidate the cache, because we cache
/// purpose along with the geometric data.
///
/// Child prims that are invisible at the requested time are
/// excluded when computing a prim's bounds.  However, if a bound is requested
/// directly for an excluded prim, it will be computed. Additionally, only prims
/// deriving from UsdGeomImageable are included in child bounds computations. 
///
/// Unlike standard UsdStage traversals, the traversal performed by the
/// UsdGeomBBoxCache includes prims that are unloaded (see UsdPrim::IsLoaded()).
/// This makes it possible to fetch bounds for a UsdStage that has been
/// opened without \em forcePopulate , provided the unloaded model prims have
/// authored extent hints (see UsdGeomModelAPI::GetExtentsHint()).
///
/// This class is optimized for computing tight <b>untransformed "object" space</b> 
/// bounds for component-models.  In the absence of component models, bounds are 
/// optimized for world-space, since there is no other easily identifiable space 
/// for which to optimize, and we cannot optimize for every prim's local space 
/// without performing quadratic work.
/// 
/// The TfDebug flag, USDGEOM_BBOX, is provided for debugging.
///
/// Warnings:
///  * This class should only be used in conjunction with valid UsdPrim objects.
///
///  * This cache does not listen for change notifications; the user is
///    responsible for clearing the cache when changes occur.
///
///  * Thread safety: multiple instances of this class can be used from multiple
///    threads, but a single instance is not thread safe. It is thread safe to 
///    query bounds once they have been cached, as long as the time and included
///    purposes have not changed.  Therefore, if you wish to share cached bounds
///    among multiple worker threads, first query the bbox of the \b root of 
///    the stage in the master-thread prior to spawning the workers.  Querying
///    the root will cause all sub-results to be cached.
///
///  * Querying bounds will use multiple threads, upto the limit set in libWork.
///
class UsdGeomBBoxCache
{
public:
    /// Construct a new BBoxCache for a specific \p time and set of \p
    /// includedPurposes.
    ///
    /// Only prims with a purpose that matches the \p includedPurposes will be
    /// considered when accumulating child bounds. See UsdGeomImageable for
    /// allowed purpose values.
    ///
    /// If \p useExtentsHint is true, then when computing the bounds for any 
    /// model-root prim, if the prim is visible at \p time, we will fetch its
    /// extents hint (via UsdGeomModelAPI::GetExtentsHint()). If it is authored,
    /// we use it to compute the bounding box for the selected combination
    /// of includedPurposes by combining bounding box hints that have been
    /// cached for various values of purposes.
    /// 
    UsdGeomBBoxCache(UsdTimeCode time, TfTokenVector includedPurposes,
                     bool useExtentsHint=false);

    /// Compute the bound of the given prim in world space, leveraging any
    /// pre-existing, cached bounds.
    ///
    /// The bound of the prim is computed, including the transform (if any)
    /// authored on the node itself, and then transformed to world space.
    ///
    /// Error handling note: No checking of \p prim validity is performed.
    /// If \p prim is invalid, this method will abort the program; therefore
    /// it is the client's responsibility to ensure \p prim is valid.
    GfBBox3d ComputeWorldBound(const UsdPrim& prim);

    /// Compute the bound of the given prim in the space of an ancestor prim, 
    /// \p relativeToAncestorPrim, leveraging any pre-existing cached bounds.
    /// 
    /// The computed bound excludes the local transform at 
    /// \p relativeToAncestorPrim. The computed bound may be incorrect if 
    /// \p relativeToAncestorPrim is not an ancestor of \p prim.
    /// 
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
    GfBBox3d ComputeUntransformedBound(const UsdPrim& prim);

    /// \overload
    /// Computes the bound of the prim's descendents while excluding the 
    /// subtrees rooted at the paths in \p pathsToSkip. Additionally, the 
    /// parameter \p ctmOverrides is used to specify overrides to the CTM values 
    /// of certain paths underneath the prim. The CTM values in the 
    /// \p ctmOverrides map are in the space of the given prim, \p prim. 
    /// 
    /// This leverages any pre-existing, cached bounds, but does not include 
    /// the transform (if any) authored on the prim itself. 
    /// 
    /// \b IMPORTANT: while the BBox does not contain the local transformation,
    /// in general it may still contain a non-identity transformation matrix to
    /// put the bounds in the correct space. Therefore, to obtain the correct
    /// axis-aligned bounding box, the client must call ComputeAlignedRange().
    ///
    /// See ComputeWorldBound() for notes on performance and error handling.
    GfBBox3d ComputeUntransformedBound(
        const UsdPrim &prim, 
        const SdfPathSet &pathsToSkip,
        const TfHashMap<SdfPath, GfMatrix4d, SdfPath::Hash> &ctmOverrides);

    /// Clears all pre-cached values.
    void Clear();

    /// Indicate the set of \p includedPurposes to use when resolving child
    /// bounds. Each child's purpose must match one of the elements of this set
    /// to be included in the computation; if it does not, child is excluded.
    ///
    /// Note the use of *child* in the docs above, purpose is ignored for the
    /// prim for whose bounds are directly queried.
    ///
    /// Changing this value  <b>does not invalidate existing caches.</b>
    /// It does mutate the BBoxCache's state, however, so be mindful of
    /// calling in multi-threaded uses.
    void SetIncludedPurposes(const TfTokenVector& includedPurposes);

    /// Get the current set of included purposes.
    const TfTokenVector& GetIncludedPurposes() { return _includedPurposes; }

    /// Returns whether authored extent hints are used to compute
    /// bounding boxes.
    bool GetUseExtentsHint() const { 
        return _useExtentsHint; 
    }

    /// Use the new \p time when computing values and may clear any existing
    /// values cached for the previous time. Setting \p time to the current time
    /// is a no-op.
    void SetTime(UsdTimeCode time);

    /// Get the current time from which this cache is reading values.
    UsdTimeCode GetTime() { return _time; }

private:
    // Worker task.
    class _BBoxTask;

    // Helper object for computing bounding boxes for instance masters.
    class _MasterBBoxResolver;

    // Map of purpose tokens to associated bboxes.
    typedef std::map<TfToken, GfBBox3d,  TfTokenFastArbitraryLessThan>
        _PurposeToBBoxMap;

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
    void _ResolvePrim(_BBoxTask* task, 
                      const UsdPrim& prim, 
                      const GfMatrix4d &inverseComponentCtm);

    struct _Entry {
        _Entry()
            : isComplete(false)
            , isVarying(false)
            , isIncluded(false)
        { }

        // The cached bboxes for the various values of purpose token.
        _PurposeToBBoxMap bboxes;      

        // True when data in the entry is valid.
        bool isComplete;    

        // True when the entry varies over time.
        bool isVarying;     

        // True when the entry is visible.
        bool isIncluded;

        // Computed purpose value of the prim that's associated with the entry.
        TfToken purpose;

        // Queries for attributes that need to be re-computed at each
        // time for this entry. This will be invalid for non-varying entries.
        boost::shared_array<UsdAttributeQuery> queries;
    };

    // Returns the cache entry for the given \p prim if one already exists.
    // If no entry exists, creates (but does not resolve) entries for
    // \p prim and all of its descendents. In this case, the master prims
    // whose bounding boxes need to be resolved in order to resolve \p prim
    // will be returned in \p masterPrims.
    _Entry* _FindOrCreateEntriesForPrim(const UsdPrim& prim, 
                                        std::vector<UsdPrim>* masterPrims);

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

    // Computes the extent for a UsdGeomPointBased prim and stores the result
    // in extent. This function will return false if the extent could not be 
    // computed, true otherwise.
    bool _ComputeMissingExtent(const UsdGeomPointBased& pointBasedObj,
        const VtVec3fArray &points,
        VtVec3fArray* extent);

    // Returns whether the children of the given prim can be pruned 
    // from the traversal to pre-populate entries.
    bool _ShouldPruneChildren(const UsdPrim &prim, _Entry *entry);

    // Helper function for computing a prim's purpose efficiently by using the 
    // parent entry's cached computed-purpose.
    TfToken  _ComputePurpose(const UsdPrim &prim);

    typedef boost::hash<UsdPrim> _UsdPrimHash;
    typedef TfHashMap<UsdPrim, _Entry, _UsdPrimHash> _PrimBBoxHashMap;

    UsdTimeCode             _time;
    TfTokenVector       _includedPurposes;
    bool                _useExtentsHint;
    UsdGeomXformCache   _ctmCache;
    _PrimBBoxHashMap    _bboxCache;

};

#endif // USDGEOM_BBOXCACHE_H
