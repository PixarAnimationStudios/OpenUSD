//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/connectorSpecs.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/indexedWeights.h"
#include "pxr/exec/vdf/inputVector.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/rawValueAccessor.h"
#include "pxr/exec/vdf/readIterator.h"
#include "pxr/exec/vdf/readWriteIterator.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/simpleExecutor.h"
#include "pxr/exec/vdf/testUtils.h"
#include "pxr/exec/vdf/typedVector.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (in)
    (input1)
    (readwrite)
    (data)
    (out)
    (weights)
);

static void
OneReadCallback(const VdfContext &context) 
{
    // All this callback does is reads the input and produces a vector
    // with all the inputs read.
    size_t size = 0;
    VdfReadIterator<double> it(context, _tokens->input1);
    for (; !it.IsAtEnd(); ++it) {
        ++size;
    }

    VdfTypedVector<double> output;
    output.template Resize<double>(size);
    VdfVector::ReadWriteAccessor<double> a = 
        output.template GetReadWriteAccessor<double>();

    size_t i = 0;
    VdfReadIterator<double> inIter(context, _tokens->input1);
    for ( ; !inIter.IsAtEnd(); ++inIter, ++i) {
        a[i] = *inIter;
    }

    // Increment past the end to trigger code coverage and ensure that
    // we're still at the end.
    ++inIter;
    TF_AXIOM(inIter.IsAtEnd());

    VdfRawValueAccessor rawValueAccessor(context);
    rawValueAccessor.SetOutputVector(
        *VdfTestUtils::OutputAccessor(context).GetOutput(),
        VdfMask::AllOnes(size),
        output);
}

VdfNode *
CreateOneReadNode(VdfNetwork *net) 
{
    VdfInputSpecs inspec;
    inspec
        .ReadConnector<double>(_tokens->input1)
        .ReadWriteConnector<double>(_tokens->readwrite, _tokens->out)
        ;

    VdfOutputSpecs outspec;
    outspec
        .Connector<double>(_tokens->out)
        ;

    return new VdfTestUtils::CallbackNode(
        net, inspec, outspec, &OneReadCallback);
}

static void
OneReadWriteCallback(const VdfContext &context) 
{
    // All this callback does is reads the input and produces a vector
    // with all the inputs read.
    size_t size = 0;
    VdfReadWriteIterator<double> it(context, _tokens->readwrite);
    for (; !it.IsAtEnd(); ++it) {
        ++size;
    }

    VdfTypedVector<double> output;
    output.template Resize<double>(size);
    VdfVector::ReadWriteAccessor<double> a = 
        output.template GetReadWriteAccessor<double>();

    size_t i = 0;
    VdfReadWriteIterator<double> inIter(context, _tokens->readwrite);
    for ( ; !inIter.IsAtEnd(); ++inIter, ++i) {
        a[i] = *inIter;
    }

    VdfRawValueAccessor rawValueAccessor(context);
    rawValueAccessor.SetOutputVector(
        *VdfTestUtils::OutputAccessor(context).GetOutput(),
        VdfMask::AllOnes(size),
        output);
}

VdfNode *
CreateOneReadWriteNode(VdfNetwork *net) 
{
    VdfInputSpecs inspec;
    inspec
        .ReadWriteConnector<double>(_tokens->readwrite, _tokens->out)
        ;

    VdfOutputSpecs outspec;
    outspec
        .Connector<double>(_tokens->out)
        ;

    return new VdfTestUtils::CallbackNode(
        net, inspec, outspec, &OneReadWriteCallback);
}


static bool
RunReadIteratorTest(VdfNode *node, const VdfMask &mask,
                     const std::vector<double> &expected)
{
    VdfRequest request(VdfMaskedOutput(node->GetOutput(), mask));
    VdfSchedule schedule;
    VdfScheduler::Schedule(request, &schedule, true /* topologicalSort */);
    VdfSimpleExecutor exec;
    exec.Run(schedule);

    VdfVector::ReadAccessor<double> result = exec.GetOutputValue(*node->
        GetOutput(), mask)->GetReadAccessor<double>();

    if (result.GetNumValues() != expected.size()) {
        TF_CODING_ERROR("Expected vector of size %zu, got size %zu",
                        expected.size(), result.GetNumValues());
        return false;
    }

    for (size_t i = 0; i < expected.size(); ++i) {
        if (result[i] != expected[i]) {
            TF_CODING_ERROR("At index %zu expected %f got %f",
                            i, expected[i], result[i]);
            return false;
        }
    }

    return true;
}



static bool
TestReadIterator() 
{
    TRACE_FUNCTION();

    // Setup a situation where we read input from an iterator with 
    // an unusual mask setup on connections.

    VdfSimpleExecutor exec;
    VdfNetwork net;

    VdfInputVector<double> *in1 = new VdfInputVector<double>(&net, 3);

    in1->SetValue(0, 1.0);
    in1->SetValue(1, 2.0);
    in1->SetValue(2, 3.0);

    // Test basic case. All ones, we should get 1, 2, 3 respectively.
    std::cout << "Testing basic all ones mask." << std::endl;
    {
        VdfNode *last = CreateOneReadNode(&net);
        VdfMask mask3 = VdfMask::AllOnes(3);

        net.Connect(in1->GetOutput(), last, _tokens->input1, mask3);

        std::vector<double> expected;
        expected.push_back(1.0);
        expected.push_back(2.0);
        expected.push_back(3.0);
        if (!RunReadIteratorTest(last, mask3, expected)) {
            return false;
        }
    }

    // Test the case where the input vector is wired in 3 times with
    // the masks such that we should expect to get the results backwards.
    std::cout << "Testing 3 connections with 3 single element masks." 
              << std::endl;
    {
        VdfNode *last = CreateOneReadNode(&net);
        VdfMask mask1(3);
        VdfMask mask2(3);
        VdfMask mask3(3);
        VdfMask allOnes(3);
        mask1.SetIndex(0);
        mask2.SetIndex(1);
        mask3.SetIndex(2);
        allOnes.SetAll();

        net.Connect(in1->GetOutput(), last, _tokens->input1, mask3);
        net.Connect(in1->GetOutput(), last, _tokens->input1, mask2);
        net.Connect(in1->GetOutput(), last, _tokens->input1, mask1);

        std::vector<double> expected;
        expected.push_back(3.0);
        expected.push_back(2.0);
        expected.push_back(1.0);
        if (!RunReadIteratorTest(last, allOnes, expected)) {
            return false;
        }
    }

    // Test a case where the first node has an empty mask.
    std::cout << "Testing empty mask on first and last nodes in input "
              << "connector." << std::endl;
    {
        VdfNode *last = CreateOneReadNode(&net);

        VdfMask emptyMask(3);
        VdfMask mask3(3);
        VdfMask allOnes = VdfMask::AllOnes(3);

        mask3.SetIndex(2);

        net.Connect(in1->GetOutput(), last, _tokens->input1, emptyMask);
        net.Connect(in1->GetOutput(), last, _tokens->input1, mask3);
        net.Connect(in1->GetOutput(), last, _tokens->input1, emptyMask);

        std::vector<double> expected;
        expected.push_back(3.0);
        if (!RunReadIteratorTest(last, allOnes, expected)) {
            return false;
        }
    }

    // Test an error condition where the input vector and the mask don't have
    // the same size.
    std::cout << "Testing an error condition where the input vector and "
              << "the mask don't have the same size."
              << std::endl;
    {
        VdfNode *last = CreateOneReadNode(&net);

        VdfMask emptyMask(0);
        VdfMask allOnes = VdfMask::AllOnes(3);

        net.Connect(in1->GetOutput(), last, _tokens->input1, emptyMask);
        std::vector<double> expected;
        if (!RunReadIteratorTest(last, allOnes, expected)) {
            return false;
        }
    }

    return true;
}


static bool
TestSparseIteration() 
{
    TRACE_FUNCTION();

    // Setup a situation where we read input from an iterator with 
    // an unusual mask setup on connections.

    VdfSimpleExecutor exec;
    VdfNetwork net;

    VdfInputVector<double> *in1 = new VdfInputVector<double>(&net, 3);
    in1->SetDebugName("InputVector");

    in1->SetValue(0, 1.0);
    in1->SetValue(1, 2.0);
    in1->SetValue(2, 3.0);

    // Test case where the connection has all set, but the request
    // mask only asks for the 2nd value.
    std::cout << "Testing sparse iteration." << std::endl;
    {
        VdfNode *last = CreateOneReadWriteNode(&net);
        last->SetDebugName("OneReadWriteNode");

        VdfMask mask3 = VdfMask::AllOnes(3);
        VdfMask requestMask(3);
        requestMask.SetIndex(1);

        net.Connect(in1->GetOutput(), last, _tokens->readwrite, mask3);

        std::vector<double> expected;
        expected.push_back(2.0);
        if (!RunReadIteratorTest(last, requestMask, expected)) {
            return false;
        }

        // Do another pull for sanity this time with a full request mask.
        requestMask.SetAll();
        expected.clear();
        expected.push_back(1.0);
        expected.push_back(2.0);
        expected.push_back(3.0);
        if (!RunReadIteratorTest(last, requestMask, expected)) {
            return false;
        }
    }

    return true;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

// The list of tests to run.
typedef bool(*TestFunction)(void);

struct Tests
{
    TestFunction func;
    const char  *name;
};

static Tests tests[] =
{
    { TestReadIterator,       "TestReadIterator"       },          
    { TestSparseIteration,    "TestSparseIteration"    },

    NULL
};


int 
main(int argc, char **argv) 
{
    int res = 0;

    TraceCollector::GetInstance().SetEnabled(true);

    {
        TRACE_SCOPE("main");

        // This test tests very basic functionality of VdfVector.

        // Run through all the registered tests, and if any of them fail
        // fail the whole test.

        for(int i = 0; tests[i].func; ++i)
        {
            printf("*** %s\n", tests[i].name);

            if (!tests[i].func())
                printf("> failed...\n"),
                res = -1;
            else
                printf("> ok...\n");
        }
    }

    TraceReporter::GetGlobalReporter()->Report(std::cout);

    return res;
}

