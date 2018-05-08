//
// Copyright 2018 Pixar
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

#include "pxr/base/trace/aggregateNode.h"

#include "pxr/pxr.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TraceAggregateNodeRefPtr
TraceAggregateNode::Append(Id id, const TfToken &key,
                  TimeStamp ts, int c, int xc)
{
    TraceAggregateNodeRefPtr n = GetChild(key);
    if (n) {
        n->_id = id;
        n->_ts += ts;
        n->_count += c;
        n->_recursiveCount += c;
        n->_exclusiveCount += xc;
        n->_exclusiveTs += ts;
        n->_recursiveExclusiveTs += ts;
    }
    else {
        n = TraceAggregateNode::New(id,key,ts,c,xc);
        _children.push_back(n);
        _childrenByKey[key] = _children.size() - 1;
    }

    // Update our exclusive time to discount our new child's time.
    _exclusiveTs = (_exclusiveTs >= ts) ? _exclusiveTs - ts : 0;
    _recursiveExclusiveTs = 
        (_recursiveExclusiveTs >= ts) ? _recursiveExclusiveTs - ts : 0;

    return n;
}

void 
TraceAggregateNode::Append(TraceAggregateNodeRefPtr child) {
    TraceAggregateNodeRefPtr n = GetChild(child->GetKey());
    if (n) {
        n->_id = child->_id;
        n->_ts += child->_ts;
        n->_count += child->_count;
        n->_recursiveCount += child->_count;
        n->_exclusiveCount += child->_exclusiveCount;
        n->_exclusiveTs += child->_ts;
        n->_recursiveExclusiveTs += child->_ts;

        for (TraceAggregateNodeRefPtr& c : child->_children) {
            n->Append(c);
        }
    }
    else {
        _children.push_back(child);
        _childrenByKey[child->GetKey()] = _children.size() - 1;
    }
    
    // Update our exclusive time to discount our new child's time.
    _exclusiveTs = (_exclusiveTs >= child->_ts) ? _exclusiveTs - child->_ts : 0;
    _recursiveExclusiveTs = (_recursiveExclusiveTs >= child->_ts)
        ? _recursiveExclusiveTs - child->_ts : 0;
}

TraceAggregateNode::TimeStamp 
TraceAggregateNode::GetExclusiveTime(bool recursive)
{ 
    return recursive ? _recursiveExclusiveTs : _exclusiveTs;
}

void
TraceAggregateNode::AppendInclusiveCounterValue(int index, double value)
{
    _counterValues[index].inclusive += value;
}

double
TraceAggregateNode::GetInclusiveCounterValue(int index) const
{
    _CounterValues::const_iterator it = _counterValues.find(index);
    return it != _counterValues.end() ? it->second.inclusive : 0.0;
}

void
TraceAggregateNode::AppendExclusiveCounterValue(int index, double value)
{
    _counterValues[index].exclusive += value;
}

double
TraceAggregateNode::GetExclusiveCounterValue(int index) const
{
    _CounterValues::const_iterator it = _counterValues.find(index);
    return it != _counterValues.end() ? it->second.exclusive : 0.0;
}

TraceAggregateNodeRefPtr
TraceAggregateNode::GetChild(const TfToken &key)
{
    _ChildDictionary::const_iterator i = _childrenByKey.find(key);
    if (i != _childrenByKey.end()) {
        return _children[i->second];
    }
    else {
        return TraceAggregateNodeRefPtr(0);
    }
}

// This stack node is a convenient container for the data we need to keep
// track of during an iterative post-order traversal of our tree.
struct _StackNode {
    _StackNode(TraceAggregateNodePtr node, int parentIdx) : eventNode(node), 
                                                   parentIdx(parentIdx)
    {
        remainingChildren = node->GetChildrenRef().size();
    }

    TraceAggregateNodePtr   eventNode;
    int            parentIdx;
    int            remainingChildren;
};

void 
TraceAggregateNode::MarkRecursiveChildren()
{
    // Trivial case, if we are already marked, there is nothing left to do.
    if (IsRecursionHead())
        return;

    // This code performs an iterative post-order traversal on our tree.
    // It is much quicker than a simpler recursive version and can handle
    // much more depth.  This comes of course at the expense of more
    // complicated code that is slightly harder to debug.
    // Our algorithm for each node is to collapse its children, then 
    // check the stack for recursion, if found, merge with the parent node
    // and set the node as a dummy marker.
    std::vector<_StackNode>  stack;

    // Push root node on the stack
    stack.push_back(_StackNode(TraceAggregateNodePtr(this), -1));

    while (stack.size())
    {
        TraceAggregateNodePtr    curNode   = stack.back().eventNode;
        int             numKids   = stack.back().remainingChildren;
        int             parentIdx = stack.back().parentIdx;

        // We're processing this node, mark it so that we don't process it
        // again, ever.
        curNode->_isRecursionProcessed = true;

        // If our current node does not have kids, process it.  Processing
        // the node means to seach the parent stack for an existing key and
        // if found, merge with it and mark ourselves as a simple marker.
        if (numKids == 0)
        {
            // Look for a matching key on the parent stack.
            int p = parentIdx;
            
            while (p != -1)
            {
                if (p > static_cast<int>(stack.size()))
                {
                    // CODE_COVERAGE_OFF
                    TF_CODING_ERROR("Corrupt stack state.");
                    // CODE_COVERAGE_ON
                }

                TraceAggregateNodePtr parentNode = stack[p].eventNode;

                if (!parentNode)
                {
                    // CODE_COVERAGE_OFF
                    TF_CODING_ERROR("Invalid stack state.");
                    // CODE_COVERAGE_ON
                }

                if (curNode->GetKey() == parentNode->GetKey())
                {
                    // We found the key, now merge up with that parent, and 
                    // leave a marker in our place.
                    parentNode->_MergeRecursive(curNode);
                    curNode->_SetAsRecursionMarker(parentNode);
                    break;
                }
                p = stack[p].parentIdx; // next parent up
            }

            // If we have a valid parent, decrease the count of children
            // remaining to be processed.
            if (parentIdx > -1)
                stack[parentIdx].remainingChildren -= 1;
            stack.pop_back();
        }
        else
        {
            // Here our node has children, so before we go on, we must
            // push our children on the child stack.  This gives us the
            // post-order traversal we need.
            int parent = stack.size() - 1;
            for (int i = 0; i < numKids; i++)
            {
                // Only process nodes that have not been previously processed
                // (by a previous call to Report() for example).  If a node
                // has already been processed, decrement it from our remaining
                // children count.
                if (!curNode->_children[i]->_isRecursionProcessed)
                    stack.push_back(_StackNode(curNode->_children[i], parent));
                else
                    stack[parent].remainingChildren -= 1;
            }
        }
    }
}

void 
TraceAggregateNode::_MergeRecursive(const TraceAggregateNodeRefPtr &node)
{
    // Merge our times with this node's times.  Note that here we only
    // use the recursion data in order to keep the original state intact.
    if (IsRecursionMarker())
    {
        // If we are a recursion marker, what we actually intend is to
        // merge with our parent (i.e. the head of the recursive call).
        if (!_recursionParent)
        {
            // CODE_COVERAGE_OFF
            TF_CODING_ERROR("Marker has no or expired parent.");
            return;
            // CODE_COVERAGE_ON
        }
        _recursionParent->_MergeRecursive(node);
        return;
    }
    else
    {
        _recursiveCount += node->GetCount(true /* recursion */);
        _recursiveExclusiveTs += node->GetExclusiveTime(true /* recursion */);
    }

    // Mark ourselves as a recursive head so that we recognize that our
    // inclusive times are invalid.
    _isRecursionHead = true;

    // Now merge our children.
    size_t size = node->_children.size();
    for (size_t i = 0; i < size; ++i)
    {
        const TraceAggregateNodeRefPtr child = node->_children[i];

        if (!child)
        {
            // CODE_COVERAGE_OFF
            TF_CODING_ERROR("NULL child is not allowed.");
            // CODE_COVERAGE_OFF_GCOV_BUG - gcov thinks this is hit but it's not
            continue;
            // CODE_COVERAGE_ON_GCOV_BUG
            // CODE_COVERAGE_ON
        }

        TfToken key = child->GetKey();
        TraceAggregateNodeRefPtr n = GetChild(key);

        if (!n)
        {
            // Create an empty node to merge with.
            n = TraceAggregateNode::New( child->GetId(), child->GetKey(), 
                                child->GetInclusiveTime(), 
                                0, child->GetExclusiveCount() );

            // On our new node, we want the exclusiveTs to be computed by
            // recursiveTs not exclusiveTs, which is done during the merge
            // (this avoids double counting exclusive time).
            n->_exclusiveTs = child->GetExclusiveTime(false /*recursive*/); 
            n->_recursiveExclusiveTs = 0;

            _children.push_back(n);
            _childrenByKey[key] = _children.size() - 1;

            // If the original node is a recursive marker, then the new
            // node should be one too
            if (child->IsRecursionMarker())
                n->_SetAsRecursionMarker(child->_recursionParent);
            else
                // We always want to merge new nodes.
                n->_MergeRecursive(child);
        }
        else
        {
            // This key already exists, determine if we want to merge it in.

            // We have to make sure that we are not merging in recursion markers
            // with one another.  Here are the possible cases:
            // 
            // non-marker into non-marker:  
            //  both nodes contain useful information, merge them.
            bool nonMarkerIntoNonMarker = (!child->IsRecursionMarker() &&
                                           !n->IsRecursionMarker());
            //
            // non-marker into maker:       
            //      this case can happen when we have two branches that come
            //      out from the same root and have the same recursive pattern.
            //      we can't control the order that the siblings will be merged,
            //      and therefore we have to handle this case.
            bool nonMarkerIntoMarker    = (!child->IsRecursionMarker() && 
                                           n->IsRecursionMarker());
            //
            // marker into non-marker:
            //  non-marker will eventually become a marker, and we already
            //      accounted for the marker's counts.
            //
            // marker into marker:
            //      trivial case, two markers with the same key are as good as
            //      one marker for that key.

            if (nonMarkerIntoNonMarker || nonMarkerIntoMarker)
            {
                n->_MergeRecursive(child);
            }
        }
    }
}

void
TraceAggregateNode::_SetAsRecursionMarker(TraceAggregateNodePtr parent)
{
    _isRecursionMarker = true;
    _recursionParent = parent;
    if (!parent)
    {
        // CODE_COVERAGE_OFF
        TF_CODING_ERROR("Marker has no or expired parent.");
        // CODE_COVERAGE_ON
    }
    // Note that we'd love to be able to blow away all of our subtrees here,
    // but because we don't the mark call to modify the integrity of the
    // tree, we have to keep them untouched.
}

PXR_NAMESPACE_CLOSE_SCOPE
