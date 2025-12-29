//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/arrayEdit.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/valueComposeOver.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stringUtils.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

std::ostream &
Vt_ArrayEditStreamImpl(
    Vt_ArrayEditOps const &ops, size_t literalsSize,
    TfFunctionRef<std::ostream &(int64_t index)> streamElem, std::ostream &out)
{
    using Ops = Vt_ArrayEditOps;
    
    auto literal = [&](int64_t idx) {
        streamElem(idx);
    };
    auto index = [&](int64_t idx) {
        out << '[' << TfStringify(idx) << ']';
    };

    size_t count = 0;
    out << "edit [";
    ops.ForEach([&](Ops::Op op, int64_t a1, int64_t a2) {
        if (count++) { out << "; "; }
        switch (op) {
        case Ops::OpWriteLiteral:
            out << "write "; literal(a1); out << " to "; index(a2);
            break;
        case Ops::OpWriteRef:
            out << "write "; index(a1); out << " to "; index(a2);
            break;
        case Ops::OpInsertLiteral:
            if (a2 == 0) {
                out << "prepend "; literal(a1);
            }
            else if (a2 == Ops::EndIndex) { 
                out << "append "; literal(a1);
            }
            else {
                out << "insert "; literal(a1); out << " at "; index(a2);
            }
            break;
        case Ops::OpInsertRef:
            if (a2 == 0) {
                out << "prepend "; index(a1);
            }
            else if (a2 == Ops::EndIndex) { 
                out << "append "; index(a1);
            }
            else {
                out << "insert "; index(a1); out << " at "; index(a2);
            }
            break;
        case Ops::OpEraseRef:
            out << "erase "; index(a1);
            break;
        case Ops::OpMinSize:
            out << "minsize " << TfStringify(a1);
            break;
        case Ops::OpMinSizeFill:
            out << "minsize " << TfStringify(a1) << " fill "; literal(a2);
            break;
        case Ops::OpSetSize:
            out << "resize " << TfStringify(a1);
            break; 
        case Ops::OpSetSizeFill:
            out << "resize " << TfStringify(a1) << " fill "; literal(a2);
            break;
        case Ops::OpMaxSize:
            out << "maxsize " << TfStringify(a1);
            break;
        };
    });
    return out << ']';
}

TF_REGISTRY_FUNCTION(VtValue)
{
#define VT_ARRAY_EDIT_REGISTER_OVER(unused, elem)                              \
    VtRegisterComposeOver(                                                     \
        +[](VtArrayEdit< VT_TYPE(elem) > const &strong,                        \
            VtArrayEdit< VT_TYPE(elem) > const &weak) {                        \
            return strong.ComposeOver(weak);                                   \
        });                                                                    \
    VtRegisterComposeOver(                                                     \
        +[](VtArrayEdit< VT_TYPE(elem) > const &strong,                        \
            VtArray< VT_TYPE(elem) > const &weak) {                            \
            return strong.ComposeOver(weak);                                   \
        });                                                                    \
    VtRegisterComposeOver(                                                     \
        +[](VtArrayEdit< VT_TYPE(elem) > const &strong,                        \
            VtBackgroundType const &) {                                        \
            return strong.ComposeOver(VtArray< VT_TYPE(elem) > {});            \
        });
    TF_PP_SEQ_FOR_EACH(VT_ARRAY_EDIT_REGISTER_OVER, ~, VT_SCALAR_VALUE_TYPES)
#undef VT_ARRAY_EDIT_REGISTER_OVER
}

PXR_NAMESPACE_CLOSE_SCOPE
