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
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/simpleExecutor.h"
#include "pxr/exec/vdf/subrangeView.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/tf/staticTokens.h"

#include <initializer_list>
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (in)
    (out)
);

namespace {

struct Result {
    int numRanges;
    int numEmpty;
    int numNonEmpty;
    int numElements;
};

}

static void
ReadCallback(const VdfContext &context) 
{
    Result result{0, 0, 0, 0};

    VdfSubrangeView<VdfReadIteratorRange<int>> subranges(context, _tokens->in);
    for (const VdfReadIteratorRange<int> &range : subranges) {
        ++result.numRanges;

        if (range.IsEmpty()) {
            ++result.numEmpty;
        } else {
            ++result.numNonEmpty;
        }

        for (const int x : range) {
            ++result.numElements;
            TF_VERIFY(x == 1);
        }
    }

    context.SetOutput(result);
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
        .Connector<Result>(_tokens->out)
        ;

    return new VdfTestUtils::CallbackNode(
        net, inspec, outspec, &ReadCallback);
}

static VdfInputVector<int> *
CreateInputNode(VdfNetwork *net, size_t num, int value)
{
    VdfInputVector<int> *in = new VdfInputVector<int>(net, num);
    for (size_t i = 0; i < num; ++i) {
        in->SetValue(i, value);
    }
    return in;
}

static void
BoxedInputCallbackA(const VdfContext &context) 
{
    const std::initializer_list<int> ten = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    const std::initializer_list<int> zero = {};

    Vdf_BoxedContainer<int> result;
    result.AppendRange(zero.begin(), zero.end());
    result.AppendRange(ten.begin(), ten.end());
    result.AppendRange(zero.begin(), zero.end());

    context.SetOutput(result);
}

static void
BoxedInputCallbackB(const VdfContext &context) 
{
    const std::initializer_list<int> ten = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    const std::initializer_list<int> zero = {};

    Vdf_BoxedContainer<int> result;
    result.AppendRange(ten.begin(), ten.end());
    result.AppendRange(zero.begin(), zero.end());
    result.AppendRange(ten.begin(), ten.end());
    result.AppendRange(ten.begin(), ten.end());

    context.SetOutput(result);
}

static void
BoxedInputCallback0(const VdfContext &context) 
{
    Vdf_BoxedContainer<int> result(0);
    context.SetOutput(result);
}

template < typename F>
static VdfNode *
CreateBoxedInputNode(VdfNetwork *net, F callback) 
{
    VdfInputSpecs inspec;

    VdfOutputSpecs outspec;
    outspec
        .Connector<int>(_tokens->out)
        ;

    return new VdfTestUtils::CallbackNode(
        net, inspec, outspec, callback);
}

static void
DoNothing(const VdfContext &context) 
{
}

static VdfNode *
CreateEmptyInputNode(VdfNetwork *net) 
{
    return new VdfTestUtils::CallbackNode(
        net,
        VdfInputSpecs(),
        VdfOutputSpecs().Connector<int>(_tokens->out),
        &DoNothing);
}

static void
VerifyField(
    int value,
    int expected,
    const char *name)
{
    if (value != expected) {
        std::cout
            << name
            << ": expected " << expected
            << ", have " << value
            << std::endl;
        TF_VERIFY(value == expected);
    }
}

static void
VerifyResult(
    const VdfSimpleExecutor &exec,
    const VdfMaskedOutput &mo,
    const Result expected)
{
    const VdfVector *v = exec.GetOutputValue(*mo.GetOutput(), mo.GetMask());
    const VdfVector::ReadAccessor<Result> a = v->GetReadAccessor<Result>();
    VerifyField(a[0].numRanges, expected.numRanges, "numRanges");
    VerifyField(a[0].numEmpty, expected.numEmpty, "numEmpty");
    VerifyField(a[0].numNonEmpty, expected.numNonEmpty, "numNonEmpty");
    VerifyField(a[0].numElements, expected.numElements, "numElements");
    std::cout << "    ... matches." << std::endl;
}

static void
TestReadIteratorSubrange() 
{
    TRACE_FUNCTION();

    std::cout << "TestReadIteratorSubrange..." << std::endl;

    VdfNetwork net;

    // Create a bunch of input nodes to supply arrays of integers
    VdfInputVector<int> *in0 = CreateInputNode(&net, 0, 1);
    VdfInputVector<int> *in10A = CreateInputNode(&net, 10, 1);
    VdfInputVector<int> *in10B = CreateInputNode(&net, 10, 1);

    // Create a bunch of input nodes to supply boxed integer values
    VdfNode *boxedIn10A = CreateBoxedInputNode(&net, &BoxedInputCallbackA);
    VdfNode *boxedIn10B = CreateBoxedInputNode(&net, &BoxedInputCallbackB);
    VdfNode *boxedIn0 = CreateBoxedInputNode(&net, &BoxedInputCallback0);
    VdfNode *empty = CreateEmptyInputNode(&net);

    // Create a bunch of nodes that read the array and boxed inputs in various
    // combinations.
    VdfNode *readA = CreateReadNode(&net);
    net.Connect(in0->GetOutput(), readA, _tokens->in, VdfMask::AllOnes(0));

    VdfNode *readB = CreateReadNode(&net);
    net.Connect(boxedIn0->GetOutput(), readB, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(in10A->GetOutput(), readB, _tokens->in, VdfMask::AllOnes(10));

    VdfNode *readC = CreateReadNode(&net);
    net.Connect(boxedIn0->GetOutput(), readC, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(in10A->GetOutput(), readC, _tokens->in, VdfMask::AllOnes(10));
    net.Connect(boxedIn0->GetOutput(), readC, _tokens->in, VdfMask::AllOnes(1));

    VdfNode *readD = CreateReadNode(&net);
    net.Connect(boxedIn0->GetOutput(), readD, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(in10A->GetOutput(), readD, _tokens->in, VdfMask::AllOnes(10));
    net.Connect(in10B->GetOutput(), readD, _tokens->in, VdfMask::AllOnes(0));
    net.Connect(boxedIn0->GetOutput(), readD, _tokens->in, VdfMask::AllOnes(1));

    VdfNode *readE = CreateReadNode(&net);
    net.Connect(boxedIn0->GetOutput(), readE, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(in10A->GetOutput(), readE, _tokens->in, VdfMask::AllOnes(10));
    net.Connect(boxedIn0->GetOutput(), readE, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(in10B->GetOutput(), readE, _tokens->in, VdfMask::AllOnes(10));

    VdfNode *readF = CreateReadNode(&net);
    net.Connect(in10A->GetOutput(), readF, _tokens->in, VdfMask::AllOnes(10));
    net.Connect(boxedIn0->GetOutput(), readF, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(
        boxedIn10A->GetOutput(), readF, _tokens->in, VdfMask::AllOnes(1));

    VdfNode *readG = CreateReadNode(&net);
    net.Connect(in10A->GetOutput(), readG, _tokens->in, VdfMask::AllOnes(10));
    net.Connect(
        boxedIn10A->GetOutput(), readG, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(in10A->GetOutput(), readG, _tokens->in, VdfMask::AllOnes(10));

    VdfNode *readH = CreateReadNode(&net);
    net.Connect(in10A->GetOutput(), readH, _tokens->in, VdfMask::AllOnes(10));
    net.Connect(
        boxedIn10A->GetOutput(), readH, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(in10A->GetOutput(), readH, _tokens->in, VdfMask::AllOnes(10));
    net.Connect(
        boxedIn10B->GetOutput(), readH, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(in10B->GetOutput(), readH, _tokens->in, VdfMask::AllOnes(10));

    VdfNode *readI = CreateReadNode(&net);
    net.Connect(in10A->GetOutput(), readI, _tokens->in, VdfMask::AllOnes(10));
    net.Connect(
        boxedIn10A->GetOutput(), readI, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(empty->GetOutput(), readI, _tokens->in, VdfMask::AllOnes(1));

    VdfNode *readJ = CreateReadNode(&net);
    net.Connect(in10A->GetOutput(), readJ, _tokens->in, VdfMask::AllOnes(10));
    net.Connect(empty->GetOutput(), readJ, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(
        boxedIn10A->GetOutput(), readJ, _tokens->in, VdfMask::AllOnes(1));

    VdfNode *readK = CreateReadNode(&net);
    net.Connect(empty->GetOutput(), readK, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(
        boxedIn10A->GetOutput(), readK, _tokens->in, VdfMask::AllOnes(1));
    net.Connect(in10A->GetOutput(), readK, _tokens->in, VdfMask::AllOnes(10));

    // Create a request with all these read nodes in it
    VdfMaskedOutputVector mos;
    mos.emplace_back(readA->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readB->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readC->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readD->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readE->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readF->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readG->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readH->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readI->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readJ->GetOutput(), VdfMask::AllOnes(1));
    mos.emplace_back(readK->GetOutput(), VdfMask::AllOnes(1));

    // Schedule the request
    VdfRequest request(mos);
    VdfSchedule schedule;
    VdfScheduler::Schedule(request, &schedule, true /* topologicalSort */);

    // Run the request on a simple executor.
    VdfSimpleExecutor exec;
    exec.Run(schedule);

    // Verify results: numRanges, numEmpty, numNonEmpty, numElements
    VerifyResult(exec, mos[0], Result{0, 0, 0, 0});
    VerifyResult(exec, mos[1], Result{2, 1, 1, 10});
    VerifyResult(exec, mos[2], Result{3, 2, 1, 10});
    VerifyResult(exec, mos[3], Result{3, 2, 1, 10});
    VerifyResult(exec, mos[4], Result{4, 2, 2, 20});
    VerifyResult(exec, mos[5], Result{5, 3, 2, 20});
    VerifyResult(exec, mos[6], Result{5, 2, 3, 30});
    VerifyResult(exec, mos[7], Result{10, 3, 7, 70});
    VerifyResult(exec, mos[8], Result{5, 3, 2, 20});
    VerifyResult(exec, mos[9], Result{5, 3, 2, 20});
    VerifyResult(exec, mos[10], Result{5, 3, 2, 20});

    std::cout << "... done" << std::endl;
}

int 
main(int argc, char **argv) 
{
    TestReadIteratorSubrange();

    return 0;
}

