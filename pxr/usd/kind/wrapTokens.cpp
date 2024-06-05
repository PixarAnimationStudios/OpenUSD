//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/base/tf/pyStaticTokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

void wrapTokens()
{
    TF_PY_WRAP_PUBLIC_TOKENS("Tokens", KindTokens,
                             KIND_TOKENS);
}
