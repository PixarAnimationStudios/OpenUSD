//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
