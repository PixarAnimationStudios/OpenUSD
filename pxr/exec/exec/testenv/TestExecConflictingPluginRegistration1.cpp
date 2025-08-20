//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/registerSchema.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_USING_DIRECTIVE

// Register a computation for a schema that conflicts with a different plugin
// that registers for the same schema.
//
EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecConflictingPluginRegistrationSchema)
{
    self.PrimComputation(TfToken("conflictingRegistrationComputation"))
        .Callback(+[](const VdfContext &ctx) { return 42.0; });
}
