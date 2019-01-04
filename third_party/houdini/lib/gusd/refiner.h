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
#ifndef __GUSD_REFINER_H__
#define __GUSD_REFINER_H__

#include "gusd/api.h"

#include <GT/GT_Refine.h>
#include <GT/GT_RefineParms.h>
#include <GU/GU_DetailHandle.h>
#include <UT/UT_SharedPtr.h>

#include <pxr/pxr.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/sdf/path.h>

#include "writeCtrlFlags.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class GusdRefiner
/// \brief Class used to refine GT prims so that they can be written to a USD file.
///
/// When we write a USD file, we create a GT_GEODetail prim from the current detail,
/// then refine it using a GusdRefiner. 
///
/// The basic idea is that the refiner looks at each prim, if it is a type that can 
/// be written to USD it adds it to the "gprim array",  if not it continues to refine it.
/// 
/// The refiner supports namespace hierarchy. Some prims types are added to the 
/// the gprim array and then add thier children as well. Packed prims do this. The
/// packed prim becomes a group node in USD. A PackedF3DGroup is similar.
///
/// The refiner calculates the primPath (location in the USD file). This can come from
/// an attribute on the prim being refined or it can be computed. The computed
/// path is based on a prefix provided by the client, a prim name and possibly
/// a hierarchy pf group names supplied by packed prims.
///
/// The gprim array can contain prims from several OBJ nodes. The obj nodes provide
/// a coordinate space and a set of options. We stash this stuff with the prims
/// in the prim array.


class GusdRefinerCollector;

class GUSD_API GusdRefiner : public GT_Refine 
{
public:

    // A struct representing GT prims refined to a USD prim.
    // localXform is the transform from the prim's space to its parent.
    // parentXform is the transform from the prim's parent's space to World.
    struct GprimArrayEntry {
        SdfPath             path;
        GT_PrimitiveHandle  prim;
        UT_Matrix4D         xform;
        TfToken             purpose;
        GusdWriteCtrlFlags  writeCtrlFlags;

        GprimArrayEntry() {}
        GprimArrayEntry( 
            const SdfPath&              path, 
            const GT_PrimitiveHandle&   prim,
            const UT_Matrix4D&          xform,
            const TfToken&              purpose,
            const GusdWriteCtrlFlags&   writeCtrlFlags )
                : path( path )
                , prim( prim )
                , xform( xform )
                , purpose(purpose)
                , writeCtrlFlags(writeCtrlFlags) {}
    };
    using GprimArray = std::vector<GprimArrayEntry>;

    ////////////////////////////////////////////////////////////////////////////

    /// Construct a refiner for refining the prims in a detail.
    /// Typically the ROP constructs a refiner for its cooked detail,
    /// and then as we process GT prims, if a GEO Packed Prim is encountered,
    /// We create a new refiner and recurse.
    /// We need to keep track of the transform as we recurse through packed prims. Note
    /// that we only write packed prims that have been tagged with a prim path. We
    /// kee track of the transform of the last group we wrote in parentToWorldXform
    /// \p localToWorldXform is initialized to the OBJ Node's transform by the ROP.
    GusdRefiner(
        GusdRefinerCollector&   collector,
        const SdfPath&          pathPrefix,
        const std::string&      pathAttrName,
        const UT_Matrix4D&      localToWorldXform );

    virtual ~GusdRefiner() {}

    virtual bool allowThreading() const override { return false; }

    virtual void addPrimitive( const GT_PrimitiveHandle& gtPrim ) override;

    void refineDetail( 
        const GU_ConstDetailHandle& detail,
        const GT_RefineParms&       parms  );

    const GprimArray& finish();

    //////////////////////////////////////////////////////////////////////////

    // If true, refine packed prims, otherwise return the prim on the 
    // prim array. This is set to false when we just want to capture
    // the prims transform.
    bool                    m_refinePackedPrims;

    // Use the "usdprimpath" intrinsic for the name of USD packed prims.
    // Used when writing overlays.
    bool                    m_useUSDIntrinsicNames;


    // Normally we only write geometry packed prims as groups if they have
    // been named. Force top level groups to always be written. This is so we 
    // can be assured we have a place to write instance ids.
    bool                    m_forceGroupTopPackedPrim;

    // Set true if we have usdinstancepath or instancepath set. If true and we
    // have packed usd, packed prims or points we will build a point instancer.
    bool                    m_buildPointInstancer;

    // If true, build prototypes which means ignoring the instancepath and not
    // building a point isntancer, and putting all geometry under the given
    // prototypes scope.
    bool                    m_buildPrototypes;

    // If we are overlaying a point instancer, this is set to the type of
    // of point instancer we need to overlay (old - "PxPointInstancer" or new
    // "PointInstancer").
    TfToken                 m_pointInstancerType;  

    GusdWriteCtrlFlags      m_writeCtrlFlags;

    /////////////////////////////////////////////////////////////////////////////

private:

    // Convert a prim's name into a prim path taking into account prefix and
    // modifying to be a valid Usd prim path.
    std::string createPrimPath( const std::string& primName);

    // Place to collect refined prims
    GusdRefinerCollector&   m_collector;

    // Refine parms are passed to refineDetail and then held on to.
    GT_RefineParms          m_refineParms; 

    // Prefix added to all relative prim paths.
    SdfPath                 m_pathPrefix;

    // The name of the attribute that specifies what USD object to write to.
    const std::string&      m_pathAttrName;  

    // The coordinate space accumulated as we recurse into packed geometry prims.
    UT_Matrix4D             m_localToWorldXform;

    // false if we have recursed into a packed prim.
    bool                    m_isTopLevel;
};

// As we recurse down a packed prim hierarchy, we create a new refiner at each
// level so we can carry the appropriate parametera. However, we need a object
// shared by all the refiners to collect the refined prims.
class GusdRefinerCollector {
public:

    using GprimArrayEntry = GusdRefiner::GprimArrayEntry;
    using GprimArray = GusdRefiner::GprimArray;

    // Struct used to keep names unique
    struct NameInfo {
        size_t firstIdx;    // index into gprim array of first use of name
        size_t count;       // number of times name has been used.

        NameInfo() : firstIdx( -1 ), count(0) {}
        NameInfo( size_t idx ) : firstIdx(idx), count( 0 ) {}
    };

    // Struct to store instance prims in.
    // A GT_PrimInstance may represent several point instancer array entries.
    // Index identifies which one.
    struct InstPrimEntry {
        GT_PrimitiveHandle  prim;
        size_t              index; 

        InstPrimEntry() : prim(NULL), index(0) {}
        InstPrimEntry( GT_PrimitiveHandle p, int i=0 ) : prim( p ), index(i) {}
    };

    ////////////////////////////////////////////////////////////////////////////

    SdfPath add( 
        const SdfPath&              path,
        bool                        explicitPrimPath,
        GT_PrimitiveHandle          prim,
        const UT_Matrix4D&          xform,
        const TfToken &             purpose,
        const GusdWriteCtrlFlags&   writeCtrlFlags );

    /// Add a prim to be added to a point instancer during finish
    void addInstPrim( const SdfPath& path, GT_PrimitiveHandle p, int index=0 );

    // Complete refining all prims.
    // When constructing point instancers, the refiner/collector gathers and 
    // holds on to all packed prims that are added. When finish is called, 
    // a GT_PrimPointMesh is created and added for each point instancer.
    void finish( GusdRefiner& refiner );

    ////////////////////////////////////////////////////////////////////////////

    // The results of the refine
    GusdRefiner::GprimArray m_gprims;

    // Map used to generate unique names for each prim
    std::map<SdfPath,NameInfo> m_names;

    // We can refine several point instancers in one session. They are partitioned
    // by a "srcPrimPath" intrinsic on USD packed prims. This map is used to 
    // sort the prims. If a prim does note have a srcPrimPath, it is added to 
    // a entry with a empty path.
    std::map<SdfPath,std::vector<InstPrimEntry>> m_instancePrims;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_GT_REFINER_H__
