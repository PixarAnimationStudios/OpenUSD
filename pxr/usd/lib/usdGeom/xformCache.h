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
#ifndef USDGEOM_XFORMCACHE_H
#define USDGEOM_XFORMCACHE_H

#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/usdGeom/xformable.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"

#include <boost/functional/hash.hpp>

/// \class UsdGeomXformCache
///
/// A caching mechanism for transform matrices. For best performance, this
/// object should be reused for multiple CTM queries.
///
/// Instances of this type can be copied, though using Swap() may result in
/// better performance.
///
/// It is valid to cache prims from multiple stages in a single XformCache.
///
/// WARNING: this class does not automatically invalidate cached values based
/// on changes to the stage from which values were cached. Additionally, a
/// separate instance of this class should be used per-thread, calling the Get*
/// methods from multiple threads is not safe, as they mutate internal state.
///
class UsdGeomXformCache
{
public:
    /// Construct a new XformCache for the specified \p time.
    USDGEOM_API
    explicit UsdGeomXformCache(const UsdTimeCode time);

    /// Construct a new XformCache for UsdTimeCode::Default().
    USDGEOM_API
    UsdGeomXformCache();

    /// Compute the transformation matrix for the given \p prim, including the
    /// transform authored on the Prim itself, if present.
    ///
    /// \note This method may mutate internal cache state and is not thread
    /// safe.
    USDGEOM_API
    GfMatrix4d GetLocalToWorldTransform(const UsdPrim& prim);

    /// Compute the transformation matrix for the given \p prim, but do NOT
    /// include the transform authored on the prim itself.
    ///
    /// \note This method may mutate internal cache state and is not thread
    /// safe.
    USDGEOM_API
    GfMatrix4d GetParentToWorldTransform(const UsdPrim& prim);

    /// Returns the local transformation of the prim. Uses the cached 
    /// XformQuery to compute the result quickly. The result of this call
    /// is not cached.
    USDGEOM_API
    GfMatrix4d GetLocalTransformation(const UsdPrim &prim,
                                      bool *resetsXformStack);

    /// Whether the attribute named \p attrName, belonging to the 
    /// given \p prim affects the local transform value at the prim.
    /// 
    /// \note This method may mutate internal cache state and is not thread
    /// safe.
    USDGEOM_API
    bool IsAttributeIncludedInLocalTransform(const UsdPrim &prim, 
                                             const TfToken &attrName);

    /// Whether the local transformation value at the prim may vary over time.
    /// 
    /// \note This method may mutate internal cache state and is not thread
    /// safe.
    USDGEOM_API
    bool TransformMightBeTimeVarying(const UsdPrim &prim);

    /// Whether the xform stack is reset at the given prim.
    /// 
    /// \note This method may mutate internal cache state and is not thread
    /// safe.
    USDGEOM_API
    bool GetResetXformStack(const UsdPrim &prim);

    /// Clears all pre-cached values.
    USDGEOM_API
    void Clear();

    /// Use the new \p time when computing values and may clear any existing
    /// values cached for the previous time. Setting \p time to the current time
    /// is a no-op.
    USDGEOM_API
    void SetTime(UsdTimeCode time);

    /// Get the current time from which this cache is reading values.
    UsdTimeCode GetTime() { return _time; }

    /// Specify the path to be used when computing transformations to
    /// world space.
    ///
    /// The default is to accumulate parent transformations until
    /// the absolute root (pseudo root) is reached. Setting this
    /// to some other path will cause accumulation of parent transforms
    /// to terminate instead when the specified path is reached.
    ///
    /// This can be used to restrict accumulation of transformations to
    /// a subtree of the scene.
    USDGEOM_API
    void SetWorldPath(const SdfPath& worldPath);

    /// Returns the path used when computing transformations
    /// to world space. The default is for this to be
    /// the absolute root path.
    const SdfPath & GetWorldPath() const { return _worldPath; }

    /// Swap the contents of this XformCache with \p other.
    USDGEOM_API
    void Swap(UsdGeomXformCache& other);

private:

    // Traverses backwards the hierarchy starting from prim
    // all the way to the root and computes the ctm
    GfMatrix4d const* _GetCtm(const UsdPrim& prim);

    // Map of cached values.
    struct _Entry {
        _Entry(const UsdGeomXformable::XformQuery & query_,
               const GfMatrix4d& ctm_,
               bool ctmIsValid_)
            : query(query_)
            , ctm(ctm_)
            , ctmIsValid(ctmIsValid_)
        { }

        UsdGeomXformable::XformQuery query;
        GfMatrix4d ctm;
        bool ctmIsValid;
    };

    // Helper function to get or create a new entry for a prim in the ctm cache.
    _Entry * _GetCacheEntryForPrim(const UsdPrim &prim);

    typedef TfHashMap<UsdPrim, _Entry, boost::hash<UsdPrim> > _PrimHashMap;
    _PrimHashMap _ctmCache;
    
    // The time at which this stack is querying and caching attribute values.
    UsdTimeCode _time;

    SdfPath _worldPath;
};

#define USDGEOM_XFORM_CACHE_API_VERSION 1

#endif // USDGEOM_XFORMCACHE_H
