//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/safeTypeCompare.h"
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DECLARE_WEAK_AND_REF_PTRS(Node);

class Node : public TfRefBase, public TfWeakBase {
public:

    static NodeRefPtr New() {
        return TfCreateRefPtr(new Node);
    }

    ~Node() {
        _nNodes--;
    }

    static size_t GetTotalNodeCount() {
        return _nNodes;
    }

    void SetChild(NodeRefPtr child) {
        if ((_child = child))
            _child->_parent = NodePtr(this);
    }

    NodeRefPtr GetChild() {
        return _child;
    }

    NodeRefPtr GetTail() {
        return _child ? _child->GetTail() : NodeRefPtr(this);
    }

    size_t GetLength() {
        return _child ? _child->GetLength() + 1 : 1;
    }

    size_t GetRevLength() {
        return _parent ? _parent->GetRevLength() + 1 : 1;
    }

protected:
    Node() {
        _nNodes++;
    }
private:

    NodeRefPtr     _child;
    NodePtr _parent;
    static  size_t _nNodes;
};


size_t Node::_nNodes = 0;

TF_DECLARE_WEAK_AND_REF_PTRS(SuperNode);

class SuperNode : public Node {
public:
    static SuperNodeRefPtr New() {
        return TfCreateRefPtr(new SuperNode);
    }
private:
    SuperNode() { }
};

static
NodeRefPtr MakeChain(size_t n) {
    if (n == 0)
        return TfNullPtr;
    else {
        NodeRefPtr root = Node::New();
        root->SetChild(MakeChain(n-1));
        return root;
    }
}


static void TakesNodePtr(NodePtr p) {}
static void TakesNodeConstPtr(NodeConstPtr p) {}
static void TakesNodeRefPtr(NodeRefPtr p) {}
static void TakesNodeConstRefPtr(NodeConstRefPtr p) {}

static void TakesSuperNodePtr(SuperNodePtr p) {}
static void TakesSuperNodeConstPtr(SuperNodeConstPtr p) {}
static void TakesSuperNodeRefPtr(SuperNodeRefPtr p) {}
static void TakesSuperNodeConstRefPtr(SuperNodeConstRefPtr p) {}

static void TestConversions() {

    // Make a SuperNodeRefPtr and try passing to a function taking NodeWeakPtr
    SuperNodeRefPtr snode = SuperNode::New();

    TakesNodePtr(snode);
    TakesNodeConstPtr(snode);
    TakesNodeRefPtr(snode);
    TakesNodeConstRefPtr(snode);

    TakesSuperNodePtr(snode);
    TakesSuperNodeConstPtr(snode);
    TakesSuperNodeRefPtr(snode);
    TakesSuperNodeConstRefPtr(snode);

    // Make a SuperNodePtr
    SuperNodePtr snodew = snode;

    TakesNodePtr(snodew);
    TakesNodeConstPtr(snodew);
    TakesNodeRefPtr(snodew);
    TakesNodeConstRefPtr(snodew);

    TakesSuperNodePtr(snodew);
    TakesSuperNodeConstPtr(snodew);
    TakesSuperNodeRefPtr(snodew);
    TakesSuperNodeConstRefPtr(snodew);
}

static void TestNullptrComparisons()
{
    NodeRefPtr p;

    TF_AXIOM(p == nullptr);
    TF_AXIOM(!(p != nullptr));
    TF_AXIOM(!(p < nullptr));
    TF_AXIOM(p <= nullptr);
    TF_AXIOM(!(p > nullptr));
    TF_AXIOM(p >= nullptr);

    // These should be exactly the same as the above comparisons to nullptr,
    // but are included to verify that the code compiles.
    TF_AXIOM(p == NULL);
    TF_AXIOM(NULL == p);
}

static bool
Test_TfRefPtr()
{
    TestConversions();
    TestNullptrComparisons();
    
    NodeRefPtr chain1 = MakeChain(10);
    NodeRefPtr chain2 = MakeChain(5);

    NodePtr gChain1 = chain1;
    NodePtr gChain2 = chain2;

    TF_AXIOM(chain1->GetLength() == 10);
    TF_AXIOM(chain2->GetLength() == 5);
    TF_AXIOM(gChain1->GetLength() == 10);
    TF_AXIOM(gChain2->GetLength() == 5);

    std::cout
        << "total nodes (should be 15): "
        << Node::GetTotalNodeCount() << std::endl;

    NodeRefPtr start = Node::New();
    start->SetChild(chain1);
    chain1 = TfNullPtr;

    TF_AXIOM(gChain1->GetLength() == 10);
    TF_AXIOM(start->GetLength() == 11);

    std::cout
        << "total nodes (should be one more than previous): "
        << Node::GetTotalNodeCount() << std::endl;

    start->SetChild(gChain2);
    chain2 = TfNullPtr;
    TF_AXIOM(start->GetLength() == 6);
    TF_AXIOM(!gChain1);
    TF_AXIOM(gChain2);

    TF_AXIOM(start->GetLength() == start->GetTail()->GetRevLength());

    std::cout
        << "total nodes (should be 10 less than last): "
        << Node::GetTotalNodeCount() << std::endl;

    start = TfNullPtr;

    TF_AXIOM(!gChain1);
    TF_AXIOM(!gChain2);

    std::cout
        << "total nodes (should be zero): "
        << Node::GetTotalNodeCount() << std::endl;

    TF_AXIOM(Node::GetTotalNodeCount() == 0);

    chain1 = MakeChain(5);
    gChain2 = chain2 = MakeChain(5);
    chain1->GetTail()->SetChild(chain2);

    TF_AXIOM(gChain2->GetRevLength() == 6);
    chain1 = TfNullPtr;
    TF_AXIOM(gChain2->GetRevLength() == 1);
    chain2 = TfNullPtr;
    TF_AXIOM(!gChain2);
    TF_AXIOM(Node::GetTotalNodeCount() == 0);

    SuperNodeRefPtr superPtr = SuperNode::New();
    NodeRefPtr basePtr = superPtr;
    NodePtr baseBackPtr = basePtr;
    
    TF_AXIOM(TfDynamic_cast<SuperNodeRefPtr>(basePtr) == superPtr);
    TF_AXIOM(TfSafeDynamic_cast<SuperNodeRefPtr>(basePtr) == superPtr);

    TF_AXIOM(TfDynamic_cast<SuperNodePtr>(baseBackPtr) == superPtr);
    TF_AXIOM(TfSafeDynamic_cast<SuperNodePtr>(baseBackPtr) == superPtr);

    // Test swap
    {
        const NodeRefPtr n1 = Node::New();
        const NodeRefPtr n2 = Node::New();

        NodeRefPtr a = n1;
        NodeRefPtr b = n2;
        TF_AXIOM(a);
        TF_AXIOM(b);
        TF_AXIOM(a != b);

        TF_AXIOM(a == n1);
        TF_AXIOM(b == n2);
        a.swap(b);
        TF_AXIOM(a == n2);
        TF_AXIOM(b == n1);

        // Test self-swap
        a.swap(a);
        TF_AXIOM(a == n2);
        b.swap(b);
        TF_AXIOM(b == n1);
    }

    return true;
}

TF_ADD_REGTEST(TfRefPtr);

