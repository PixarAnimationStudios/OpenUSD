//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esf/object.h"

#include "pxr/exec/esf/editReason.h"
#include "pxr/exec/esf/journal.h"
#include "pxr/exec/esf/prim.h"

PXR_NAMESPACE_OPEN_SCOPE

EsfObjectInterface::~EsfObjectInterface() = default;

bool
EsfObjectInterface::IsValid(EsfJournal *journal) const
{
    // If the path is empty, the object is invalid, but we don't want to
    // journal for the empty path;
    const SdfPath path = _GetPath();
    if (path.IsEmpty()) {
        return false;
    }

    if (journal) {
        journal->Add(path, EsfEditReason::ResyncedObject);
    }
    return _IsValid();
}

SdfPath
EsfObjectInterface::GetPath(EsfJournal *journal) const
{
    if (journal) {
        journal->Add(_GetPath(), EsfEditReason::ResyncedObject);
    }
    return _path;
}

TfToken
EsfObjectInterface::GetName(EsfJournal *journal) const
{
    if (journal) {
        journal->Add(_GetPath(), EsfEditReason::ResyncedObject);
    }
    return _GetName();
}

EsfPrim
EsfObjectInterface::GetPrim(EsfJournal *journal) const
{
    if (journal) {
        journal->Add(_GetPath().GetPrimPath(), EsfEditReason::ResyncedObject);
    }
    return _GetPrim();
}

EsfSchemaConfigKey
EsfObjectInterface::GetSchemaConfigKey(EsfJournal *journal) const
{
    // We need to handle the pseudo-root specially, to avoid journaling for
    // the empty path.
    if (_GetPath().IsAbsoluteRootPath()) {
        return EsfSchemaConfigKey();
    }

    if (journal) {
        journal->Add(_GetPath().GetPrimPath(), EsfEditReason::ResyncedObject);
    }
    return _GetSchemaConfigKey();
}

PXR_NAMESPACE_CLOSE_SCOPE
