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
/// \file RelationshipSpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/accessorHelpers.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/childrenUtils.h"
#include "pxr/usd/sdf/childrenPolicies.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/tf/type.h"
#include "pxr/base/trace/trace.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DEFINE_SPEC(
    SdfSchema, SdfSpecTypeRelationship, SdfRelationshipSpec, SdfPropertySpec);

//
// Primary API
//

SdfRelationshipSpecHandle
SdfRelationshipSpec::New(
    const SdfPrimSpecHandle& owner,
    const std::string& name,
    bool custom,
    SdfVariability variability)
{
    TRACE_FUNCTION();

    if (!owner) {
        TF_CODING_ERROR("NULL owner prim");
        return TfNullPtr;
    }

    if (!Sdf_ChildrenUtils<Sdf_RelationshipChildPolicy>::IsValidName(name)) {
        TF_CODING_ERROR("Cannot create a relationship on %s with "
            "invalid name: %s", owner->GetPath().GetText(), name.c_str());
        return TfNullPtr;
    }

    SdfPath relPath = owner->GetPath().AppendProperty(TfToken(name));
    if (!relPath.IsPropertyPath()) {
        TF_CODING_ERROR(
            "Cannot create relationship at invalid path <%s.%s>",
            owner->GetPath().GetText(), name.c_str());
        return TfNullPtr;
    }
    
    // RelationshipSpecs are considered initially to have only required fields 
    // only if they are not custom.
    bool hasOnlyRequiredFields = (!custom);

    SdfChangeBlock block;

    if (!Sdf_ChildrenUtils<Sdf_RelationshipChildPolicy>::CreateSpec(
            owner->GetLayer(), relPath, SdfSpecTypeRelationship, 
            hasOnlyRequiredFields)) {
        return TfNullPtr;
    }

    SdfRelationshipSpecHandle spec =
        owner->GetLayer()->GetRelationshipAtPath(relPath);

    spec->SetField(SdfFieldKeys->Custom, custom);
    spec->SetField(SdfFieldKeys->Variability, variability);

    return spec;
}

//
// Relationship Targets
//

SdfPath 
SdfRelationshipSpec::_CanonicalizeTargetPath(const SdfPath& path) const
{
    // Relationship target paths are always absolute. If a relative path
    // is passed in, it is considered to be relative to the relationship's
    // owning prim.
    return path.MakeAbsolutePath(GetPath().GetPrimPath());
}

SdfPath 
SdfRelationshipSpec::_MakeCompleteTargetSpecPath(
    const SdfPath& targetPath) const
{
    SdfPath absPath = _CanonicalizeTargetPath(targetPath);
    return GetPath().AppendTarget(absPath);
}

SdfSpecHandle 
SdfRelationshipSpec::_GetTargetSpec(const SdfPath& path) const
{
    return GetLayer()->GetObjectAtPath(
        _MakeCompleteTargetSpecPath(path));
}

SdfTargetsProxy
SdfRelationshipSpec::GetTargetPathList() const
{
    return SdfGetPathEditorProxy(
        SdfCreateHandle(this), SdfFieldKeys->TargetPaths);
}

bool
SdfRelationshipSpec::HasTargetPathList() const
{
    return GetTargetPathList().HasKeys();
}

void
SdfRelationshipSpec::ClearTargetPathList() const
{
    GetTargetPathList().ClearEdits();
}

static boost::optional<SdfPath>
_ReplacePath(
    const SdfPath &oldPath, const SdfPath &newPath, const SdfPath &path)
{
    // Replace oldPath with newPath, and also remove any existing
    // newPath entries in the list op.
    if (path == oldPath) {
        return newPath;
    }
    if (path == newPath) {
        return boost::none;
    }
    return path;
}

void
SdfRelationshipSpec::ReplaceTargetPath(
    const SdfPath& oldPath,
    const SdfPath& newPath)
{
    // Check permissions; this is done here to catch the case where ChangePaths
    // is not called due to an erroneous oldPath being supplied, and ModifyEdits
    // won't check either if there are no changes made.
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("ReplaceTargetPath: Permission denied.");
        return;
    }

    const SdfPath& relPath = GetPath();
    const SdfLayerHandle& layer = GetLayer();

    SdfPath oldTargetPath = _CanonicalizeTargetPath(oldPath);
    SdfPath newTargetPath = _CanonicalizeTargetPath(newPath);
    
    if (oldTargetPath == newTargetPath) {
        return;
    }

    // Get the paths of all the existing target specs
    std::vector<SdfPath> siblingPaths = 
	layer->GetFieldAs<std::vector<SdfPath> >(
            relPath, SdfChildrenKeys->RelationshipTargetChildren);

    int oldTargetSpecIndex = -1;
    int newTargetSpecIndex = -1;
    for (size_t i = 0, n = siblingPaths.size(); i != n; ++i) {
        if (siblingPaths[i] == oldTargetPath) {
            oldTargetSpecIndex = i;
        }
        else if (siblingPaths[i] == newTargetPath) {
            newTargetSpecIndex = i;
        }
    }
    
    // If there is a target spec, then update the children field.
    if (oldTargetSpecIndex != -1) {
        SdfPath oldTargetSpecPath = relPath.AppendTarget(oldTargetPath);
        SdfPath newTargetSpecPath = relPath.AppendTarget(newTargetPath);

        if (layer->HasSpec(newTargetSpecPath)) {
            // Target already exists.  If the target has no child specs
            // then we'll allow the replacement.  If it does have
            // attributes then we must refuse.
            const SdfSchemaBase& schema = GetSchema();
            for (const TfToken& field : layer->ListFields(newTargetSpecPath)) {
                if (schema.HoldsChildren(field)) {
                    TF_CODING_ERROR("Can't replace target %s with target %s in "
                                    "relationship %s: %s",
                                    oldPath.GetText(),
                                    newPath.GetText(),
                                    relPath.GetString().c_str(),
                                    "Target already exists");
                    return;
                }
            }

            // Remove the existing spec at the new target path.
            _DeleteSpec(newTargetSpecPath);

            TF_VERIFY (!layer->HasSpec(newTargetSpecPath));
        }
        
        // Move the spec and all the fields under it.
        if (!_MoveSpec(oldTargetSpecPath, newTargetSpecPath)) {
            TF_CODING_ERROR("Cannot move %s to %s", oldTargetPath.GetText(),
                newTargetPath.GetText());
            return;
        }

        // Update and set the siblings
        siblingPaths[oldTargetSpecIndex] = newTargetPath;
        if (newTargetSpecIndex != -1) {
            siblingPaths.erase(siblingPaths.begin() + newTargetSpecIndex);
        }
        
        layer->SetField(relPath, SdfChildrenKeys->RelationshipTargetChildren,
            siblingPaths);
    }

    // Get the list op.
    SdfPathListOp targetsListOp = 
        layer->GetFieldAs<SdfPathListOp>(relPath, SdfFieldKeys->TargetPaths);

    // Update the list op.
    if (targetsListOp.HasItem(oldTargetPath)) {
        targetsListOp.ModifyOperations(
            std::bind(_ReplacePath, oldTargetPath, newTargetPath,
                std::placeholders::_1));
        layer->SetField(relPath, SdfFieldKeys->TargetPaths, targetsListOp);
    }
}

void
SdfRelationshipSpec::RemoveTargetPath(
    const SdfPath& path,
    bool preserveTargetOrder)
{
    // Csd expects to see remove property notices for all of our
    // relational attributes.  The change below won't send them since
    // they're implied by the removal of their owner.
    // XXX: Csd should implicitly assume these notices.
    const SdfPath targetSpecPath = 
        GetPath().AppendTarget(_CanonicalizeTargetPath(path));

    SdfChangeBlock block;
    Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::SetChildren(
        GetLayer(), targetSpecPath, 
        std::vector<SdfAttributeSpecHandle>());

    // The SdfTargetsProxy will manage conversion of the SdfPaths and changes to
    // both the list edits and actual object hierarchy underneath.
    if (preserveTargetOrder) {
        GetTargetPathList().Erase(path);
    }
    else {
        GetTargetPathList().RemoveItemEdits(path);
    }
}

//
// Metadata, Property Value API, and Spec Properties
// (methods built on generic SdfSpec accessor macros)
//

// Initialize accessor helper macros to associate with this class and optimize
// out the access predicate
#define SDF_ACCESSOR_CLASS                   SdfRelationshipSpec
#define SDF_ACCESSOR_READ_PREDICATE(key_)    SDF_NO_PREDICATE
#define SDF_ACCESSOR_WRITE_PREDICATE(key_)   SDF_NO_PREDICATE

SDF_DEFINE_GET_SET(NoLoadHint, SdfFieldKeys->NoLoadHint, bool);

#undef SDF_ACCESSOR_CLASS                   
#undef SDF_ACCESSOR_READ_PREDICATE
#undef SDF_ACCESSOR_WRITE_PREDICATE

PXR_NAMESPACE_CLOSE_SCOPE
