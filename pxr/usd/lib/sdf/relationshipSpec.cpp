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
#include "pxr/usd/sdf/vectorListEditor.h"

#include "pxr/base/tf/type.h"
#include "pxr/base/trace/trace.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DEFINE_SPEC(SdfRelationshipSpec, SdfPropertySpec);

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

SdfSpecHandle 
SdfRelationshipSpec::_FindOrCreateTargetSpec(const SdfPath& path)
{
    const SdfPath targetPath = _CanonicalizeTargetPath(path);

    SdfSpecHandle relTargetSpec = _GetTargetSpec(targetPath);
    if (!relTargetSpec) {
        SdfAllowed allowed;
        if (!PermissionToEdit()) { 
            allowed = SdfAllowed("Permission denied");
        }
        else {
            allowed = SdfSchema::IsValidRelationshipTargetPath(targetPath);
        }

        const SdfPath targetSpecPath = _MakeCompleteTargetSpecPath(targetPath);
        if (allowed) {
            if (Sdf_ChildrenUtils<Sdf_RelationshipTargetChildPolicy>::CreateSpec(
                    GetLayer(), targetSpecPath, 
                    SdfSpecTypeRelationshipTarget)) {
                relTargetSpec = _GetTargetSpec(targetPath);
            }
        }
        else {
            TF_CODING_ERROR("Create spec <%s>: %s", 
                            targetPath.GetText(),
                            allowed.GetWhyNot().c_str());
        }
    }

    return relTargetSpec;
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
    if (path == oldPath) {
        return newPath;
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

    // Replace the path in the targets list
    bool targetSpecExists = false;
    TF_FOR_ALL(i, siblingPaths) {
	if (*i == oldTargetPath) {
	    *i = newTargetPath;
            targetSpecExists = true;
	    break;
	}
    }
    
    // If there is a target spec, then update the children field.
    if (targetSpecExists) {
        // Set the siblings
        layer->SetField(relPath, SdfChildrenKeys->RelationshipTargetChildren,
            siblingPaths);

        SdfPath oldTargetSpecPath = relPath.AppendTarget(oldTargetPath);
        SdfPath newTargetSpecPath = relPath.AppendTarget(newTargetPath);

        if (layer->HasSpec(newTargetSpecPath)) {
            // Target already exists.  If the target has no attributes
            // then we'll allow the replacement.  If it does have
            // attributes then we must refuse.
            if (!GetAttributesForTargetPath(newTargetPath).empty()) {
                TF_CODING_ERROR("Can't replace target %s with target %s in "
                                "relationship %s: %s",
                                oldPath.GetText(),
                                newPath.GetText(),
                                relPath.GetString().c_str(),
                                "Target already exists");
                return;
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
    }

    // Get the list op.
    SdfPathListOp targetsListOp = 
        layer->GetFieldAs<SdfPathListOp>(relPath, SdfFieldKeys->TargetPaths);

    // Update the list op.
    if (targetsListOp.ModifyOperations(
            std::bind(_ReplacePath, oldTargetPath, newTargetPath,
                      std::placeholders::_1))) {
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
// Relational Attributes
//

SdfRelationalAttributeSpecView
SdfRelationshipSpec::GetAttributesForTargetPath(
    const SdfPath& path) const
{
    SdfPath targetPath = GetPath().AppendTarget(
        _CanonicalizeTargetPath(path));
    return SdfRelationalAttributeSpecView(GetLayer(),
        targetPath, SdfChildrenKeys->PropertyChildren);
}

void
SdfRelationshipSpec::SetAttributesForTargetPath(
    const SdfPath& path,
    const SdfAttributeSpecHandleVector& newAttrs)
{
    // Determine the path of the relationship target
    SdfPath absPath = _CanonicalizeTargetPath(path);
    SdfPath targetPath = GetPath().AppendTarget(absPath);

    // Create the relationship target if it doesn't already exist
    SdfTargetsProxy targets = GetTargetPathList();
    if (!targets.ContainsItemEdit(absPath, true /*onlyAddOrExplicit*/)) {
        targets.Add(absPath);
    }
        
    Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::SetChildren(
        GetLayer(), targetPath, newAttrs);
}

bool
SdfRelationshipSpec::InsertAttributeForTargetPath(
    const SdfPath& path,
    const SdfAttributeSpecHandle& attr,
    int index)
{
    if (!attr) {
        TF_CODING_ERROR("Invalid attribute spec");
        return false;
    }

    SdfChangeBlock block;
        
    // Ensure that the parent relationship target spec object has been 
    // created.
    const SdfPath targetPath = _CanonicalizeTargetPath(path);

    SdfSpecHandle relTargetSpec = _FindOrCreateTargetSpec(targetPath);
    if (!relTargetSpec) {
        TF_CODING_ERROR("Insert relational attribute: Failed to create "
            "target <%s>", targetPath.GetText());
        return false;
    }

    return Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::InsertChild(
        GetLayer(), relTargetSpec->GetPath(), attr, index);
}

void
SdfRelationshipSpec::RemoveAttributeForTargetPath(
    const SdfPath& path,
    const SdfAttributeSpecHandle& attr)
{
    if (!attr) {
        TF_CODING_ERROR("Invalid attribute spec");
        return;
    }

    // Ensure that the given attribute is in fact a relational attribute
    // on the given target path.
    const SdfPath targetSpecPath = 
        GetPath().AppendTarget(_CanonicalizeTargetPath(path));

    if (attr->GetLayer() != GetLayer() ||
        attr->GetPath().GetParentPath() != targetSpecPath) {
        TF_CODING_ERROR("'%s' is not an attribute for target <%s>",
            attr->GetName().c_str(),
            targetSpecPath.GetText());
        return;
    }

    Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::RemoveChild(
        GetLayer(), targetSpecPath, attr->GetNameToken());
}

SdfPathVector
SdfRelationshipSpec::GetAttributeTargetPaths() const
{
    // Construct the path to each RelationshipTargetSpec for this object and
    // check each one for attributes keys.
    SdfPathVector paths;
    
    const std::vector<SdfPath> targets = 
        GetFieldAs<std::vector<SdfPath> >(
            SdfChildrenKeys->RelationshipTargetChildren);
    TF_FOR_ALL(i, targets) {
        const SdfPath targetPath = GetPath().AppendTarget(*i);
        if (GetLayer()->HasField(
                targetPath, SdfChildrenKeys->PropertyChildren)) {
            paths.push_back(*i);
        }
    }

    return paths;
}

SdfPath
SdfRelationshipSpec::GetTargetPathForAttribute(
    const SdfAttributeSpecConstHandle& attr) const
{
    if (!attr) {
        TF_CODING_ERROR("Invalid attribute spec");
        return SdfPath::EmptyPath();
    }

    // Verify that the given attribute is actually a relational attribute
    // spec.
    if (!attr->GetPath().IsRelationalAttributePath()) {
        TF_CODING_ERROR("<%s> is not a relational attribute",
            attr->GetPath().GetText());
        return SdfPath();
    }

    // Verify that this attribute's parent is a relationship target
    // in this layer and that relationship target's parent is this object.
    const SdfSpecHandle relTargetSpec = 
        attr->GetLayer()->GetObjectAtPath(attr->GetPath().GetParentPath());
    if (!relTargetSpec ||
        relTargetSpec->GetLayer() != GetLayer() ||
        relTargetSpec->GetPath().GetParentPath() != GetPath()) {
        TF_CODING_ERROR("<%s> is not an attribute of relationship '<%s>'",
            attr->GetPath().GetText(),
            GetPath().GetText());
        return SdfPath();
    }

    return relTargetSpec->GetPath().GetTargetPath();
}

//
// Relational Attribute Ordering
//

boost::shared_ptr<Sdf_ListEditor<SdfNameTokenKeyPolicy> >
SdfRelationshipSpec::_GetTargetAttributeOrderEditor(const SdfPath& path) const
{
    boost::shared_ptr<Sdf_ListEditor<SdfNameTokenKeyPolicy> > editor;
    SdfSpecHandle relTargetSpec = _GetTargetSpec(path);
    if (relTargetSpec) {
        editor.reset(new Sdf_VectorListEditor<SdfNameTokenKeyPolicy>(
                relTargetSpec, 
                SdfFieldKeys->PropertyOrder, SdfListOpTypeOrdered));
    }
    return editor;
}

void
SdfRelationshipSpec::SetTargetAttributeOrders(
    const AttributeOrderMap& orders)
{
    // Explicitly check permission here to ensure that any editing operation
    // (even no-ops) trigger an error.
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Set target attribute orders: Permission denied");
        return;
    }

    // Replace all target attribute orders on the relationship; clear out
    // all current orderings and add in the orderings from the given dict.
    SdfChangeBlock block;

    const SdfPathVector oldAttrOrderPaths = GetAttributeOrderTargetPaths();
    TF_FOR_ALL(oldPath, oldAttrOrderPaths) {
        GetAttributeOrderForTargetPath(*oldPath).clear();
    }

    TF_FOR_ALL(newOrder, orders) {
        GetOrCreateAttributeOrderForTargetPath(newOrder->first) = 
            newOrder->second;
    }
}

bool 
SdfRelationshipSpec::HasAttributeOrderForTargetPath(const SdfPath& path) const
{
    const SdfPath targetSpecPath = _MakeCompleteTargetSpecPath(path);
    const std::vector<TfToken> ordering = 
        GetLayer()->GetFieldAs<std::vector<TfToken> >(
            targetSpecPath, SdfFieldKeys->PropertyOrder);
    return !ordering.empty();
}

SdfNameOrderProxy 
SdfRelationshipSpec::GetAttributeOrderForTargetPath(const SdfPath& path) const
{
    if (!HasAttributeOrderForTargetPath(path)) {
        return SdfNameOrderProxy(SdfListOpTypeOrdered);
    }

    return SdfNameOrderProxy(_GetTargetAttributeOrderEditor(path),
                            SdfListOpTypeOrdered);
}

SdfNameOrderProxy 
SdfRelationshipSpec::GetOrCreateAttributeOrderForTargetPath(
    const SdfPath& path)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot create attribute order for target path <%s> in "
                        "relationship <%s>: Permission denied.", 
                        path.GetText(), GetPath().GetText());
        return SdfNameOrderProxy(SdfListOpTypeOrdered);
    }

    const bool specRetrievedOrCreated = (bool)_FindOrCreateTargetSpec(path);

    if (!specRetrievedOrCreated) {
        TF_CODING_ERROR("Can't create attribute ordering for target path "
                        "<%s> in relationship <%s>: Couldn't create target.",
                        path.GetText(), GetPath().GetText());
        return SdfNameOrderProxy(SdfListOpTypeOrdered);
    }

    return SdfNameOrderProxy(_GetTargetAttributeOrderEditor(path),
                            SdfListOpTypeOrdered);
}

SdfPathVector
SdfRelationshipSpec::GetAttributeOrderTargetPaths() const
{
    SdfPathVector paths;

    const std::vector<SdfPath> targets = 
        GetFieldAs<std::vector<SdfPath> >(
            SdfChildrenKeys->RelationshipTargetChildren);
    TF_FOR_ALL(i, targets) {
        if (HasAttributeOrderForTargetPath(*i)) {
            paths.push_back(*i);
        }
    }

    return paths;
}

void
SdfRelationshipSpec::ApplyAttributeOrderForTargetPath(
    const SdfPath& path,
    std::vector<TfToken>* vec) const
{
    boost::shared_ptr<Sdf_ListEditor<SdfNameTokenKeyPolicy> > editor = 
        _GetTargetAttributeOrderEditor(path);
    if (editor) {
        editor->ApplyEdits(vec);
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
