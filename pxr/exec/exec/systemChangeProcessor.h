//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_SYSTEM_CHANGE_PROCESSOR_H
#define PXR_EXEC_EXEC_SYSTEM_CHANGE_PROCESSOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"
#include "pxr/exec/exec/system.h"

#include "pxr/base/tf/token.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/// Public API to deliver scene changes from ExecSystem derived classes.
///
/// Classes derived from ExecSystem are responsible for notifying ExecSystem
/// when scene changes occur. They do so by constructing an
/// ExecSystem::_ChangeProcessor from their parent ExecSystem, and invoking
/// methods corresponding to the scene changes.
///
class ExecSystem::_ChangeProcessor
{
public:
    EXEC_API
    explicit _ChangeProcessor(ExecSystem *system);

    EXEC_API
    ~_ChangeProcessor();

    /// Notifies the ExecSystem that a scene object has been resynced.
    ///
    /// \see UsdNotice::ObjectsChanged::GetResyncedPaths.
    ///
    EXEC_API
    void DidResync(const SdfPath &path);

    /// Notifies the ExecSystem that a scene object's fields have changed, but
    /// the object has *not* been resynced.
    ///
    /// \see UsdNotice::ObjectsChanged::GetChangedInfoOnlyPaths.
    /// \see UsdNotice::ObjectsChanged::GetChangedFields.
    ///
    EXEC_API
    void DidChangeInfoOnly(
        const SdfPath &path,
        const TfTokenVector &changedFields);

private:
    // Processes accumulated state from changes before the processor goes out of
    // scope.
    // 
    void _PostProcessChanges();

private:
    ExecSystem *const _system;

    // State accumulated over a round of change processing.
    struct _State;
    std::unique_ptr<_State> _state;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif