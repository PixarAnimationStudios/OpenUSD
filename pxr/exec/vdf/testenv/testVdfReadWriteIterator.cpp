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
#include "pxr/exec/vdf/readWriteIterator.h"
#include "pxr/exec/vdf/readWriteIteratorRange.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/simpleExecutor.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/tf/staticTokens.h"

#include <algorithm>
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (in)
    (out)
);

constexpr size_t N = 1000;

static void
ReadCallback(const VdfContext &context) 
{
    VdfReadIterator<int> rit(context, _tokens->in);
    TF_VERIFY(!rit.IsAtEnd());

    // Allocate a boxed value with a named output.
    VdfReadWriteIterator<int> it =
        VdfReadWriteIterator<int>::Allocate(context, _tokens->out, N);
    VdfReadWriteIteratorRange<int> range(it);
    std::fill(range.begin(), range.end(), 1);
    TF_VERIFY(!it.IsAtEnd());

    // Store something.
    for (; !it.IsAtEnd(); ++it) {
        TF_VERIFY(*it == 1);
        *it = 2;
    }

    // Allocate another boxed value. This should replace the existing value.
    it = VdfReadWriteIterator<int>::Allocate(context, N);
    range = VdfReadWriteIteratorRange<int>(it);
    std::fill(range.begin(), range.end(), 0);
    TF_VERIFY(!it.IsAtEnd());

    // Iterate and increment input values.
    for (; !it.IsAtEnd() && !rit.IsAtEnd(); ++it, ++rit) {
        TF_VERIFY(*it == 0);
        *it = *rit + 1;
    }

    // Verify that both iterators are at end.
    TF_VERIFY(it.IsAtEnd());
    TF_VERIFY(rit.IsAtEnd());

    // Create another read/write iterator.
    VdfReadWriteIterator<int> jt(context);
    TF_VERIFY(!jt.IsAtEnd());

    // Increment once more.
    for(; !jt.IsAtEnd(); ++jt) {
        *jt += 1;
    }

    // At end?
    TF_VERIFY(it.IsAtEnd());
    TF_VERIFY(jt.IsAtEnd());

    // Both should compare equal.
    TF_VERIFY(it == jt);

    // Create another read/write iterator, advance it to-end and compare.
    VdfReadWriteIterator<int> kt(context);
    kt.AdvanceToEnd();
    TF_VERIFY(jt == kt);
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

static void
ReadWriteCallback(const VdfContext &context) 
{
    // Create two read/write iterators.
    VdfReadWriteIterator<int> it(context, _tokens->in);
    TF_VERIFY(!it.IsAtEnd());

    VdfReadWriteIterator<int> jt(context, _tokens->out);
    TF_VERIFY(!jt.IsAtEnd());

    // Increment input values with the first iterator.
    for (; !it.IsAtEnd(); ++it) {
        *it += 1;
    }

    // Should be at end now.
    TF_VERIFY(it.IsAtEnd());
    TF_VERIFY(!jt.IsAtEnd());

    // Increment values once again with the second iterator. It should be
    // able to observe the results from the first round of iteration.
    for (; !jt.IsAtEnd(); ++jt) {
        *jt += 1;
    }

    // Both at end?
    TF_VERIFY(it.IsAtEnd());
    TF_VERIFY(jt.IsAtEnd());

    // Increment once more via std::transform.
    VdfReadWriteIterator<int> begin(context);
    VdfReadWriteIterator<int> end(begin);
    end.AdvanceToEnd();
    std::transform(begin, end, begin, [](int v) {
        return v + 1;
    });
}

static VdfNode *
CreateReadWriteNode(VdfNetwork *net) 
{
    VdfInputSpecs inspec;
    inspec
        .ReadWriteConnector<int>(_tokens->in, _tokens->out)
        ;

    VdfOutputSpecs outspec;
    outspec
        .Connector<int>(_tokens->out)
        ;

    return new VdfTestUtils::CallbackNode(
        net, inspec, outspec, &ReadWriteCallback);
}

static VdfInputVector<int> *
CreateInputNode(VdfNetwork *net, size_t num, int offset)
{
    VdfInputVector<int> *in = new VdfInputVector<int>(net, num);
    for (size_t i = 0; i < num; ++i) {
        in->SetValue(i, 0);
    }
    return in;
}

static void
BoxedInputCallback(const VdfContext &context) 
{
    TRACE_FUNCTION();

    const int num = context.GetInputValue<int>(_tokens->in);

    VdfReadWriteIteratorRange<int> range(
        VdfReadWriteIterator<int>::Allocate(context, num));
    std::fill(range.begin(), range.end(), 0);
}

static VdfNode *
CreateBoxedInputNode(VdfNetwork *net) 
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
        net, inspec, outspec, &BoxedInputCallback);
}

static void
VerifyResults(
    const VdfSimpleExecutor &exec,
    const VdfMaskedOutput &mo,
    const size_t begin,
    const size_t end,
    const int expected)
{
    const VdfVector *v = exec.GetOutputValue(*mo.GetOutput(), mo.GetMask());
    const VdfVector::ReadAccessor<int> a = v->GetReadAccessor<int>();
    for (size_t i = begin; i < end; ++i) {
        if (a[i] != expected) {
            std::cout
                << "   a[" << i << "] = " << a[i]
                << ", expected = " << expected
                << std::endl;
            TF_VERIFY(a[i] == expected);
        }
    }
}

static void
TestReadWriteIterator() 
{
    TRACE_FUNCTION();

    std::cout << "TestReadWriteIterator..." << std::endl;

    VdfNetwork net;

    // Create an input node that supplies a vector of integers
    VdfInputVector<int> *inVec = CreateInputNode(&net, N, 0);

    // Create an input node that supplies a boxed vector of integers
    VdfInputVector<int> *num = new VdfInputVector<int>(&net, 1);
    num->SetValue(0, N);
    VdfNode *inBoxed = CreateBoxedInputNode(&net);
    net.Connect(num->GetOutput(), inBoxed, _tokens->in, VdfMask::AllOnes(1));

    // Create a node that reads the vector of integers.
    VdfNode *readVec0 = CreateReadNode(&net);
    net.Connect(inVec->GetOutput(), readVec0, _tokens->in, VdfMask::AllOnes(N));

    // Create a small chain of nodes that read the boxed vector of integers.
    VdfNode *readBoxed0 = CreateReadNode(&net);
    net.Connect(
        inBoxed->GetOutput(), readBoxed0, _tokens->in, VdfMask::AllOnes(1));

    VdfNode *readBoxed1 = CreateReadNode(&net);
    net.Connect(
        readBoxed0->GetOutput(), readBoxed1, _tokens->in, VdfMask::AllOnes(1));

    // Create a small chain of nodes that read/write the vector of integers.
    VdfNode *readWriteVec0 = CreateReadWriteNode(&net);
    readWriteVec0->GetOutput()->SetAffectsMask(VdfMask::AllOnes(N));
    net.Connect(
        inVec->GetOutput(), readWriteVec0, _tokens->in, VdfMask::AllOnes(N));

    VdfMask::Bits bits(N, 0, (N / 2) - 1);
    VdfNode *readWriteVec1 = CreateReadWriteNode(&net);
    readWriteVec1->GetOutput()->SetAffectsMask(VdfMask(bits));
    net.Connect(
        readWriteVec0->GetOutput(),
        readWriteVec1, _tokens->in, VdfMask::AllOnes(N));

    // Create a small chain of nodes that read/write the boxed vector.
    VdfNode *readWriteBoxed0 = CreateReadWriteNode(&net);
    readWriteBoxed0->GetOutput()->SetAffectsMask(VdfMask::AllOnes(1));
    net.Connect(
        inBoxed->GetOutput(),
        readWriteBoxed0, _tokens->in, VdfMask::AllOnes(1));

    VdfNode *readWriteBoxed1 = CreateReadWriteNode(&net);
    readWriteBoxed1->GetOutput()->SetAffectsMask(VdfMask::AllOnes(1));
    net.Connect(
        readWriteBoxed0->GetOutput(),
        readWriteBoxed1, _tokens->in, VdfMask::AllOnes(1));

    // Create a request with all the leaf nodes in it.
    VdfMaskedOutputVector mos;
    mos.emplace_back(readVec0->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readBoxed1->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readWriteVec1->GetOutput(), VdfMask::AllOnes(N));
    mos.emplace_back(readWriteBoxed1->GetOutput(), VdfMask::AllOnes(1));

    // Schedule the request
    VdfRequest request(mos);
    VdfSchedule schedule;
    VdfScheduler::Schedule(request, &schedule, true /* topologicalSort */);

    // Run the request on a simple executor.
    VdfSimpleExecutor exec;
    exec.Run(schedule);

    // Verify results for each output.
    std::cout << "   Verify read with vectorized data." << std::endl;
    VerifyResults(exec, mos[0], 0, N, 2);

    std::cout << "   Verify read with boxed data." << std::endl;
    VerifyResults(exec, mos[1], 0, N, 4);

    std::cout << "   Verify read/write with vectorized data." << std::endl;
    VerifyResults(exec, mos[2], 0, N / 2, 6);
    VerifyResults(exec, mos[2], N / 2, N, 3);

    std::cout << "   Verify read/write with boxed data." << std::endl;
    VerifyResults(exec, mos[3], 0, N, 6);

    std::cout << "... done" << std::endl;
}

int 
main(int argc, char **argv) 
{
    TestReadWriteIterator();

    return 0;
}

