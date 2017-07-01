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

#ifndef __GUSD_GU_PACKEDIMPL_H__
#define __GUSD_GU_PACKEDIMPL_H__


#include <GU/GU_PackedImpl.h>
#include <GT/GT_Handles.h>

#include <pxr/pxr.h>
#include "pxr/usd/usd/prim.h"
#include "gusd/USD_Proxy.h"
#include "gusd/USD_DataCache.h"
#include "gusd/purpose.h"

class GU_PrimPacked;
class GT_RefineParms;

PXR_NAMESPACE_OPEN_SCOPE

/// A GU implementation of a packed USD prim. 
///
/// This is a file backed prim that holds a reference to a prim in a usd file
/// at a particular frame. The prim can be a group of prims.
///
/// When a packed prim that references a USD group is unpacked, the result is
/// packed prims that represent the contents of that group. Those packed prims
/// may also be groups. To unpack down to the leafs, you may have to unpack may
/// times.
///
/// When we write USD packed prim to a USD file, we write a reference to the 
/// original file. USD has a limitation that it can only make references to root
/// nodes. 
///
/// When we write a reference into a USD file, we might want to use a different
/// file path than we use in the Houdini session. For example, we might want to
/// use a relative path vs. an absolute one. We might want to use a coalesced 
/// file vs per frame files. We don't want to enshrine this logic in the core so
/// we provide a second alternative file name that can be used for this. If
/// this fileName is left empty, we just us the primary file name.
///

class GusdGU_PackedUSD : public GU_PackedImpl
{
public:

    static GU_PrimPacked* Build( 
                            GU_Detail&              detail,
                            const UT_StringHolder&  fileName, 
                            const SdfPath&          primPath, 
                            const UsdTimeCode&      frame, 
                            const char*             lod = NULL,
                            GusdPurposeSet          purposes = GUSD_PURPOSE_PROXY );
    static GU_PrimPacked* Build( 
                            GU_Detail&              detail,
                            const UT_StringHolder&  fileName, 
                            const SdfPath&          primPath, 
                            const SdfPath&          srcPrimPath, 
                            int                     index,
                            const UsdTimeCode&      frame, 
                            const char*             lod = NULL,
                            GusdPurposeSet          purposes = GUSD_PURPOSE_PROXY );

    GusdGU_PackedUSD();
    GusdGU_PackedUSD(const GusdGU_PackedUSD &src );
    virtual ~GusdGU_PackedUSD();

    static void install(GA_PrimitiveFactory &factory);
    static GA_PrimitiveTypeId typeId();

    const UT_StringHolder& fileName() const { return m_fileName; }
    UT_StringHolder intrinsicFileName() const { return m_fileName; }
    void setFileName( const UT_StringHolder& fileName );

    const UT_StringHolder& altFileName() const { return m_altFileName; }
    UT_StringHolder intrinsicAltFileName() const { return m_altFileName; }
    void setAltFileName( const UT_StringHolder& fileName );

    const SdfPath& primPath() const { return m_primPath; }
    UT_StringHolder intrinsicPrimPath() const { return m_primPath.GetText(); }
    void setPrimPath( const UT_StringHolder& p );
    void setPrimPath( const SdfPath& primPath  );

    // If this prim was unpacked from a point instancer, srcPrimPath is the path
    // to the instancer.
    const SdfPath& srcPrimPath() const { return m_srcPrimPath; }
    UT_StringHolder intrinsicSrcPrimPath() const { return m_srcPrimPath.GetText(); }
    void setSrcPrimPath( const UT_StringHolder& p );
    void setSrcPrimPath( const SdfPath& primPath  );

    // If this prim was unpacked from a point instancer, index is the array 
    // index in the source point instancer.
    exint index() const { return m_index; }
    void setIndex( exint i );

    // Return true if this is a prim that has been unpacked from a point instancer.
    bool isPointInstance() const { return m_index >= 0; }
    
    // return the USD prim type
    UT_StringHolder intrinsicType() const;

    GA_Size usdLocalToWorldTransformSize() const { return 16; }
    void usdLocalToWorldTransform(fpreal64* val, exint size) const;

    const UsdTimeCode& frame() const { return m_frame; }
    fpreal intrinsicFrame() const { return m_frame.GetValue(); }
    void setFrame( const UsdTimeCode& frame );
    void setFrame( fpreal frame );

    GusdPurposeSet getPurposes() const { return m_purposes; }
    void setPurposes( GusdPurposeSet purposes );

    exint getNumPurposes() const;
    void getIntrinsicPurposes( UT_StringArray& purposes ) const;
    void setIntrinsicPurposes( const UT_StringArray& purposes );

    virtual GU_PackedFactory    *getFactory() const override;
    virtual GU_PackedImpl   *copy() const override;
    virtual void         clearData() override;

    virtual bool     isValid() const override;
    virtual bool     load(const UT_Options &options, const GA_LoadMap &map) override;
    virtual void     update(const UT_Options &options) override;
    virtual bool     save(UT_Options &options, const GA_SaveMap &map) const override;

    virtual bool     getBounds(UT_BoundingBox &box) const override;
    virtual bool     getRenderingBounds(UT_BoundingBox &box) const override;
    virtual void     getVelocityRange(UT_Vector3 &min, UT_Vector3 &max) const override;
    virtual void     getWidthRange(fpreal &min, fpreal &max) const override;

    virtual bool     getLocalTransform(UT_Matrix4D &m) const override;

    virtual bool     unpack(GU_Detail &destgdp) const override;
    virtual bool     unpackUsingPolygons(GU_Detail &destgdp) const override;

    bool visibleGT() const;   
    GT_PrimitiveHandle fullGT() const;

    // Return a structure that can be hashed to sort instances by prototype.
    bool getInstanceKey(UT_Options& key) const;
    
    /// Report memory usage (includes all shared memory)
    virtual int64 getMemoryUsage(bool inclusive) const override;

    /// Count memory usage using a UT_MemoryCounter in order to count
    /// shared memory correctly.
    virtual void countMemory(UT_MemoryCounter &counter, bool inclusive) const override;

    UsdPrim getUsdPrim(GusdUSD_PrimHolder::ScopedLock &lock,
                       GusdUT_ErrorContext* err = NULL) const;

    GusdUSD_StageProxyHandle getProxy() const;

    bool unpackGeometry( GU_Detail &destgdp,
                         const char* primvarPattern ) const;

    const UT_Matrix4D& getUsdTransform() const;
    
private:

    bool unpackPrim( 
            GU_Detail&              destgdp,
            UsdGeomImageable        prim, 
            const SdfPath&          primPath,
            const UT_Matrix4D&      xform,
            const GT_RefineParms&   rparms,
            bool                    addPathAttributes ) const;

    void resetCaches();
    void updateTransform();

    // intrinsics
    UT_StringHolder m_fileName;
    UT_StringHolder m_altFileName;
    SdfPath         m_srcPrimPath;
    int             m_index;
    SdfPath         m_primPath;
    UsdTimeCode     m_frame;
    GusdPurposeSet  m_purposes;

    mutable GusdUSD_PrimHolder  m_usdPrim;
    mutable GusdUSD_StageProxyHandle    m_stageProxy;

    // caches    
    mutable UT_BoundingBox      m_boundsCache;
    mutable bool                m_transformCacheValid;
    mutable UT_Matrix4D         m_transformCache;
    mutable GT_PrimitiveHandle  m_gtPrimCache;
    mutable bool                m_masterPathCacheValid;
    mutable std::string         m_masterPathCache;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_GU_PACKEDIMPL_H__
