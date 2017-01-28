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
#include "pxr/usd/sdf/connectionListEditor.h"
#include "pxr/usd/sdf/childrenUtils.h"
#include "pxr/usd/sdf/layer.h"

#include <set>

PXR_NAMESPACE_OPEN_SCOPE

template <class ChildPolicy>
Sdf_ConnectionListEditor<ChildPolicy>::Sdf_ConnectionListEditor(
    const SdfSpecHandle& connectionOwner,
    const TfToken& connectionListField,
    const SdfPathKeyPolicy& typePolicy)
    : Parent(connectionOwner, connectionListField, typePolicy)
{
}

template <class ChildPolicy>
void 
Sdf_ConnectionListEditor<ChildPolicy>::_OnEditShared(
    SdfListOpType op,
    SdfSpecType specType,
    const std::vector<SdfPath>& oldItems, 
    const std::vector<SdfPath>& newItems) const
{
    if (op != SdfListOpTypeAdded && op != SdfListOpTypeExplicit) {
        return;
    }

    const SdfPath propertyPath = GetPath();
    SdfLayerHandle layer = GetLayer();
    const std::set<value_type> oldItemSet(oldItems.begin(), oldItems.end());
    const std::set<value_type> newItemSet(newItems.begin(), newItems.end());

    // Need to remove all children in oldItems that are not in newItems.
    std::vector<SdfPath> childrenToRemove;
    std::set_difference(oldItemSet.begin(), oldItemSet.end(),
                        newItemSet.begin(), newItemSet.end(), 
                        std::back_inserter(childrenToRemove));
    TF_FOR_ALL(child, childrenToRemove) {
        if (!Sdf_ChildrenUtils<ChildPolicy>::RemoveChild(
                layer, propertyPath, *child)) {

            const SdfPath specPath = 
                ChildPolicy::GetChildPath(propertyPath, *child);
            TF_CODING_ERROR("Failed to remove spec at <%s>", specPath.GetText());
        }
    }

    // Need to add all children in newItems that are not in oldItems.
    std::vector<SdfPath> childrenToAdd;
    std::set_difference(newItemSet.begin(), newItemSet.end(), 
                        oldItemSet.begin(), oldItemSet.end(),
                        std::back_inserter(childrenToAdd));
    TF_FOR_ALL(child, childrenToAdd) {
        const SdfPath specPath = ChildPolicy::GetChildPath(propertyPath, *child);
        if (layer->GetObjectAtPath(specPath)) {
            continue;
        }

        if (!Sdf_ChildrenUtils<ChildPolicy>::CreateSpec(layer, specPath,
                specType)) {
            TF_CODING_ERROR("Failed to create spec at <%s>", specPath.GetText());
        }
    }
}

template <class ChildPolicy>
Sdf_ConnectionListEditor<ChildPolicy>::~Sdf_ConnectionListEditor() = default;

////////////////////////////////////////
// Sdf_AttributeConnectionListEditor
////////////////////////////////////////

Sdf_AttributeConnectionListEditor::Sdf_AttributeConnectionListEditor(
    const SdfSpecHandle& owner,
    const SdfPathKeyPolicy& typePolicy)
    : Parent(owner, SdfFieldKeys->ConnectionPaths, typePolicy)
{
}

Sdf_AttributeConnectionListEditor::~Sdf_AttributeConnectionListEditor() = default;

void 
Sdf_AttributeConnectionListEditor::_OnEdit(
    SdfListOpType op,
    const std::vector<SdfPath>& oldItems, 
    const std::vector<SdfPath>& newItems) const
{
    return Sdf_ConnectionListEditor<Sdf_AttributeConnectionChildPolicy>::_OnEditShared(
        op, SdfSpecTypeConnection, oldItems, newItems);
}

////////////////////////////////////////
// Sdf_RelationshipTargetListEditor
////////////////////////////////////////

Sdf_RelationshipTargetListEditor::Sdf_RelationshipTargetListEditor(
    const SdfSpecHandle& owner,
    const SdfPathKeyPolicy& typePolicy)
    : Parent(owner, SdfFieldKeys->TargetPaths, typePolicy)
{
}

Sdf_RelationshipTargetListEditor::~Sdf_RelationshipTargetListEditor() = default;

void 
Sdf_RelationshipTargetListEditor::_OnEdit(
    SdfListOpType op,
    const std::vector<SdfPath>& oldItems, 
    const std::vector<SdfPath>& newItems) const
{
    return Sdf_ConnectionListEditor<Sdf_RelationshipTargetChildPolicy>::_OnEditShared(
        op, SdfSpecTypeRelationshipTarget, oldItems, newItems);
}

PXR_NAMESPACE_CLOSE_SCOPE
