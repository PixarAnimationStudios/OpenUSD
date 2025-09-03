//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/grapher.h"
#include "pxr/exec/vdf/grapherOptions.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/networkUtil.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

#include <fstream>
#include <functional>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (axis)
    (moves)
    (out)
    (out1)
    (out2)
);

static void
CallbackFunction(const VdfContext &context) 
{
}

VdfNode *
BuildTestNetwork(VdfTestUtils::Network &graph, VdfSchedule *schedule) 
{
    VdfMask bigMask = VdfMask::AllOnes(100);
    VdfMask littleMask(2); 
    littleMask.SetIndex(1);

    // We're going to build a network like this:
    //
    //          GN1  GN2  
    //          |\   /| 
    //          | MON |
    //          | / \ |
    //          TN1  TN2 
    //           \   /
    //            TN3
    //            

    VdfTestUtils::CallbackNodeType generatorType(&CallbackFunction);
    generatorType
        .Out<int>(_tokens->out)
        ;

    VdfTestUtils::CallbackNodeType multipleOutputType(&CallbackFunction);
    multipleOutputType
        .Read<int>(_tokens->axis)
        .Read<int>(_tokens->moves)
        .Out<int>(_tokens->out1)
        .Out<int>(_tokens->out2)
        ;

    VdfTestUtils::CallbackNodeType translateType(&CallbackFunction);
    translateType
        .Read<int>(_tokens->axis)
        .ReadWrite<int>(_tokens->moves, _tokens->out)
        ;

    graph.Add("gn1", generatorType);
    graph.Add("gn2", generatorType);
    graph.Add("mon", multipleOutputType);
    graph.Add("tn1", translateType);
    graph.Add("tn2", translateType);
    graph.Add("tn3", translateType);

    graph["gn1"] >> graph["mon"].In(_tokens->axis, littleMask);
    graph["gn1"] >> graph["tn1"].In(_tokens->axis, littleMask);

    graph["gn2"] >> graph["mon"].In(_tokens->moves, bigMask);
    graph["gn2"] >> graph["tn2"].In(_tokens->moves, bigMask);

    graph["mon"].Output(_tokens->out1) >> 
        graph["tn1"].In(_tokens->moves, littleMask);
    graph["mon"].Output(_tokens->out2) >> 
        graph["tn2"].In(_tokens->axis, littleMask);

    graph["tn1"] >> graph["tn3"].In(_tokens->axis, bigMask);
    graph["tn2"] >> graph["tn3"].In(_tokens->moves, bigMask);

    VdfRequest request( 
        VdfMaskedOutput( graph["tn3"].GetVdfNode()->GetOutput(), bigMask ) );
    VdfScheduler::Schedule(request, schedule, true /* topologicalSort */);

    return graph["tn3"];
}

static bool
WriteToFile(const VdfNode &node, std::ostream *os)
{
    (*os) << node.GetDebugName() << std::endl;

    // Keep traversing.
    return true;
}

int 
main(int argc, char **argv) 
{
    // Build a test network.
    VdfTestUtils::Network testNetwork;
    VdfSchedule schedule;
    VdfNode *source = BuildTestNetwork(testNetwork, &schedule);
    VdfNetwork &net = testNetwork.GetNetwork();

    {
        VdfGrapherOptions options;
        options.SetUniqueIds(false); // so that tests don't have node addresses.

        // Test graphing the entire netowrk.
        VdfGrapher::GraphToFile(net, "test.dot");

        // Test graphing the entire network, this time without unique ids.
        VdfGrapher::GraphToFile(net, "network.dot", options);

        // Test graphing a subset of the network.
        options.AddNodeToGraph(*testNetwork["tn2"], 1, 0);
        options.SetDrawMasks(true);
        VdfGrapher::GraphToFile(net, "subset.dot", options);
    }

    // Test graphing a subset of the network where the graphed neighborhood of
    // the first node includes the second node.  We need to check that the
    // second node's neighborhood is fully expanded.
    {
        VdfGrapherOptions opts;
        opts.SetUniqueIds(false);
        opts.AddNodeToGraph(*testNetwork["tn2"], 1, 0);
        opts.AddNodeToGraph(*testNetwork["mon"], 1, 0);
        VdfGrapher::GraphToFile(net, "overlapping_subsets.dot", opts);
    }

    // Now we will test the various display styles
    {
        VdfGrapherOptions opts;
        opts.SetUniqueIds(false); // so that tests don't have node addresses.

        for(size_t i=0; i<net.GetNodeCapacity(); i++) {

            const VdfNode *node = net.GetNode(i);            

            if (!node)
                continue;

            TfToken color(
                TfStringStartsWith(node->GetDebugName(), "Gen") ? "red" : "blue");
            opts.SetColor(node, color);
        }

        // Full (the default)
        opts.SetDisplayStyle(VdfGrapherOptions::DisplayStyleFull);
        VdfGrapher::GraphToFile(net, "displayFull.dot", opts);

        // NoLabels
        opts.SetDisplayStyle(VdfGrapherOptions::DisplayStyleNoLabels);
        VdfGrapher::GraphToFile(net, "displayNoLabels.dot", opts);

        // Summary
        opts.SetDisplayStyle(VdfGrapherOptions::DisplayStyleSummary);
        VdfGrapher::GraphToFile(net, "displaySummary.dot", opts);
    }


    // Test the traversal API on network.
    std::ofstream out("traverse.out");
    VdfTraverseTopologicalSourceNodes(
        *source, std::bind(&WriteToFile, std::placeholders::_1, &out));

    // Cover code by calling the dot command method.
    std::string dotCommand = 
        VdfGrapher::GetDotCommand("test.dot");


    return 0;
}
