//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/callbackNode.h"

#include "pxr/exec/vdf/context.h"

PXR_NAMESPACE_OPEN_SCOPE

void
Exec_CallbackNode::Compute(const VdfContext &context) const
{
    try {
        _callback(context);
    }

    catch(const std::exception &e) {
        context.CodingError(
            "Exception thrown during callback node execution: '%s'", e.what());
    }

    catch(...) {
        context.CodingError("Exception thrown during callback node execution.");
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
