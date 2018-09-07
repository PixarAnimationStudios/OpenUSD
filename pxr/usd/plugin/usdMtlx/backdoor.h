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
#ifndef USDMTLX_BACKDOOR_H
#define USDMTLX_BACKDOOR_H
 
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

#endif // USDMTLX_BACKDOOR_H
