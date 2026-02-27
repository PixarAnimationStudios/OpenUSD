//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_IR_TOKENS_H
#define PXR_EXEC_EXEC_IR_TOKENS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/execIr/api.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define EXEC_IR_TOKENS                          \
    (forwardCompute)                            \
    (inverseCompute)                            \
                                                \
    ((parentSpaceToken, "ParentIn:Space"))      \
                                                \
    ((defaultSpaceToken, "In:DefaultSpace"))    \
                                                \
    ((txToken, "In:Tx"))                        \
    ((tyToken, "In:Ty"))                        \
    ((tzToken, "In:Tz"))                        \
    ((rxToken, "In:Rx"))                        \
    ((ryToken, "In:Ry"))                        \
    ((rzToken, "In:Rz"))                        \
    ((rspinToken, "In:Rspin"))                  \
    ((rotationOrderToken, "In:RotationOrder"))  \
                                                \
    ((outSpaceToken, "Out:Space"))

TF_DECLARE_PUBLIC_TOKENS(ExecIrTokens, EXECIR_API, EXEC_IR_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
