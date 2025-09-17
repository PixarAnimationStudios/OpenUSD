//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/connectorSpecs.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"
#include "pxr/exec/vdf/inputValuesPointer.h"
#include "pxr/exec/vdf/inputVector.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/readWriteIterator.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/simpleExecutor.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/tf/span.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (in)
    (out)
);

static void
ReadCallback(const VdfContext &context) 
{
    TRACE_FUNCTION();

    VdfInputValuesPointer<int> ptr(context, _tokens->in);

    TF_AXIOM(
        ptr.GetSize() == 100 || (ptr.GetSize() == 0 && !ptr.GetData()));

    for (size_t i = 0; i < ptr.GetSize(); ++i) {
        TF_AXIOM(ptr.GetData()[i] == static_cast<int>(i));
    }

    // Test the TfSpan type conversion.  This won't compile without it.
    const TfSpan<const int> span = ptr;
    TF_AXIOM(span.data() == ptr.GetData());
    TF_AXIOM(span.size() == ptr.GetSize());

    context.SetOutput(1);
}

static VdfNode *
CreateReadNode(VdfNetwork *net) 
{
    VdfInputSpecs inspec;
    inspec
        .ReadConnector<int>(_tokens->in)
        ;

    VdfOutputSpecs outspec;
    outspec
        .Connector<int>(_tokens->out)
        ;

    return new VdfTestUtils::CallbackNode(
        net, inspec, outspec, &ReadCallback);
}

static VdfInputVector<int> *
CreateInputNode(VdfNetwork *net, size_t num, int offset)
{
    VdfInputVector<int> *in = new VdfInputVector<int>(net, num);
    for (size_t i = 0; i < num; ++i) {
        in->SetValue(i, i + offset);
    }
    return in;
}

static VdfEmptyInputVector *
CreateEmptyInputNode(VdfNetwork *net)
{
    static const TfType intType = TfType::Find<int>();

    return new VdfEmptyInputVector(net, intType);
}

static void
BoxedInputCallback(const VdfContext &context) 
{
    TRACE_FUNCTION();

    const std::pair<int, int> numAndOffset =
        context.GetInputValue<std::pair<int, int>>(_tokens->in);

    VdfReadWriteIterator<int> rwit =
        VdfReadWriteIterator<int>::Allocate(context, numAndOffset.first);
    for (size_t i = 0; !rwit.IsAtEnd(); ++i, ++rwit) {
        *rwit = i + numAndOffset.second;
    }
}

static VdfNode *
CreateBoxedInputNode(VdfNetwork *net) 
{
    VdfInputSpecs inspec;
    inspec
        .ReadConnector<std::pair<int, int>>(_tokens->in)
        ;

    VdfOutputSpecs outspec;
    outspec
        .Connector<int>(_tokens->out)
        ;

    return new VdfTestUtils::CallbackNode(
        net, inspec, outspec, &BoxedInputCallback);
}

static void
TestInputValuesPointer() 
{
    TRACE_FUNCTION();

    std::cout << "TestInputValuesPointer..." << std::endl;

    // Register int type needed to create empty input nodes.
    VdfExecutionTypeRegistry::Define<int>(0);

    VdfNetwork net;

    // Create a bunch of input nodes to supply arrays of integers
    VdfEmptyInputVector *in_empty = CreateEmptyInputNode(&net);

    VdfInputVector<int> *in100 = CreateInputNode(&net, 100, 0);

    VdfInputVector<int> *in50_1 = CreateInputNode(&net, 50, 0);
    VdfInputVector<int> *in50_2 = CreateInputNode(&net, 50, 50);

    VdfInputVector<int> *in20_1 = CreateInputNode(&net, 20, 0);
    VdfInputVector<int> *in20_2 = CreateInputNode(&net, 20, 20);
    VdfInputVector<int> *in20_3 = CreateInputNode(&net, 20, 40);
    VdfInputVector<int> *in20_4 = CreateInputNode(&net, 20, 60);
    VdfInputVector<int> *in20_5 = CreateInputNode(&net, 20, 80);

    // Create a bunch of input nodes to supply boxed integer values
    VdfInputVector<std::pair<int, int>> *num100 =
        new VdfInputVector<std::pair<int, int>>(&net, 1);
    num100->SetValue(0, std::make_pair(100, 0));
    VdfNode *boxedIn100 = CreateBoxedInputNode(&net);
    net.Connect(
        num100->GetOutput(), boxedIn100, _tokens->in, VdfMask::AllOnes(1));

    VdfInputVector<std::pair<int, int>> *num50_1 =
        new VdfInputVector<std::pair<int, int>>(&net, 1);
    num50_1->SetValue(0, std::make_pair(50, 0));
    VdfInputVector<std::pair<int, int>> *num50_2 =
        new VdfInputVector<std::pair<int, int>>(&net, 1);
    num50_2->SetValue(0, std::make_pair(50, 50));
    VdfNode *boxedIn50_1 = CreateBoxedInputNode(&net);
    VdfNode *boxedIn50_2 = CreateBoxedInputNode(&net);
    net.Connect(
        num50_1->GetOutput(), boxedIn50_1, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(
        num50_2->GetOutput(), boxedIn50_2, _tokens->in, VdfMask::AllOnes(1));

    // Create a bunch of nodes that read the array and boxed inputs in various
    // combinations, always totalling 100 elements. The elements will be read
    // using the VdfInputValuesPointer. Not all of these combinations will
    // result in contiguous memory layout in the output buffers.
    VdfNode *read0 = CreateReadNode(&net);

    VdfNode *read0Empty = CreateReadNode(&net);
    net.Connect(
        in_empty->GetOutput(), read0Empty, _tokens->in, VdfMask::AllOnes(1));

    VdfNode *read100 = CreateReadNode(&net);
    net.Connect(
        in100->GetOutput(), read100, _tokens->in, VdfMask::AllOnes(100));

    VdfNode *read100AndEmpty = CreateReadNode(&net);
    net.Connect(
        in100->GetOutput(), read100AndEmpty, _tokens->in,
        VdfMask::AllOnes(100));
    net.Connect(
        in_empty->GetOutput(), read100AndEmpty, _tokens->in,
        VdfMask::AllOnes(1));

    VdfNode *read50 = CreateReadNode(&net);
    net.Connect(
        in50_1->GetOutput(), read50, _tokens->in, VdfMask::AllOnes(50));
    net.Connect(
        in50_2->GetOutput(), read50, _tokens->in, VdfMask::AllOnes(50));

    VdfNode *read20 = CreateReadNode(&net);
    net.Connect(
        in20_1->GetOutput(), read20, _tokens->in, VdfMask::AllOnes(20));
    net.Connect(
        in20_2->GetOutput(), read20, _tokens->in, VdfMask::AllOnes(20));
    net.Connect(
        in20_3->GetOutput(), read20, _tokens->in, VdfMask::AllOnes(20));
    net.Connect(
        in20_4->GetOutput(), read20, _tokens->in, VdfMask::AllOnes(20));
    net.Connect(
        in20_5->GetOutput(), read20, _tokens->in, VdfMask::AllOnes(20));

    VdfNode *readBoxed100 = CreateReadNode(&net);
    net.Connect(
        boxedIn100->GetOutput(),
        readBoxed100, _tokens->in, VdfMask::AllOnes(1));

    VdfNode *readBoxed50 = CreateReadNode(&net);
    net.Connect(
        boxedIn50_1->GetOutput(),
        readBoxed50, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(
        boxedIn50_2->GetOutput(),
        readBoxed50, _tokens->in, VdfMask::AllOnes(1));

    VdfNode *readBoxedMixed = CreateReadNode(&net);
    net.Connect(
        boxedIn50_1->GetOutput(),
        readBoxedMixed, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(
        in50_2->GetOutput(),
        readBoxedMixed, _tokens->in, VdfMask::AllOnes(50));

    // Create a request with all these read nodes in it
    VdfMaskedOutputVector mos;
    mos.emplace_back(read0->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(read0Empty->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(read100->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(read100AndEmpty->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(read50->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(read20->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readBoxed100->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readBoxed50->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readBoxedMixed->GetOutput(), VdfMask::AllOnes(1));

    // Schedule the request
    VdfRequest request(mos);
    VdfSchedule schedule;
    VdfScheduler::Schedule(request, &schedule, true /* topologicalSort */);

    // Run the request on a simple executor.
    VdfSimpleExecutor exec;
    exec.Run(schedule);

    std::cout << "... done" << std::endl;
}

int 
main(int argc, char **argv) 
{
    TraceCollector::GetInstance().SetEnabled(true);

    {
        TRACE_SCOPE("main");
        TestInputValuesPointer();
    }

    TraceCollector::GetInstance().SetEnabled(false);
    TraceReporter::GetGlobalReporter()->Report(std::cout);

    return 0;
}

