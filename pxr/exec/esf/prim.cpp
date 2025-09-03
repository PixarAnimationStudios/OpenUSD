//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esf/prim.h"

#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/editReason.h"
#include "pxr/exec/esf/journal.h"
#include "pxr/exec/esf/relationship.h"

PXR_NAMESPACE_OPEN_SCOPE

EsfPrimInterface::~EsfPrimInterface() = default;

const TfTokenVector &
EsfPrimInterface::GetAppliedSchemas(EsfJournal *journal) const
{
    if (journal) {
        journal->Add(_GetPath(), EsfEditReason::ResyncedObject);
    }
    return _GetAppliedSchemas();
}

EsfAttribute
EsfPrimInterface::GetAttribute(
    const TfToken &attributeName,
    EsfJournal *journal) const
{
    if (journal) {
        journal->Add(
            _GetPath().AppendProperty(attributeName),
            EsfEditReason::ResyncedObject);
    }
    return _GetAttribute(attributeName);
}

EsfRelationship
EsfPrimInterface::GetRelationship(
    const TfToken &relationshipName,
    EsfJournal *journal) const
{
    if (journal) {
        journal->Add(
            _GetPath().AppendProperty(relationshipName),
            EsfEditReason::ResyncedObject);
    }
    return _GetRelationship(relationshipName);
}

EsfPrim
EsfPrimInterface::GetParent(EsfJournal *journal) const
{
    if (journal) {
        journal->Add(_GetPath(), EsfEditReason::ResyncedObject);
    }
    return _GetParent();
}

TfType
EsfPrimInterface::GetType(EsfJournal *journal) const
{
    if (journal) {
        journal->Add(_GetPath(), EsfEditReason::ResyncedObject);
    }
    return _GetType();
}

PXR_NAMESPACE_CLOSE_SCOPE
