//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/grapherOptions.h"
#include "pxr/exec/vdf/grapher.h"
#include "pxr/exec/vdf/isolatedSubnetwork.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/rawValueAccessor.h"
#include "pxr/exec/vdf/readIterator.h"
#include "pxr/exec/vdf/readWriteIterator.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/simpleExecutor.h"
#include "pxr/exec/vdf/testUtils.h"
#include "pxr/exec/vdf/typedVector.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/vt/value.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (axis)
    (moves)
    (input1)
    (input2)
    (out)
);

static const int NUM_POINTS = 10;

static void
GenerateDouble(const VdfContext &context)
{
    const double result = 1.0;
    context.SetOutput(result);
}

static void
GeneratePoints(const VdfContext &context) 
{
    const int size = NUM_POINTS;
    VdfTypedVector<GfVec3d> result;
    result.template Resize<GfVec3d>(size);

    VdfVector::ReadWriteAccessor<GfVec3d> a = 
        result.template GetReadWriteAccessor<GfVec3d>();
    for (int i = 0; i < size; ++i) {
        a[i] = GfVec3d(0, 0, 0);
    }

    VdfRawValueAccessor rawValueAccessor(context);
    rawValueAccessor.SetOutputVector(
        *VdfTestUtils::OutputAccessor(context).GetOutput(),
        VdfMask::AllOnes(size),
        result);
}


static void
TranslatePoints(const VdfContext &context)
{
    //TRACE_FUNCTION();

    // We only expect one value for the "axis" input -- so we use the 
    // GetInputValue API, which is very simple.
    GfVec3d axis = context.GetInputValue<GfVec3d>(_tokens->axis);

    // We don't know how many inputs we will have for the "moves" input, so
    // we will use an iterator, that we'll also use to output our data into.
    VdfReadWriteIterator<GfVec3d> iter(context, _tokens->moves);

    // Now loop over all of our inputs and translate the points.
    for ( ; !iter.IsAtEnd(); ++iter) {
        *iter += axis;
    }

}

static void
AddPoints(const VdfContext &context)
{
    size_t numPoints = 0;
    VdfReadIterator<GfVec3d> it(context, _tokens->input1);
    for (; !it.IsAtEnd(); ++it) {
        ++numPoints;
    }

    VdfTypedVector<GfVec3d> result;
    result.template Resize<GfVec3d>(numPoints);
    VdfVector::ReadWriteAccessor<GfVec3d> a = 
        result.template GetReadWriteAccessor<GfVec3d>();

    VdfReadIterator<GfVec3d> iter(context, _tokens->input1);
    if (context.HasInputValue<GfVec3d>(_tokens->input2)) {
        VdfReadIterator<GfVec3d> iter2(context, _tokens->input2);
    
        size_t i = 0;
        for (; !iter.IsAtEnd(); ++iter, ++iter2, ++i)
            a[i] = (*iter + *iter2);
    } else {
        size_t i = 0;
        for (; !iter.IsAtEnd(); ++iter, ++i)
            a[i] = *iter;
    }

    VdfRawValueAccessor rawValueAccessor(context);
    rawValueAccessor.SetOutputVector(
        *VdfTestUtils::OutputAccessor(context).GetOutput(),
        VdfMask::AllOnes(numPoints),
        result);
}

std::string
MakeTranslateChain(VdfTestUtils::Network &graph, 
                   VdfTestUtils::CallbackNodeType &translateNode,
                   const std::string &first, const std::string &axis, 
                   const VdfMask &axisMask, int num) 
{
    VdfMask allOnes = VdfMask::AllOnes(NUM_POINTS);

    std::string prev = first;
    std::string current = "";
    for (int i = 0; i < num; ++i) {

        current = first + "_" + TfStringify(i);
        graph.Add(current, translateNode);

        graph[axis] >> graph[current].In(_tokens->axis, axisMask);
        graph[prev] >> graph[current].In(_tokens->moves, allOnes);

        prev = current;
    }
    return prev;
}

VdfNode *
BuildTestNetwork1(VdfTestUtils::Network &graph)
{
    // We're going to build a network like this:
    //
    //        Axis1 InputPoints1  Axis2  InputPoints2  Axis3  IP3  Axis4  IP4
    //           \   /               \   /              \      /     \     /
    //          Translate1       Translate2                T3           T4
    //              \                /                      \          /
    //                  AddPoints1                           AddPoints2
    //                        \                                 /
    //                                   AddPointsFinal

    graph.AddInputVector<GfVec3d>("axisInputs", 4);
    graph["axisInputs"]
        .SetValue(0, GfVec3d(1, 0, 0))
        .SetValue(1, GfVec3d(0, 1, 0))
        .SetValue(2, GfVec3d(1, 0, 0))
        .SetValue(3, GfVec3d(0, 1, 0));

    VdfMask axis1Mask(4);
    VdfMask axis2Mask(4);
    VdfMask axis3Mask(4);
    VdfMask axis4Mask(4);
    axis1Mask.SetIndex(0);
    axis2Mask.SetIndex(1);
    axis3Mask.SetIndex(2);
    axis4Mask.SetIndex(3);


    VdfTestUtils::CallbackNodeType generatePoints(&GeneratePoints);
    generatePoints
        .Out<GfVec3d>(_tokens->out);

    graph.Add("inputPoints1", generatePoints);
    graph.Add("inputPoints2", generatePoints);
    graph.Add("inputPoints3", generatePoints);
    graph.Add("inputPoints4", generatePoints);


    VdfTestUtils::CallbackNodeType translatePoints(&TranslatePoints);
    translatePoints
        .Read<GfVec3d>(_tokens->axis)
        .ReadWrite<GfVec3d>(_tokens->moves, _tokens->out)
        ;

    graph.Add("Translate1", translatePoints);
    graph.Add("Translate2", translatePoints);
    graph.Add("Translate3", translatePoints);
    graph.Add("Translate4", translatePoints);


    VdfTestUtils::CallbackNodeType addPoints(&AddPoints);

    addPoints
        .Read<GfVec3d>(_tokens->input1)
        .Read<GfVec3d>(_tokens->input2)
        .Out<GfVec3d>(_tokens->out);
        ;

    graph.Add("AddPoints1",     addPoints);
    graph.Add("AddPoints2",     addPoints);
    graph.Add("AddPointsFinal", addPoints);


    VdfMask allOnes = VdfMask::AllOnes(NUM_POINTS);

    const int numTranslates = 1;

    graph["axisInputs"] >> graph["Translate1"].In(_tokens->axis, axis1Mask);
    graph["inputPoints1"] >> graph["Translate1"].In(_tokens->moves, allOnes);


    std::string lastChain1 = MakeTranslateChain(graph, translatePoints,
            "Translate1", "axisInputs", axis1Mask, numTranslates);

    graph["axisInputs"] >> graph["Translate2"].In(_tokens->axis, axis2Mask);
    graph["inputPoints2"] >> graph["Translate2"].In(_tokens->moves, allOnes);


    std::string lastChain2 = MakeTranslateChain(graph, translatePoints, 
            "Translate2", "axisInputs", axis2Mask, numTranslates);

    graph["axisInputs"] >> graph["Translate3"].In(_tokens->axis, axis3Mask);
    graph["inputPoints3"] >> graph["Translate3"].In(_tokens->moves, allOnes);

    std::string lastChain3 = MakeTranslateChain(graph, translatePoints,
            "Translate3", "axisInputs", axis3Mask, numTranslates);

    graph["axisInputs"] >> graph["Translate4"].In(_tokens->axis, axis4Mask);
    graph["inputPoints4"] >> graph["Translate4"].In(_tokens->moves, allOnes);

    std::string lastChain4 = MakeTranslateChain(graph, translatePoints,
            "Translate4", "axisInputs", axis4Mask, numTranslates);

    graph[lastChain1] >> graph["AddPoints1"].In(_tokens->input1, allOnes);
    graph[lastChain2] >> graph["AddPoints1"].In(_tokens->input2, allOnes);
    graph[lastChain3] >> graph["AddPoints2"].In(_tokens->input1, allOnes);
    graph[lastChain4] >> graph["AddPoints2"].In(_tokens->input2, allOnes);

    graph["AddPoints1"] >> graph["AddPointsFinal"].In(_tokens->input1, allOnes);
    graph["AddPoints2"] >> graph["AddPointsFinal"].In(_tokens->input2, allOnes);

    return graph["AddPointsFinal"];
}

// Utility class to run, stat and graph a network multiple times.
//
class Runner 
{
public :

    Runner(const VdfNetwork &net, VdfNode *out) :
        _allOnes(NUM_POINTS),
        _net(net),
        _out(out)
    {
        _allOnes.SetAll();

        _options.SetUniqueIds(false);
        _options.SetDrawMasks(true);
        _options.SetPrintSingleOutputs(true);
    }

    GfVec3d Snapshot(const std::string &purpose, bool run = true)
    {
        char filename[256];
    
        //
        // Graph network
        //
    
        printf("\n/// Snapshot: %s\n\n", purpose.c_str());

        sprintf(filename, "%s.dot", purpose.c_str());
        VdfGrapher::GraphToFile(_net, filename, _options);
    
        if (!run)
            return GfVec3d(0);

        //
        // Stat network
        //
        _net.DumpStats(std::cerr);

        //
        // Run network
        //
    
        VdfRequest request(VdfMaskedOutput(_out->GetOutput(), _allOnes));

        VdfScheduler::Schedule(request, &_schedule, true /* topologicalSort */);

        _exec.Run(_schedule);

        GfVec3d res = _exec.GetOutputValue(
            *_out->GetOutput(_tokens->out), _allOnes)
                ->GetReadAccessor<GfVec3d>()[0];

        std::cout << "Result is: " << res << std::endl << std::endl;

        return res;
    }

    void Invalidate(const VdfNode &node)
    {
        VdfMaskedOutputVector outputs;
        
        TF_FOR_ALL(i, node.GetOutputsIterator()) {

            VdfOutput *output = i->second;
            VdfMask mask;
        
            if (output->GetAffectsMask()) 
                mask = *output->GetAffectsMask();
            else 
                mask = VdfMask::AllOnes(output->GetNumDataEntries());
        
            outputs.push_back(VdfMaskedOutput(output, mask));
        }
        _exec.InvalidateValues(outputs);
    }

    const VdfNode *FindNode(
        const VdfNetwork &network, 
        const std::string &name) const 
    {
        std::vector<const VdfNode *> nodes = 
            VdfGrapher::GetNodesNamed(network, name);

        return nodes.size() == 1 ? nodes[0] : NULL;
    }

private :

    VdfMask           _allOnes;
    const VdfNetwork &_net;
    VdfNode          *_out;
    VdfSchedule       _schedule;
    VdfSimpleExecutor _exec;
    VdfGrapherOptions _options;
};

bool
TestNodeIds()
{
    VdfTestUtils::Network testNetwork;
    VdfNetwork &net = testNetwork.GetNetwork();

    // Build a test network.
    BuildTestNetwork1(testNetwork);

    // Verify the node indices and versions
    size_t nodeCapacity = net.GetNodeCapacity();
    for (size_t i = 0; i < nodeCapacity; ++i) {
        const VdfNode *node = net.GetNode(i);
        TF_AXIOM(node);
        TF_AXIOM(VdfNode::GetVersionFromId(node->GetId()) == 0);
        TF_AXIOM(VdfNode::GetIndexFromId(node->GetId()) == i);
    }

    // Clear the network.
    net.Clear();

    // Make sure all nodes have been deleted.
    TF_AXIOM(net.GetNodeCapacity() == 0);
    for (size_t i = 0; i < nodeCapacity; ++i) {
        TF_AXIOM(!net.GetNode(i));
    }

    // Rebuild the network.
    BuildTestNetwork1(testNetwork);

    // Make sure the versions have been incremented and that all nodes
    // are again available.
    TF_AXIOM(net.GetNodeCapacity() == nodeCapacity);
    for (size_t i = 0; i < nodeCapacity; ++i) {
        const VdfNode *node = net.GetNode(i);
        TF_AXIOM(node);
        TF_AXIOM(VdfNode::GetVersionFromId(node->GetId()) == 1);
        TF_AXIOM(VdfNode::GetIndexFromId(node->GetId()) == i);
    }

    // Add a new node.
    VdfTestUtils::CallbackNodeType generateDouble1(&GenerateDouble);
    generateDouble1
        .Out<double>(_tokens->out);
    testNetwork.Add("inputDouble1", generateDouble1);

    // Verify the node has been added to the end.
    TF_AXIOM(net.GetNodeCapacity() > nodeCapacity);
    nodeCapacity = net.GetNodeCapacity();
    VdfNode *newNode1 = net.GetNode(nodeCapacity - 1);
    TF_AXIOM(VdfNode::GetVersionFromId(newNode1->GetId()) == 1);
    TF_AXIOM(VdfNode::GetIndexFromId(newNode1->GetId()) == (nodeCapacity - 1));

    // Add another new node.
    VdfTestUtils::CallbackNodeType generateDouble2(&GenerateDouble);
    generateDouble2
        .Out<double>(_tokens->out);
    testNetwork.Add("inputDouble2", generateDouble2);

    // Verify the node has been added to the end.
    TF_AXIOM(net.GetNodeCapacity() > nodeCapacity);
    nodeCapacity = net.GetNodeCapacity();
    VdfNode *newNode2 = net.GetNode(nodeCapacity - 1);
    TF_AXIOM(VdfNode::GetVersionFromId(newNode2->GetId()) == 1);
    TF_AXIOM(VdfNode::GetIndexFromId(newNode2->GetId()) == (nodeCapacity - 1));

    // Delete a node (leaving a "hole" in the node array)
    net.Delete(newNode1);
    TF_AXIOM(net.GetNodeCapacity() == nodeCapacity);
    TF_AXIOM(!net.GetNode(nodeCapacity - 2));

    // Add a nother new node. It should alias the previously deleted node index
    // but have a different version number.
    VdfTestUtils::CallbackNodeType generateDouble3(&GenerateDouble);
    generateDouble3
        .Out<double>(_tokens->out);
    testNetwork.Add("inputDouble3", generateDouble3);

    TF_AXIOM(net.GetNodeCapacity() == nodeCapacity);
    VdfNode *newNode3 = net.GetNode(nodeCapacity - 2);
    TF_AXIOM(newNode3);
    TF_AXIOM(VdfNode::GetVersionFromId(newNode3->GetId()) == 2);
    TF_AXIOM(VdfNode::GetIndexFromId(newNode3->GetId()) == (nodeCapacity - 2));

    // Nothing changed about the last node.
    TF_AXIOM(VdfNode::GetVersionFromId(newNode2->GetId()) == 1);
    TF_AXIOM(VdfNode::GetIndexFromId(newNode2->GetId()) == (nodeCapacity - 1));

    // Delete the same node again, and add one more new node in its place.
    net.Delete(newNode3);
    TF_AXIOM(net.GetNodeCapacity() == nodeCapacity);
    TF_AXIOM(!net.GetNode(nodeCapacity - 2));

    VdfTestUtils::CallbackNodeType generateDouble4(&GenerateDouble);
    generateDouble4
        .Out<double>(_tokens->out);
    testNetwork.Add("inputDouble3", generateDouble4);

    // Verify that node versions will be incremented past version 1.
    VdfNode *newNode4 = net.GetNode(nodeCapacity - 2);
    TF_AXIOM(newNode4);
    TF_AXIOM(VdfNode::GetVersionFromId(newNode4->GetId()) == 3);
    TF_AXIOM(VdfNode::GetIndexFromId(newNode4->GetId()) == (nodeCapacity - 2));

    return true;
}

bool
TestEdits()
{
    VdfTestUtils::Network testNetwork;

    VdfNetwork &net = testNetwork.GetNetwork();
    size_t prevVersion = net.GetVersion();

    VdfNode    *out = BuildTestNetwork1(testNetwork);

    TF_AXIOM(net.GetVersion() != prevVersion);

    Runner runner(net, out);
    GfVec3d res;

    res = runner.Snapshot("original");

    if (res != GfVec3d(4, 4, 0)) {
        std::cout << "*** Test failed, unexpected result: " << res << std::endl;
        return false;
    }

    ////////////////////////////////////////////////////////////////////////

    // Applying edit operation...
    std::cout << "/// Editing network..." << std::endl;

    TfErrorMark allErrors;

    // Test all cases where not all inputs/outputs are automatically removed and
    // thus the node is still at least partially connected when trying to delete
    // it. ~ This needs to raise a coding error.

    printf("=== Expected Error Output Begin ===\n");

    int editStep = 0;

    for(int i=0; i<2; i++) {
        TfErrorMark m;

        printf("Deleting 'Translate4_0' /w deleteBranch= %s\n",
            (i&1) ? "true" : "false");

        runner.Invalidate(*testNetwork["Translate4_0"]);

        bool error;

        if (i == 0) {

            // We don't expect Tralsnate4_0 to be deleted as long as it still
            // has inputs that are connected.
            prevVersion = net.GetVersion();
            error = TF_HAS_ERRORS(m, net.Delete(testNetwork["Translate4_0"]));
            TF_AXIOM(net.GetVersion() == prevVersion);

        } else {

            struct Monitor : public VdfNetwork::EditMonitor
            {
                void WillClear() override {}

                void DidConnect(const VdfConnection *connection) override {
                    printf("> Connect CONN: %p %s\n",
                           connection, connection->GetDebugName().c_str());
                }
                void WillDelete(const VdfNode *node) override {
                    printf("> Delete NODE: %p %s\n",
                           node, node->GetDebugName().c_str());
                }
                void WillDelete(const VdfConnection *conn) override {
                    printf("> Delete CONN: %p %s\n",
                           conn, conn->GetDebugName().c_str());
                }

                void DidAddNode(const VdfNode *) override {}
            } monitor;

            // Isolate a sub graph around Translate4_0.
            const char *names[] = {
                "Translate4_0:out -> AddPoints2:input2",
                "axisInputs:out -> Translate4:axis",
                "axisInputs:out -> Translate4_0:axis"
            };

            net.RegisterEditMonitor(&monitor);

            for(size_t j=0; j<sizeof(names)/sizeof(char *); j++) {
                prevVersion = net.GetVersion();
                net.Disconnect(testNetwork.GetConnection(names[j]));
                TF_AXIOM(net.GetVersion() != prevVersion);

                res = runner.Snapshot("edit_step_" + TfStringify(editStep++), false);
            }

            const auto neverFilter = [](const VdfNode *) { return true; };

            prevVersion = net.GetVersion();
            error = TF_HAS_ERRORS(m,
                VdfIsolatedSubnetwork::IsolateBranch(
                    testNetwork["Translate4_0"], neverFilter));
            TF_AXIOM(net.GetVersion() != prevVersion);

            net.UnregisterEditMonitor(&monitor);
        }

        bool errorExpected = i == 0;

        if (error != errorExpected) {
            printf("*** Test failed, error %sraised.\n", errorExpected ? "not " : " ");
            return false;
        }

        res = runner.Snapshot("edit_step_" + TfStringify(editStep++), !errorExpected);
    }

    if (res != GfVec3d(4, 2, 0)) {
        std::cout << "*** Test failed, unexpected result: " << res << std::endl;
        return false;
    }

    size_t numErrors;
    allErrors.GetBegin(&numErrors);
    printf("=== Expected Error Output End (%zu errors found) ===\n", numErrors);

    if (numErrors != 1) {
        printf("*** Test failed, expected one error.\n");
        return false;
    }

    ////////////////////////////////////////////////////////////////////////

    const int numEdits = 6;
    const int nodeSet  = 4;

    const char *connectionNames[numEdits] = {
        "Translate3:out -> Translate3_0:moves",
        "axisInputs -> Translate3:axis",
        "inputPoints3:out -> Translate3:moves",
        "axisInputs -> Translate3_0:axis",
        "Translate3_0:out -> AddPoints2:input1",
        "AddPoints2:out -> AddPointsFinal:input2"
    };

    int nodesPresentAfterEditStep[numEdits] = {
        4, 4, 2, 2, 1, 0
    };

    const char *nodesToRemove[nodeSet] = {
        "VdfTestUtils::DependencyCallbackNode inputPoints3",
        "VdfTestUtils::DependencyCallbackNode Translate3",
        "VdfTestUtils::DependencyCallbackNode Translate3_0",
        "VdfTestUtils::DependencyCallbackNode AddPoints2"
    };

    for(size_t i=0; i<numEdits; i++) {

        VdfConnection *connection = testNetwork.GetConnection(connectionNames[i]);

        if (!connection) {
            printf("*** Test failed, expected to find connection: %s.\n",
                   connectionNames[i]);
            return false;
        }

        printf("Deleting connection: %s\n", connectionNames[i]);

        // Note: By design, the client needs to invalidate nodes before edits.
        runner.Invalidate(connection->GetTargetNode());

        prevVersion = net.GetVersion();

        // Get pointers to source and target nodes, before connection is deleted.
        VdfNode *tgtNode = &connection->GetTargetNode(),
                *srcNode = &connection->GetSourceNode();

        net.Disconnect(connection);

        // If target node became orphaned, delete...
        if (!tgtNode->HasInputConnections() && 
            !tgtNode->HasOutputConnections()) {
            net.Delete(tgtNode);
        }

        // If source node became orphaned, delete...
        if (!srcNode->HasInputConnections() && 
            !srcNode->HasOutputConnections()) {
            net.Delete(srcNode);
        }

        TF_AXIOM(net.GetVersion() != prevVersion);

        if (testNetwork.GetConnection(connectionNames[i])) {
            printf("*** Test failed, expected connection to be removed.\n");
            return false;
        }

        // Verify that orphaned node removal works.
        int nodesFound = 0;

        for(int j=0; j<nodeSet; j++)
            if (runner.FindNode(net, nodesToRemove[j]))
                nodesFound++;

        if (nodesFound != nodesPresentAfterEditStep[i]) {
            printf("*** Test failed, unexpected number of nodes after "
                   "connection removal %zu. Found %d, expected %d.\n",
                   i, nodesFound, nodesPresentAfterEditStep[i]);
            return false;
        }

        res = runner.Snapshot("removed_edge_" + TfStringify(i), i+1 == numEdits);
    }

    if (res != GfVec3d(2, 2, 0)) {
        std::cout << "*** Test failed, unexpected result: " << res << std::endl;
        return false;
    }

    //
    // Test DeleteBranch and EditFilters
    //

    struct Filter
    {
        Filter(Runner &runner)
        :   _runner(runner),
            _nodesAsked(0) {}

        bool operator()(const VdfNode *node) const
        {
            printf("> asking: %s\n", node->GetDebugName().c_str());
            _nodesAsked++;
            return true;
        }

        Runner &_runner;
        mutable size_t _nodesAsked;
    };

    Filter filter(runner);

    VdfConnection *connection = testNetwork.GetConnection(
        "Translate2_0:out -> AddPoints1:input2");

    TF_AXIOM(connection);

    printf("Deleting branch: %s\n", connection->GetDebugName().c_str());

    std::unique_ptr<VdfIsolatedSubnetwork> subgraph =
        VdfIsolatedSubnetwork::IsolateBranch(connection, filter);

    size_t num = subgraph->GetIsolatedNodes().size();
    printf("> num = %zu\n", num);

    if (num != 3) {
        printf("*** Test failed, expected 3 nodes to be deleted.\n");
        return false;
    }

    printf("> filter._nodesAsked = %zu\n", filter._nodesAsked);

    // We get asked for each node seen along each path.
    if (filter._nodesAsked != 5) {
        printf("*** Test failed, expected 6 nodes to be filtered.\n");
        return false;
    }

    res = runner.Snapshot("removed_branch");

    if (res != GfVec3d(2, 0, 0)) {
        std::cout << "*** Test failed, unexpected result: " << res << std::endl;
        return false;
    }

//XXX: test that a diamond is deleted correctly by DeleteBranch

    return true;
}

int 
main(int argc, char **argv) 
{
    std::cout << "TestNodeIds..." << std::endl;
    if (!TestNodeIds()) {
        return 1;
    }
    std::cout << "... done" << std::endl;

    std::cout << "TestEdits..." << std::endl;
    if (!TestEdits()) {
        return 1;
    }
    std::cout << "... done" << std::endl;

    return 0;
}

