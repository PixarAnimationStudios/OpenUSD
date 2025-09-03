//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"
#include "pxr/exec/vdf/grapher.h"
#include "pxr/exec/vdf/grapherOptions.h"
#include "pxr/exec/vdf/isolatedSubnetwork.h"
#include "pxr/exec/vdf/readIterator.h"
#include "pxr/exec/vdf/sparseInputPathFinder.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/staticTokens.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (select)
    (s1)
    (s2)
    (points)
    (out)
    (enable)
    (axis)
    ((pool, ".pool"))
);

TF_REGISTRY_FUNCTION(VdfExecutionTypeRegistry)
{
    VdfExecutionTypeRegistry::Define(false);
    VdfExecutionTypeRegistry::Define(GfVec3d(0));
}

static void
_Compute(const VdfContext &context) 
{
}

static VdfMask::Bits
_ComputeDependencies(
    const VdfMaskedOutput &maskedOutput,
    const VdfConnection &inputConnection)
{
    VdfMask::Bits bits(1);

    TF_AXIOM(maskedOutput.GetOutput()->GetName() == _tokens->out);
    TF_AXIOM(maskedOutput.GetMask().GetSize() == 2);

    const TfToken &in = inputConnection.GetTargetInput().GetName();

    if (in == _tokens->axis) {
        bits.SetAll();
    } else if (in == _tokens->enable && maskedOutput.GetMask().IsSet(1)) {
        bits.SetAll();
    }

    return bits;
}

void
BuildTestNetwork(
    VdfTestUtils::Network &graph, int style)
{
    VdfTestUtils::CallbackNodeType translateNodeType(&_Compute);
    translateNodeType
        .ReadWrite<GfVec3d>(_tokens->pool, _tokens->pool)
        .Read<GfVec3d>(_tokens->axis);

    VdfTestUtils::CallbackNodeType selectNodeType(&_Compute);
    selectNodeType
        .Read<GfVec3d>(_tokens->s1)
        .Read<GfVec3d>(_tokens->s2)
        .Read<bool>(_tokens->select)
        .Out<GfVec3d>(_tokens->out);

    VdfTestUtils::CallbackNodeType expressionNodeType(&_Compute);
    expressionNodeType
        .Read<bool>(_tokens->enable)
        .Read<GfVec3d>(_tokens->axis)
        .Out<GfVec3d>(_tokens->out);

    VdfTestUtils::CallbackNodeType expressionNodeTypeDependencies(&_Compute);
    expressionNodeTypeDependencies
        .Read<bool>(_tokens->enable)
        .Read<GfVec3d>(_tokens->axis)
        .Out<GfVec3d>(_tokens->out)
        .ComputeInputDependencyMaskCallback(&_ComputeDependencies);

    graph.AddInputVector<GfVec3d>("pool");
    graph.AddInputVector<GfVec3d>("axis1");
    graph.AddInputVector<GfVec3d>("axis2");
    graph.AddInputVector<GfVec3d>("disconnectedOutput");
    graph.AddInputVector<bool>("select1");
    graph.AddInputVector<bool>("select2");
    graph.AddInputVector<bool>("extraNode");
    graph.Add("translate1", translateNodeType);
    graph.Add("translate2", translateNodeType);
    graph.Add("selectNode1", selectNodeType);
    graph.Add("selectNode2", selectNodeType);
    graph.Add("expression1", expressionNodeType);
    graph.Add("expression2", expressionNodeTypeDependencies);

    VdfMask m01(2), m10(2);
    m01.SetIndex(0);
    m10.SetIndex(1);

    graph["pool"] >> graph["translate1"].In(_tokens->pool, VdfMask::AllOnes(2));
    graph["translate1"] >> graph["translate2"].In(_tokens->pool, VdfMask::AllOnes(2));

    graph["select1"] >> graph["selectNode1"].In(_tokens->select, VdfMask::AllOnes(1));
    graph["select2"] >> graph["selectNode2"].In(_tokens->select, VdfMask::AllOnes(1));

    graph["selectNode1"] >> graph["expression1"].In(_tokens->axis, VdfMask::AllOnes(1));
    graph["selectNode2"] >> graph["expression2"].In(_tokens->axis, VdfMask::AllOnes(1));

    graph["expression1"] >> graph["translate1"].In(_tokens->axis, VdfMask::AllOnes(1));
    graph["expression2"] >> graph["translate2"].In(_tokens->axis, m01);
    graph["expression2"] >> graph["selectNode1"].In(_tokens->s2, m10);

    graph["extraNode"] >> graph["expression2"].In(_tokens->enable, VdfMask::AllOnes(1));

    graph["axis1"] >> graph["selectNode1"].In(_tokens->s1, VdfMask::AllOnes(1));
    graph["axis1"] >> graph["selectNode2"].In(_tokens->s1, VdfMask::AllOnes(1));

    // Add connection to be disconnected on the source output side.
    graph["disconnectedOutput"] >> graph["selectNode1"].In(_tokens->s1, VdfMask::AllOnes(1));

    if (style == 0)
        graph["axis2"] >> graph["selectNode2"].In(_tokens->s2, VdfMask::AllOnes(1));
    else
        graph["expression1"] >> graph["selectNode2"].In(_tokens->s2, VdfMask::AllOnes(1));

    VdfMask mask10(2);
    mask10.SetIndex(0);
    VdfMask mask01(2);
    mask01.SetIndex(1);

    graph["translate1"].GetVdfNode()->GetOutput()->SetAffectsMask(mask10);
    graph["translate2"].GetVdfNode()->GetOutput()->SetAffectsMask(mask01);
}

static bool _enableSelectNodeDetection = false;

static
bool _InputCB(const VdfInput &input)
{
    // If _enableSelectNodeDetection is false we report interesting nodes.
    if (!_enableSelectNodeDetection)
        return false;

    return input.GetName() == _tokens->s1 ||
           input.GetName() == _tokens->s2;
}

// -----------------------------------------------------------------------------

static
std::string _PathToString(const VdfConnectionConstVector &path)
{
    std::string res;

    // Note that we stringify the path in reverse order for better 
    // readability.
    TF_REVERSE_FOR_ALL(i, path)
    {
        if (!res.empty())
            res += " | ";

        res += (*i)->GetDebugName();
    }

    return res;
}

static 
std::set<std::string> _PathVectorToStringSet(
    const std::vector<VdfConnectionConstVector> &paths)
{
    std::set<std::string> res;

    TF_FOR_ALL(i, paths)
        res.insert(_PathToString(*i));

    return res;
}

static bool
testPathFinderNoCycles(const VdfGrapherOptions &options)
{
    printf("\n*** Testing traversal in output-to-input direction, no cycle.\n");

    VdfTestUtils::Network testNetwork;
    BuildTestNetwork(testNetwork, 0 /* no cycles */);

    VdfMask oneOneMask(VdfMask::AllOnes(1));
    VdfMask mask10(2);
    VdfMask mask01(2);
    mask10.SetIndex(0);
    mask01.SetIndex(1);

    // Graph the network in order to understand the test, graph is viewable
    // from the mentor test result page.
    VdfGrapher::GraphToFile(testNetwork.GetNetwork(), "withoutCycles.dot", options);

    std::vector<VdfConnectionConstVector> paths;
    std::set<std::string>                 pathStrings;

    printf("\nSearching 'select2' from 'translate2' via 0b10, combining all paths..\n");
    paths.clear();
    VdfSparseInputPathFinder::Traverse(
        VdfMaskedOutput(testNetwork["translate2"].GetOutput(), mask10),
        VdfMaskedOutput(testNetwork["select2"].GetOutput(), oneOneMask),
        &_InputCB, &paths);
    TF_FOR_ALL(i, paths)
        printf(" - %s\n", _PathToString(*i).c_str());
    TF_AXIOM(paths.size() == 1);
    pathStrings = _PathVectorToStringSet(paths);
    TF_AXIOM(pathStrings.count(
        "VdfInputVector<bool> select2[out] -> "
        "[select]VdfTestUtils::DependencyCallbackNode selectNode2 | "
        "VdfTestUtils::DependencyCallbackNode selectNode2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode expression2 | "
        "VdfTestUtils::DependencyCallbackNode expression2[out] -> "
        "[s2]VdfTestUtils::DependencyCallbackNode selectNode1 | "
        "VdfTestUtils::DependencyCallbackNode selectNode1[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode expression1 | "
        "VdfTestUtils::DependencyCallbackNode expression1[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode translate1 | "
        "VdfTestUtils::DependencyCallbackNode translate1[.pool] -> "
        "[.pool]VdfTestUtils::DependencyCallbackNode translate2") == 1);

    printf("\nSearching 'select2' from 'translate2' via 0b01, combining all paths..\n");
    paths.clear();
    VdfSparseInputPathFinder::Traverse(
        VdfMaskedOutput(testNetwork["translate2"].GetOutput(), mask01),
        VdfMaskedOutput(testNetwork["select2"].GetOutput(), oneOneMask),
        &_InputCB, &paths);
    TF_FOR_ALL(i, paths)
        printf(" - %s\n", _PathToString(*i).c_str());
    TF_AXIOM(paths.size() == 1);
    pathStrings = _PathVectorToStringSet(paths);
    TF_AXIOM(pathStrings.count(
        "VdfInputVector<bool> select2[out] -> "
        "[select]VdfTestUtils::DependencyCallbackNode selectNode2 | "
        "VdfTestUtils::DependencyCallbackNode selectNode2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode expression2 | "
        "VdfTestUtils::DependencyCallbackNode expression2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode translate2") == 1);

    printf("\nSearching 'select2' from 'translate2' via 0b11, combining all paths.\n");
    // _enableSelectNodeDetection is false, so we expect only one path.
    paths.clear();
    VdfSparseInputPathFinder::Traverse(
        VdfMaskedOutput(testNetwork["translate2"].GetOutput(), VdfMask::AllOnes(2)),
        VdfMaskedOutput(testNetwork["select2"].GetOutput(), oneOneMask),
        &_InputCB, &paths);
    TF_FOR_ALL(i, paths)
        printf(" - %s\n", _PathToString(*i).c_str());
    TF_AXIOM(paths.size() == 1);
    pathStrings = _PathVectorToStringSet(paths);
    TF_AXIOM(pathStrings.count(
        "VdfInputVector<bool> select2[out] -> "
        "[select]VdfTestUtils::DependencyCallbackNode selectNode2 | "
        "VdfTestUtils::DependencyCallbackNode selectNode2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode expression2 | "
        "VdfTestUtils::DependencyCallbackNode expression2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode translate2") == 1);

    printf("\nSearching 'select2' from 'translate2' via 0b11, seperating all paths.\n");
    // _enableSelectNodeDetection is true, so we expect two paths.
    _enableSelectNodeDetection = true;
    paths.clear();
    VdfSparseInputPathFinder::Traverse(
        VdfMaskedOutput(testNetwork["translate2"].GetOutput(), VdfMask::AllOnes(2)),
        VdfMaskedOutput(testNetwork["select2"].GetOutput(), oneOneMask),
        &_InputCB, &paths);
    TF_FOR_ALL(i, paths)
        printf(" - %s\n", _PathToString(*i).c_str());
    TF_AXIOM(paths.size() == 2);
    pathStrings = _PathVectorToStringSet(paths);
    TF_AXIOM(pathStrings.count(
        "VdfInputVector<bool> select2[out] -> "
        "[select]VdfTestUtils::DependencyCallbackNode selectNode2 | "
        "VdfTestUtils::DependencyCallbackNode selectNode2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode expression2 | "
        "VdfTestUtils::DependencyCallbackNode expression2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode translate2") == 1);
    TF_AXIOM(pathStrings.count(
        "VdfInputVector<bool> select2[out] -> "
        "[select]VdfTestUtils::DependencyCallbackNode selectNode2 | "
        "VdfTestUtils::DependencyCallbackNode selectNode2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode expression2 | "
        "VdfTestUtils::DependencyCallbackNode expression2[out] -> "
        "[s2]VdfTestUtils::DependencyCallbackNode selectNode1 | "
        "VdfTestUtils::DependencyCallbackNode selectNode1[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode expression1 | "
        "VdfTestUtils::DependencyCallbackNode expression1[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode translate1 | "
        "VdfTestUtils::DependencyCallbackNode translate1[.pool] -> "
        "[.pool]VdfTestUtils::DependencyCallbackNode translate2") == 1);

    return true;
}

static bool
testPathFinderWithCycle(const VdfGrapherOptions &options)
{
    printf("\n*** Testing traversal in output-to-input direction, with cycle.\n");

    VdfTestUtils::Network testNetwork;
    BuildTestNetwork(testNetwork, 1 /* cycles */);

    VdfMask oneOneMask(VdfMask::AllOnes(1));
    VdfMask mask10(2);
    VdfMask mask01(2);
    mask10.SetIndex(0);
    mask01.SetIndex(1);

    // Graph the network in order to understand the test, graph is viewable
    // from the mentor test result page.
    VdfGrapher::GraphToFile(testNetwork.GetNetwork(), "withCycles.dot", options);

    std::vector<VdfConnectionConstVector> paths;
    std::set<std::string>                 pathStrings;

    // Disable select node detection and thus merge all paths together.
    printf("\nSearching 'select2' from 'translate2' via 0b01, seperating all paths.\n");
    _enableSelectNodeDetection = false;
    paths.clear();
    VdfSparseInputPathFinder::Traverse(
        VdfMaskedOutput(testNetwork["translate2"].GetOutput(), mask01),
        VdfMaskedOutput(testNetwork["select2"].GetOutput(), oneOneMask),
        &_InputCB, &paths);
    TF_FOR_ALL(i, paths)
        printf(" - %s\n", _PathToString(*i).c_str());
    TF_AXIOM(paths.size() == 1);
    pathStrings = _PathVectorToStringSet(paths);
    TF_AXIOM(pathStrings.count(
        "VdfInputVector<bool> select2[out] -> "
        "[select]VdfTestUtils::DependencyCallbackNode selectNode2 | "
        "VdfTestUtils::DependencyCallbackNode selectNode2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode expression2 | "
        "VdfTestUtils::DependencyCallbackNode expression2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode translate2") == 1);

    // Set _enableSelectNodeDetection is true, so we expect multiple paths.
    // This case checks that loops via select nodes are dealt with.
    printf("\nSearching 'select2' from 'translate2' via 0b01, combining all paths.\n");
    _enableSelectNodeDetection = true;
    paths.clear();
    VdfSparseInputPathFinder::Traverse(
        VdfMaskedOutput(testNetwork["translate2"].GetOutput(), mask01),
        VdfMaskedOutput(testNetwork["select2"].GetOutput(), oneOneMask),
        &_InputCB, &paths);
    TF_FOR_ALL(i, paths)
        printf(" - %s\n", _PathToString(*i).c_str());
    TF_AXIOM(paths.size() == 2);
    pathStrings = _PathVectorToStringSet(paths);
    TF_AXIOM(pathStrings.count(
        "VdfInputVector<bool> select2[out] -> "
        "[select]VdfTestUtils::DependencyCallbackNode selectNode2 | "
        "VdfTestUtils::DependencyCallbackNode selectNode2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode expression2 | "
        "VdfTestUtils::DependencyCallbackNode expression2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode translate2") == 1);
    TF_AXIOM(pathStrings.count(
        "VdfInputVector<bool> select2[out] -> "
        "[select]VdfTestUtils::DependencyCallbackNode selectNode2 | "
        "VdfTestUtils::DependencyCallbackNode selectNode2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode expression2 | "
        "VdfTestUtils::DependencyCallbackNode expression2[out] -> "
        "[s2]VdfTestUtils::DependencyCallbackNode selectNode1 | "
        "VdfTestUtils::DependencyCallbackNode selectNode1[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode expression1 | "
        "VdfTestUtils::DependencyCallbackNode expression1[out] -> "
        "[s2]VdfTestUtils::DependencyCallbackNode selectNode2 | "
        "VdfTestUtils::DependencyCallbackNode selectNode2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode expression2 | "
        "VdfTestUtils::DependencyCallbackNode expression2[out] -> "
        "[axis]VdfTestUtils::DependencyCallbackNode translate2") == 1);

    return true;
}

// -----------------------------------------------------------------------------

int 
main(int argc, char **argv) 
{
    VdfGrapherOptions options;
    options.SetDrawMasks(true);
    options.SetDrawAffectsMasks(true);
    options.SetPrintSingleOutputs(true);
    options.SetPageSize(-1, -1);

    if (!testPathFinderNoCycles(options))
        return -1;

    if (!testPathFinderWithCycle(options))
        return -1;

    //XXX: Test case for loop using irrelevant path, see that it isn't reported.

    //XXX: Same as above but make it relevant.

    //XXX: Test case that checks re-traversal works with existing path ids.

    return 0;
}

