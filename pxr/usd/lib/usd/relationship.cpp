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
#include "pxr/pxr.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/instanceCache.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/valueUtils.h"

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

PXR_NAMESPACE_OPEN_SCOPE


// ------------------------------------------------------------------------- //
// UsdRelationship
// ------------------------------------------------------------------------- //

static SdfPath
_MapTargetPath(const UsdStage *stage, const SdfPath &anchor,
               const SdfPath &target)
{
    // If this is a relative target path, we have to map both the anchor
    // and target path and then re-relativize them.
    const UsdEditTarget &editTarget = stage->GetEditTarget();
    if (target.IsAbsolutePath()) {
        return editTarget.MapToSpecPath(target).StripAllVariantSelections();
    } else {
        const SdfPath anchorPrim = anchor.GetPrimPath();
        const SdfPath translatedAnchorPrim =
            editTarget.MapToSpecPath(anchorPrim)
            .StripAllVariantSelections();
        const SdfPath translatedTarget =
            editTarget.MapToSpecPath(target.MakeAbsolutePath(anchorPrim))
            .StripAllVariantSelections();
        return translatedTarget.MakeRelativePath(translatedAnchorPrim);
    }
}

SdfPath
UsdRelationship::_GetTargetForAuthoring(const SdfPath &target,
                                        std::string* whyNot) const
{
    if (!target.IsEmpty()) {
        SdfPath absTarget =
            target.MakeAbsolutePath(GetPath().GetAbsoluteRootOrPrimPath());
        if (Usd_InstanceCache::IsPathInMaster(absTarget)) {
            if (whyNot) { 
                *whyNot = "Cannot target a master or an object within a "
                    "master.";
            }
            return SdfPath();
        }
    }

    UsdStage *stage = _GetStage();
    SdfPath mappedPath = _MapTargetPath(stage, GetPath(), target);
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
UsdRelationship::AddTarget(const SdfPath& target,
                           UsdListPosition position) const
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

    if (!relSpec)
        return false;

    Usd_InsertListItem( relSpec->GetTargetPathList(), targetToAuthor,
                        position );
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

    if (!relSpec)
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

    if (!relSpec)
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

    if (!relSpec)
        return false;

    relSpec->GetTargetPathList().ClearEditsAndMakeExplicit();
    relSpec->GetTargetPathList().GetExplicitItems() = mappedPaths;

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

    if (!relSpec)
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
UsdRelationship::GetTargets(SdfPathVector* targets) const
{
    TRACE_FUNCTION();

    UsdStage *stage = _GetStage();
    PcpErrorVector pcpErrors;
    std::vector<std::string> otherErrors;
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
    if (!targets->empty() && _Prim()->IsInMaster()) {
        Usd_PrimDataConstPtr master = get_pointer(_Prim());
        while (!master->IsMaster()) { 
            master = master->GetParent();  
        }
        
        // Paths that point to an object under the master's source prim index
        // are internal to the master and need to be translated to either
        // the master or instance we're currently looking at.
        const SdfPath& masterSourcePrimIndexPath = 
            master->GetSourcePrimIndex().GetPath();

        if (GetPrim().IsInMaster()) {
            // Translate any paths that point to an object at or under the
            // source prim index to our master.
            for (SdfPath& target : *targets) {
                target = target.ReplacePrefix(
                    masterSourcePrimIndexPath, master->GetPath());
            }
        }
        else if (GetPrim().IsInstanceProxy()) {
            // Translate any paths that point to an object at or under the
            // source prim index to our instance.
            UsdPrim instance = GetPrim();
            while (!instance.IsInstance()) { 
                instance = instance.GetParent(); 
            }

            for (SdfPath& target : *targets) {
                target = target.ReplacePrefix(
                    masterSourcePrimIndexPath, instance.GetPath());
            }
        }
    }

    // TODO: handle errors
    const bool hasErrors = !(pcpErrors.empty() && otherErrors.empty());
    if (hasErrors) {
        stage->_ReportErrors(
            pcpErrors, otherErrors,
            TfStringPrintf("Getting targets for relationship <%s>",
                           GetPath().GetText()));
    }

    return !hasErrors;
}

bool
UsdRelationship::_GetForwardedTargets(SdfPathSet* visited,
                                      SdfPathSet* uniqueTargets,
                                      SdfPathVector* targets,
                                      bool includeForwardingRels) const
{
    // Track recursive composition errors, starting with the first batch of
    // targets.
    SdfPathVector curTargets;
    bool success = GetTargets(&curTargets);

    // Process all targets at this relationship.
    for (SdfPath const &target: curTargets) {
        if (target.IsPrimPropertyPath()) {
            // Resolve forwarding if this target points at a relationship.
            if (UsdPrim prim = GetStage()->GetPrimAtPath(target.GetPrimPath())) {
                if (UsdRelationship rel =
                    prim.GetRelationship(target.GetNameToken())) {
                    if (visited->insert(rel.GetPath()).second) {
                        // Only do this rel if we've not yet seen it.
                        success &= rel._GetForwardedTargets(
                            visited, uniqueTargets, targets,
                            includeForwardingRels);
                    }
                    if (!includeForwardingRels)
                        continue;
                }
            }
        }            
        if (uniqueTargets->insert(target).second)
            targets->push_back(target);
    }

    return success;
}

bool
UsdRelationship::_GetForwardedTargets(
    SdfPathVector *targets,
    bool includeForwardingRels) const
{
    SdfPathSet visited, uniqueTargets;
    return _GetForwardedTargets(&visited, &uniqueTargets, targets,
                                includeForwardingRels);
}

bool
UsdRelationship::GetForwardedTargets(SdfPathVector* targets) const
{
    if (!targets) {
        TF_CODING_ERROR("Passed null pointer for targets on <%s>",
                        GetPath().GetText());
        return false;
    }
    targets->clear();
    return _GetForwardedTargets(targets,
                                /*includeForwardingRels=*/false);
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
            stage->_CreatePrimSpecForEditing(GetPrim()),
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

PXR_NAMESPACE_CLOSE_SCOPE

