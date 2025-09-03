//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esf/property.h"

#include "pxr/exec/esf/editReason.h"
#include "pxr/exec/esf/journal.h"

PXR_NAMESPACE_OPEN_SCOPE

EsfPropertyInterface::~EsfPropertyInterface() = default;

TfToken
EsfPropertyInterface::GetBaseName(EsfJournal *journal) const
{
    if (journal) {
        journal->Add(_GetPath(), EsfEditReason::ResyncedObject);
    }
    return _GetBaseName();
}

TfToken
EsfPropertyInterface::GetNamespace(EsfJournal *journal) const
{
    if (journal) {
        journal->Add(_GetPath(), EsfEditReason::ResyncedObject);
    }
    return _GetNamespace();
}

PXR_NAMESPACE_CLOSE_SCOPE