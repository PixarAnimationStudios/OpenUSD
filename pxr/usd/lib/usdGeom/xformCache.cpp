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
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdGeom/xform.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE



UsdGeomXformCache::UsdGeomXformCache(const UsdTimeCode time)
    : _time(time)
{
}

UsdGeomXformCache::UsdGeomXformCache()
    : _time(UsdTimeCode::Default())
{
}

GfMatrix4d
UsdGeomXformCache::GetLocalToWorldTransform(const UsdPrim& prim)
{
    TRACE_FUNCTION();
    return *_GetCtm(prim);
}

GfMatrix4d
UsdGeomXformCache::GetParentToWorldTransform(const UsdPrim& prim)
{
    TRACE_FUNCTION();
    return *_GetCtm(prim.GetParent());
}

bool 
UsdGeomXformCache::TransformMightBeTimeVarying(const UsdPrim &prim)
{
    // Get or create an entry for the prim in the CTM cache. 
    // The validity of the ctm value itself is irrelevant here.
    _Entry* entry = _GetCacheEntryForPrim(prim);
    if (TF_VERIFY(entry))
        return entry->query.TransformMightBeTimeVarying();

    // Being conservative and assuming that the trasnsform may vary over time.
    // Poor performance is better than wrong results!
    return true;
}

bool 
UsdGeomXformCache::GetResetXformStack(const UsdPrim &prim)
{
    // Get or create an entry for the prim in the CTM cache. 
    // The validity of the ctm value itself is irrelevant here.
    _Entry* entry = _GetCacheEntryForPrim(prim);
    if (TF_VERIFY(entry))
        return entry->query.GetResetXformStack();

    return false;
}

bool 
UsdGeomXformCache::IsAttributeIncludedInLocalTransform(const UsdPrim &prim, 
                                                       const TfToken &attrName)
{
    // Get or create an entry for the prim in the CTM cache. 
    // The validity of the ctm value itself is irrelevant here.
    _Entry* entry = _GetCacheEntryForPrim(prim);
    if (TF_VERIFY(entry))
        return  entry->query.IsAttributeIncludedInLocalTransform(attrName);

    return false;
}

UsdGeomXformCache::_Entry *
UsdGeomXformCache::_GetCacheEntryForPrim(const UsdPrim &prim)
{
    auto iresult = _ctmCache.insert({ prim, _Entry() });
    _Entry *result = &iresult.first->second;
    if (iresult.second) {
        if (UsdGeomXformable xf = UsdGeomXformable(prim)) {
            result->query = UsdGeomXformable::XformQuery(xf);
        }
        result->ctm.SetIdentity();
        result->ctmIsValid = false;
    }
    return result;
}

GfMatrix4d 
UsdGeomXformCache::GetLocalTransformation(const UsdPrim &prim, 
                                          bool *resetsXformStack)
{
    if(!resetsXformStack) {
        TF_CODING_ERROR("'resetsXformStack' pointer is null.");
        return GfMatrix4d(1);
    }

    _Entry *entry = _GetCacheEntryForPrim(prim);
    GfMatrix4d xform(1.); 
    if (TF_VERIFY(entry)) {
        entry->query.GetLocalTransformation(&xform, _time);
        *resetsXformStack = entry->query.GetResetXformStack();
    } else {
        *resetsXformStack = false;
    }

    return xform;
}

GfMatrix4d
UsdGeomXformCache::ComputeRelativeTransform(const UsdPrim &prim,
                                            const UsdPrim &ancestor,
                                            bool *resetXformStack)
{
    GfMatrix4d xform(1.);

    if(!resetXformStack) {
        TF_CODING_ERROR("'resetXformStack' pointer is null.");
        return xform;
    }

    for(UsdPrim p = prim; p && p != ancestor; p = p.GetParent()) {
        xform *= GetLocalTransformation(p, resetXformStack);
        if(*resetXformStack) {
            break;
        }
    }
    return xform;
}

GfMatrix4d const*
UsdGeomXformCache::_GetCtm(const UsdPrim& prim)
{
    // Local identity matrix to return by pointer.
    static GfMatrix4d const IDENTITY(1.0);

    // Base case: check for the pseudo root, which is always implicitly
    // identity.
    if (!prim)
        return &IDENTITY;

    // Check for a cached matrix.
    _Entry* entry = _GetCacheEntryForPrim(prim);
    if (entry->ctmIsValid)
        return &entry->ctm;

    // Recursively compute the ctm.
    GfMatrix4d xform(1.);
    entry->query.GetLocalTransformation(&xform, _time);
    bool resetsXformStack = entry->query.GetResetXformStack();
    
    xform = !resetsXformStack ? (xform * (*_GetCtm(prim.GetParent())))
                                 : xform;

    // Return the address of the inserted Matrix.
    entry->ctm = xform;
    entry->ctmIsValid = true;

    return &entry->ctm;
}

void
UsdGeomXformCache::SetTime(UsdTimeCode time)
{
    if (time == _time) 
        return;

    // Mark all cached CTMs as invalid, but leave the queries behind.
    TF_FOR_ALL(it, _ctmCache) {
        it->second.ctmIsValid = false;
    }

    _time = time;
}

void 
UsdGeomXformCache::Clear() { 
    _ctmCache.clear();
}

void
UsdGeomXformCache::Swap(UsdGeomXformCache& other)
{
    _ctmCache.swap(other._ctmCache);
    std::swap(_time, other._time);
}

PXR_NAMESPACE_CLOSE_SCOPE

