//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/rawValueAccessor.h"
#include "pxr/exec/vdf/readIterator.h"
#include "pxr/exec/vdf/readWriteIterator.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/simpleExecutor.h"
#include "pxr/exec/vdf/testUtils.h"
#include "pxr/exec/vdf/typedVector.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"

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

static const int NUM_POINTS = 10000;

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
    VdfReadWriteIterator<GfVec3d> iter(context, _tokens->input1);

    VdfReadIterator<GfVec3d> iter2(context, _tokens->input2);

    for ( ; !iter.IsAtEnd(); ++iter, ++iter2) {
        *iter += *iter2;
    }
}

static std::string
MakeTranslateChain(VdfTestUtils::Network &graph, 
                   VdfTestUtils::CallbackNodeType &translateNodeType,
                   const std::string &first, const std::string &axis, 
                   const VdfMask &axisMask, int num) 
{
    VdfMask allOnes = VdfMask::AllOnes(NUM_POINTS);

    std::string prev = first;
    std::string current = "";
    for (int i = 0; i < num; ++i) {

        current = first + "_" + TfStringify(i);
        graph.Add(current, translateNodeType);

        graph[axis] >> graph[current].In(_tokens->axis, axisMask);
        graph[prev] >> graph[current].In(_tokens->moves, allOnes);

        prev = current;
    }
    return prev;
}

static VdfNode *
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


    VdfTestUtils::CallbackNodeType generatePointsType(&GeneratePoints);
    generatePointsType
        .Out<GfVec3d>(_tokens->out);

    graph.Add("inputPoints1", generatePointsType);
    graph.Add("inputPoints2", generatePointsType);
    graph.Add("inputPoints3", generatePointsType);
    graph.Add("inputPoints4", generatePointsType);


    VdfTestUtils::CallbackNodeType translatePointsType(&TranslatePoints);
    translatePointsType
        .Read<GfVec3d>(_tokens->axis)
        .ReadWrite<GfVec3d>(_tokens->moves, _tokens->out)
        ;

    graph.Add("Translate1", translatePointsType);
    graph.Add("Translate2", translatePointsType);
    graph.Add("Translate3", translatePointsType);
    graph.Add("Translate4", translatePointsType);


    VdfTestUtils::CallbackNodeType addPointsType(&AddPoints);
    addPointsType
        .ReadWrite<GfVec3d>(_tokens->input1, _tokens->out)
        .Read<GfVec3d>(_tokens->input2)
        ;

    graph.Add("AddPoints1",     addPointsType);
    graph.Add("AddPoints2",     addPointsType);
    graph.Add("AddPointsFinal", addPointsType);


    VdfMask allOnes = VdfMask::AllOnes(NUM_POINTS);

    const int numTranslates = 50;

    graph["axisInputs"] >> graph["Translate1"].In(_tokens->axis, axis1Mask);
    graph["inputPoints1"] >> graph["Translate1"].In(_tokens->moves, allOnes);


    std::string lastChain1 = MakeTranslateChain(graph, translatePointsType,
            "Translate1", "axisInputs", axis1Mask, numTranslates);

    graph["axisInputs"] >> graph["Translate2"].In(_tokens->axis, axis2Mask);
    graph["inputPoints2"] >> graph["Translate2"].In(_tokens->moves, allOnes);


    std::string lastChain2 = MakeTranslateChain(graph, translatePointsType, 
            "Translate2", "axisInputs", axis2Mask, numTranslates);

    graph["axisInputs"] >> graph["Translate3"].In(_tokens->axis, axis3Mask);
    graph["inputPoints3"] >> graph["Translate3"].In(_tokens->moves, allOnes);

    std::string lastChain3 = MakeTranslateChain(graph, translatePointsType,
            "Translate3", "axisInputs", axis3Mask, numTranslates);

    graph["axisInputs"] >> graph["Translate4"].In(_tokens->axis, axis4Mask);
    graph["inputPoints4"] >> graph["Translate4"].In(_tokens->moves, allOnes);

    std::string lastChain4 = MakeTranslateChain(graph, translatePointsType,
            "Translate4", "axisInputs", axis4Mask, numTranslates);

    graph[lastChain1] >> graph["AddPoints1"].In(_tokens->input1, allOnes);
    graph[lastChain2] >> graph["AddPoints1"].In(_tokens->input2, allOnes);
    graph[lastChain3] >> graph["AddPoints2"].In(_tokens->input1, allOnes);
    graph[lastChain4] >> graph["AddPoints2"].In(_tokens->input2, allOnes);

    graph["AddPoints1"] >> graph["AddPointsFinal"].In(_tokens->input1, allOnes);
    graph["AddPoints2"] >> graph["AddPointsFinal"].In(_tokens->input2, allOnes);

    return graph["AddPointsFinal"];
}

static bool
runSimpleTest()
{
    VdfSimpleExecutor exec;
    VdfTestUtils::Network graph;

    VdfNode *node = BuildTestNetwork1(graph);

    // Print the network.
    // This covers the code in dump stats and checks that it doesn't crash.
    // That's all we really ask of DumpStats().
    graph.GetNetwork().DumpStats(std::cerr);

    VdfMask allOnes(NUM_POINTS);
    allOnes.SetAll();
    VdfRequest request(VdfMaskedOutput( node->GetOutput(), allOnes ) );

    VdfSchedule schedule;
    VdfScheduler::Schedule(request, &schedule, true /* topologicalSort */);

    TraceCollector::GetInstance().SetEnabled(true);

    {
        TRACE_SCOPE("Singlethreaded");
        exec.Run(schedule);
    }

    GfVec3d result = exec.GetOutputValue(*node->GetOutput(), allOnes)
        ->GetReadAccessor<GfVec3d>()[0];

    std::cout << "Result is " << result << std::endl;
    std::cout << "------" << std::endl;

    TraceReporter::GetGlobalReporter()->Report(std::cout);

    std::cout << "runSimpleTest() PASSED" << std::endl;
    return true;
}

///////////////////////////////////////////////////////////////////////////////

// This is a simple test for the VdfContext::SetEmptyOutput() API.

static void
EmptyOutputProducer(const VdfContext &context)
{
    // If we have an input value, set it on our output. Otherwise, set an
    // empty output value.
    if (context.HasInputValue<int>(_tokens->input1)) {
        context.SetOutput(context.GetInputValue<int>(_tokens->input1));
    } else {
        context.SetEmptyOutput<int>();
    }
}

static void
EmptyOutputConsumer(const VdfContext &context)
{
    if (context.HasInputValue<int>(_tokens->input1)) {
        context.SetOutput(std::string("got value"));
    } else {
        context.SetOutput(std::string("no value"));
    }
}

static bool
runEmptyOutputTest()
{
    VdfSimpleExecutor exec;
    VdfTestUtils::Network graph;

    const VdfMask allOnes = VdfMask::AllOnes(2);

    // A producer node type that sets an empty value on its output if its input
    // is connected. Otherwise, it copies its input value to its output.
    VdfTestUtils::CallbackNodeType producerType(&EmptyOutputProducer);
    producerType
        .Read<int>(_tokens->input1)
        .Out<int>(_tokens->out)
        ;

    // A consumer node type that reads its input and outputs "got value" if it
    // gets a non-empty value and "no value" otherwise.
    VdfTestUtils::CallbackNodeType consumerType(&EmptyOutputConsumer);
    consumerType
        .Read<int>(_tokens->input1)
        .Out<std::string>(_tokens->out)
        ;

    // Connect a producer that outputs an empty value to a consumer.
    graph.Add("nodeWithEmptyResult", producerType);
    graph.Add("nodeWithEmptyInputValue", consumerType);
    graph["nodeWithEmptyResult"]
        >> graph["nodeWithEmptyInputValue"].In(_tokens->input1, allOnes);

    // Connect a source input to a producer, which will then output a non-empty
    // value, and connect the producer's output to a consumer.
    graph.AddInputVector<int>("sourceInput", 2);
    graph["sourceInput"].SetValue(0, 42);
    graph.Add("nodeWithNonEmptyResult", producerType);
    graph["sourceInput"]
        >> graph["nodeWithNonEmptyResult"].In(_tokens->input1, allOnes);
    graph.Add("nodeWithNonEmptyInputValue", consumerType);
    graph["nodeWithNonEmptyResult"]
        >> graph["nodeWithNonEmptyInputValue"].In(_tokens->input1, allOnes);

    VdfNode *const nodeWithEmptyResult =
        graph["nodeWithEmptyResult"].GetVdfNode();
    TF_AXIOM(nodeWithEmptyResult);
    VdfNode *const nodeWithEmptyInputValue =
        graph["nodeWithEmptyInputValue"].GetVdfNode();
    TF_AXIOM(nodeWithEmptyInputValue);
    VdfNode *const nodeWithNonEmptyInputValue =
        graph["nodeWithNonEmptyInputValue"].GetVdfNode();
    TF_AXIOM(nodeWithNonEmptyInputValue);

    VdfRequest request(
        {VdfMaskedOutput(nodeWithEmptyResult->GetOutput(), allOnes),
         VdfMaskedOutput(nodeWithEmptyInputValue->GetOutput(), allOnes),
         VdfMaskedOutput(nodeWithNonEmptyInputValue->GetOutput(), allOnes)});
    VdfSchedule schedule;
    VdfScheduler::Schedule(request, &schedule, /* topologicalSort */ true);
    exec.Run(schedule);

    // Get a result from an output that was set to an empty value.
    VdfVector::ReadAccessor<int> emptyResult =
        exec.GetOutputValue(
            *nodeWithEmptyResult->GetOutput(),
            allOnes)->GetReadAccessor<int>();
    TF_AXIOM(emptyResult.GetNumValues() == 0);

    // Get a result from an output that resulted from the node that read an
    // empty value.
    VdfVector::ReadAccessor<std::string> emptyInputResult =
        exec.GetOutputValue(
            *nodeWithEmptyInputValue->GetOutput(),
            allOnes)->GetReadAccessor<std::string>();
    TF_AXIOM(emptyInputResult.GetNumValues() == 1);
    TF_AXIOM(emptyInputResult[0] == "no value");

    // Get a result from an output that resulted from the node that read a
    // non-empty value.
    VdfVector::ReadAccessor<std::string> nonEmptyInputResult =
        exec.GetOutputValue(
            *nodeWithNonEmptyInputValue->GetOutput(),
            allOnes)->GetReadAccessor<std::string>();
    TF_AXIOM(nonEmptyInputResult.GetNumValues() == 1);
    TF_AXIOM(nonEmptyInputResult[0] == "got value");

    std::cout << "runEmptyOutputTest() PASSED" << std::endl;
    return true;
}

///////////////////////////////////////////////////////////////////////////////

// This is a simple test for the VdfContext::SetOutputToReferenceInput() API.

static void
ReferencerNode(const VdfContext &context)
{
    context.SetOutputToReferenceInput(_tokens->input1);
}

static bool
runReferenceTest()
{
    VdfSimpleExecutor exec;
    VdfTestUtils::Network graph;

    VdfMask allOnes = VdfMask::AllOnes(2);
    graph.AddInputVector<int>("refInputs", 2);
    graph["refInputs"]
        .SetValue(0, 42)
        .SetValue(1, 24)
        ;

    VdfTestUtils::CallbackNodeType referencerType(&ReferencerNode);
    referencerType
        .Read<int>(_tokens->input1)
        .Out<int>(_tokens->out)
        ;

    graph.Add("refNode", referencerType);

    graph["refInputs"] >> graph["refNode"].In(_tokens->input1, allOnes);

    VdfNode *refNode = graph["refNode"].GetVdfNode();

    VdfRequest request(VdfMaskedOutput(refNode->GetOutput(), allOnes));

    VdfSchedule schedule;
    VdfScheduler::Schedule(request, &schedule, true /* topologicalSort */);

    exec.Run(schedule);

    VdfVector::ReadAccessor<int> result = exec.GetOutputValue(*refNode->
        GetOutput(), allOnes)->GetReadAccessor<int>();

    TF_AXIOM(result.GetNumValues() == 2);
    TF_AXIOM(result[0] == 42);
    TF_AXIOM(result[1] == 24);

    std::cout << "runReferenceTest() PASSED" << std::endl;
    return true;
}

///////////////////////////////////////////////////////////////////////////////

int 
main(int argc, char **argv) 
{
    int numErrors = 0;
    if (!runSimpleTest()) {
        std::cout << "Error running SimpleTest" << std::endl;
        numErrors++;
    }

    if (!runEmptyOutputTest()) {
        std::cout << "Error running runEmptyOutputTest" << std::endl;
        numErrors++;
    }

    if (!runReferenceTest()) {
        std::cout << "Error running runReferenceTest" << std::endl;
        numErrors++;
    }

    return numErrors;
}
