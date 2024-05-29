//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/primRange.h"

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
    UsdPrimRange ret(firstChild, /* end = */ nullptr, SdfPath(), predicate);
    // The PrimRange uses a depth count to know when it's about to pop out of
    // the subtree it was walking so it can stop and avoid walking into siblings
    // of the initial prim.  Since we're proactively descending to the first
    // child under the stage's pseudo-root, we need to preincrement _depth so we
    // *do* continue to siblings of the initial prim.
    if (!ret.empty())
        ++ret._initDepth;
    return ret;
}

void
UsdPrimRange::iterator::PruneChildren()
{
    if (_underlyingIterator == _range->_end) {
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
UsdPrimRange::iterator::increment()
{
    _UnderlyingIterator end = _range->_end;
    if (ARCH_UNLIKELY(_isPost)) {
        _isPost = false;
        if (Usd_MoveToNextSiblingOrParent(_underlyingIterator, _proxyPrimPath,
                                          end, _range->_predicate)) {
            if (_depth) {
                --_depth;
                _isPost = true;
            } else {
                _underlyingIterator = end;
                _proxyPrimPath = SdfPath();
            }
        }
    } else if (!_pruneChildrenFlag &&
               Usd_MoveToChild(_underlyingIterator, _proxyPrimPath, end,
                               _range->_predicate)) {
        ++_depth;
    } else {
        if (_range->_postOrder) {
            _isPost = true;
        } else {
            while (Usd_MoveToNextSiblingOrParent(_underlyingIterator,
                                                 _proxyPrimPath,
                                                 end,
                                                 _range->_predicate)) {
                if (_depth) {
                    --_depth;
                } else {
                    _underlyingIterator = end;
                    _proxyPrimPath = SdfPath();
                    break;
                }
            }
        }
        _pruneChildrenFlag = false;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

