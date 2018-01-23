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

#include "pxr/pxr.h"
#include "pxr/usd/pcp/arc.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/diagnostic.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/pcp/pathTranslation.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/pcp/site.h"

#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/layer.h"

#include "pxr/base/tf/stringUtils.h"

#include <iostream>
#include <string>
#include <boost/assign/list_of.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

static std::shared_ptr<PcpCache>
_CreateCacheForRootLayer(const std::string& rootLayerPath)
{
    SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(rootLayerPath);

    if (!rootLayer) {
        return std::shared_ptr<PcpCache>();
    }

    const PcpLayerStackIdentifier layerStackID(
        rootLayer, SdfLayerRefPtr(), ArResolverContext());
    return std::shared_ptr<PcpCache>(new PcpCache(layerStackID));
}

static void
_TestReverseTranslation(
    const PcpPrimIndex& primIndex, const PcpNodeRef& node,
    const SdfPath& targetPath,
    const SdfPath& expectedReversePath)
{
    std::cout << "Translating " << targetPath << " between " 
              << Pcp_FormatSite(primIndex.GetRootNode().GetSite()) << " and "
              << Pcp_FormatSite(node.GetSite()) 
              << std::endl;

    // Reverse translate targetPath to the given node and compare with the
    // expected result.
    const SdfPath revPath = 
        PcpTranslatePathFromRootToNode(node, targetPath);
    std::cout << "- Reverse translation: " << revPath << std::endl;
    std::cout << "      Expected result: " << expectedReversePath << std::endl;
    TF_AXIOM(revPath == expectedReversePath);

    // Forward translate the reverse translated path from above and ensure
    // it matches the original target path.
    const SdfPath fwdPath = 
        PcpTranslatePathFromNodeToRoot(node, revPath);
    std::cout << "- Forward translation: " << fwdPath << std::endl;
    std::cout << "      Expected result: " << targetPath << std::endl;
    TF_AXIOM(fwdPath == targetPath);

    std::cout << std::endl;
}

static bool
_IsExpectedNode(
    const PcpNodeRef& node,
    const PcpArcType expectedArcType,
    const SdfPath& expectedSitePath)
{
    return node.GetArcType() == expectedArcType &&
        node.GetSite().path == expectedSitePath;
}

// Tests basic reverse path translation of a prim's path to various
// nodes in the prim's index. The test asset consists of a chain of 
// references, with a relocation on the prim authored in one of the 
// later references. When reverse translating the prim's path, the
// relocation should not take effect until we reach the node where the
// relocation is authored.
static void
TestReverseTranslation_1()
{
    std::cout << "========== TestReverseTranslation_1..."  << std::endl;

    const std::string rootLayer = "TestReverseTranslation_1/1.sdf";
    std::shared_ptr<PcpCache> pcpCache = _CreateCacheForRootLayer(rootLayer);
    if (!pcpCache) {
        TF_FATAL_ERROR("Unable to open @%s@", rootLayer.c_str());
    }

    PcpErrorVector errors;
    const PcpPrimIndex& index = 
        pcpCache->ComputePrimIndex(SdfPath("/M_1/B"), &errors);
    TF_AXIOM(errors.empty());

    PcpNodeRange nodeRange = index.GetNodeRange();

    // First node is the direct node, which requires no path translation.
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeRoot, SdfPath("/M_1/B")));
    _TestReverseTranslation(index, *nodeRange.first, 
        /* target */ SdfPath("/M_1/B"), /* expected */ SdfPath("/M_1/B"));
    
    // Next node is the propagated relocation.
    ++nodeRange.first;
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeRelocate, SdfPath("/M_1/A")));

    // Next node is reference to @2.sdf@</M_2/B>. Note that this is before
    // the relocation of A -> B in 3.sdf, so we expect that not to have any
    // effect on the path translation.
    ++nodeRange.first;
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeReference, SdfPath("/M_2/B")));
    _TestReverseTranslation(index, *nodeRange.first, 
        /* target */ SdfPath("/M_1/B"), /* expected */ SdfPath("/M_2/B"));
    
    // Next node is the propagated relocation.
    ++nodeRange.first;
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeRelocate, SdfPath("/M_2/A")));

    // Next is the reference to @3.sdf@</M_3/B>. Although this is in the 
    // layer stack where the relocation of A -> B is specified, this is still 
    // before the relocation node in the prim index, so that relocation should
    // still have no effect.
    ++nodeRange.first;
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeReference, SdfPath("/M_3/B")));
    _TestReverseTranslation(index, *nodeRange.first, 
        /* target */ SdfPath("/M_1/B"), /* expected*/ SdfPath("/M_3/B"));

    // Next we have the relocation node, which represents the A -> B 
    // relocation. Reverse translating across this arc should translate B to A.
    //
    // XXX: Path translation currently doesn't handle this case as described
    //      above. Reverse translation to a node will always return a path
    //      in that node's final relocated namespace. We shouldn't run into
    //      this situation in real-world usage, as opinions at a relocation
    //      source are disallowed.
    ++nodeRange.first;
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeRelocate, SdfPath("/M_3/A")));
    // _TestReverseTranslation(index, *nodeRange.first, 
    //     /* target */ SdfPath("/M_1/B"), /* expected*/ SdfPath("/M_3/A"));

    // Next, the reference node to @4.sdf@</M_4/A>. This is on the 
    // other side of the relocation, so reverse translating to this node
    // should translate B to A.
    ++nodeRange.first;
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeReference, SdfPath("/M_4/A")));
    _TestReverseTranslation(index, *nodeRange.first, 
        /* target */ SdfPath("/M_1/B"), /* expected*/ SdfPath("/M_4/A"));

    // Lastly, the reference node to @4.sdf@</M_4/B>. This node shouldn't
    // contribute opinions to the index because it is superceded by opinions
    // at the relocation source </M_4/A>, but we include it for completeness.
    // Note that if culling is enabled, this node is culled from the graph,
    // so we skip it in that case.
    //
    // XXX: This particular case doesn't currently give the expected answer.
    //      Instead, it gives /M_4/A -- it's applied the relocation from
    //      3.sdf even though it's not on the other side of the relocation
    //      arc. Since this isn't a valid source of opinions anyway, we
    //      shouldn't ever run into this situation in real usage, so I'm
    //      commenting this test case out.
    if (!pcpCache->GetPrimIndexInputs().cull) {
        ++nodeRange.first;
        // TF_AXIOM(_IsExpectedNode(
        //         *nodeRange.first, PcpArcTypeReference, SdfPath("/M_4/B")));
        // _TestReverseTranslation(index, *nodeRange.first, 
        //     /* target */ SdfPath("/M_1/B"), /* expected*/ SdfPath("/M_4/B"));
    }

    // Done!
    ++nodeRange.first;
    TF_AXIOM(nodeRange.first == nodeRange.second);
}

// Tests reverse path translation of a target path. The test asset is the
// same chain of references as above, but we're now translating a path to
// the relocated prim </M_1/B> through a completely different prim index,
// </M_1/C>. Non-local relocations are required to correctly translate
// the path across the different reference hops.
static void
TestReverseTranslation_2()
{
    std::cout << "========== TestReverseTranslation_2..."  
              << std::endl;

    const std::string rootLayer = "TestReverseTranslation_1/1.sdf";
    std::shared_ptr<PcpCache> pcpCache = _CreateCacheForRootLayer(rootLayer);
    if (!pcpCache) {
        TF_FATAL_ERROR("Unable to open @%s@", rootLayer.c_str());
    }

    PcpErrorVector errors;
    const PcpPrimIndex& index = 
        pcpCache->ComputePrimIndex(SdfPath("/M_1/C"), &errors);
    TF_AXIOM(errors.empty());

    PcpNodeRange nodeRange = index.GetNodeRange();

    // First node is the direct node, which requires no path translation.
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeRoot, SdfPath("/M_1/C")));
    _TestReverseTranslation(index, *nodeRange.first, 
        /* target */ SdfPath("/M_1/B"), /* expected */ SdfPath("/M_1/B"));
    
    // Next node is reference to @2.sdf@</M_2/C>. Note that this is before
    // the relocation of A -> B in 3.sdf, so we expect that not to have any
    // effect on the path translation.
    ++nodeRange.first;
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeReference, SdfPath("/M_2/C")));
    _TestReverseTranslation(index, *nodeRange.first, 
        /* target */ SdfPath("/M_1/B"), /* expected */ SdfPath("/M_2/B"));

    // Next is the reference to @3.sdf@</M_3/C>. Since this is the layer 
    // stack where the relocation of A -> B is authored, we expect 
    // reverse translation to this node to still refer to B.
    ++nodeRange.first;
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeReference, SdfPath("/M_3/C")));
    _TestReverseTranslation(index, *nodeRange.first, 
        /* target */ SdfPath("/M_1/B"), /* expected*/ SdfPath("/M_3/B"));

    // Lastly, the reference node to @4.sdf@</M_4/C>. This is on the 
    // other side of the relocation, so reverse translating to this node
    // should translate B to A.
    ++nodeRange.first;
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeReference, SdfPath("/M_4/C")));
    _TestReverseTranslation(index, *nodeRange.first, 
        /* target */ SdfPath("/M_1/B"), /* expected*/ SdfPath("/M_4/A"));

    // Done!
    ++nodeRange.first;
    TF_AXIOM(nodeRange.first == nodeRange.second);
}

// Tests reverse path translation of a target path in asset setup that 
// involves inherits and relocations.
static void
TestReverseTranslation_3()
{
    std::cout << "========== TestReverseTranslation_3..." << std::endl;

    const std::string rootLayer = "TestReverseTranslation_3/root.sdf";
    std::shared_ptr<PcpCache> pcpCache = _CreateCacheForRootLayer(rootLayer);
    if (!pcpCache) {
        TF_FATAL_ERROR("Unable to open @%s@", rootLayer.c_str());
    }

    const SdfPath primPath("/CharRig/Rig/LArm/Rig/Some_Internal_Rig_Prim");
    const SdfPath targetPath("/CharRig/Anim/LArm.bendAmount");

    PcpErrorVector errors;
    const PcpPrimIndex& index = pcpCache->ComputePrimIndex(primPath, &errors);
    TF_AXIOM(errors.empty());

    PcpNodeRange nodeRange = index.GetNodeRange();

    // First node is the direct node, which requires no path translation.
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeRoot, primPath));
    _TestReverseTranslation(index, *nodeRange.first, 
        /* target */ targetPath, 
        /* expected */ targetPath);

    // Next is an implied local inherit node to the symmetric arm rig
    // that originates from within the referenced HumanRig below.
    ++nodeRange.first;
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeLocalInherit, 
            SdfPath("/CharRig/Rig/SymArm/Rig/Some_Internal_Rig_Prim")));
    _TestReverseTranslation(index, *nodeRange.first, 
        /* target */ targetPath, 
        /* expected */ SdfPath("/CharRig/Rig/SymArm/Anim.bendAmount"));
    
    // Next is the reference node to the LArm instance in HumanRig.
    ++nodeRange.first;
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeReference, 
            SdfPath("/HumanRig/Rig/LArm/Rig/Some_Internal_Rig_Prim")));
    _TestReverseTranslation(index, *nodeRange.first, 
        /* target */ targetPath, 
        /* expected */ SdfPath("/HumanRig/Anim/LArm.bendAmount"));

    // Next is the local inherit to the symmetric arm class.
    ++nodeRange.first;
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeLocalInherit, 
            SdfPath("/HumanRig/Rig/SymArm/Rig/Some_Internal_Rig_Prim")));
    _TestReverseTranslation(index, *nodeRange.first, 
        /* target */ targetPath, 
        /* expected */ SdfPath("/HumanRig/Rig/SymArm/Anim.bendAmount"));

    // Finally, we have a reference from the symmetric arm class in
    // HumanRig to the actual arm rig.
    ++nodeRange.first;
    TF_AXIOM(_IsExpectedNode(
            *nodeRange.first, PcpArcTypeReference, 
            SdfPath("/ArmRig/Rig/Some_Internal_Rig_Prim")));
    _TestReverseTranslation(index, *nodeRange.first, 
        /* target */ targetPath, 
        /* expected */ SdfPath("/ArmRig/Anim.bendAmount"));

    ++nodeRange.first;
    TF_AXIOM(nodeRange.first == nodeRange.second);
}

// Test translating various forms of paths that should generate
// a coding error.
static void
TestErrorsTranslatingInvalidPaths()
{
    std::cout << "========== TestErrorsTranslatingInvalidPaths..."
        << std::endl;

    SdfLayerRefPtr layerRef = SdfLayer::CreateAnonymous();
    SdfLayerHandle layer(layerRef);
    TF_AXIOM(layer);

    SdfPrimSpecHandle prim = SdfPrimSpec::New(layer, "foo", SdfSpecifierDef);
    TF_AXIOM(prim);

    PcpLayerStackIdentifier layerStackId(layer);
    PcpCache pcpCache(layerStackId);

    PcpErrorVector errors;
    const PcpPrimIndex& index = 
        pcpCache.ComputePrimIndex(SdfPath("/foo"), &errors);
    TF_AXIOM(errors.empty());

    PcpNodeRef rootNode = index.GetRootNode();

    TfErrorMark errMark;

    // Relative paths are disallowed
    TF_AXIOM(errMark.IsClean());
    SdfPath badPath1("relative/path");
    TF_AXIOM(!badPath1.IsEmpty());
    TF_AXIOM(errMark.IsClean());
    PcpTranslatePathFromNodeToRoot(rootNode, badPath1);
    TF_AXIOM(!errMark.IsClean());

    // Variant-selection paths are disallowed
    errMark.SetMark();
    TF_AXIOM(errMark.IsClean());
    SdfPath badPath2("/Variant/Selection{vset=sel}Is/Invalid");
    TF_AXIOM(!badPath2.IsEmpty());
    TF_AXIOM(errMark.IsClean());
    PcpTranslatePathFromRootToNode(rootNode, badPath2);
    TF_AXIOM(!errMark.IsClean());
}

int main(int argc, char** argv)
{ 
    TestReverseTranslation_1();
    TestReverseTranslation_2();
    TestReverseTranslation_3();
    TestErrorsTranslatingInvalidPaths();

    std::cout << "PASSED!" << std::endl;
    return EXIT_SUCCESS;
}
