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
#include "pxr/usd/usd/treeIterator.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE


UsdPrimRange
UsdPrimRange::Stage(const UsdStagePtr &stage,
                       const Usd_PrimFlagsPredicate &predicate)
{
    Usd_PrimDataConstPtr firstChild = 
        stage->GetPseudoRoot()._Prim()->GetFirstChild();
    UsdPrimRange ret(firstChild, /* end = */ nullptr, 
                        firstChild ? firstChild->GetPath() : SdfPath(), 
                        predicate);
    // The PrimRange uses a depth count to know when it's about to pop out of
    // the subtree it was walking so it can stop and avoid walking into siblings
    // of the initial prim.  Since we're proactively descending to the first
    // child under the stage's pseudo-root, we need to preincrement _depth so we
    // *do* continue to siblings of the initial prim.
    if (ret)
        ++ret._depth;
    return ret;
}

void
UsdPrimRange::PruneChildren()
{
    if (!*this) {
        TF_CODING_ERROR("Iterator past-the-end");
        return;
    }
    if (_isPost) {
        TF_CODING_ERROR("Cannot prune children during post-visit because the"
                        "children have already been processed. "
                        "Current node: %s", (*this)->GetPath().GetText());
        return;
    }
    _pruneChildrenFlag = true;
}

void
UsdPrimRange::increment()
{
    base_type &base = base_reference();
    if (ARCH_UNLIKELY(_isPost)) {
        _isPost = false;
        if (Usd_MoveToNextSiblingOrParent(base, _primPath, _end, _predicate)) {
            if (_depth) {
                --_depth;
                _isPost = true;
            } else {
                base = _end;
                _primPath = SdfPath();
            }
        }
    } else if (!_pruneChildrenFlag &&
               Usd_MoveToChild(base, _primPath, _end, _predicate)) {
        ++_depth;
    } else {
        if (_postOrder) {
            _isPost = true;
        } else {
            while (Usd_MoveToNextSiblingOrParent(base, _primPath, _end, 
                                                 _predicate)) {
                if (_depth) {
                    --_depth;
                } else {
                    base = _end;
                    _primPath = SdfPath();
                    break;
                }
            }
        }
        _pruneChildrenFlag = false;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

