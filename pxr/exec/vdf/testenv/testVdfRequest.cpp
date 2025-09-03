//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/request.h"
#include "pxr/exec/vdf/testUtils.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (axis)
    (moves)
    (out)
    (out1)
    (out2)
);

static void
CallbackFunction(const VdfContext &context) 
{
}

void 
BuildTestNetwork(VdfTestUtils::Network& graph)
{
    VdfMask bigMask = VdfMask::AllOnes(100);
    VdfMask littleMask(2); 
    littleMask.SetIndex(1);

    // We're going to build a network like this:
    //
    //          GN1  GN2  
    //          |\   /| 
    //          | MON |
    //          | / \ |
    //          TN1  TN2 
    //           \   /
    //            TN3
    //            

    VdfTestUtils::CallbackNodeType generatorType(&CallbackFunction);
    generatorType
        .Out<int>(_tokens->out)
        ;

    VdfTestUtils::CallbackNodeType multipleOutputType(&CallbackFunction);
    multipleOutputType
        .Read<int>(_tokens->axis)
        .Read<int>(_tokens->moves)
        .Out<int>(_tokens->out1)
        .Out<int>(_tokens->out2)
        ;

    VdfTestUtils::CallbackNodeType translateType(&CallbackFunction);
    translateType
        .Read<int>(_tokens->axis)
        .ReadWrite<int>(_tokens->moves, _tokens->out)
        ;

    graph.Add("gn1", generatorType);
    graph.Add("gn2", generatorType);
    graph.Add("mon", multipleOutputType);
    graph.Add("tn1", translateType);
    graph.Add("tn2", translateType);
    graph.Add("tn3", translateType);

    graph["gn1"] >> graph["mon"].In(_tokens->axis, littleMask);
    graph["gn1"] >> graph["tn1"].In(_tokens->axis, littleMask);

    graph["gn2"] >> graph["mon"].In(_tokens->moves, bigMask);
    graph["gn2"] >> graph["tn2"].In(_tokens->moves, bigMask);

    graph["mon"].Output(_tokens->out1) >> 
        graph["tn1"].In(_tokens->moves, littleMask);
    graph["mon"].Output(_tokens->out2) >> 
        graph["tn2"].In(_tokens->axis, littleMask);

    graph["tn1"] >> graph["tn3"].In(_tokens->axis, bigMask);
    graph["tn2"] >> graph["tn3"].In(_tokens->moves, bigMask); 
}

int 
TestConstruction()
{
    // Empty construction
    VdfRequest r = VdfRequest();
    TF_AXIOM(r.GetSize() == 0);
    TF_AXIOM(r.IsEmpty());

    // Test single output construction
    r  = VdfRequest(VdfMaskedOutput());
    TF_AXIOM(r.GetSize() == 1);
    TF_AXIOM(!r.IsEmpty());

    // Test creation from VdfMaskedOutputVector
    VdfTestUtils::Network graph;
    BuildTestNetwork(graph);
    VdfMaskedOutput out = 
        VdfMaskedOutput(graph["tn3"].GetVdfNode()->GetOutput(), VdfMask());
    VdfMaskedOutputVector v {out, out, out};
    r = VdfRequest(v);
    TF_AXIOM(r.GetSize() == 1);
    TF_AXIOM(!r.IsEmpty());
    TF_AXIOM(v.size() == 3);

    // Test creation from moving vector
    r = VdfRequest(std::move(v));
    TF_AXIOM(r.GetSize() == 1);
    TF_AXIOM(!r.IsEmpty());
    TF_AXIOM(v.size() == 0);

    // Test creation from vector with 2 unique masked outputs.
    VdfMaskedOutput out1 = 
        VdfMaskedOutput(
            graph["tn2"].GetVdfNode()->GetOutput(), 
            VdfMask::AllOnes(3));
    v = {out, out, out1, out, out1};
    r = VdfRequest(v);
    TF_AXIOM(r.GetSize() == 2);
    TF_AXIOM(!r.IsEmpty());
    TF_AXIOM(v.size() == 5);

    VdfRequest r1 = VdfRequest(std::move(v));
    TF_AXIOM(r1.GetSize() == 2);
    TF_AXIOM(!r1.IsEmpty());
    TF_AXIOM(v.size() == 0);
    TF_AXIOM(r == r1);

    return 0;
}

int 
TestQueries()
{
    VdfTestUtils::Network graph;
    BuildTestNetwork(graph);

    // Test get network.
    VdfRequest r(
        VdfMaskedOutput(graph["tn3"].GetVdfNode()->GetOutput(), VdfMask()));
    TF_AXIOM(&graph.GetNetwork() == r.GetNetwork());
    return 0;
}

int
TestFullRequestIterator()
{
    VdfTestUtils::Network graph;
    BuildTestNetwork(graph);

    VdfMaskedOutput out1 = 
        VdfMaskedOutput(graph["gn1"].GetVdfNode()->GetOutput(), VdfMask());
    VdfMaskedOutput out2 = 
        VdfMaskedOutput(graph["gn2"].GetVdfNode()->GetOutput(), VdfMask());
    VdfMaskedOutput out3 = 
        VdfMaskedOutput(graph["tn1"].GetVdfNode()->GetOutput(), VdfMask());
    VdfMaskedOutput out4 = 
        VdfMaskedOutput(graph["tn2"].GetVdfNode()->GetOutput(), VdfMask());
    VdfMaskedOutputVector v = {out1, out2, out3, out4};
    VdfRequest r(v);

    // Test direct use of the iterator.
    VdfRequest::const_iterator i = r.begin();
    int count = 0;
    for(; i != r.end(); ++i) {
        TF_AXIOM(*i == v[count]);
        ++count;
    }

    TF_AXIOM(count == 4);

    // Test iterator in a range-based loop.
    count = 0;
    for(const VdfMaskedOutput &i : r) {
        TF_AXIOM(i == v[count]);
        ++count;
    }

    TF_AXIOM(count == 4);
    return 0;
}

int 
TestSubsetOperators()
{
    VdfTestUtils::Network graph;
    BuildTestNetwork(graph);

    VdfMaskedOutput out1 = 
        VdfMaskedOutput(graph["gn1"].GetVdfNode()->GetOutput(), VdfMask());
    VdfMaskedOutput out2 = 
        VdfMaskedOutput(graph["gn2"].GetVdfNode()->GetOutput(), VdfMask());
    VdfMaskedOutput out3 = 
        VdfMaskedOutput(graph["tn1"].GetVdfNode()->GetOutput(), VdfMask());
    VdfMaskedOutput out4 = 
        VdfMaskedOutput(graph["tn2"].GetVdfNode()->GetOutput(), VdfMask());
    VdfMaskedOutputVector v = {out1, out2, out3, out4};
    VdfRequest r(v);
    VdfRequest r_copy(r);

    TF_AXIOM(r.GetSize() == 4);
    TF_AXIOM(r_copy.GetSize() == 4);
    TF_AXIOM(r == r_copy);
    TF_AXIOM(VdfRequest::Hash()(r) == VdfRequest::Hash()(r_copy));

    VdfRequest::const_iterator it = r_copy.begin();
    r.Remove(it);
    std::cout << r.GetSize() << std::endl;
    TF_AXIOM(r.GetSize() == 3);
    TF_AXIOM(r_copy.GetSize() == 4);
    TF_AXIOM(r != r_copy);
    TF_AXIOM(VdfRequest::Hash()(r) != VdfRequest::Hash()(r_copy));
    int count = 0;
    for(const VdfMaskedOutput &i : r) {
        TF_AXIOM(i == v[count + 1]);
        ++count;
    }
    TF_AXIOM(count == 3);

    ++it; ++it; // iterator at index = 2
    r.Remove(it);
    TF_AXIOM(r.GetSize() == 2);
    TF_AXIOM(r_copy.GetSize() == 4);
    TF_AXIOM(r != r_copy);
    TF_AXIOM(VdfRequest::Hash()(r) != VdfRequest::Hash()(r_copy));
    count = 0;
    for(const VdfMaskedOutput &i : r) {
        TF_AXIOM(i == v[(2 * count) + 1]);
        ++count;
    }
    TF_AXIOM(count == 2);

    r.RemoveAll();
    TF_AXIOM(r.GetSize() == 0);
    TF_AXIOM(r_copy.GetSize() == 4);
    TF_AXIOM(r != r_copy);
    TF_AXIOM(VdfRequest::Hash()(r) != VdfRequest::Hash()(r_copy));
    count = 0;
    for(const VdfMaskedOutput &i : r) {
        TF_AXIOM(i == v[0]);
        ++count;
    }
    TF_AXIOM(count == 0);

    ++it; // iterator at index = 3
    r.Add(it);
    TF_AXIOM(r.GetSize() == 1);
    TF_AXIOM(r_copy.GetSize() == 4);
    TF_AXIOM(r != r_copy);
    TF_AXIOM(VdfRequest::Hash()(r) != VdfRequest::Hash()(r_copy));
    count = 0;
    for(const VdfMaskedOutput &i : r) {
        TF_AXIOM(i == v[3]);
        ++count;
    }
    TF_AXIOM(count == 1);

    r.AddAll();
    TF_AXIOM(r.GetSize() == 4);
    TF_AXIOM(r == r_copy);
    TF_AXIOM(VdfRequest::Hash()(r) == VdfRequest::Hash()(r_copy));

    return 0;
}

int 
TestHash()
{
    return 0;
}

typedef int(*TestFunction)(void);
static const TestFunction tests[] = {
    TestConstruction,
    TestQueries,
    TestFullRequestIterator,
    TestSubsetOperators,
    nullptr
};

int 
main(int argc, char** argv) 
{
    for (int i = 0; tests[i] != nullptr; ++i) {
        if (int result = tests[i]()) {
            return result;
        }
    }

    return 0;
}
