//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/token.h"

#include "pxr/base/tf/pyNanobindBridge.h"

PXR_NAMESPACE_USING_DIRECTIVE

PXR_NAMESPACE_OPEN_SCOPE

void TfDumpTokenStats(); // Defined in token.cpp.

PXR_NAMESPACE_CLOSE_SCOPE

void wrapToken(nanobind::module_& m)
{    
    // Stats.
    m.def("DumpTokenStats", TfDumpTokenStats);
}
