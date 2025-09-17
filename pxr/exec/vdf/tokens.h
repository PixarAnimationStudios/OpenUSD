//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_TOKENS_H
#define PXR_EXEC_VDF_TOKENS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define VDF_TOKENS  \
    ((empty, ""))   \
    (in)            \
    (out)

TF_DECLARE_PUBLIC_TOKENS(VdfTokens, VDF_API, VDF_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
