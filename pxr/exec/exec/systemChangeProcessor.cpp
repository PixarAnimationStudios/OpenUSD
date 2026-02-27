//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/systemChangeProcessor.h"

#include "pxr/exec/exec/program.h"
#include "pxr/exec/exec/uncompiler.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

struct ExecSystem::_ChangeProcessor::_State {
    explicit _State(ExecSystem *const system)
        : uncompiler(system->_program.get(), system->_runtime.get())
    {}

    // Accumulates scene paths to attributes with invalid authored values, so
    // that program and executor invalidation can be batch-processed.
    TfSmallVector<SdfPath, 1> attributesWithInvalidAuthoredValues;

    // Accumulates scene paths and field names that indicate invalid metadata
    // values.
    TfSmallVector<std::pair<SdfPath, TfToken>, 1>
    objectsWithInvalidMetadataValues;
    
    // Uncompiles changed network.
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
        bool didRecordAttributeValueChange = false;
        for (const TfToken &field : changedFields) {
            if (!didRecordAttributeValueChange &&
                (field == SdfFieldKeys->Default ||
                 field == SdfFieldKeys->Spline ||
                 field == SdfFieldKeys->TimeSamples)) {
                _state->attributesWithInvalidAuthoredValues.push_back(path);
                didRecordAttributeValueChange = true;
            }
            else if (field == SdfFieldKeys->TargetPaths) {
                _state->uncompiler.UncompileForSceneChange(
                    path, EsfEditReason::ChangedTargetPaths);
            }
            else if (field == SdfFieldKeys->ConnectionPaths) {
                _state->uncompiler.UncompileForSceneChange(
                    path, EsfEditReason::ChangedConnectionPaths);
            }
            else {
                // TODO: The field name may not even be a valid metadata key.
                // It works correctly to pass along all invalid field names
                // here, since we will only invalidate when they correspond to
                // metadata input nodes that have been compiled. But we could
                // potentially avoid unnecessary map lookups if we could filter
                // upstream--but it's not totally clear which strategy actually
                // yields the best performance.
                _state->objectsWithInvalidMetadataValues.emplace_back(
                    path, field);
            }
        }
    }
    else if (path.IsPrimPath()) {
        for (const TfToken &field : changedFields) {
            _state->objectsWithInvalidMetadataValues.emplace_back(path, field);
        }
    }
}

void
ExecSystem::_ChangeProcessor::_PostProcessChanges()
{
    if (_state->uncompiler.DidUncompile() ||
        _system->_program->WasInterrupted()) {
        _system->_InvalidateDisconnectedInputs();
    }

    if (!_state->attributesWithInvalidAuthoredValues.empty()) {
        _system->_InvalidateAttributeValues(
            _state->attributesWithInvalidAuthoredValues);
    }

    if (!_state->objectsWithInvalidMetadataValues.empty()) {
        _system->_InvalidateMetadataValues(
            _state->objectsWithInvalidMetadataValues);
    }

    if (_system->_program->WasInterrupted()) {
        _system->_InvalidateUnknownValues();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
