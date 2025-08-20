//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esf/relationship.h"

#include "pxr/exec/esf/editReason.h"
#include "pxr/exec/esf/journal.h"
#include "pxr/exec/esf/stage.h"

PXR_NAMESPACE_OPEN_SCOPE

EsfRelationshipInterface::~EsfRelationshipInterface() = default;

SdfPathVector
EsfRelationshipInterface::GetTargets(EsfJournal *const journal) const
{
    if (journal) {
        journal->Add(
            _GetPath(),
            EsfEditReason::ResyncedObject |
            EsfEditReason::ChangedTargetPaths);
    }
    return _GetTargets();
}

static void
_GetForwardedTargetsImpl(
    const EsfRelationshipInterface *const relationship,
    const EsfStageInterface *const stage,
    SdfPathSet *const visitedRels,
    SdfPathSet *const uniqueTargets,
    SdfPathVector *const result,
    EsfJournal *const journal)
{
    const SdfPathVector targets = relationship->GetTargets(journal);
    if (targets.empty()) {
        return;
    }

    for (const SdfPath &target: targets) {
        if (target.IsPrimPropertyPath()) {

            // Resolve forwarding if this target points at a relationship.
            const EsfRelationship rel =
                stage->GetRelationshipAtPath(target, journal);
            if (rel->IsValid(journal)) {

                // Skip if we already saw this relationship.
                const bool inserted =
                    visitedRels->insert(rel->GetPath(journal)).second;
                if (inserted) {
                    _GetForwardedTargetsImpl(
                        rel.Get(), stage,
                        visitedRels, uniqueTargets, result,
                        journal);
                }
            }
        }

        const bool inserted = uniqueTargets->insert(target).second;
        if (inserted) {
            result->push_back(target);
        }
    }
}

SdfPathVector
EsfRelationshipInterface::GetForwardedTargets(EsfJournal *const journal) const
{
    SdfPathVector targets;
    SdfPathSet visitedRels, uniqueTargets;
    _GetForwardedTargetsImpl(
        this,
        _GetStage().Get(),
        &visitedRels,
        &uniqueTargets,
        &targets,
        journal);

    return targets;
}

PXR_NAMESPACE_CLOSE_SCOPE
