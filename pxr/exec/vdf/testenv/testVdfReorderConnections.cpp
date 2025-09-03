//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/readIterator.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/simpleExecutor.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

#include <iostream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (out)
    (string)
);

static VdfNode *
BuildTestNetwork2(VdfTestUtils::Network &graph)
{
    // Build a network like this:
    //
    //        StringA StringB  StringC StringD
    //              \      /    \      /
    //              Combine1    Combine2
    //                   \       /
    //                    Combine3

    graph.AddInputVector<std::string>("StringA", 1);
    graph["StringA"]
        .SetValue(0, std::string("A"));

    graph.AddInputVector<std::string>("StringB", 1);
    graph["StringB"]
        .SetValue(0, std::string("B"));

    graph.AddInputVector<std::string>("StringC", 1);
    graph["StringC"]
        .SetValue(0, std::string("C"));

    graph.AddInputVector<std::string>("StringD", 1);
    graph["StringD"]
        .SetValue(0, std::string("D"));


    VdfTestUtils::CallbackNodeType combineStrings(
        +[](const VdfContext &context) {
            std::string result;
            for (VdfReadIterator<std::string> i(context, _tokens->string);
                !i.IsAtEnd(); ++i) {
                if (!result.empty()) {
                    result += ", ";
                }
                result += *i;
            }

            context.SetOutput(result);
        });
    combineStrings
        .Read<std::string>(_tokens->string)
        .Out<std::string>(_tokens->out)
        ;

    graph.Add("Combine1", combineStrings);
    graph.Add("Combine2", combineStrings);
    graph.Add("Combine3", combineStrings);
    graph.Add("Combine4", combineStrings);


    const VdfMask oneOne = VdfMask::AllOnes(1);

    graph["StringA"] >> graph["Combine1"].In(_tokens->string, oneOne);
    graph["StringB"] >> graph["Combine1"].In(_tokens->string, oneOne);

    graph["StringC"] >> graph["Combine2"].In(_tokens->string, oneOne);
    graph["StringD"] >> graph["Combine2"].In(_tokens->string, oneOne);

    graph["Combine1"] >> graph["Combine3"].In(_tokens->string, oneOne);
    graph["Combine2"] >> graph["Combine3"].In(_tokens->string, oneOne);

    return graph["Combine3"];
}

// Test macro that mirrors the gtest API.
#define ASSERT_EQ(a, b)                                                         \
    if ((a) != (b)) {                                                           \
        TF_FATAL_ERROR(                                                         \
            "Test failure:\n%s != %s\n",                                        \
            TfStringify(a).c_str(),                                             \
            TfStringify(b).c_str());                                            \
        return false;                                                           \
    }

static bool
TestReorderInputConnections()
{
    VdfTestUtils::Network testNetwork;

    VdfNetwork &network = testNetwork.GetNetwork();
    size_t prevVersion = network.GetVersion();

    VdfNode *const out = BuildTestNetwork2(testNetwork);

    TF_AXIOM(network.GetVersion() != prevVersion);

    const VdfMask oneOne = VdfMask::AllOnes(1);
    VdfRequest request(VdfMaskedOutput(out->GetOutput(), oneOne));

    VdfSchedule schedule;
    VdfScheduler::Schedule(request, &schedule, /* topologicalSort */ true);

    VdfSimpleExecutor exec;
    exec.Run(schedule);

    std::string result;
    result = exec.GetOutputValue(*out->GetOutput(_tokens->out), oneOne)
        ->GetReadAccessor<std::string>()[0];
    ASSERT_EQ(result, "A, B, C, D");

    // Apply edit operation...
    std::cout << "/// Reordering input connections..." << std::endl;

    VdfNode *const combine1Node = testNetwork["Combine1"].GetVdfNode();
    exec.InvalidateValues({{combine1Node->GetOutput(), oneOne}});

    VdfInput *const combine1Input = combine1Node->GetInput(_tokens->string);

    network.ReorderInputConnections(
        combine1Input,
        std::vector<VdfConnectionVector::size_type>({1, 0}));

    exec.Run(schedule);
    result = exec.GetOutputValue(*out->GetOutput(_tokens->out), oneOne)
        ->GetReadAccessor<std::string>()[0];
    ASSERT_EQ(result, "B, A, C, D");

    // Error cases
    printf("=== Expected Error Output Begin ===\n");

    // Attempt to reorder with repeated indices.
    {
        TfErrorMark errorMark;
        network.ReorderInputConnections(
            combine1Input,
            std::vector<VdfConnectionVector::size_type>({0, 0}));

        size_t numErrors;
        errorMark.GetBegin(&numErrors);
        ASSERT_EQ(numErrors, 1);
    }

    // Attempt to reorder with out-of-range indices.
    {
        TfErrorMark errorMark;
        network.ReorderInputConnections(
            combine1Input,
            std::vector<VdfConnectionVector::size_type>({1, 2}));

        size_t numErrors;
        errorMark.GetBegin(&numErrors);
        ASSERT_EQ(numErrors, 1);
    }

    // Attempt to reorder with too many indices.
    {
        TfErrorMark errorMark;
        network.ReorderInputConnections(
            combine1Input,
            std::vector<VdfConnectionVector::size_type>({0, 1, 2}));

        size_t numErrors;
        errorMark.GetBegin(&numErrors);
        ASSERT_EQ(numErrors, 1);
    }

    printf("=== Expected Error Output End ===\n");

    return true;
}

int 
main(int argc, char **argv) 
{
    std::cout << "TestReorderInputConnections..." << std::endl;
    if (!TestReorderInputConnections()) {
        return 1;
    }
    std::cout << "... done" << std::endl;

    return 0;
}
