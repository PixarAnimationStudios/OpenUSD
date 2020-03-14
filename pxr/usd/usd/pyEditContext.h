//
// Copyright 2017 Pixar
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
#ifndef PXR_USD_USD_PY_EDIT_CONTEXT_H
#define PXR_USD_USD_PY_EDIT_CONTEXT_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/editTarget.h"

#include <memory>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(UsdStage);

// Utility class for returning UsdEditContexts to python.  For use in wrapping
// code.
struct UsdPyEditContext
{
    USD_API
    explicit UsdPyEditContext(
        const std::pair<UsdStagePtr, UsdEditTarget> &stageTarget);
    USD_API
    explicit UsdPyEditContext(const UsdStagePtr &stage,
                              const UsdEditTarget &editTarget=UsdEditTarget());
private:
    friend struct Usd_PyEditContextAccess;

    UsdStagePtr _stage;
    UsdEditTarget _editTarget;
    std::shared_ptr<UsdEditContext> _editContext;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_PY_EDIT_CONTEXT_H
