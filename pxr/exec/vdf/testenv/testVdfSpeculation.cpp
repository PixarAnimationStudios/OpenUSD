//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"
#include "pxr/exec/vdf/readIterator.h"
#include "pxr/exec/vdf/readWriteIterator.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/simpleExecutor.h"
#include "pxr/exec/vdf/speculationExecutor.h"
#include "pxr/exec/vdf/speculationNode.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/staticTokens.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (axis)
    ((pool, ".pool"))
    (speculation)
    
);

TF_REGISTRY_FUNCTION(VdfExecutionTypeRegistry)
{
    VdfExecutionTypeRegistry::Define(GfVec3d(0));
}

static void
TranslatePoints(const VdfContext &context) 
{
    const GfVec3d &axis = context.GetInputValue<GfVec3d>(_tokens->axis);
    VdfReadWriteIterator<GfVec3d> iter(context, _tokens->pool);
    for ( ; !iter.IsAtEnd(); ++iter) {
        *iter += axis;
    }
}

// Create a dummy speculation node, with no inputs and no outputs. 
// The sole purpose of this node is for it to be used to initialize 
// a SpeculationExecutor
static VdfSpeculationNode*
CreateDummySpeculationNode(VdfNetwork *network)
{
    VdfInputSpecs inputSpecs;
    VdfOutputSpecs outputSpecs;
    return new VdfSpeculationNode(network, inputSpecs, outputSpecs);
}

VdfNode *
BuildTestNetwork1(VdfTestUtils::Network &graph)
{
    // We're going to build a network like this:
    //                                       ._____.
    //           points (2 points)           |     |
    //             |                    speculate  |
    //             | [01]   ______[10]____/        |
    //             |       /                       |
    //            translate1                      [10]
    //             |        axis                   |
    //             | [10]  /                       |
    //             |      /                       / 
    //            translate2                     / 
    //             |____________________________/
    //
    //              

    graph.AddInputVector<GfVec3d>("points", 2);    
    graph["points"]
        .SetValue(0, GfVec3d(1, 0, 0))
        .SetValue(1, GfVec3d(0, 1, 0));

    graph.AddInputVector<GfVec3d>("axis");
    graph["axis"]
        .SetValue(0, GfVec3d(1, 0, 0));

    VdfMask point1Mask(2);
    VdfMask point2Mask(2);
    point1Mask.SetIndex(0);
    point2Mask.SetIndex(1);

    VdfMask oneOneMask = VdfMask::AllOnes(1);
    VdfMask twoOnesMask = VdfMask::AllOnes(2);

    VdfTestUtils::CallbackNodeType translateNodeType(&TranslatePoints);
    translateNodeType
        .ReadWrite<GfVec3d>(_tokens->pool, _tokens->pool)
        .Read<GfVec3d>(_tokens->axis);

    graph.Add("translate1", translateNodeType);
    graph.Add("translate2", translateNodeType);

    // Create a speculation node with a matched input/output pair for the
    // single attribute we're speculating about
    VdfInputSpecs inputSpecs;
    VdfOutputSpecs outputSpecs;
    // name of the input and output need to match
    inputSpecs.ReadConnector(TfType::Find<GfVec3d>(), _tokens->speculation);
    outputSpecs.Connector(TfType::Find<GfVec3d>(), _tokens->speculation);
    graph.Add("speculate", new VdfSpeculationNode(
                  &graph.GetNetwork(), inputSpecs, outputSpecs));

    graph["points"]     >> graph["translate1"].In(_tokens->pool, twoOnesMask);
    graph["translate1"].GetVdfNode()->GetOutput()->SetAffectsMask(point2Mask);
    graph["speculate"]  >> graph["translate1"].In(_tokens->axis, point1Mask);

    graph["translate1"] >> graph["translate2"].In(_tokens->pool, twoOnesMask);
    graph["translate2"].GetVdfNode()->GetOutput()->SetAffectsMask(point1Mask);
    graph["axis"]       >> graph["translate2"].In(_tokens->axis, oneOneMask);

    graph["translate2"] >> graph["speculate"].In(_tokens->speculation, 
                                                 point1Mask);

    return graph["translate2"];
}

static bool
testBasicSpeculation()
{
    std::cout << std::endl << "Testing basic speculation..." << std::endl;
    VdfTestUtils::Network graph;

    VdfNode *output = BuildTestNetwork1(graph);

    ////////////////////////////////////////////////////////////////////////

    VdfMask allOnes(2);
    allOnes.SetAll();
    VdfRequest request(VdfMaskedOutput( output->GetOutput(), allOnes ) );

    VdfSchedule schedule;
    VdfScheduler::Schedule(request, &schedule, true /*topologicalSort*/);

    // We use a speculation executor here, instead of the simple executor. Due
    // to the changes to scheduling speculation node inputs, the simple
    // executor can no longer solely rely on the topological ordering of
    // nodes. We use the speculation executor, since it is a pull based
    // executor defined in Vdf. Ideally, we would like to use a
    // SimplePullBasedExecutor instead, or do away with the topological
    // ordering all together.
    VdfSpeculationNode *dummy = CreateDummySpeculationNode(&graph.GetNetwork());
    VdfSimpleExecutor parentExec;
    parentExec.Resize(*schedule.GetNetwork());
    std::unique_ptr<VdfSpeculationExecutorBase> exec(
        VdfTestUtils::CreateSpeculationExecutor(dummy, &parentExec));
    exec->Run(schedule);

    GfVec3d result1 =
        exec->GetOutputValue(*output->GetOutput(_tokens->pool), allOnes)
                                ->GetReadAccessor<GfVec3d>()[0];
    GfVec3d result2 =
        exec->GetOutputValue(*output->GetOutput(_tokens->pool), allOnes)
                                ->GetReadAccessor<GfVec3d>()[1];

    std::cout << "Results are: " << std::endl;
    std::cout << "\tpoint 1  = " << result1 << std::endl;
    std::cout << "\tpoint 2  = " << result2 << std::endl;

    GfVec3d expected1 = GfVec3d(2, 0, 0);
    GfVec3d expected2 = GfVec3d(2, 1, 0);

    if (result1 != expected1 || result2 != expected2) {
        std::cout << "Expected: " << std::endl;
        std::cout << "\tpoint 1  = " << expected1 << std::endl;
        std::cout << "\tpoint 2  = " << expected2 << std::endl;
        std::cout << "TEST FAILED" << std::endl;
        return false; // test failed!
    } else {
        std::cout << "as expected." << std::endl;
    }
    return true;
}

// ---------------------------------------------------------------------------

VdfNode *
BuildNestedSpeculationTestNetwork(VdfTestUtils::Network &graph)
{
    // We're going to build a network like this:
    //                                       ._____.
    //           points (3 points)           |     |
    //             |                    speculate1 |
    //             | [010]  ___[100]____/          |
    //             |       /                       |
    //            translate1               .____.  |
    //             |                      /     |  |
    //             |                 speculate2 |  |
    //             | [100] ___[001]__/         /   |
    //             |      /                   /    |
    //            translate2                 /     |
    //             |                        /    [100]
    //             |                       /       |
    //             | [001]  axis        [001]      |
    //             |        /            /         |
    //             |       /            /          |
    //            translate3           /           |
    //             |\_________________/            |
    //             |        axis                   |
    //             | [100] /                       |
    //             |      /                       / 
    //            translate4                     / 
    //             |____________________________/
    //
    //              

    graph.AddInputVector<GfVec3d>("points", 3);
    graph["points"]
        .SetValue(0, GfVec3d(1, 0, 0))
        .SetValue(1, GfVec3d(0, 1, 0))
        .SetValue(2, GfVec3d(0, 0, 1));

    graph.AddInputVector<GfVec3d>("axis");
    graph["axis"]
        .SetValue(0, GfVec3d(1, 0, 0));

    VdfMask point1Mask(3);
    VdfMask point2Mask(3);
    VdfMask point3Mask(3);
    point1Mask.SetIndex(0);
    point2Mask.SetIndex(1);
    point3Mask.SetIndex(2);

    VdfMask oneOneMask = VdfMask::AllOnes(1);
    VdfMask threeOnesMask = VdfMask::AllOnes(3);

    VdfTestUtils::CallbackNodeType translateNodeType(&TranslatePoints);
    translateNodeType
        .ReadWrite<GfVec3d>(_tokens->pool, _tokens->pool)
        .Read<GfVec3d>(_tokens->axis);

    graph.Add("translate1", translateNodeType);
    graph.Add("translate2", translateNodeType);
    graph.Add("translate3", translateNodeType);
    graph.Add("translate4", translateNodeType);

    // Create a speculation node with a matched input/output pair for the
    // single attribute we're speculating about
    VdfInputSpecs inputSpecs;
    VdfOutputSpecs outputSpecs;
    // name of the input and output need to match
    inputSpecs.ReadConnector(TfType::Find<GfVec3d>(), _tokens->speculation);
    outputSpecs.Connector(TfType::Find<GfVec3d>(), _tokens->speculation);
    graph.Add("speculate1", new VdfSpeculationNode(
                  &graph.GetNetwork(), inputSpecs, outputSpecs));
    graph.Add("speculate2", new VdfSpeculationNode(
                  &graph.GetNetwork(), inputSpecs, outputSpecs));

    // This axiom is to code cover VdfNode::_IsDerivedEqual()
    TF_AXIOM(!graph["speculate1"].GetVdfNode()->IsEqual(
                            *graph["speculate2"].GetVdfNode()));

    graph["points"]      >> graph["translate1"].In(_tokens->pool, threeOnesMask);
    graph["translate1"].GetVdfNode()->GetOutput()->SetAffectsMask(point2Mask);
    graph["speculate1"]  >> graph["translate1"].In(_tokens->axis, point1Mask);

    graph["translate1"]  >> graph["translate2"].In(_tokens->pool, threeOnesMask);
    graph["translate2"].GetVdfNode()->GetOutput()->SetAffectsMask(point1Mask);
    graph["speculate2"]  >> graph["translate2"].In(_tokens->axis, point3Mask);
 
    graph["translate2"]  >> graph["translate3"].In(_tokens->pool, threeOnesMask);
    graph["translate3"].GetVdfNode()->GetOutput()->SetAffectsMask(point3Mask);
    graph["axis"]        >> graph["translate3"].In(_tokens->axis, oneOneMask);

    graph["translate3"]  >> graph["translate4"].In(_tokens->pool, threeOnesMask);
    graph["translate4"].GetVdfNode()->GetOutput()->SetAffectsMask(point1Mask);
    graph["axis"]        >> graph["translate4"].In(_tokens->axis, oneOneMask);

    graph["translate3"]  >> graph["speculate2"].In(_tokens->speculation, 
                                                   point3Mask);
    graph["translate4"]  >> graph["speculate1"].In(_tokens->speculation,
                                                   point1Mask);

    return graph["translate4"];
}

static bool
testNestedSpeculation()
{
    std::cout << std::endl << "Testing nested speculation..." << std::endl;
    VdfTestUtils::Network graph;

    VdfNode *output = BuildNestedSpeculationTestNetwork(graph);

    VdfMask allOnes(3);
    allOnes.SetAll();
    VdfRequest request(VdfMaskedOutput( output->GetOutput(), allOnes ) );

    VdfSchedule schedule;
    VdfScheduler::Schedule(request, &schedule, true /* topologicalSort */);

    // We use a speculation executor here, instead of the simple executor. Due
    // to the changes to scheduling speculation node inputs, the simple
    // executor can no longer solely rely on the topological ordering of
    // nodes. We use the speculation executor, since it is a pull based
    // executor defined in Vdf. Ideally, we would like to use a
    // SimplePullBasedExecutor instead, or do away with the topological
    // ordering all together.
    VdfSpeculationNode *dummy = CreateDummySpeculationNode(&graph.GetNetwork());
    VdfSimpleExecutor parentExec;
    parentExec.Resize(*schedule.GetNetwork());
    std::unique_ptr<VdfSpeculationExecutorBase> exec(
        VdfTestUtils::CreateSpeculationExecutor(dummy, &parentExec));
    exec->Run(schedule);

    GfVec3d result1 =
        exec->GetOutputValue(*output->GetOutput(_tokens->pool), allOnes)
                                ->GetReadAccessor<GfVec3d>()[0];
    GfVec3d result2 =
        exec->GetOutputValue(*output->GetOutput(_tokens->pool), allOnes)
                                ->GetReadAccessor<GfVec3d>()[1];
    GfVec3d result3 =
        exec->GetOutputValue(*output->GetOutput(_tokens->pool), allOnes)
                                ->GetReadAccessor<GfVec3d>()[2];

    std::cout << "Results are: " << std::endl;
    std::cout << "\tpoint 1  = " << result1 << std::endl;
    std::cout << "\tpoint 2  = " << result2 << std::endl;
    std::cout << "\tpoint 3  = " << result3 << std::endl;

    GfVec3d expected1 = GfVec3d(3, 0, 1);
    GfVec3d expected2 = GfVec3d(3, 1, 1);
    GfVec3d expected3 = GfVec3d(1, 0, 1);

    if (result1 != expected1 || result2 != expected2 || result3 != expected3) {
        std::cout << "Expected: " << std::endl;
        std::cout << "\tpoint 1  = " << expected1 << std::endl;
        std::cout << "\tpoint 2  = " << expected2 << std::endl;
        std::cout << "\tpoint 3  = " << expected3 << std::endl;
        std::cout << "TEST FAILED" << std::endl;
        return false; // test failed!
    } else {
        std::cout << "as expected." << std::endl;
    }
    return true;
}

// ---------------------------------------------------------------------------

int 
main(int argc, char **argv) 
{
    if (!testBasicSpeculation()) {
        return -1;
    }

    if (!testNestedSpeculation()) {
        return -1;
    }

    return 0;
}
