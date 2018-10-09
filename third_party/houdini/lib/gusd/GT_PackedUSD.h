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
#ifndef __GUSD_GT_PACKEDUSD_H__
#define __GUSD_GT_PACKEDUSD_H__

#include "gusd/api.h"
#include "gusd/purpose.h"

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>

#include <GT/GT_GEOPrimCollect.h>
#include <GT/GT_Primitive.h>

class GU_PrimPacked;

PXR_NAMESPACE_OPEN_SCOPE

/// A GT implementation of a packed USD prim. 
///
/// This is a file backed prim that holds a reference to a prim in a usd file
/// at a particular frame. The prim can be a group of prims.
///
/// When we write USD packed prim to a USD file, we write a reference to the 
/// original file. USD has a limitation that it can only make references to root
/// nodes. 
///
/// When we write a reference into a USD file, we might want to use a different
/// file path than we use in the Houdini session. For example, we might want to
/// use a relative path vs. an absolute one. We might want to use a coalesced 
/// file vs per frame files. We don't want to enshrine this logic in the core so
/// we provide a second "auxilary" file name that can be used for this. If
/// this fileName is left empty, we just us the primary file name.
///

class GusdGT_PackedUSD : public GT_Primitive
{
public:
    GusdGT_PackedUSD( 
        const UT_StringHolder& fileName,
        const UT_StringHolder& auxFileName,
        const SdfPath& primPath,
        const SdfPath& srcPrimPath,
        exint index,
        UsdTimeCode frame,
        GusdPurposeSet purpose,	// Unused
        GT_AttributeListHandle pointAttributes,
        GT_AttributeListHandle vertexAttributes,
        GT_AttributeListHandle uniformAttributes,
        GT_AttributeListHandle detailAttributes,
        const GU_PrimPacked* prim );

    GusdGT_PackedUSD(const GusdGT_PackedUSD& other);

    virtual ~GusdGT_PackedUSD();
    
    // GT_Primitive interface --------------------------------------------------
public: 
    virtual const char* className() const override;

    virtual GT_PrimitiveHandle doSoftCopy() const override;

    GUSD_API
    static int getStaticPrimitiveType();

    virtual int getPrimitiveType() const override;
    
    virtual void    enlargeBounds(UT_BoundingBox boxes[],
                    int nsegments) const override;

    virtual int     getMotionSegments() const override;

    virtual int64   getMemoryUsage() const override;

    virtual const GT_AttributeListHandle& getPointAttributes() const override;
    virtual const GT_AttributeListHandle& getVertexAttributes() const override;
    virtual const GT_AttributeListHandle& getUniformAttributes() const override;
    virtual const GT_AttributeListHandle& getDetailAttributes() const override;

    // -------------------------------------------------------------------------

    // Get the name of the referenced USD file
    const UT_StringHolder& getFileName() const { return m_fileName; }

    // Get the name of the referenced USD file
    const UT_StringHolder& getAuxFileName() const { return m_auxFileName; }

    // Get the node path in the referenced USD file
    const SdfPath& getPrimPath() const { return m_primPath; }

    const SdfPath& getSrcPrimPath() const { return m_srcPrimPath; }

    const exint& getInstanceIndex() const { return m_instanceIndex; }

private:
    UT_StringHolder m_fileName;
    UT_StringHolder m_auxFileName;
    SdfPath         m_primPath;
    SdfPath         m_srcPrimPath;
    exint           m_instanceIndex;
    UsdTimeCode     m_frame;
    UT_BoundingBox  m_box;

    GT_AttributeListHandle  m_pointAttributes;
    GT_AttributeListHandle  m_vertexAttributes;
    GT_AttributeListHandle  m_uniformAttributes;
    GT_AttributeListHandle  m_detailAttributes;
};


//------------------------------------------------------------------------------
// class GusdGT_PackedUSDMesh
//------------------------------------------------------------------------------

/*
 * A utility class which supports meshes which have been coalesced together
 * for viewport efficiency. Based on GT_PackedAlembicMesh in the HDK.
 */

class GusdGT_PackedUSDMesh : public GT_Primitive
{
public:
    GusdGT_PackedUSDMesh(
            const GT_PrimitiveHandle& mesh,
            int64 id,
            UT_Array<GT_PrimitiveHandle>& sourceMeshes);

    virtual ~GusdGT_PackedUSDMesh();
    
    virtual const char* className() const override;

    static int getStaticPrimitiveType();

    virtual int getPrimitiveType() const override;
    
    virtual GT_PrimitiveHandle doSoftCopy() const override;

    virtual bool refine(
            GT_Refine& refiner,
            const GT_RefineParms* parms=nullptr) const override;

    virtual void enlargeBounds(
            UT_BoundingBox boxes[],
            int nsegments) const override;

    virtual int	getMotionSegments() const override;

    virtual int64 getMemoryUsage() const override;

    virtual bool getUniqueID(int64& id) const override;

private:
    GT_PrimitiveHandle m_mesh;
    int64 m_id;
    // Handles to uncoalesced meshes must be kept alive for viewport picking
    // to work correctly.
    UT_Array<GT_PrimitiveHandle> m_sourceMeshes;
};


//------------------------------------------------------------------------------
// class GusdGT_PrimCollect
//------------------------------------------------------------------------------

/*
 * A collector for Packed USD primitives which creates corresponding
 * GusdGT_PackedUSD prims
 */

// XXX This could be named something more explicit like GusdGT_PackedUSDCollect

class GusdGT_PrimCollect : public GT_GEOPrimCollect
{
public:
    GusdGT_PrimCollect() {}
    virtual ~GusdGT_PrimCollect();

    virtual GT_GEOPrimCollectData *
        beginCollecting(
            const GT_GEODetailListHandle &geometry,
            const GT_RefineParms *parms) const;

    virtual GT_PrimitiveHandle
        collect(
            const GT_GEODetailListHandle &geo,
            const GEO_Primitive *const* prim_list,
            int nsegments,
            GT_GEOPrimCollectData *data) const;

    virtual GT_PrimitiveHandle
        endCollecting(
            const GT_GEODetailListHandle &geometry,
            GT_GEOPrimCollectData *data) const;
};

PXR_NAMESPACE_CLOSE_SCOPE


#endif  // __GUSD_GU_PRIMCOLLECT_H__
