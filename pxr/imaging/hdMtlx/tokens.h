//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_MTLX_TOKENS_H
#define PXR_IMAGING_HD_MTLX_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdMtlx/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDMTLX_TOKENS                           \
    ((surfaceshaderName, "Surface"))            \
    ((displacementshaderName, "Displacement"))

TF_DECLARE_PUBLIC_TOKENS(HdMtlxTokens, HDMTLX_API, HDMTLX_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_MTLX_TOKENS_H
