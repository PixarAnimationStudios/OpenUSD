//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/error.h"

#include "pxr/exec/vdf/grapher.h"

#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

void 
Vdf_ErrorHelper::FatalError(const VdfNode &node, char const *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    FatalError(node, TfVStringPrintf(fmt, ap));
    va_end(ap);
}

void 
Vdf_ErrorHelper::FatalError(const VdfNode &node, std::string const &msg) const
{
    VdfGrapher::GraphNodeNeighborhood(node, 5, 5);
    Tf_DiagnosticHelper::IssueFatalError(msg);
}

PXR_NAMESPACE_CLOSE_SCOPE
