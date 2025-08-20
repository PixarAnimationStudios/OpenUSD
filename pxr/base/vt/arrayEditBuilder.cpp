//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/arrayEditBuilder.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"

PXR_NAMESPACE_OPEN_SCOPE

inline bool
Vt_ArrayEditOpsBuilder::_CheckArity(Ops::Op op, int count) {
    if (Vt_ArrayEditOps::GetArity(op) != count) {
        _IssueArityError(op, count);
        return false;
    }
    return true;
}

void
Vt_ArrayEditOpsBuilder::AddOp(Ops::Op op, int64_t a1, int64_t a2) {
    _AddOp(op);
    if (_CheckArity(op, 2)) {
        _ins.push_back(a1);
        _ins.push_back(a2);
    }
}

void
Vt_ArrayEditOpsBuilder::AddOp(Ops::Op op, int64_t a1) {
    // Disallow negative-size size ops.
    if (a1 < 0 &&
        (op == Ops::OpMinSize ||
         op == Ops::OpMaxSize ||
         op == Ops::OpSetSize)) {
        _IssueNegativeSizeError(op, a1);
        return;
    }
    else {
        _AddOp(op);
        if (_CheckArity(op, 1)) {
            _ins.push_back(a1);
        }
    }
}

void
Vt_ArrayEditOpsBuilder::_AddOp(Ops::Op op) {        
    // If this is the first op, or this op differs from the prior, push a new
    // one.
    if (_ins.empty() || Ops::_ToOpAndCount(_ins[_lastOpIdx]).op != op) {
        _lastOpIdx = _ins.size();
        _ins.push_back(Ops::_ToInt64({ 1, op }));
    }
    else {
        // Otherwise bump the count.
        Ops::OpAndCount oc = Ops::_ToOpAndCount(_ins[_lastOpIdx]);
        ++oc.count;
        _ins[_lastOpIdx] = Ops::_ToInt64(oc);
    }
}

void
Vt_ArrayEditOpsBuilder::_IssueArityError(Ops::Op op, int count)
{
    TF_CODING_ERROR("Op '%s' requires %d arguments, got %d",
                    TfEnum::GetName(op).c_str(),
                    Vt_ArrayEditOps::GetArity(op), count);
}

void
Vt_ArrayEditOpsBuilder::_IssueNegativeSizeError(Ops::Op op, int64_t size)
{
    TF_CODING_ERROR("Op '%s' invalid negative size argument: %zd",
                    TfEnum::GetName(op).c_str(), size);
}

PXR_NAMESPACE_CLOSE_SCOPE
