//
// Copyright 2019 Pixar
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
#include "agentUtils.h"

#include "error.h"
#include "GU_PackedUSD.h"
#include "GU_USD.h"
#include "stageCache.h"
#include "UT_Gf.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/tf/span.h"

#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdSkel/binding.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/topology.h"

#include <GA/GA_AIFIndexPair.h>
#include <GA/GA_AIFTuple.h>
#include <GA/GA_Handle.h>
#include <GA/GA_SplittableRange.h>
#include <GEO/GEO_AttributeCaptureRegion.h>
#include <GEO/GEO_AttributeIndexPairs.h>
#include <GEO/GEO_Detail.h>
#include <GU/GU_DetailHandle.h>
#include <GU/GU_PrimPacked.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_JSONWriter.h>
#include <UT/UT_ParallelUtil.h>


PXR_NAMESPACE_OPEN_SCOPE


// TODO: Encoding of namespaced properties is subject to change in
// future releases.
#define GUSD_SKEL_JOINTINDICES_ATTR "skel_jointIndices"_UTsh
#define GUSD_SKEL_JOINTWEIGHTS_ATTR "skel_jointWeights"_UTsh


namespace {


void
Gusd_ConvertTokensToStrings(const VtTokenArray& tokens,
                            UT_StringArray& strings)
{
    strings.setSize(tokens.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        strings[i] = GusdUSD_Utils::TokenToStringHolder(tokens[i]);
    }
}


/// Get names for each joint in \p skel, for use in a GU_AgentRig.
bool
Gusd_GetJointNames(const UsdSkelSkeleton& skel,
                   const VtTokenArray& joints,
                   VtTokenArray& jointNames)
{
    // Skeleton may optionally specify explicit joint names.
    // If so, use those instead of paths.
    if (skel.GetJointNamesAttr().Get(&jointNames)) {
        if (jointNames.size() != joints.size()) {
            GUSD_WARN().Msg("%s -- size of jointNames [%zu] "
                            "!= size of joints [%zu]",
                            skel.GetPrim().GetPath().GetText(),
                            jointNames.size(), joints.size());
            return false;
        }
    } else {
        // No explicit joint names authored.
        // Use the joint paths instead.
        // Although the path tokens could be converted to SdfPath objects,
        // and the tail of those paths could be extracted, they may not
        // be unique: uniqueness is only required for full joint paths.
        jointNames = joints;
    }
    return true;
}


/// Compute an ordered array giving the number of children for each
/// joint in \p topology.
bool
Gusd_GetChildCounts(const UsdSkelTopology& topology,
                    UT_IntArray& childCounts)
{
    childCounts.setSize(topology.GetNumJoints());
    childCounts.constant(0);
    for (size_t i = 0; i < topology.GetNumJoints(); ++i) {
        const int parent = topology.GetParent(i);
        if (parent >= 0) {
            UT_ASSERT_P(static_cast<size_t>(parent) < topology.GetNumJoints());
            ++childCounts[parent];
        }
    }
    return true;
}


/// Compute an ordered array of the children of all joints in \p topology.
bool
Gusd_GetChildren(const UsdSkelTopology& topology,
                 const UT_IntArray& childCounts,
                 UT_IntArray& children)
{
    UT_ASSERT_P(childCounts.size() == topology.GetNumJoints());

    const size_t numJoints = topology.GetNumJoints();

    // Create an array of (nextChild,numAdded) per joint.
    // This will be filled with (startIndex,0) for every joint,
    // then used to to populate the 'children' array.
    UT_Array<std::pair<exint,int> > childIters(numJoints, numJoints);
    exint startIndex = 0;
    exint numChildren = 0;
    for (exint i = 0; i < childCounts.size(); ++i) {
        childIters[i].first = startIndex;
        childIters[i].second = 0;
        startIndex += childCounts[i];
        numChildren += childCounts[i];
    }

    // Now use the iterators above to insert all children, in order.
    children.setSize(numChildren);
    for (size_t i = 0; i < numJoints; ++i) {
        int parent = topology.GetParent(i);
        if (parent >= 0) {
            exint childIndex = childIters[parent].first;
            children[childIndex] = static_cast<int>(i);
            ++childIters[parent].first;
            ++childIters[parent].second;

            if (!TF_VERIFY(childIters[parent].second <= childCounts[parent])) {
                return false;
            }
        }
    }
    return true;
}


} // namespace


GU_AgentRigPtr
GusdCreateAgentRig(const char* name, const UsdSkelSkeleton& skel)
{
    TRACE_FUNCTION();

    if (!skel) {
        TF_CODING_ERROR("'skel' is invalid");
        return nullptr;
    }

    VtTokenArray joints;
    if (!skel.GetJointsAttr().Get(&joints)) {
        GUSD_WARN().Msg("%s -- 'joints' attr is invalid",
                        skel.GetPrim().GetPath().GetText());
        return nullptr;
    }

    VtTokenArray jointNames;
    if (!Gusd_GetJointNames(skel, joints, jointNames)) {
        return nullptr;
    }

    UsdSkelTopology topology(joints);
    std::string reason;
    if (!topology.Validate(&reason)) {
        GUSD_WARN().Msg("%s -- invalid topology: %s",
                        skel.GetPrim().GetPath().GetText(),
                        reason.c_str());
        return nullptr;
    }
    return GusdCreateAgentRig(name, topology, jointNames);
}


GU_AgentRigPtr
GusdCreateAgentRig(const char* name,
                   const UsdSkelTopology& topology,
                   const VtTokenArray& jointNames)
{
    TRACE_FUNCTION();

    if (jointNames.size() != topology.GetNumJoints()) {
        TF_CODING_ERROR("jointNames size [%zu] != num joints [%zu]",
                        jointNames.size(), topology.GetNumJoints());
        return nullptr;
    }

    UT_IntArray childCounts;
    if (!Gusd_GetChildCounts(topology, childCounts)) {
        return nullptr;
    }
    UT_IntArray children;
    if (!Gusd_GetChildren(topology, childCounts, children)) {
        return nullptr;
    }

    UT_ASSERT(children.size() == jointNames.size());
    UT_ASSERT(std::accumulate(childCounts.begin(),
                              childCounts.end(), 0) == children.size());

    UT_StringArray names;
    Gusd_ConvertTokensToStrings(jointNames, names);

    // Add a __locomotion__ transform for root motion.
    names.append("__locomotion__");
    childCounts.append(0);

    const auto rig = GU_AgentRig::addRig(name);
    UT_ASSERT_P(rig);

    if (rig->construct(names, childCounts, children)) {
        return rig;
    } else {
        // XXX: Would be nice if we got a reasonable warning/error...
        GUSD_WARN().Msg("internal error constructing agent rig '%s'", name);
    }
    return nullptr;
}


namespace {


/// Create capture attrs on \p gd, in the form expected for LBS skinning.
/// This expects \p gd to have already imported 'primvars:skel:jointIndices'
/// and 'primvars:skel:jointWeights' -- as defined by the UsdSkelBindingAPI.
/// If \p deleteInfluencePrimvars=true, the original primvars imported for
/// UsdSkel are deleted after conversion.
bool
Gusd_CreateCaptureAttributes(
    GEO_Detail& gd,
    const VtMatrix4dArray& inverseBindTransforms,
    const VtTokenArray& jointNames,
    bool deleteInluencePrimvars=true,
    UT_ErrorSeverity sev=UT_ERROR_ABORT)
{
    TRACE_FUNCTION();

    // Expect to find the jointIndices/jointWeights properties already
    // imported onto the detail. We could query them from USD ourselves,
    // but then we would need to worry about things like winding order, etc.

    GA_ROHandleI jointIndicesHnd(&gd, GA_ATTRIB_POINT,
                                 GUSD_SKEL_JOINTINDICES_ATTR);
    if (jointIndicesHnd.isInvalid()) {
         GUSD_WARN().Msg("Could not find int skel_jointIndices attribute.");
        return false;
    }
    GA_ROHandleF jointWeightsHnd(&gd, GA_ATTRIB_POINT,
                                 GUSD_SKEL_JOINTWEIGHTS_ATTR);
    if (jointWeightsHnd.isInvalid()) {
         GUSD_WARN().Msg("Could not find float skel_jointWeights attribute.");
        return false;
    }
    if (jointIndicesHnd.getTupleSize() != jointWeightsHnd.getTupleSize()) {
         GUSD_WARN().Msg("Tuple size of skel_jointIndices [%d] != "
                        "tuple size of skel_JointWeights [%d]",
                        jointIndicesHnd.getTupleSize(),
                        jointWeightsHnd.getTupleSize());
        return false;
    }

    const int tupleSize = jointIndicesHnd.getTupleSize();
    const int numJoints = static_cast<int>(jointNames.size());

    int regionsPropId = -1;

    GA_RWAttributeRef captureAttr =
        gd.addPointCaptureAttribute(GEO_Detail::geo_NPairs(tupleSize));
    GA_AIFIndexPairObjects* joints =
        GEO_AttributeCaptureRegion::getBoneCaptureRegionObjects(
            captureAttr, regionsPropId);
    joints->setObjectCount(numJoints);


    // Set the names of each joint.
    {
        GEO_RWAttributeCapturePath jointPaths(&gd);
        for (int i = 0; i < numJoints; ++i) {
            // TODO: Elide the string copy.
            jointPaths.setPath(i, jointNames[i].GetText());
        }
    }

    // Store the inverse bind transforms of each joint.
    {
        const GfMatrix4d* xforms = inverseBindTransforms.cdata();
        for (int i = 0; i < numJoints; ++i) {

            GEO_CaptureBoneStorage r;
            r.myXform = GusdUT_Gf::Cast(xforms[i]);

            joints->setObjectValues(i, regionsPropId, r.floatPtr(),
                                    GEO_CaptureBoneStorage::tuple_size);
        }
    }

    // Copy weights and indices.
    const GA_AIFTuple* jointIndicesTuple = jointIndicesHnd->getAIFTuple();
    const GA_AIFTuple* jointWeightsTuple = jointWeightsHnd->getAIFTuple();

    const GA_AIFIndexPair* indexPair = captureAttr->getAIFIndexPair();
    indexPair->setEntries(captureAttr, tupleSize);

    UTparallelFor(
        GA_SplittableRange(gd.getPointRange()),
        [&](const GA_SplittableRange& r)
        {
            UT_FloatArray weights(tupleSize, tupleSize);
            UT_IntArray indices(tupleSize, tupleSize);

            auto* boss = UTgetInterrupt();
            char bcnt = 0;

            GA_Offset o,end;
            for (GA_Iterator it(r); it.blockAdvance(o,end); ) {
                if (ARCH_UNLIKELY(!++bcnt && boss->opInterrupt())) {
                    return;
                }

                for ( ; o < end; ++o) {
                    if (jointIndicesTuple->get(jointIndicesHnd.getAttribute(),
                                               o, indices.data(), tupleSize) &&
                        jointWeightsTuple->get(jointWeightsHnd.getAttribute(),
                                               o, weights.data(), tupleSize)) {

                        // Normalize in-place.
                        float sum = 0;
                        for (int c = 0; c < tupleSize; ++c)
                            sum += weights[c];
                        if (sum > 1e-6) {
                            for (int c = 0; c < tupleSize; ++c) {
                                weights[c] /= sum;
                            }
                        }

                        for (int c = 0; c < tupleSize; ++c) {
                            indexPair->setIndex(captureAttr, o, c, indices[c]);
                            indexPair->setData(captureAttr, o, c, weights[c]);
                        }
                    }
                }
            }
        });

    if (deleteInluencePrimvars) {
        gd.destroyPointAttrib(GUSD_SKEL_JOINTINDICES_ATTR);
        gd.destroyPointAttrib(GUSD_SKEL_JOINTWEIGHTS_ATTR);
    }
    return true;
}


bool
Gusd_ReadSkinnablePrims(const UsdSkelBinding& binding,
                        const VtTokenArray& jointNames,
                        const VtMatrix4dArray& invBindTransforms,
                        UsdTimeCode time,
                        const char* lod,
                        GusdPurposeSet purpose,
                        UT_ErrorSeverity sev,
                        const GT_RefineParms* refineParms,
                        UT_Array<GU_DetailHandle>& details)
{
    TRACE_FUNCTION();

    UT_AutoInterrupt task("Read USD shapes for shapelib");
    
    const size_t numTargets = binding.GetSkinningTargets().size();

    details.clear();
    details.setSize(numTargets);

    GusdErrorTransport errTransport;

    // Read in details for all skinning targets in parallel.
    UTparallelForHeavyItems(
        UT_BlockedRange<size_t>(0, numTargets),
        [&](const UT_BlockedRange<size_t>& r)
        {
            const GusdAutoErrorTransport autoErrTransport(errTransport);

            for (size_t i = r.begin(); i < r.end(); ++i) {

                if (task.wasInterrupted())
                    return;

                const UsdGeomImageable ip(
                    binding.GetSkinningTargets()[i].GetPrim());
                if (!ip) {
                    continue;
                }
                if (ip.ComputeVisibility(time) == UsdGeomTokens->invisible) {
                    continue;
                }
                if (!GusdPurposeInSet(ip.ComputePurpose(), purpose)) {
                    continue;
                }

                GU_DetailHandle gdh;
                gdh.allocateAndSet(new GU_Detail);

                const GU_DetailHandleAutoWriteLock gdl(gdh);
                
                if (GusdReadSkinnablePrim(
                        *gdl.getGdp(), binding.GetSkinningTargets()[i],
                        jointNames, invBindTransforms,
                        time, lod, purpose, sev, refineParms)) {
                    details[i] = gdh;
                } else if (sev >= UT_ERROR_ABORT) {
                    return;
                }
            }
        });

    return !task.wasInterrupted();
}


void
Gusd_InvertTransforms(TfSpan<GfMatrix4d> xforms)
{
    UTparallelForLightItems(
        UT_BlockedRange<size_t>(0, xforms.size()),
        [&](const UT_BlockedRange<size_t>& r)
        {
            for (size_t i = r.begin(); i < r.end(); ++i) {
                xforms[i] = xforms[i].GetInverse();
            }
        });
}


bool
Gusd_ReadSkinnablePrims(const UsdSkelBinding& binding,  
                        UsdTimeCode time,
                        const char* lod,
                        GusdPurposeSet purpose,
                        UT_ErrorSeverity sev,
                        const GT_RefineParms* refineParms,
                        UT_Array<GU_DetailHandle>& details)
{
    const UsdSkelSkeleton& skel = binding.GetSkeleton();

    VtTokenArray joints;
    if (!skel.GetJointsAttr().Get(&joints)) {
        GUSD_WARN().Msg("%s -- 'joints' attr is invalid",
                        skel.GetPrim().GetPath().GetText());
        return false;
    }
    VtTokenArray jointNames;
    if (!Gusd_GetJointNames(skel, joints, jointNames)) {
        return false;
    }

    VtMatrix4dArray invBindTransforms;
    if (!skel.GetBindTransformsAttr().Get(&invBindTransforms)) {
        GUSD_WARN().Msg("%s -- no authored bindTransforms",
                        skel.GetPrim().GetPath().GetText());
        return false;
    }
    if (invBindTransforms.size() != joints.size()) {
        GUSD_WARN().Msg("%s -- size of 'bindTransforms' [%zu] != "
                        "size of 'joints' [%zu].",
                        skel.GetPrim().GetPath().GetText(),
                        invBindTransforms.size(), joints.size());
        return false;
    }
    // XXX: Want *inverse* bind transforms when writing out capture data.
    Gusd_InvertTransforms(invBindTransforms);
    
    return Gusd_ReadSkinnablePrims(binding, jointNames, invBindTransforms, time,
                                   lod, purpose, sev, refineParms, details);
}


} // namespace


bool
GusdReadSkinnablePrims(const UsdSkelBinding& binding,
                       UT_Array<GU_DetailHandle>& details,
                       UsdTimeCode time,
                       const char* lod,
                       GusdPurposeSet purpose,
                       UT_ErrorSeverity sev,
                       const GT_RefineParms* refineParms)
{
    const UsdSkelSkeleton& skel = binding.GetSkeleton();

    VtTokenArray joints;
    if (!skel.GetJointsAttr().Get(&joints)) {
        GUSD_WARN().Msg("%s -- 'joints' attr is invalid",
                        skel.GetPrim().GetPath().GetText());
        return false;
    }
    VtTokenArray jointNames;
    if (!Gusd_GetJointNames(skel, joints, jointNames)) {
        return false;
    }

    VtMatrix4dArray invBindTransforms;
    if (!skel.GetBindTransformsAttr().Get(&invBindTransforms)) {
        GUSD_WARN().Msg("%s -- no authored bindTransforms",
                        skel.GetPrim().GetPath().GetText());
        return false;
    }
    if (invBindTransforms.size() != joints.size()) {
        GUSD_WARN().Msg("%s -- size of 'bindTransforms' [%zu] != "
                        "size of 'joints' [%zu].",
                        skel.GetPrim().GetPath().GetText(),
                        invBindTransforms.size(), joints.size());
        return false;
    }
    // XXX: Want *inverse* bind transforms when writing out capture data.
    Gusd_InvertTransforms(invBindTransforms);
    
    return Gusd_ReadSkinnablePrims(binding, jointNames, invBindTransforms, time,
                                   lod, purpose, sev, refineParms, details);
}


bool
GusdReadSkinnablePrim(GU_Detail& gd,
                      const UsdSkelSkinningQuery& skinningQuery,
                      const VtTokenArray& jointNames,
                      const VtMatrix4dArray& invBindTransforms,
                      UsdTimeCode time,
                      const char* lod,
                      GusdPurposeSet purpose,
                      UT_ErrorSeverity sev,
                      const GT_RefineParms* refineParms)
{
    TRACE_FUNCTION();

    // TODO: Support rigid deformations.
    // Should be trivial when constraining to a single joint,
    // but multi-joint rigid deformations might not be supported.
    if (skinningQuery.IsRigidlyDeformed()) {
        return false;
    }

    // Convert joint names in Skeleton order to the order specified
    // on this skinnable prim (if any).
    VtTokenArray localJointNames = jointNames;
    if (skinningQuery.GetMapper()) {
        if (!skinningQuery.GetMapper()->Remap(
                jointNames, &localJointNames)) {
            return false;
        }
    }

    const GfMatrix4d geomBindTransform = skinningQuery.GetGeomBindTransform();
    const UsdPrim& skinnedPrim = skinningQuery.GetPrim();
    const char* primvarPattern = "Cd skel:jointIndices skel:jointWeights";

    return (GusdGU_USD::ImportPrimUnpacked(
                gd, skinnedPrim, time, lod, purpose, primvarPattern,
                &GusdUT_Gf::Cast(geomBindTransform), refineParms) &&
            Gusd_CreateCaptureAttributes(
                gd, invBindTransforms, localJointNames, sev));
}


GU_AgentShapeLibPtr
GusdCreateAgentShapeLib(const UsdSkelBinding& binding,  
                        UsdTimeCode time,
                        const char* lod,
                        GusdPurposeSet purpose,
                        UT_ErrorSeverity sev,
                        const GT_RefineParms* refineParms)
{
    const UsdSkelSkeleton& skel = binding.GetSkeleton();

    // Read geom for each skinning target into its own detail.

    UT_Array<GU_DetailHandle> details;
    if (!Gusd_ReadSkinnablePrims(binding, time, lod, purpose,
                                 sev, refineParms, details)) {
        return nullptr;
    }

    const size_t numTargets = binding.GetSkinningTargets().size();
    UT_ASSERT_P(details.size() == numTargets);

    auto shapeLib =
        GU_AgentShapeLib::addLibrary(skel.GetPrim().GetPath().GetText());

    // Add the resulting details to the shape lib.
    for (size_t i = 0; i < numTargets; ++i) {
        if (const auto& gdh = details[i]) {
            const UsdPrim& prim = binding.GetSkinningTargets()[i].GetPrim();

            const UT_StringHolder name(prim.GetPath().GetString());
            shapeLib->addShape(name, gdh);
        }
    }
    return shapeLib;
}


namespace {

// TODO: This is the bottle neck in import.
bool
_CoalesceShapes(GEO_Detail& coalescedGd,
                const UT_Array<GU_DetailHandle>& details)
{
    UT_AutoInterrupt task("Coalesce shapes");

    for (const auto& gdh : details) {
        if (task.wasInterrupted())
            return false;

        const GU_DetailHandleAutoReadLock gdl(gdh);
        if (const GU_Detail* gdp = gdl.getGdp()) {
            coalescedGd.merge(*gdp);
        }
    }
    return !task.wasInterrupted();
}

} // namespace


bool
GusdCoalesceAgentShapes(GEO_Detail& gd,
                        const UsdSkelBinding& binding,
                        UsdTimeCode time,
                        const char* lod,
                        GusdPurposeSet purpose,
                        UT_ErrorSeverity sev,
                        const GT_RefineParms* refineParms)
{
    UT_Array<GU_DetailHandle> details;
    if (GusdReadSkinnablePrims(binding, details, time, lod,
                               purpose, sev, refineParms)) {
        return _CoalesceShapes(gd, details);
    }
    return false;
}


PXR_NAMESPACE_CLOSE_SCOPE
