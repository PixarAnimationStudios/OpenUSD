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
#ifndef _GUSD_GU_USD_H_
#define _GUSD_GU_USD_H_

#include <GA/GA_Attribute.h>
#include <GA/GA_Handle.h>
#include <GA/GA_Range.h>
#include <UT/UT_ErrorManager.h>

#include <pxr/pxr.h>

#include "gusd/USD_Proxy.h"
#include "gusd/USD_StageCache.h"
#include "gusd/USD_Traverse.h"
#include "gusd/USD_Utils.h"

class GA_AttributeFilter;
class GU_Detail;
class GU_PrimPacked;

PXR_NAMESPACE_OPEN_SCOPE

/** The default names of the USD ref attributes.
    @{ */
#define GUSD_PATH_ATTR          "usdpath"
#define GUSD_PRIMPATH_ATTR      "usdprimpath"
#define GUSD_FRAME_ATTR         "frame"
#define GUSD_VARIANTS_ATTR      "usdvariants"
#define GUSD_CONSTRAINT_ATTR    "usdconstraint"
/** @} */


/** Set of helpers for working with ranges of prims/points, etc.*/
class GusdGU_USD
{
public:
    /** Compute an array of offsets from a range.*/
    static bool OffsetArrayFromRange(const GA_Range& r,
                                     GA_OffsetArray& offsets);

    /** Compute an array mapping offset->range_index for the given range.*/
    static bool ComputeRangeIndexMap(const GA_Range& r,
                                     GA_OffsetArray& indexMap);

    /** Configuration for generic (packed or attribute) prim binding.*/
    struct BindOptions
    {   
        BindOptions();
        
        // Parms specific to binding to attributes.
        UT_StringHolder pathAttr, primPathAttr, variantsAttr, frameAttr;
        bool            packedPrims;    /*! Bind to packed prims.
                                            If false, bindings use attributes
                                            to determine prim references.*/
    };

    /** Convenience helper for binding a range of prims.*/
    struct PrimHandle
    {
        PrimHandle() {}
        PrimHandle(GusdUSD_StageCache& stageCache,
                   const ArResolverContext& resolverCtx)
            : cache(stageCache, resolverCtx) {}

        bool    Bind(const BindOptions& opts,
                     const GA_Detail& gd,
                     const GA_Range& rng,
                     UT_Array<SdfPath>* variants=NULL,
                     UT_Array<GusdPurposeSet>* purposes=NULL,
                     GusdUSD_Utils::PrimTimeMap* timeMap=NULL,
                     GusdUT_ErrorContext* err=NULL);

        GusdUSD_StageCacheContext           cache;
        GusdUSD_StageProxy::MultiAccessor   accessor;
        UT_Array<UsdPrim>                   prims;
        bool                                packedPrims;
    };

    static bool BindPrims(const BindOptions& opts,
                          const GA_Detail& gd,
                          GusdUSD_StageProxy::MultiAccessor& accessor,
                          UT_Array<UsdPrim>& prims,
                          const GA_Range& rng,
                          GusdUSD_StageCacheContext& cache,
                          UT_Array<SdfPath>* variants=NULL,
                          UT_Array<GusdPurposeSet>* purposes=NULL,
                          GusdUSD_Utils::PrimTimeMap* timeMap=NULL,
                          GusdUT_ErrorContext* err=NULL);

    /** Bind prims from references defined in the given attributes.
        This creates an entry in @a prims for each entry in the given range,
        mapped to the corresponding prim.
        If @a variants is non-null, resolved variant paths are stored
        in the given array.*/
    static bool BindPrimsFromAttrs(
                    GusdUSD_StageProxy::MultiAccessor& accessor,
                    UT_Array<UsdPrim>& prims,
                    const GA_Range& rng,
                    const GA_Attribute& pathAttr,
                    const GA_Attribute& primPathAttr,
                    const GA_Attribute* variantsAttr,
                    GusdUSD_StageCacheContext& cache,
                    UT_Array<SdfPath>* variants=NULL,
                    GusdUT_ErrorContext* err=NULL);

    static bool BindPrimsFromPackedPrims(
                    GusdUSD_StageProxy::MultiAccessor& accessor,
                    UT_Array<UsdPrim>& prims,
                    const GA_Range& rng,
                    GusdUSD_StageCacheContext& cache,
                    UT_Array<SdfPath>* variants=NULL,
                    UT_Array<GusdPurposeSet>* purposes = NULL,
                    GusdUT_ErrorContext* err=NULL);

    static bool GetTimeCodesFromAttr(
                    const GA_Range& rng,
                    const GA_Attribute& attr,
                    UT_Array<UsdTimeCode>& times,
                    GusdUT_ErrorContext* err=NULL);

    static bool GetTimeCodesFromPackedPrims(
                    const GA_Range& rng,
                    UT_Array<UsdTimeCode>& times,
                    GusdUT_ErrorContext* err=NULL);

    /** Given a string attribute that represents prim paths,
        return an array of actual prim paths.
        @{ */
    static bool GetPrimPathsFromStringAttr(const GA_Attribute& attr,
                                           UT_Array<SdfPath>& paths,
                                           GusdUT_ErrorContext* err=NULL);

    static bool GetPrimPathsFromStringAttr(const GA_Attribute& attr,
                                           const GA_Range& rng,
                                           UT_Array<SdfPath>& paths,
                                           GusdUT_ErrorContext* err=NULL);
    /** @} */

    
    /** Givena  string attribute, return an array of tokens.
        @{ */
    static bool GetTokensFromStringAttr(const GA_Attribute& attr,
                                        UT_Array<TfToken>& tokens,
                                        const char* nameSpace=NULL,
                                        GusdUT_ErrorContext* err=NULL);

    static bool GetTokensFromStringAttr(const GA_Attribute& attr,
                                        const GA_Range& rng,
                                        UT_Array<TfToken>& tokens,
                                        const char* nameSpace=NULL,
                                        GusdUT_ErrorContext* err=NULL);
    /** @} */


    /** Get stage proxies from attributes over the given range.
        If @a variants is non-null, resolved variant paths are stored
        in the given array.*/
    static bool GetStageProxiesFromAttrs(
                    const GA_Range& rng,
                    const GA_Attribute& pathAttr,
                    const GA_Attribute* variantsAttr,
                    UT_Array<GusdUSD_StageProxyHandle>& proxies,
                    GusdUSD_StageCacheContext& cache,
                    UT_Array<SdfPath>* variants=NULL,
                    GusdUT_ErrorContext* err=NULL);

    /** Append points to a detail that represent references to prims.
        The point offsets are contiguous, and the offset of the first
        point is returned. If any failures occur, and invalid
        offset is returned.*/
    static GA_Offset    AppendRefPoints(
                            GU_Detail& gd,
                            const UT_Array<UsdPrim>& prims,
                            const char* pathAttrName=GUSD_PATH_ATTR,
                            const char* primPathAttrName=GUSD_PRIMPATH_ATTR,
                            GusdUT_ErrorContext* err=NULL);

    typedef GU_PrimPacked* (*PackedPrimBuildFunc)( 
                                    GU_Detail&              detail,
                                    const UT_StringHolder&  fileName, 
                                    const SdfPath&          primPath, 
                                    const UsdTimeCode&      frame, 
                                    const char*             lod,
                                    const GusdPurposeSet    purposes );

    /** Register a function to be used by AppendPackedPrims to build
        a packed prim of the given type. */
    static void         RegisterPackedPrimBuildFunc( const TfToken& typeName,
                                                     PackedPrimBuildFunc func );

    /** Append packed prims to the given detail that reference the 
        given prims with the given variants. */
    static bool         AppendPackedPrims(
                            GU_Detail& gd,
                            const UT_Array<UsdPrim>& prims,
                            const UT_Array<SdfPath>& variants,
                            const GusdUSD_Utils::PrimTimeMap& timeMap,
                            const UT_StringArray& viewportLOD,
                            const UT_Array<GusdPurposeSet>& purposes,
                            GusdUT_ErrorContext* err=NULL);

    typedef GusdUSD_Traverse::PrimIndexPair PrimIndexPair;

    /** Append prims @a prims, as an expansion of prims defined on
        @a srcRange. The prim index pairs provide the prim found in
        the expansion, and the index of the prim in the source range
        whose expansion produced that prim.
        Attributes matching @a filter are copied from the source
        to the newly created ref points.*/
    static GA_Offset    AppendExpandedRefPoints(
                            GU_Detail& gd,
                            const GA_Detail& srcGd,
                            const GA_Range& srcRng,
                            const UT_Array<PrimIndexPair>& prims,
                            const GA_AttributeFilter& filter,
                            const char* pathAttrName=GUSD_PATH_ATTR,
                            const char* primPathAttrName=GUSD_PRIMPATH_ATTR,
                            GusdUT_ErrorContext* err=NULL);

    static bool         AppendExpandedPackedPrims(
                            GU_Detail& gd,
                            const GA_Detail& srcGd,
                            const GA_Range& srcRng,
                            const UT_Array<PrimIndexPair>& primIndexPairs,
                            const UT_Array<SdfPath>& variants,
                            const GusdUSD_Utils::PrimTimeMap& timeMap,
                            const GA_AttributeFilter& filter,
                            bool unpackToPolygons,
                            const UT_String& primvarPattern,
                            GusdUT_ErrorContext* err=NULL);

    /** Apply all variant selections in @a selections to each prim
        in the range, storing the resulting variant path in @a variantsAttr.
        For each source prim, this will first validate that the
        variant selection is valid on the target prims.
        If @a prevVariants is supplied, the variant selections are added
        on top of any variant selections in the given paths.*/
    static bool         WriteVariantSelectionsToAttr(
                            GU_Detail& gd,
                            const GA_Range& rng,
                            const UT_Array<UsdPrim>& prims,
                            const GusdUSD_Utils::VariantSelArray& selections,
                            const char* variantsAttr=GUSD_VARIANTS_ATTR,
                            const UT_Array<SdfPath>* prevVariants=NULL,
                            GusdUT_ErrorContext* err=NULL);

    static bool         WriteVariantSelectionsToPackedPrims(
                            GU_Detail& gd,
                            const GA_Range& rng,
                            const UT_Array<UsdPrim>& prims,
                            const GusdUSD_Utils::VariantSelArray& selections,
                            const UT_Array<SdfPath>* prevVariants=NULL,
                            GusdUT_ErrorContext* err=NULL);
                            
    /** Append variant selections defined by @a orderedVariants and
        @a variantIndices as an expansion of prims from @a srcRng.
        Attributes matching @a attrs filterare copied from the source
        to the newly created ref points.*/
    static GA_Offset    AppendRefPointsForExpandedVariants(
                            GU_Detail& gd,
                            const GA_Detail& srcGd,
                            const GA_Range& srcRng,
                            const UT_Array<UT_StringHolder>& orderedVariants,
                            const GusdUSD_Utils::IndexPairArray& variantIndices,
                            const GA_AttributeFilter& filter,
                            const char* variantsAttr=GUSD_VARIANTS_ATTR,
                            GusdUT_ErrorContext* err=NULL);

    static GA_Offset    AppendPackedPrimsForExpandedVariants(
                            GU_Detail& gd,
                            const GA_Detail& srcGd,
                            const GA_Range& srcRng,
                            const UT_Array<UT_StringHolder>& orderedVariants,
                            const GusdUSD_Utils::IndexPairArray& variantIndices,
                            const GA_AttributeFilter& filter,
                            GusdUT_ErrorContext* err=NULL);

    /** Copy attributes from a source to dest range.*/
    static bool         CopyAttributes(
                            const GA_Range& srcRng,
                            const GA_Range& dstRng,
                            const GA_IndexMap& dstMap,
                            const UT_Array<const GA_Attribute*>& attrs);

    static bool GetPackedPrimViewportLODAndPurposes(
                            const GA_Detail& gd,
                            const GA_OffsetArray& offsets,
                            UT_StringArray& viewportLOD,
                            UT_Array<GusdPurposeSet>& purposes,
                            GusdUT_ErrorContext* err=NULL);

    /** Compute world transforms from attributes over an array of offsets.

        In addition to using the standard instancing attributes,
        this supports an additional schema for non-orthonormal transforms,
        where the basis vectors (rows) of a rotation matrix are stored
        as normal attributes 'i', 'j', 'k'.*/
    static bool ComputeTransformsFromAttrs(const GA_Detail& gd,
                                           GA_AttributeOwner owner,
                                           const GA_OffsetArray& offsets,
                                           UT_Matrix4D* xforms);

    static bool ComputeTransformsFromPackedPrims(const GA_Detail& gd,
                                                 const GA_OffsetArray& offsets,
                                                 UT_Matrix4D* xforms,
                                                 GusdUT_ErrorContext* err=NULL);

    /** Support representations of attributes when authoring new transforms.*/
    enum OrientAttrRepresentation
    {
        ORIENTATTR_ORIENT,  //! quaternion orient.
        ORIENTATTR_IJK,     //! vec3 i,j,k (can be non-orthogonal).
        ORIENTATTR_IGNORE
    };

    enum ScaleAttrRepresentation
    {
        SCALEATTR_SCALE,    //! scale (vec3).
        SCALEATTR_PSCALE,   //! single pscale.
        SCALEATTR_IGNORE
    };

    /** Create and set transform attributes over the given range.
        The @a indexMap maps offset->range_index, as computed by
        ComputeRangeIndexMap().*/
    static bool SetTransformAttrs(GU_Detail& gd,
                                  const GA_Range& r,
                                  const GA_OffsetArray& indexMap,
                                  OrientAttrRepresentation orientRep,
                                  ScaleAttrRepresentation scaleRep,
                                  const UT_Matrix4D* xforms,
                                  GusdUT_ErrorContext* err=NULL);

    static bool SetPackedPrimTransforms(GU_Detail& gd,
                                        const GA_Range& r,
                                        const UT_Matrix4D* xforms,
                                        GusdUT_ErrorContext* err=NULL);
 
    static bool MultTransformableAttrs(GU_Detail& gd,
                                       const GA_Range& r,
                                       const GA_OffsetArray& indexMap,
                                       const UT_Matrix4D* xforms,
                                       bool keepLengths=false,
                                       const GA_AttributeFilter* filter=NULL);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_GU_USD_H_*/
