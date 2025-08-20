//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/registerSchema.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (anotherComputation)
    (input1)
    (input2)
    (myComputation)
    (unregisteredComputation)
    );

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecPluginComputationSchema) 
{
    // Register a computation that the test looks for first, causing this plugin
    // to be loaded.
    self.PrimComputation(_tokens->myComputation)
        .Callback(+[](const VdfContext &ctx) { return 42.0; })
        .Inputs(
            AttributeValue<double>(_tokens->input1),
            NamespaceAncestor<double>(_tokens->input2)
        );

    // Register another computation that the test looks for second, after plugin
    // loading has happened
    self.PrimComputation(_tokens->anotherComputation)
        .Callback(+[](const VdfContext &ctx) { return 42.0; })
        .Inputs(
            AttributeValue<double>(_tokens->input1)
        );
}

// Register a computation on a different schema, to confirm that the computation
// is defined when we load plugins for the schema above.
//
EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecExtraPluginComputationSchema) 
{
    self.PrimComputation(_tokens->myComputation)
        .Callback(+[](const VdfContext &ctx) { return 42.0; });
}

// Attempt to register a computation for a schema that's already been
// registered.
//
EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecComputationRegistrationCustomSchema)
{
    self.PrimComputation(_tokens->unregisteredComputation)
        .Callback(+[](const VdfContext &ctx) { return 42.0; });
}
