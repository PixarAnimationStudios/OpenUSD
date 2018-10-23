//
// Copyright 2017 Pixar
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
#include "boundsCache.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

using std::cerr;
using std::endl;

////////////////////////////////////////////////////////////////////////////////

/* static */ 
GusdBoundsCache &
GusdBoundsCache::GetInstance()
{
    static GusdBoundsCache cache;
    return cache;
}

GusdBoundsCache::GusdBoundsCache() 
{
}

GusdBoundsCache::~GusdBoundsCache()
{
}

bool 
GusdBoundsCache::ComputeWorldBound(
    const UsdPrim &prim,
    UsdTimeCode time,
    const TfTokenVector &includedPurposes,
    UT_BoundingBox &bounds )
{
    return _ComputeBound( 
                prim, 
                time, 
                includedPurposes, 
                &UsdGeomBBoxCache::ComputeWorldBound,
                bounds );
}

bool 
GusdBoundsCache::ComputeUntransformedBound(
    const UsdPrim &prim,
    UsdTimeCode time,
    const TfTokenVector &includedPurposes,
    UT_BoundingBox &bounds )
{
    return _ComputeBound( 
                prim, 
                time, 
                includedPurposes, 
                &UsdGeomBBoxCache::ComputeUntransformedBound,
                bounds );
}

bool 
GusdBoundsCache::_ComputeBound(
    const UsdPrim &prim,
    UsdTimeCode time,
    const TfTokenVector &includedPurposes,
    ComputeFunc boundFunc,
    UT_BoundingBox &bounds )
{
    if( !prim.IsValid() )
        return false;

    TfToken stageId( prim.GetStage()->GetRootLayer()->GetRealPath() );

    MapType::accessor accessor;
    if( !m_map.find( accessor, Key( stageId, includedPurposes ))) {
        m_map.insert( accessor, Key( stageId, includedPurposes ) );
        accessor->second = new Item( time, includedPurposes );
    }
    std::lock_guard<std::mutex> lock(accessor->second->lock);
    UsdGeomBBoxCache& cache = accessor->second->bboxCache;

    cache.SetTime( time );

    // boundFunc is either ComputeWorldBound or ComputeLocalBound
    GfBBox3d primBBox = (cache.*boundFunc)(prim);

    if( !primBBox.GetRange().IsEmpty() ) 
    {
        const GfRange3d rng = primBBox.ComputeAlignedRange();

        bounds = 
            UT_BoundingBox( 
                rng.GetMin()[0],
                rng.GetMin()[1],
                rng.GetMin()[2],
                rng.GetMax()[0],
                rng.GetMax()[1],
                rng.GetMax()[2]);
        return true;
    }

    return false;
}

void
GusdBoundsCache::Clear()
{
    m_map.clear();
}

int64 
GusdBoundsCache::Clear(const UT_StringSet& paths)
{
    int64 freed = 0;

    UT_Array<Key> keys;
    for( auto const& entry : m_map ) {
        if( paths.contains( entry.first.path.GetString() ) ) {
            keys.append( entry.first );
        }
    }

    for( auto const& k : keys ) {
        m_map.erase( k );
    }
    return freed;    
}

PXR_NAMESPACE_CLOSE_SCOPE
