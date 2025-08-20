//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esf/attribute.h"

#include "pxr/exec/esf/editReason.h"
#include "pxr/exec/esf/journal.h"

PXR_NAMESPACE_OPEN_SCOPE

EsfAttributeInterface::~EsfAttributeInterface() = default;

SdfValueTypeName
EsfAttributeInterface::GetValueTypeName(EsfJournal *journal) const
{
    if (journal) {
        journal->Add(_GetPath(), EsfEditReason::ResyncedObject);
    }
    return _GetValueTypeName();
}

EsfAttributeQuery
EsfAttributeInterface::GetQuery() const
{
    return _GetQuery();
}

PXR_NAMESPACE_CLOSE_SCOPE