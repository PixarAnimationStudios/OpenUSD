//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_ERROR_H
#define PXR_EXEC_VDF_ERROR_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfNode;

struct Vdf_ErrorHelper : public Tf_DiagnosticHelper {
    Vdf_ErrorHelper(TfCallContext const &context,
                    TfDiagnosticType type = TF_DIAGNOSTIC_INVALID_TYPE) :
        Tf_DiagnosticHelper(context, type) {}
    VDF_API
    void FatalError(const VdfNode &node, std::string const &msg) const;
    VDF_API
    void FatalError(const VdfNode &node, char const *fmt, ...) const
        ARCH_PRINTF_FUNCTION(3,4);
};

/// Issues a fatal error to end the program in the spirit of TF_FATAL_ERROR.
///
/// In addition to the functionality provided by Tf, VDF_FATAL_ERROR also
/// produces a graph of the network around \p node.
///
#define VDF_FATAL_ERROR \
    Vdf_ErrorHelper(TF_CALL_CONTEXT).FatalError

/// Axioms that the condition \p cond is true in the spirit of TF_AXIOM.
///
/// In addition to the functionality provided by Tf, VDF_AXIOM also
/// produces a graph of the network around \p node.
///
#define VDF_AXIOM(node, cond)                                       \
    do {                                                            \
        if (ARCH_UNLIKELY(!(cond)))                                 \
            Vdf_ErrorHelper(TF_CALL_CONTEXT).                       \
                FatalError(node, "Failed axiom: ' %s '", #cond);    \
    } while (0)

PXR_NAMESPACE_CLOSE_SCOPE

#endif 
