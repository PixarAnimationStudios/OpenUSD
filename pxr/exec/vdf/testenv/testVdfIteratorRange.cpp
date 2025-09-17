//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/connectorSpecs.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/inputVector.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/readIterator.h"
#include "pxr/exec/vdf/readIteratorRange.h"
#include "pxr/exec/vdf/readWriteIterator.h"
#include "pxr/exec/vdf/readWriteIteratorRange.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/simpleExecutor.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/tf/staticTokens.h"

#include <algorithm>
#include <iostream>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (in)
    (out)
);

template < int N >
static void
ReadCallback(const VdfContext &context) 
{
    VdfReadIteratorRange<int> range(context, _tokens->in);

    // If this is an empty range ...
    if (N == 0) {
        TF_AXIOM(range.IsEmpty());
        TF_AXIOM(range.begin() == range.end());
        TF_AXIOM(range.begin().IsAtEnd());
        TF_AXIOM(range.end().IsAtEnd());

        context.SetOutput(N);
        return;
    }

    // Range size must match N
    TF_AXIOM(range.begin().ComputeSize() == N);

    // Ranges should match up with VdfReadIterator
    VdfReadIterator<int> it(context, _tokens->in);

    // Begin should not be at end at this point.
    VdfReadIterator<int> begin = range.begin();
    TF_AXIOM(!begin.IsAtEnd());
    TF_AXIOM(begin == it);

    // End should always be at end.
    const VdfReadIterator<int> end = range.end();
    TF_AXIOM(end.IsAtEnd());

    // Iterate and compare.
    for (; begin != end; ++begin, ++it) {
        TF_AXIOM(begin == it);
        TF_AXIOM(*begin == *it);
    }
    TF_AXIOM(begin.IsAtEnd());
    TF_AXIOM(it.IsAtEnd());
    TF_AXIOM(it == begin && it == end);

    // Range-based for loops should work on iterator ranges.
    int i = 0;
    for (int x : range) {
        TF_AXIOM(x == i);
        ++i;
    }
    TF_AXIOM(i == N);

    // Vector insertion should work on iterator ranges.
    std::vector<int> v1(range.begin(), range.end());
    for (size_t i = 0; i < v1.size(); ++i) {
        TF_AXIOM(v1[i] == static_cast<int>(i));
    }

    // Copy should work on iterator ranges.
    std::vector<int> v2(range.begin().ComputeSize());
    std::copy(range.begin(), range.end(), v2.begin());
    for (size_t i = 0; i < v2.size(); ++i) {
        TF_AXIOM(v2[i] == static_cast<int>(i));
    }

    // Count should work on iterator ranges.
    TF_AXIOM(std::count(range.begin(), range.end(), 0) == 1);

    // Find should work on iterator ranges.
    TF_AXIOM(std::find(range.begin(), range.end(), 0) == range.begin());
    TF_AXIOM(std::find(range.begin(), range.end(), N + 1) == range.end());

    context.SetOutput(N);
}

template < int N >
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
        net, inspec, outspec, &ReadCallback<N>);
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

static void
BoxedInputCallback(const VdfContext &context) 
{
    TRACE_FUNCTION();

    const std::pair<int, int> numAndOffset =
        context.GetInputValue<std::pair<int, int>>(_tokens->in);
    const int num = numAndOffset.first;
    const int offset = numAndOffset.second;

    // Create a new boxed value of size num.
    VdfReadWriteIterator<int>::Allocate(context, num);

    // Filling a range should work.
    VdfReadWriteIteratorRange<int> range(context);
    std::fill(range.begin(), range.end(), 1);

    // Counting on a range should work.
    TF_AXIOM(std::count(range.begin(), range.end(), 1) == num);

    // Find should work.
    TF_AXIOM(std::find(range.begin(), range.end(), 1) == range.begin());
    TF_AXIOM(std::find(range.begin(), range.end(), num + 1) == range.end());

    // Copying from a vector should work.
    std::vector<int> v(num, 2);
    std::copy(v.begin(), v.end(), range.begin());

    // Transforming should work.
    std::transform(range.begin(), range.end(), range.begin(), [](int v) {
        return v + 1;
    });

    // Range-based for loop should work.
    for (int v : range) {
        TF_AXIOM(v == 3);
    }

    // Fill the output value with values different for each element.
    VdfReadWriteIterator<int> rwit(context);
    for (size_t i = 0; !rwit.IsAtEnd(); ++i, ++rwit) {
        *rwit = i + offset;
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
TestIteratorRange() 
{
    TRACE_FUNCTION();

    std::cout << "TestIteratorRange..." << std::endl;

    VdfNetwork net;

    // Create a bunch of input nodes to supply arrays of integers
    VdfInputVector<int> *in100 = CreateInputNode(&net, 100, 0);

    VdfInputVector<int> *in50_1 = CreateInputNode(&net, 50, 0);
    VdfInputVector<int> *in50_2 = CreateInputNode(&net, 50, 50);

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
    VdfNode *boxedIn50_1 = CreateBoxedInputNode(&net);
    net.Connect(
        num50_1->GetOutput(), boxedIn50_1, _tokens->in, VdfMask::AllOnes(1));

    // Create a bunch of nodes that read the array and boxed inputs in various
    // combinations, always totalling 100 elements. The elements will be read
    // using the VdfReadIteratorRange.
    VdfNode *read0 = CreateReadNode<0>(&net);

    VdfNode *read100 = CreateReadNode<100>(&net);
    net.Connect(
        in100->GetOutput(), read100, _tokens->in, VdfMask::AllOnes(100));

    VdfNode *read50 = CreateReadNode<50>(&net);
    net.Connect(
        in50_1->GetOutput(), read50, _tokens->in, VdfMask::AllOnes(50));

    VdfNode *read50_50 = CreateReadNode<100>(&net);
    net.Connect(
        in50_1->GetOutput(), read50_50, _tokens->in, VdfMask::AllOnes(50));
    net.Connect(
        in50_2->GetOutput(), read50_50, _tokens->in, VdfMask::AllOnes(50));

    VdfNode *readBoxed50 = CreateReadNode<50>(&net);
    net.Connect(
        boxedIn50_1->GetOutput(),
        readBoxed50, _tokens->in, VdfMask::AllOnes(1));

    VdfNode *readBoxedMixed = CreateReadNode<100>(&net);
    net.Connect(
        boxedIn50_1->GetOutput(),
        readBoxedMixed, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(
        in50_2->GetOutput(),
        readBoxedMixed, _tokens->in, VdfMask::AllOnes(50));

    // Create a request with all these read nodes in it
    VdfMaskedOutputVector mos;
    mos.emplace_back(read0->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(read100->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(read50->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(read50_50->GetOutput(), VdfMask::AllOnes(1));
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
    TestIteratorRange();

    return 0;
}

