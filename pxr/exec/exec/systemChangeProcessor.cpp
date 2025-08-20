//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/systemChangeProcessor.h"

#include "pxr/exec/exec/types.h"
#include "pxr/exec/exec/uncompiler.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"

PXR_NAMESPACE_OPEN_SCOPE

struct ExecSystem::_ChangeProcessor::_State {
    explicit _State(ExecSystem *const system)
        : uncompiler(system->_program.get(), system->_runtime.get())
    {}

    // Accumulate scene paths to attributes with invalid authored values, so
    // that program and executor invalidation can be batch-processed.
    TfSmallVector<SdfPath, 1> attributesWithInvalidAuthoredValues;
    
    // Uncompile changed network.
    Exec_Uncompiler uncompiler;
};

ExecSystem::_ChangeProcessor::_ChangeProcessor(ExecSystem *const system)
    : _system(system)
    , _state(std::make_unique<_State>(system))
{
    TF_VERIFY(system);
}

ExecSystem::_ChangeProcessor::~_ChangeProcessor()
{
    _PostProcessChanges();
}

void
ExecSystem::_ChangeProcessor::DidResync(const SdfPath &path)
{
    // TODO: Resyncs on an object may trigger edit reasons on related objects.
    // (E.g. resync on /Prim.attr would trigger a ChangedPropertyList on /Prim)
    // That would be handled here. For now, resync is the only supported edit
    // reason.
    _state->uncompiler.UncompileForSceneChange(
        path, EsfEditReason::ResyncedObject);
}

void
ExecSystem::_ChangeProcessor::DidChangeInfoOnly(
    const SdfPath &path,
    const TfTokenVector &changedFields)
{
    if (path.IsPropertyPath()) {
        bool didRecordAuthoredValueChange = false;
        for (const TfToken &field : changedFields) {
            if (!didRecordAuthoredValueChange &&
                    (field == SdfFieldKeys->Default ||
                    field == SdfFieldKeys->Spline ||
                    field == SdfFieldKeys->TimeSamples)) {
                _state->attributesWithInvalidAuthoredValues.push_back(path);
                didRecordAuthoredValueChange = true;
            }
            else if (field == SdfFieldKeys->TargetPaths) {
                _state->uncompiler.UncompileForSceneChange(
                    path, EsfEditReason::ChangedTargetPaths);
            }
        }
    }
}

void
ExecSystem::_ChangeProcessor::_PostProcessChanges()
{
    if (_state->uncompiler.DidUncompile()) {
        _system->_InvalidateDisconnectedInputs();
    }

    if (!_state->attributesWithInvalidAuthoredValues.empty()) {
        _system->_InvalidateAuthoredValues(
            _state->attributesWithInvalidAuthoredValues);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
