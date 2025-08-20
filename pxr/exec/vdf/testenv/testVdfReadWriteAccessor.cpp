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
#include "pxr/exec/vdf/readWriteAccessor.h"
#include "pxr/exec/vdf/readWriteIterator.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/simpleExecutor.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/work/loops.h"

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

    // There shouldn't be any data at the output at this point.
    VdfReadWriteAccessor<int> empty(context);
    TF_VERIFY(empty.IsEmpty());
    TF_VERIFY(empty.GetSize() == 0);

    // Allocate a boxed value with a named output.
    VdfReadWriteIterator<int> it =
        VdfReadWriteIterator<int>::Allocate(context, _tokens->out, N);
    TF_VERIFY(!it.IsAtEnd());

    // Fill with values.
    for (int i = 0; !it.IsAtEnd(); ++i, ++it) {
        *it = i;
    }
    TF_VERIFY(it.IsAtEnd());

    // Read back the values with the accessor.
    VdfReadWriteAccessor<int> a(context, _tokens->out);
    TF_VERIFY(a.GetSize() == N);
    TF_VERIFY(!a.IsEmpty());
    for (size_t i = 0; i < N; ++i) {
        TF_VERIFY(a[i] == static_cast<int>(i));
    }

    // Write different output values with the same accessor.
    TF_VERIFY(a.GetSize() == N);
    TF_VERIFY(!a.IsEmpty());
    for (int i = N - 1; i >= 0; --i) {
        TF_VERIFY(a[i] == i);
        a[i] = 2;
    }

    // Create a const accessor and read back.
    const VdfReadWriteAccessor<int> b(context, _tokens->out);
    TF_VERIFY(b.GetSize() == N);
    TF_VERIFY(!b.IsEmpty());
    for (size_t i = 0; i < N; ++i) {
        TF_VERIFY(b[i] == 2);
    }

    // Create a different accessor, which should read the same values. Fill
    // with new values.
    VdfReadWriteAccessor<int> c(context);
    TF_VERIFY(c.GetSize() == a.GetSize());
    TF_VERIFY(a.IsEmpty() == c.IsEmpty());
    for (size_t i = 0; i < N && !rit.IsAtEnd(); ++i, ++rit) {
        TF_VERIFY(a[i] == 2);
        a[i] += *rit;
    }
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
    // Create a read/write accessor with the associated input.
    VdfReadWriteAccessor<int> a(context, _tokens->in);
    TF_VERIFY(!a.IsEmpty());

    // Increment input values.
    for (size_t i = 0; i < a.GetSize(); ++i) {
        a[i] += 1;
    }

    // Create a read/write accessor with the associated output.
    VdfReadWriteAccessor<int> b(context, _tokens->out);
    TF_VERIFY(!b.IsEmpty());
    TF_VERIFY(a.GetSize() == b.GetSize());

    // Increment input values.
    for (size_t i = 0; i < b.GetSize(); ++i) {
        a[i] += 1;
    }

    // Parallel increment by re-using the first accessor.
    WorkParallelForN(a.GetSize(), [&a](size_t b, size_t e){
        for (size_t i = b; i != e; ++i) {
            a[i] += 1;
        }
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

    VdfReadWriteIterator<int> it = 
        VdfReadWriteIterator<int>::Allocate(context, num);
    for (; !it.IsAtEnd(); ++it) {
        *it = 0;
    }
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

            if (a[i] != expected) {
                VDF_FATAL_ERROR(mo.GetOutput()->GetNode(), "error");
            }

            TF_VERIFY(a[i] == expected);
        }
    }
}

static void
TestReadWriteAccessor() 
{
    TRACE_FUNCTION();

    std::cout << "TestReadWriteAccessor..." << std::endl;

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
    // All-ones mask:
    VdfNode *readWriteVec0 = CreateReadWriteNode(&net);
    readWriteVec0->GetOutput()->SetAffectsMask(VdfMask::AllOnes(N));
    net.Connect(
        inVec->GetOutput(), readWriteVec0, _tokens->in, VdfMask::AllOnes(N));

    // Contiguous mask:
    VdfMask::Bits bits1(N, 0, (N / 2) - 1);
    VdfNode *readWriteVec1 = CreateReadWriteNode(&net);
    readWriteVec1->GetOutput()->SetAffectsMask(VdfMask(bits1));
    net.Connect(
        readWriteVec0->GetOutput(),
        readWriteVec1, _tokens->in, VdfMask::AllOnes(N));

    // Non-contiguous mask:
    VdfMask::Bits bits2;
    bits2.Append(N / 2, true);
    bits2.Append(N / 4, false);
    bits2.Append(N / 4, true);
    VdfNode *readWriteVec2 = CreateReadWriteNode(&net);
    readWriteVec2->GetOutput()->SetAffectsMask(VdfMask(bits2));
    net.Connect(
        readWriteVec1->GetOutput(),
        readWriteVec2, _tokens->in, VdfMask::AllOnes(N));

    // Contiguous mask with offset:
    VdfMask::Bits bits3;
    bits3.Append(N / 4, false);
    bits3.Append(N / 4, true);
    bits3.Append(N / 2, false);
    VdfNode *readWriteVec3 = CreateReadWriteNode(&net);
    readWriteVec3->GetOutput()->SetAffectsMask(VdfMask(bits3));
    net.Connect(
        readWriteVec2->GetOutput(),
        readWriteVec3, _tokens->in, VdfMask::AllOnes(N));

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
    mos.emplace_back(readWriteVec3->GetOutput(), VdfMask::AllOnes(N));
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
    VerifyResults(exec, mos[2], 0, N / 4, 9);
    VerifyResults(exec, mos[2], N / 4, N / 2, 12);
    VerifyResults(exec, mos[2], N / 2, (N / 4) * 3, 3);
    VerifyResults(exec, mos[2], (N / 4) * 3, N, 6);

    std::cout << "   Verify read/write with boxed data." << std::endl;
    VerifyResults(exec, mos[3], 0, N, 6);

    std::cout << "... done" << std::endl;
}

int 
main(int argc, char **argv) 
{
    TestReadWriteAccessor();

    return 0;
}

