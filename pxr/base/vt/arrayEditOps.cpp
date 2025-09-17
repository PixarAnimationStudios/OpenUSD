//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/arrayEditOps.h"
#include "pxr/base/vt/debugCodes.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(Vt_ArrayEditOps::OpWriteLiteral);
    TF_ADD_ENUM_NAME(Vt_ArrayEditOps::OpWriteRef);
    TF_ADD_ENUM_NAME(Vt_ArrayEditOps::OpInsertLiteral);
    TF_ADD_ENUM_NAME(Vt_ArrayEditOps::OpInsertRef);
    TF_ADD_ENUM_NAME(Vt_ArrayEditOps::OpEraseRef);
    TF_ADD_ENUM_NAME(Vt_ArrayEditOps::OpMinSize);
    TF_ADD_ENUM_NAME(Vt_ArrayEditOps::OpMinSizeFill);
    TF_ADD_ENUM_NAME(Vt_ArrayEditOps::OpSetSize);
    TF_ADD_ENUM_NAME(Vt_ArrayEditOps::OpSetSizeFill);
    TF_ADD_ENUM_NAME(Vt_ArrayEditOps::OpMaxSize);
}

void
Vt_ArrayEditOps::_LiteralOutOfBounds(int64_t idx, size_t size)
{
    TF_DEBUG(VT_ARRAY_EDIT_BOUNDS).Msg(
        "Index %zd out of bounds for literal value (max = %zu)\n",
        idx, size);
}

void
Vt_ArrayEditOps::_ReferenceOutOfBounds(int64_t idx, size_t size)
{
    TF_DEBUG(VT_ARRAY_EDIT_BOUNDS).Msg(
        "Array reference index %zd out of bounds (size = %zu)\n",
        idx, size);
}

void
Vt_ArrayEditOps::_InsertOutOfBounds(int64_t idx, size_t size)
{
    TF_DEBUG(VT_ARRAY_EDIT_BOUNDS).Msg(
        "Array insert index %zd out of bounds (size = %zu)\n",
        idx, size);
}

void
Vt_ArrayEditOps::_NegativeSizeArg(Op op, int64_t size)
{
    TF_DEBUG(VT_ARRAY_EDIT_BOUNDS).Msg(
        "Sizing operation '%s' with negative size argument %zd\n",
        TfEnum::GetName(op).c_str(), size);
}

void
Vt_ArrayEditOps::_IssueInvalidInsError(
    OpAndCount oc, size_t offset, size_t required, size_t actual) const
{
    TF_RUNTIME_ERROR("Invalid array edit op '%s' at offset %zd requires %zd "
                     "arguments, but only %zd exist",
                     TfEnum::GetName(oc.op).c_str(),
                     offset, required, actual);
}
    
void
Vt_ArrayEditOps::_IssueInvalidOpError(OpAndCount oc, size_t offset) const
{
    TF_RUNTIME_ERROR("Invalid array edit op code %d at offset %zd",
                     static_cast<int>(oc.op), offset);
}

PXR_NAMESPACE_CLOSE_SCOPE
