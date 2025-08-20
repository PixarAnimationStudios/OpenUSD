//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_INPUT_RESOLVER_H
#define PXR_EXEC_EXEC_INPUT_RESOLVER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/outputKey.h"

#include "pxr/exec/esf/schemaConfigKey.h"

PXR_NAMESPACE_OPEN_SCOPE

class EsfObject;
class EsfStage;

/// Returns output keys whose outputs connect to the input for \p inputKey on a
/// computation provided by \p origin on \p stage.
///
/// \p dispatchingSchemaKey is used to find the requested computation on the
/// resolved provider. If \p dispatchingSchemaKey is empty, this will never
/// resolve to a dispatched computation. If \p inputKey requests dispatched
/// comptuations, \p dispatchingSchemaKey will be used in the output keys, if
/// the input resolves to a dispatched function, to support recursively
/// dispatched functions.
///
/// Determines the resulting output keys by traversing the scene graph, starting
/// from \p origin, recording each step of the traversal into the \p journal.
/// This \p journal must be provided while making connections to the
/// corresponding input. By doing this, the input will be uncompiled on scene
/// changes that invalidate the results of this traversal.
///
EXEC_API
Exec_OutputKeyVector Exec_ResolveInput(
    const EsfStage &stage,
    const EsfObject &origin,
    EsfSchemaConfigKey dispatchingSchemaKey,
    const Exec_InputKey &inputKey,
    EsfJournal *journal);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
