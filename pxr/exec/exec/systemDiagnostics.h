//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_SYSTEM_DIAGNOSTICS_H
#define PXR_EXEC_EXEC_SYSTEM_DIAGNOSTICS_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/exec/exec/system.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfGrapherOptions;

/// Utility class with various diagnostic functions for ExecSystem.
class ExecSystem::Diagnostics
{
public:
    EXEC_API
    explicit Diagnostics(ExecSystem *system);

    /// Invalidates all of the internal state of the exec system, resetting it
    /// to a state equivalent to when it was first constructed.
    ///
    EXEC_API
    void InvalidateAll();

    /// Produces a dot graph of the currently compiled exec network and writes
    /// its contents to \p filename.
    ///
    EXEC_API
    void GraphNetwork(const char *filename) const;

    /// Produces a dot graph of the currently compiled exec network according to
    /// the provided \p grapherOptions and writes its contents to \p filename.
    ///
    EXEC_API
    void GraphNetwork(
        const char *filename,
        const VdfGrapherOptions &grapherOptions) const;

private:
    ExecSystem *const _system;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
