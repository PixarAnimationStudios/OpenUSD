//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/instanceCache.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/pcp/targetIndex.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/base/tracelite/trace.h"

#include <algorithm>
#include <set>
#include <vector>

// ------------------------------------------------------------------------- //
// UsdRelationship
// ------------------------------------------------------------------------- //

static SdfPath
_MapTargetPath(const UsdStage *stage, const SdfPath &path)
{
    return stage->GetEditTarget().
        MapToSpecPath(path).StripAllVariantSelections();
}

SdfPath
UsdRelationship::_GetTargetForAuthoring(const SdfPath &target,
                                        std::string* whyNot) const
{
    if (!target.IsEmpty()) {
        SdfPath absTarget =
            target.MakeAbsolutePath(GetPath().GetAbsoluteRootOrPrimPath());
        if (Usd_InstanceCache::IsPathMasterOrInMaster(absTarget)) {
            if (whyNot) { 
                *whyNot = "Cannot target a master or an object within a "
                    "master.";
            }
            return SdfPath();
        }
    }

    UsdStage *stage = _GetStage();
    SdfPath mappedPath = _MapTargetPath(stage, target);
    if (mappedPath.IsEmpty()) {
        if (whyNot) {
            *whyNot = TfStringPrintf("Cannot map <%s> to layer @%s@ via stage's "
                                     "EditTarget",
                                     target.GetText(), stage->GetEditTarget().
                                     GetLayer()->GetIdentifier().c_str());
        }
    }

    return mappedPath;
}

bool
UsdRelationship::AddTarget(const SdfPath& target) const
{
    std::string errMsg;
    const SdfPath targetToAuthor = _GetTargetForAuthoring(target, &errMsg);
    if (targetToAuthor.IsEmpty()) {
        TF_CODING_ERROR("Cannot add target <%s> to relationship <%s>: %s",
                        target.GetText(), GetPath().GetText(), errMsg.c_str());
        return false;
    }

    // NOTE! Do not insert any code that modifies scene description between the
    // changeblock and the call to _CreateSpec!  Explanation: _CreateSpec calls
    // code that inspects the composition graph and then does some authoring.
    // We want that authoring to be inside the change block, but if any scene
    // description changes are made after the block is created but before we
    // call _CreateSpec, the composition structure may be invalidated.
    SdfChangeBlock block;
    SdfRelationshipSpecHandle relSpec = _CreateSpec();

    if (not relSpec)
        return false;

    relSpec->GetTargetPathList().Add(targetToAuthor);
    return true;
}

bool
UsdRelationship::RemoveTarget(const SdfPath& target) const
{
    std::string errMsg;
    const SdfPath targetToAuthor = _GetTargetForAuthoring(target, &errMsg);
    if (targetToAuthor.IsEmpty()) {
        TF_CODING_ERROR("Cannot remove target <%s> from relationship <%s>: %s",
                        target.GetText(), GetPath().GetText(), errMsg.c_str());
        return false;
    }

    // NOTE! Do not insert any code that modifies scene description between the
    // changeblock and the call to _CreateSpec!  Explanation: _CreateSpec calls
    // code that inspects the composition graph and then does some authoring.
    // We want that authoring to be inside the change block, but if any scene
    // description changes are made after the block is created but before we
    // call _CreateSpec, the composition structure may be invalidated.
    SdfChangeBlock block;
    SdfRelationshipSpecHandle relSpec = _CreateSpec();

    if (not relSpec)
        return false;

    relSpec->GetTargetPathList().Remove(targetToAuthor);
    return true;
}

bool
UsdRelationship::BlockTargets() const
{
    // NOTE! Do not insert any code that modifies scene description between the
    // changeblock and the call to _CreateSpec!  Explanation: _CreateSpec calls
    // code that inspects the composition graph and then does some authoring.
    // We want that authoring to be inside the change block, but if any scene
    // description changes are made after the block is created but before we
    // call _CreateSpec, the composition structure may be invalidated.
    SdfChangeBlock block;
    SdfRelationshipSpecHandle relSpec = _CreateSpec();

    if (not relSpec)
        return false;

    relSpec->GetTargetPathList().ClearEditsAndMakeExplicit();
    return true;
}

bool
UsdRelationship::SetTargets(const SdfPathVector& targets) const
{
    SdfPathVector mappedPaths;
    mappedPaths.reserve(targets.size());
    for (const SdfPath &target: targets) {
        std::string errMsg;
        mappedPaths.push_back(_GetTargetForAuthoring(target, &errMsg));
        if (mappedPaths.back().IsEmpty()) {
            TF_CODING_ERROR("Cannot set target <%s> on relationship <%s>: %s",
                            target.GetText(), GetPath().GetText(), 
                            errMsg.c_str());
            return false;
        }
    }

    // NOTE! Do not insert any code that modifies scene description between the
    // changeblock and the call to _CreateSpec!  Explanation: _CreateSpec calls
    // code that inspects the composition graph and then does some authoring.
    // We want that authoring to be inside the change block, but if any scene
    // description changes are made after the block is created but before we
    // call _CreateSpec, the composition structure may be invalidated.
    SdfChangeBlock block;
    SdfRelationshipSpecHandle relSpec = _CreateSpec();

    if (not relSpec)
        return false;

    relSpec->GetTargetPathList().ClearEditsAndMakeExplicit();
    for (const SdfPath &path: mappedPaths) {
        relSpec->GetTargetPathList().Add(path);
    }

    return true;
}

bool
UsdRelationship::ClearTargets(bool removeSpec) const
{
    // NOTE! Do not insert any code that modifies scene description between the
    // changeblock and the call to _CreateSpec!  Explanation: _CreateSpec calls
    // code that inspects the composition graph and then does some authoring.
    // We want that authoring to be inside the change block, but if any scene
    // description changes are made after the block is created but before we
    // call _CreateSpec, the composition structure may be invalidated.
    SdfChangeBlock block;
    SdfRelationshipSpecHandle relSpec = _CreateSpec();

    if (not relSpec)
        return false;

    if (removeSpec){
        SdfPrimSpecHandle  owner = 
            TfDynamic_cast<SdfPrimSpecHandle>(relSpec->GetOwner());
        owner->RemoveProperty(relSpec);
    }
    else {
        relSpec->GetTargetPathList().ClearEdits();
    } 
    return true;
}

bool
UsdRelationship::GetTargets(SdfPathVector* targets,
                            bool forwardToObjectsInMasters) const
{
    TRACE_FUNCTION();

    UsdStage *stage = _GetStage();
    PcpErrorVector pcpErrors;
    PcpTargetIndex targetIndex;
    {
        // Our intention is that the following code requires read-only
        // access to the PcpCache, so use a const-ref.
        const PcpCache& pcpCache(*stage->_GetPcpCache());
        // In USD mode, Pcp does not cache property indexes, so we
        // compute one here ourselves and use that.  First, we need
        // to get the prim index of the owning prim.
        const PcpPrimIndex &primIndex = _Prim()->GetPrimIndex();
        // PERFORMANCE: Here we can't avoid constructing the full property path
        // without changing the Pcp API.  We're about to do serious
        // composition/indexing, though, so the added expense may be neglible.
        const PcpSite propSite(pcpCache.GetLayerStackIdentifier(), GetPath());
        PcpPropertyIndex propIndex;
        PcpBuildPrimPropertyIndex(propSite.path, pcpCache, primIndex,
                                  &propIndex, &pcpErrors);
        PcpBuildTargetIndex(propSite, propIndex, SdfSpecTypeRelationship,
                            &targetIndex, &pcpErrors);
    }

    targets->swap(targetIndex.paths);

    // Translate any target paths that point to descendents of instance prims
    // to the corresponding prim in the instance's master if desired.
    std::vector<std::string> otherErrors;
    if (forwardToObjectsInMasters) {
        const bool relationshipInMaster = _Prim()->IsInMaster();
        SdfPath masterPath;
        if (relationshipInMaster) {
            masterPath = _Prim()->GetPath();
            while (not masterPath.IsRootPrimPath()) { 
                masterPath = masterPath.GetParentPath();
            }
        }

        const Usd_InstanceCache* instanceCache = stage->_instanceCache.get();
        for (SdfPath& path : *targets) {
            const SdfPath& primPath = path.GetPrimPath();
            const SdfPath& primInMasterPath = 
                instanceCache->GetPrimInMasterForPrimIndexAtPath(primPath);
            const bool pathIsObjectInMaster = not primInMasterPath.IsEmpty();

            if (pathIsObjectInMaster) {
                path = path.ReplacePrefix(primPath, primInMasterPath);
            }
            else if (relationshipInMaster) {
                // Relationships authored within an instance cannot target
                // the instance itself or any its properties, since doing so
                // would break the encapsulation of the instanced scene
                // description and the ability to share that data across 
                // multiple instances.
                // 
                // We can detect this situation by checking whether this
                // target has a corresponding master prim. If so, issue
                // an error and elide this target.
                if (stage->_instanceCache->GetMasterForPrimIndexAtPath(primPath) 
                    == masterPath) {
                    // XXX: 
                    // The path in this error message is potentially
                    // confusing because it is the composed target path and
                    // not necessarily what's authored in a layer. In order
                    // to be more useful, we'd need to return more info
                    // from the relationship target composition.
                    otherErrors.push_back(TfStringPrintf(
                        "Target path <%s> is not allowed because it is "
                        "authored in instanced scene description but targets "
                        "its owning instance.",
                        path.GetText()));
                    path = SdfPath();
                }
            }
        }

        targets->erase(
            std::remove(targets->begin(), targets->end(), SdfPath()),
            targets->end());
    }

    // TODO: handle errors
    const bool hasErrors = not (pcpErrors.empty() and otherErrors.empty());
    if (hasErrors) {
        stage->_ReportErrors(
            pcpErrors, otherErrors,
            TfStringPrintf("Getting targets for relationship <%s>",
                           GetPath().GetText()));
    }

    return not hasErrors;
}

bool
UsdRelationship::_GetForwardedTargets(SdfPathSet* visited,
                                      SdfPathSet* uniqueTargets,
                                      SdfPathVector* targets,
                                      bool forwardToObjectsInMasters) const
{
    if (not visited->insert(GetPath()).second) {
        // Cycle or dag fan-in detected.
        // We're not sure if it's a cycle or a closed-loop in the dag, so
        // silently continue.
        return true;
    }

    SdfPathVector curTargets;
    // Track recursive composition errors, starting with the first batch of
    // targets.
    bool success = GetTargets(&curTargets, forwardToObjectsInMasters);

    // Process all targets at this relationship.
    for (const auto& target : curTargets) {
        UsdPrim nextPrim = GetStage()->GetPrimAtPath(target.GetPrimPath());

        if (nextPrim) {
            if (UsdRelationship rel =
                        nextPrim.GetRelationship(target.GetNameToken())) {
                // It doesn't matter if we fail here, just track the error
                // state and continue attempting to gather targets.
                success = success and 
                      rel._GetForwardedTargets(visited, uniqueTargets, targets,
                                               forwardToObjectsInMasters);
                // Never append paths that target a relationship.
                continue;
            } 
        }
        
        if (uniqueTargets->insert(target).second) {
            targets->push_back(target);
        }
    }

    return success;
}

bool
UsdRelationship::GetForwardedTargets(SdfPathVector* targets,
                                     bool forwardToObjectsInMasters) const
{
    if (not targets) {
        TF_CODING_ERROR("Received null target pointer while processing <%s>\n",
                GetPath().GetText());
        return false;
    } else if (not targets->empty()) {
        TF_CODING_ERROR("Received non-empty targets while processing <%s>\n",
                GetPath().GetText());
        targets->clear();
    }

    SdfPathSet visited, uniqueTargets;
    return _GetForwardedTargets(&visited, &uniqueTargets, targets,
                                forwardToObjectsInMasters);
}

bool
UsdRelationship::HasAuthoredTargets() const
{
    return HasAuthoredMetadata(SdfFieldKeys->TargetPaths);
}

SdfRelationshipSpecHandle
UsdRelationship::_CreateSpec(bool fallbackCustom) const
{
    UsdStage *stage = _GetStage();
    // Try to create a spec for editing either from the definition or from
    // copying existing spec info.
    TfErrorMark m;
    if (SdfRelationshipSpecHandle relSpec =
        stage->_CreateRelationshipSpecForEditing(*this)) {
        return relSpec;
    }

    // If creating the spec on the stage failed without issuing an error, that
    // means there was no existing authored scene description to go on (i.e. no
    // builtin info from prim type, and no existing authored spec).  Stamp a
    // spec with the provided default values.
    if (m.IsClean()) {
        SdfChangeBlock block;
        return SdfRelationshipSpec::New(
            stage->_CreatePrimSpecForEditing(GetPrimPath()),
            _PropName().GetString(),
            /* custom = */ fallbackCustom, SdfVariabilityUniform);
    }
    return TfNullPtr;
}

bool
UsdRelationship::_Create(bool fallbackCustom) const
{
    return bool(_CreateSpec(fallbackCustom));
}
