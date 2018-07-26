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
#ifndef USDMAYA_SKEL_BINDINGS_PROCESSOR_H
#define USDMAYA_SKEL_BINDINGS_PROCESSOR_H

/// \file usdMaya/skelBindingsProcessor.h

#include "usdMaya/api.h"
#include "usdMaya/util.h"

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathTable.h"

#include <maya/MDagPath.h>

#include <set>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE


/// This class encapsulates all of the logic for writing or modifying
/// SkelRoot prims for all scopes that have skel bindings.
class UsdMaya_SkelBindingsProcessor
{
public:
    UsdMaya_SkelBindingsProcessor();

    /// Mark \p path as containing bindings utilizing the skeleton
    /// at \p skelPath.
    /// Bindings are marked so that SkelRoots may be post-processed.
    /// Valid values for \p config are:
    /// - PxrUsdExportJobArgsTokens->explicit_: search for an existing SkelRoot
    /// - PxrUsdExportJobArgsTokens->auto_: create a SkelRoot if needed
    /// PxrUsdExportJobArgsTokens->none is not valid for \p config; it will
    /// mark an invalid binding.
    void MarkBindings(const SdfPath& path,
                      const SdfPath& skelPath,
                      const TfToken& config);

    /// Performs final processing for skel bindings.
    bool PostProcessSkelBindings(const UsdStagePtr& stage) const;

private:

    bool _VerifyOrMakeSkelRoots(const UsdStagePtr& stage) const;

    using _Entry = std::pair<SdfPath,TfToken>;

    std::unordered_map<SdfPath, _Entry, SdfPath::Hash> _bindingToSkelMap;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
