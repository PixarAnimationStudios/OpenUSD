//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esf/stage.h"

#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/journal.h"
#include "pxr/exec/esf/object.h"
#include "pxr/exec/esf/prim.h"
#include "pxr/exec/esf/property.h"
#include "pxr/exec/esf/relationship.h"

PXR_NAMESPACE_OPEN_SCOPE

EsfStageInterface::~EsfStageInterface() = default;

EsfAttribute
EsfStageInterface::GetAttributeAtPath(
    const SdfPath &path,
    EsfJournal *journal) const
{
    if (journal) {
        journal->Add(path, EsfEditReason::ResyncedObject);
    }
    return _GetAttributeAtPath(path);
}

EsfObject
EsfStageInterface::GetObjectAtPath(
    const SdfPath &path,
    EsfJournal *journal) const
{
    if (journal) {
        journal->Add(path, EsfEditReason::ResyncedObject);
    }
    return _GetObjectAtPath(path);
}

EsfPrim
EsfStageInterface::GetPrimAtPath(
    const SdfPath &path,
    EsfJournal *journal) const
{
    if (journal) {
        journal->Add(path, EsfEditReason::ResyncedObject);
    }
    return _GetPrimAtPath(path);
}

EsfProperty
EsfStageInterface::GetPropertyAtPath(
    const SdfPath &path,
    EsfJournal *journal) const
{
    if (journal) {
        journal->Add(path, EsfEditReason::ResyncedObject);
    }
    return _GetPropertyAtPath(path);
}

EsfRelationship
EsfStageInterface::GetRelationshipAtPath(
    const SdfPath &path,
    EsfJournal *journal) const
{
    if (journal) {
        journal->Add(path, EsfEditReason::ResyncedObject);
    }
    return _GetRelationshipAtPath(path);
}

PXR_NAMESPACE_CLOSE_SCOPE
