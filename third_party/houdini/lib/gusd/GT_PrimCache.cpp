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

#include "GT_PrimCache.h"

#include "USD_StdTraverse.h"
#include "primWrapper.h"
#include "UT_Gf.h"
#include "USD_PropertyMap.h"
#include "USD_XformCache.h"

#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdGeom/boundable.h"

#include <GT/GT_PrimCollect.h>
#include <GT/GT_PrimPolygonMesh.h>
#include <GT/GT_TransformArray.h>
#include <GT/GT_PrimInstance.h>
#include <GT/GT_RefineCollect.h>
#include <GT/GT_RefineParms.h>
#include <GT/GT_CatPolygonMesh.h>
#include <SYS/SYS_Hash.h>
#include <UT/UT_HDKVersion.h>

#if HDK_API_VERSION >= 16050446
#include <GT/GT_PackedAlembic.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

using std::vector;
using std::cerr;
using std::endl;


typedef UT_IntrusivePtr<GT_PrimPolygonMesh> GT_PrimPolygonMeshHandle;


////////////////////////////////////////////////////////////////////////////////////////////

namespace {

    struct CacheKeyValue
    {
        CacheKeyValue() : hash(0) {}
        
        CacheKeyValue(const UsdPrim& prim, UsdTimeCode time,
                      GusdPurposeSet purposes)
            : prim(prim)
            , time(time)
            , purposes(purposes)
            , hash(ComputeHash(prim,time,purposes)) {}

        static std::size_t  ComputeHash(const UsdPrim& prim, UsdTimeCode time,
                                        GusdPurposeSet purposes)
                            {
                                std::size_t h = hash_value(prim);
                                boost::hash_combine(h, time);
                                boost::hash_combine(h, purposes);
                                return h; 
                            }

        bool                operator==(const CacheKeyValue& o) const
                            { return prim == o.prim &&
                                     time == o.time &&
                                     purposes == o.purposes; }

        friend size_t       hash_value(const CacheKeyValue& o)
                            { return o.hash; }

        struct HashCmp
        {
            static std::size_t  hash(const CacheKeyValue& key)
                                { return key.hash; }
            static bool         equal(const CacheKeyValue& a,
                                      const CacheKeyValue& b)
                                { return a == b; }
        };

        UsdPrim             prim;
        UsdTimeCode         time;
        GusdPurposeSet      purposes;
        std::size_t         hash;
    };



   struct CacheEntry : public UT_CappedItem {
        CacheEntry() : prim( NULL ) {}
        CacheEntry( GT_PrimitiveHandle p ) : prim(p) {}
        CacheEntry( const CacheEntry &rhs ) : prim( rhs.prim ) {}

        virtual int64   getMemoryUsage () const { 
            int64 rv = sizeof(*this);
            if( prim )
                rv += prim->getMemoryUsage();
            return rv;
        }

        GT_PrimitiveHandle prim;
    };

#if HDK_API_VERSION < 16050000
    static inline void intrusive_ptr_add_ref(const CacheEntry *o) { const_cast<CacheEntry *>(o)->incref(); }
    static inline void intrusive_ptr_release(const CacheEntry *o) { const_cast<CacheEntry *>(o)->decref(); }
#endif

    struct CreateEntryFn {

        CreateEntryFn( GusdGT_PrimCache &cache ) : m_cache( cache ) {}    

        UT_IntrusivePtr<UT_CappedItem> operator()( 
            const UsdPrim& prim, 
            UsdTimeCode time, 
            GusdPurposeSet purposes,
            bool skipRoot ) const;

        GusdGT_PrimCache& m_cache;
    }; 

    // Override the refiner to recurse on subdivs and collections
    struct Refiner : public GT_RefineCollect {

        vector<GT_CatPolygonMesh> coalescedMeshes;
        vector<SYS_HashType> coalescedIds;
        
        virtual void addPrimitive( const GT_PrimitiveHandle &prim )
        {
            if( prim->getPrimitiveType() == GT_PRIM_SUBDIVISION_MESH ||
                prim->getPrimitiveType() == GT_PRIM_COLLECT) {
                prim->refine( *this );
            }

            else if( prim->getPrimitiveType() == GT_PRIM_POLYGON_MESH ) {  


                // There are significant performace advantages to combining as 
                // many meshes as possible. 

                GT_PrimPolygonMeshHandle mesh = UTverify_cast<GT_PrimPolygonMesh*>(prim.get());
                int64 meshId = 0;
                mesh->getUniqueID( meshId );

                // Flatten transforms on the mesh
                UT_Matrix4D m;
                mesh->getPrimitiveTransform()->getMatrix( m );
                if( !m.isEqual( UT_Matrix4D::getIdentityMatrix() )) {
                    GT_AttributeListHandle newShared = mesh->getShared()->transform(  mesh->getPrimitiveTransform() );
                    mesh = new GT_PrimPolygonMesh( *mesh, newShared, mesh->getVertex(), mesh->getUniform(), mesh->getDetail() );
                }

                // Houdini is going to compute normals if we don't. Doing it here 
                // allows them to be cached.
                auto normMesh = mesh->createPointNormalsIfMissing();
                if( normMesh ) {
                    mesh = normMesh;
                }

                // If GT_CatPolygonMesh will combine meshes with the same
                // attribute sets until it reaches some maximum size.
                // If we fail to concate the new mesh with any of our existing
                // concatenated meshs, just create a new one. 
                bool appended = false;
                for( size_t i = 0; i < coalescedMeshes.size(); ++i ) {
                    if( coalescedMeshes[i].append( mesh )) {
                        SYShashCombine<int64>( coalescedIds[i], meshId );
                        appended = true;
                        break;
                    }
                }
                if( !appended ) {
                    coalescedMeshes.push_back( GT_CatPolygonMesh() );
                    coalescedMeshes.back().append( mesh );
                    coalescedIds.push_back( meshId );
                }
            }
            else {
                GT_RefineCollect::addPrimitive( prim );
            }
        }
    };

    typedef GusdUT_CappedKey<CacheKeyValue, CacheKeyValue::HashCmp> CacheKey;

}; // end namespace 

////////////////////////////////////////////////////////////////////////////////

/* static */ 
GusdGT_PrimCache &
GusdGT_PrimCache::GetInstance()
{
    static GusdGT_PrimCache cache;
    return cache;
}

////////////////////////////////////////////////////////////////////////////////

GusdGT_PrimCache::GusdGT_PrimCache() : 
    _prims( "GusdGT_PrimCache", 1024 )
{
}

GusdGT_PrimCache::~GusdGT_PrimCache()
{
}

GT_PrimitiveHandle 
GusdGT_PrimCache::GetPrim( const UsdPrim &usdPrim, 
                           UsdTimeCode time, 
                           GusdPurposeSet purposes,
                           bool skipRoot )
{
    // We need to skip the root when walking into instance masters.

    if( !usdPrim.IsValid() ) {
        return GT_PrimitiveHandle();
    }

    CacheKey key(CacheKeyValue(usdPrim, time, purposes));

    CreateEntryFn createFunc(*this);
    auto entry = _prims.FindOrCreate<CacheEntry>( key, createFunc,
                                                  usdPrim, time, 
                                                  purposes, skipRoot );
    
    return entry ? entry->prim : NULL;    
}

void
GusdGT_PrimCache::Clear()
{
    _prims.clear();
}

int64
GusdGT_PrimCache::Clear(const UT_StringSet& paths)
{
    return _prims.ClearEntries(
        [&](const UT_CappedKeyHandle& key,
            const UT_CappedItemHandle& item) {

        return GusdUSD_DataCache::ShouldClearPrim(
            (*UTverify_cast<const CacheKey*>(key.get()))->prim, paths);
    });
}

////////////////////////////////////////////////////////////////////////////////

UT_IntrusivePtr<UT_CappedItem> 
CreateEntryFn::operator()( 
    const UsdPrim &prim, 
    UsdTimeCode time, 
    GusdPurposeSet purposes,
    bool skipRoot ) const 
{ 

    // Build a cache entry for a USD Prim. A cache entry contains a GT_Primitive
    // that can be used to draw the usd prim. 
    //
    // Handle 3 different cases differently. 
    //
    // USD gprims (leaves in the hierarchy) are just converted to GT_Primitives. 
    //
    // For USD native instances, find the instance's master or the prim in
    // master corresponding to an instance proxy, and recurse on that. This way
    // each instance should share a cache with its master.
    //
    // Any other USD primitive represents a branch of the USD hierarchy. Find 
    // all the instances and leaves in this branch and build a GT_PrimCollect 
    // that represent the branch.
    //
    // The viewport doesn't seem to like nested collections very much. So we 
    // use a refiner to flatten the collections.
    
    Refiner refiner;

    // Tell the wrapper classes that we are refining for the viewport. In this case we just load the geometry and color. No
    // other primvars. Also load curves as polylines.
    GT_RefineParms refineParms;
    refineParms.setPackedViewportLOD( true );

    bool isInstance = prim.IsInstance();
    bool isInstanceProxy = prim.IsInstanceProxy();
    if( isInstance || isInstanceProxy)
    {
        DBG( cerr << "Create prim cache for instance " << prim.GetPath() << " at " << time << endl; )
        // Look for a cache entry from the instance master
        UsdPrim masterPrim = isInstance ? prim.GetMaster() : prim.GetPrimInMaster();
        GT_PrimitiveHandle instancePrim = 
                m_cache.GetPrim( masterPrim,
                                 time, 
                                 purposes,
                                 true );

        if( !instancePrim ) {
            return NULL;
        }
        return new CacheEntry( instancePrim );
    }
    else if( prim.IsA<UsdGeomBoundable>() )
    {
        UsdGeomImageable imageable( prim );

        DBG( cerr << "Create prim cache for gprim " << prim.GetPath() << " at " << time << endl; )

        GT_PrimitiveHandle gp = 
            GusdPrimWrapper::defineForRead( 
                                imageable,
                                time,
                                purposes );
        if( gp ) {              
            gp->refine( refiner, &refineParms );
        }
    }
    else {

        DBG( cerr << "Create prim cache for group " << prim.GetPath() << " at " << time << endl; )
        
        // Find all the gprims in the group.
        UT_Array<UsdPrim> gprims;
        GusdUSD_StdTraverse::GetBoundableTraversal().FindPrims( 
            prim, 
            time,
            purposes,
            gprims,
            skipRoot );

        std::map<GT_PrimitiveHandle, std::vector<UsdPrim>> primSort;

        if( gprims.size() > 0 ) {

            // All the gprims and instances in this group need to be transformed 
            // into the groups space.

            UT_Matrix4D invGroupXform;
            GusdUSD_XformCache::GetInstance().GetLocalToWorldTransform( 
                    prim, time, invGroupXform ); 
            invGroupXform.invert();


            // Iterate though all the prims and find matching instances.
            for( auto it = gprims.begin(); it != gprims.end(); ++it ) 
            {
                UsdPrim p = *it;

                GT_PrimitiveHandle gtPrim = 
                    m_cache.GetPrim( p,
                                     time, 
                                     purposes,
                                     false );
                if( gtPrim ) {

                    auto sortIt = primSort.find( gtPrim );
                    if( sortIt == primSort.end() ) {
                        primSort[gtPrim] = std::vector<UsdPrim>( 1, p );
                    }
                    else {
                        sortIt->second.push_back( p );
                    }
                }
            }

            // Iterate through the results of the sort
            for( auto const &kv : primSort ) {

                GT_PrimitiveHandle gtPrim = kv.first;
                const std::vector<UsdPrim> &usdPrims = kv.second;

                if( usdPrims.size() == 1 ) {

                    UsdPrim p = usdPrims[0];
                    UT_Matrix4D gprimXform;
                    GusdUSD_XformCache::GetInstance().GetLocalToWorldTransform( 
                            p, time, gprimXform ); 

                    UT_Matrix4D m = gprimXform * invGroupXform;

                    refiner.addPrimitive( 
                        gtPrim->copyTransformed( new GT_Transform( &m, 1 )));
                }
                else {

                    // Build GT_PrimInstances for prims that share the same geometry
                    auto transforms = new GT_TransformArray;

                    for( auto const &p : usdPrims ) {

                        UT_Matrix4D gprimXform;
                        GusdUSD_XformCache::GetInstance().GetLocalToWorldTransform( 
                                p, time, gprimXform ); 

                        UT_Matrix4D m = gprimXform * invGroupXform;

                        transforms->append( new GT_Transform( &m, 1 ));
                    }
                    refiner.addPrimitive( new GT_PrimInstance( gtPrim, transforms ) );
                }
            }
        }
    }
    exint numPrims = refiner.getPrimCollect()->entries() + 
                                    refiner.coalescedMeshes.size();
    if( numPrims == 0 ) {
        return NULL;
    }
    // If we only created one mesh, return that one directly, otherwise
    // create a collection. 
    if ( numPrims == 1 ) {
        if( refiner.getPrimCollect()->entries() > 0 ) {
            return new CacheEntry( refiner.getPrimCollect()->getPrim( 0 ) );
        }
        else {
#if HDK_API_VERSION >= 16050446
            auto meshHndl = refiner.coalescedMeshes[0].result();
            // XXX In houdini 16.5 we'll crash if we don't wrap the output
            // of GT_CatPolygonMesh in a GT_PackedAlembicMesh, similar to 
            // SideFX's alembic code.
            return new CacheEntry( new GT_PackedAlembicMesh( 
                        refiner.coalescedMeshes[0].result(),
                        refiner.coalescedIds[0]) );
#else
            return new CacheEntry( refiner.coalescedMeshes[0].result() );
#endif
        }
    }
    GT_PrimCollect* collect = new GT_PrimCollect( *refiner.getPrimCollect() );
    for( size_t i = 0; i < refiner.coalescedMeshes.size(); ++i ) {
        auto& catMesh = refiner.coalescedMeshes[i];
        GT_PrimitiveHandle meshPrim = catMesh.result();
#if HDK_API_VERSION >= 16050446
        collect->appendPrimitive(
                new GT_PackedAlembicMesh( meshPrim, refiner.coalescedIds[i] ) );
#else
        collect->appendPrimitive( meshPrim );
#endif
    }
    return new CacheEntry( collect );
}

PXR_NAMESPACE_CLOSE_SCOPE



