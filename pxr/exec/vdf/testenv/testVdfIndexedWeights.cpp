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
#include "pxr/exec/vdf/iterators.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/simpleExecutor.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"

#include <iostream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

enum TestCases
{
    SkipNotExplicitlySetWeights,
    SkipExplicitlySetWeights,

    SkipNotExplicitlySetWeightsVectorCtor,    
    SkipExplicitlySetWeightsVectorCtor,    

    NumTestCases
};

static const char *TestCaseNames[NumTestCases] = 
{
    "SkipNotExplicitlySetWeights",
    "SkipExplicitlySetWeights",
    "SkipNotExplicitlySetWeightsVectorCtor",
    "SkipExplicitlySetWeightsVectorCtor"    
};

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (inout)
    (w1)
    (w2)
    (w3)
    (w4)
    (out)
);

bool perf = false;

struct WeightPerfCallbackNode : public VdfNode
{
    WeightPerfCallbackNode(
        VdfNetwork           *network,
        const VdfInputSpecs  &inputSpecs,
        const VdfOutputSpecs &outputSpecs)
        : VdfNode(network, inputSpecs, outputSpecs)
    {}

    void Compute(const VdfContext &context) const override;
};

void
WeightPerfCallbackNode::Compute(const VdfContext &context) const
{
    TRACE_FUNCTION();

    // All this callback does is reads the input and produces a vector
    // with all the inputs read.

    // Use constructor that doesn't take a std::vector
    for (VdfWeightedReadWriteIterator<double> i(
        context, { _tokens->w1, _tokens->w2, _tokens->w3, _tokens->w4 },
        _tokens->inout); !i.IsAtEnd(); ++i) {

        *i += i.GetWeight(0) + i.GetWeight(1) + i.GetWeight(2) + i.GetWeight(3);
    }
}

struct WeightCorrectnessCallbackNode final : public WeightPerfCallbackNode
{
    WeightCorrectnessCallbackNode(
        VdfNetwork           *network,
        const VdfInputSpecs  &inputSpecs,
        const VdfOutputSpecs &outputSpecs,
        size_t                requestWidth)
        : WeightPerfCallbackNode(network, inputSpecs, outputSpecs)
        , _requestWidth(requestWidth)
    {}

    void Compute(const VdfContext &context) const override;

    size_t _requestWidth;
};

void
WeightCorrectnessCallbackNode::Compute(const VdfContext &context) const
{
    TRACE_FUNCTION();

    // Also call the perf callback so we check its result for correctness.
    WeightPerfCallbackNode::Compute(context);

    const size_t requestWidth = _requestWidth;

    for (VdfWeightedReadIterator<double> i(
        context, { _tokens->w1, _tokens->w2, _tokens->w3, _tokens->w4 },
        _tokens->inout); !i.IsAtEnd(); ++i) {

        printf("%d: %f %f %f %f - *iter %f\n", 
            Vdf_GetIteratorIndex(i),
            i.GetWeight(0), i.GetWeight(1), i.GetWeight(2), i.GetWeight(3), *i);
    }

    VdfMask visitMask(requestWidth);
    for (size_t i=0; i<requestWidth; i+=2) {
        visitMask.SetIndex(i);
    }

    for (VdfMaskedReadIterator<double> i(
        context, visitMask, _tokens->inout); !i.IsAtEnd(); ++i) {

        printf("%d: - *iter %f\n", Vdf_GetIteratorIndex(i), *i);
    }

    for (VdfWeightedMaskedReadIterator<double> i(
        context, { _tokens->w1, _tokens->w2, _tokens->w3, _tokens->w4 },
        visitMask, _tokens->inout); !i.IsAtEnd(); ++i) {
        
        printf("%d: %f %f %f %f - *iter %f\n", 
            Vdf_GetIteratorIndex(i),
            i.GetWeight(0), i.GetWeight(1), i.GetWeight(2), i.GetWeight(3), *i);
    }

    for (VdfMaskedReadIterator<double, VdfMaskedIteratorMode::VisitUnset> i(
        context, visitMask, _tokens->inout); !i.IsAtEnd(); ++i) {

        printf("%d: - *iter %f\n", Vdf_GetIteratorIndex(i), *i);
    }

    for (VdfWeightedMaskedReadIterator<double, VdfMaskedIteratorMode::VisitUnset> i(
        context, { _tokens->w1, _tokens->w2, _tokens->w3, _tokens->w4 },
        visitMask, _tokens->inout); !i.IsAtEnd(); ++i) {

        printf("%d: %f %f %f %f - *iter %f\n", 
            Vdf_GetIteratorIndex(i),
            i.GetWeight(0), i.GetWeight(1), i.GetWeight(2), i.GetWeight(3), *i);
    }
}


static void
WeightedCallbackVectorCtor(const VdfContext &context) 
{
    TRACE_FUNCTION();

    // All this callback does is reads the input and produces a vector
    // with all the inputs read.

    std::vector<TfToken> weightNames;
    weightNames.push_back(_tokens->w1);
    weightNames.push_back(_tokens->w2);
    weightNames.push_back(_tokens->w3);
    weightNames.push_back(_tokens->w4);
    
    // Use constructor that takes a std::vector.
    VdfWeightedReadWriteIterator<double> iter(
        context, weightNames, _tokens->inout);

    for ( ; !iter.IsAtEnd(); ++iter) {

        *iter += iter.GetWeight(0) + iter.GetWeight(1) 
               + iter.GetWeight(2) + iter.GetWeight(3);
    }
}

VdfNode *
CreateWeightedNode(VdfNetwork *net, bool useVectorCtor, size_t requestWidth)
{
    TRACE_FUNCTION();

    VdfInputSpecs inspec;
    inspec
        .ReadWriteConnector<double>(_tokens->inout, _tokens->out)
        .ReadConnector<VdfIndexedWeights>(_tokens->w1)
        .ReadConnector<VdfIndexedWeights>(_tokens->w2)
        .ReadConnector<VdfIndexedWeights>(_tokens->w3)
        .ReadConnector<VdfIndexedWeights>(_tokens->w4)
        ;

    VdfOutputSpecs outspec;
    outspec
        .Connector<double>(_tokens->out)
        ;

    // DIfferent weightedIterator constructor used in two callbacks
    if (useVectorCtor) {
        return new VdfTestUtils::CallbackNode(
            net, inspec, outspec, &WeightedCallbackVectorCtor);
    }

    if (perf) {
        return new WeightPerfCallbackNode(
            net, inspec, outspec);
    }
    else {
        return new WeightCorrectnessCallbackNode(
            net, inspec, outspec, requestWidth);
    }
}

static bool
RunIteratorTest(VdfNode *node, const VdfMask &mask,
                const std::vector<double> &expected, TfStopwatch *watch)
{
    VdfRequest request(VdfMaskedOutput(node->GetOutput(), mask));

    VdfSchedule schedule;
    VdfScheduler::Schedule(request, &schedule, true /* topologicalSort */);
    VdfSimpleExecutor exec;

    if (watch)
    {
        TRACE_SCOPE("solve");

        watch->Start();
        exec.Run(schedule);
        watch->Stop();
    }
    else
        exec.Run(schedule);

    VdfVector::ReadAccessor<double> result = exec.GetOutputValue(*node->
        GetOutput(), mask)->GetReadAccessor<double>();

    if (result.GetNumValues() != expected.size()) {
        std::cerr << "\tERROR: Expected vector of size "
                  << expected.size() << ", got size " 
                  << result.GetNumValues() << std::endl;
        return false;
    }

    for (size_t i = 0; i < expected.size(); ++i) {
        if (result[i] != expected[i]) {
            std::cerr << "\tERROR: At index " << i << " expected " 
                      << expected[i] << " got " << result[i]
                      << std::endl;
            return false;
        }
    }

    return true;
}



static bool
TestWeightedIterator(TestCases testCase, TfStopwatch *watch) 
{
    TRACE_FUNCTION();

    bool useVectorCtor = false;
    if (testCase == SkipNotExplicitlySetWeightsVectorCtor) {
        testCase = SkipNotExplicitlySetWeights;
        useVectorCtor = true;
    }
    if (testCase == SkipExplicitlySetWeightsVectorCtor) {
        testCase = SkipExplicitlySetWeights;
        useVectorCtor = true;
    }

    // For correctness tests, we just use a small set.
    const size_t requestWidth = watch ? 1000000 : 50;

    VdfSimpleExecutor exec;
    VdfNetwork net;

    VdfInputVector<double> *in1 =
        new VdfInputVector<double>(&net, requestWidth);

    for(size_t i = 0; i < requestWidth; i++)
        in1->SetValue(i, i+1.0);

    // Create four indexed weights nodes
    VdfInputVector<VdfIndexedWeights> *iwn1 =
        new VdfInputVector<VdfIndexedWeights>(&net, 1);
    VdfInputVector<VdfIndexedWeights> *iwn2 =
        new VdfInputVector<VdfIndexedWeights>(&net, 1);
    VdfInputVector<VdfIndexedWeights> *iwn3 =
        new VdfInputVector<VdfIndexedWeights>(&net, 1);
    VdfInputVector<VdfIndexedWeights> *iwn4 =
        new VdfInputVector<VdfIndexedWeights>(&net, 1);

    VdfIndexedWeights w1;
    VdfIndexedWeights w2;
    VdfIndexedWeights w3;
    VdfIndexedWeights w4;

    if (testCase == SkipExplicitlySetWeights)
    {
        // Fill up weight vectors with empty 0.0 weights that cause 
        // _GetFirstWeightIndex(); to do a lot of work finding the first weight.

        for(size_t i = 0; i < requestWidth - 3; i++) {
            w1.Add(i, 0.0);
            w2.Add(i, 0.0);
            w3.Add(i, 0.0);
            w4.Add(i, 0.0);
        }
        // An axiom to exercise the operator== of VdfIndexedData.
        TF_AXIOM(w1 == w2);
    }

    // But to make this case a little harder, we set the first weight explicitly.
    // This way, we also test that skipping 'holes' of unexplicit weights is
    // done quick.

    if (testCase == SkipNotExplicitlySetWeights) {
        w1.Add(0, 0.0);
        w2.Add(1, 0.0);
        w3.Add(2, 0.0);
        w4.Add(3, 0.0);
    }

    // Create weights, so that only the last most three weights are set.
    w1.Add(requestWidth-3, 1.0);
    w2.Add(requestWidth-2, 0.5);
    w3.Add(requestWidth-3, 0.75);
    w4.Add(requestWidth-2, 0.5);

    // Test basic weight index finding in VdfIndexedWeights
    VdfIndexedWeights wa;
    for (int i = 1; i < 100; ++i) {
        wa.Add(i * 3, 0.0);
    }

    // Find the first weight and third weights
    TF_AXIOM(wa.GetFirstDataIndex(0) == wa.GetFirstDataIndex(0, 0));
    TF_AXIOM(wa.GetFirstDataIndex(9) == wa.GetFirstDataIndex(9, 1));
    TF_AXIOM(wa.GetFirstDataIndex(9) == wa.GetFirstDataIndex(9, 2));

    // Find weight index with value 99
    size_t testIdx = wa.GetFirstDataIndex(99);
    TF_AXIOM(testIdx == wa.GetFirstDataIndex(99, 0));
    TF_AXIOM(testIdx == wa.GetFirstDataIndex(99, testIdx));
    TF_AXIOM(testIdx == wa.GetFirstDataIndex(99, testIdx - 1));

    // Find first weight index with value 297
    testIdx = wa.GetFirstDataIndex(297);
    TF_AXIOM(testIdx == wa.GetFirstDataIndex(297, 0));
    TF_AXIOM(testIdx == wa.GetFirstDataIndex(297, testIdx));
    TF_AXIOM(testIdx == wa.GetFirstDataIndex(297, testIdx - 1));

    // Find a non-existant weight
    testIdx = wa.GetFirstDataIndex(303);
    TF_AXIOM(testIdx == wa.GetSize());
    TF_AXIOM(testIdx == wa.GetFirstDataIndex(303, 0));
    TF_AXIOM(testIdx == wa.GetFirstDataIndex(303, 101));

    iwn1->SetValue(0, w1);
    iwn2->SetValue(0, w2);
    iwn3->SetValue(0, w3);
    iwn4->SetValue(0, w4);

    VdfNode *last = CreateWeightedNode(&net, useVectorCtor, requestWidth);

    VdfMask maskRequestWidth(requestWidth);

    if (testCase == SkipNotExplicitlySetWeights)
    {
        // In this case we test seeking quickly forward to the first explicit
        // weight even when the mask specifies to iterate over all elements.
        maskRequestWidth.SetAll();
    }
    else if (testCase == SkipExplicitlySetWeights)
    {
        // In this case we test seeking quickly forward to the first element as
        // specified by the mask, even if there are a lot of explicitly set 
        // weights.
        for(size_t i = requestWidth - 3; i < requestWidth; i++)
            maskRequestWidth.SetIndex(i);
    }
    else
        return false;

    net.Connect(in1->GetOutput(), last, _tokens->inout, maskRequestWidth);

    VdfMask oneMask(1);
    oneMask.SetAll();

    net.Connect(iwn1->GetOutput(), last, _tokens->w1, oneMask);
    net.Connect(iwn2->GetOutput(), last, _tokens->w2, oneMask);
    net.Connect(iwn3->GetOutput(), last, _tokens->w3, oneMask);
    net.Connect(iwn4->GetOutput(), last, _tokens->w4, oneMask);


    // Our inputs look liks this:
    //
    //  [1.0, 2.0, 3.0]  with weights:
    //
    //          [1.0   -   -  ]
    //          [ -   0.5  -  ]
    //          [0.75  -   -  ]
    //          [ -   0.5  -  ]
    //
    //

    std::vector<double> expected;

    for(size_t i = 0; i < requestWidth; i++)
    {
        double v = i + 1.0;

        if (i == requestWidth - 3)
            v += 1.75;
        else if (i == requestWidth - 2)
            v += 1.00;

        expected.push_back(v);
    }

    return RunIteratorTest(last, maskRequestWidth, expected, watch);
}

int 
main(int argc, char **argv) 
{
    bool        failed = false;
    std::string opt((argc == 2) ? argv[1] : "");
    TfStopwatch solveTimer;        

    // -c is the default...
    if (opt.empty())
        opt = "-c";

    if (opt != "-c" && opt != "-p")
    {
        printf("Need to run with either:\n");
        printf(" -c = correctness mode (default)\n");
        printf(" -p = perfmode mode\n");
        return -1;
    }

    TraceCollector::GetInstance().SetEnabled(true);

    perf = opt == "-p";
    double totalSolveTime = 0.0;

    for(int i=0; i<NumTestCases; i++)
    {
        printf("> %s\n", TestCaseNames[i]);

        if (!TestWeightedIterator((TestCases)i, perf ? &solveTimer : NULL))
            failed = true;

        totalSolveTime += solveTimer.GetSeconds();
    }

    // performance mode selected?
    if (perf)
    {
        if (FILE *outputFile = fopen("perfstats.raw", "w"))
        {
            fprintf(outputFile,
                    "{'profile':'solve_time','metric':'time','value':%f,'samples':1}\n",
                    totalSolveTime);
            fclose(outputFile);
        }
    }

    if (perf)
    {
        TraceReporter::GetGlobalReporter()->Report(std::cout);
    }

    printf("> test %s\n", failed ? "failed" : "ok");

    return failed ? -1 : 0;
}
