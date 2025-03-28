//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USDMTLX_BACKDOOR_H
#define PXR_USD_USDMTLX_BACKDOOR_H
 
#include "pxr/pxr.h"

#include "pxr/usd/usdMtlx/api.h"

#include "pxr/base/tf/declarePtrs.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(UsdStage);

/// Return MaterialX XML in \p buffer converted to a USD stage.
/// This is to allow testing from Python.  If \p nodeGraphs is true
/// then only node graphs are read, otherwise everything else is read.
USDMTLX_API
UsdStageRefPtr UsdMtlx_TestString(const std::string& buffer,
                                  bool nodeGraphs = false);

/// Return MaterialX XML in file at \p pathname converted to a USD stage.
/// This is to allow testing from Python.  If \p nodeGraphs is true
/// then only node graphs are read, otherwise everything else is read.
USDMTLX_API
UsdStageRefPtr UsdMtlx_TestFile(const std::string& pathname,
                                bool nodeGraphs = false);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USDMTLX_BACKDOOR_H
