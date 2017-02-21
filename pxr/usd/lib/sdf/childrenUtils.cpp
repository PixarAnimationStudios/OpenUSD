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
/// \file ChildrenUtils.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/childrenPolicies.h"
#include "pxr/usd/sdf/childrenUtils.h"
#include "pxr/usd/sdf/cleanupTracker.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/mapperSpec.h"
#include "pxr/usd/sdf/mapperArgSpec.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/ostreamMethods.h"

PXR_NAMESPACE_OPEN_SCOPE

//
// ChildrenUtils
//

template <class ChildPolicy>
struct Sdf_IsValidPathComponent {
    bool operator()(const SdfPath &newComponent) { return true; }
    bool operator()(const TfToken &newComponent) {
        return ChildPolicy::IsValidIdentifier(newComponent);
    }
    template <class T>
    bool operator()(const T &newComponent) { return false; }
};

// Helper function which returns \a path with \a newName appended.
template <class ChildPolicy>
static SdfPath
_ComputeMovedPath(const SdfPath &path,
    const typename ChildPolicy::FieldType &newName)
{
    if (!Sdf_IsValidPathComponent<ChildPolicy>()(newName)) {
        return SdfPath();
    }
    
    return ChildPolicy::GetChildPath(path, newName);
}

// Helper function which returns \a path with the name changed to \a newName
template <class ChildPolicy>
static SdfPath
_ComputeRenamedPath(const SdfPath &path,
    const typename ChildPolicy::FieldType &newName)
{
    return ChildPolicy::GetChildPath(ChildPolicy::GetParentPath(path), newName);
}

template <class ChildPolicy>
bool Sdf_ChildrenUtils<ChildPolicy>::CreateSpec(
    const SdfLayerHandle &layer,
    const SdfPath &childPath,
    SdfSpecType specType,
    bool inert)
{
    // Create the spec in the layer. Note that this will fail if a spec already
    // exists at the given path.
    if (specType == SdfSpecTypeUnknown) {
        TF_CODING_ERROR("Invalid object type");
        return false;
    }

    // Use a change block to ensure all layer data manipulations below are
    // treated atomically.
    SdfChangeBlock block;

    if (!layer->_CreateSpec(childPath, specType, inert)) {
        TF_CODING_ERROR("Failed to create spec of type \'%s\' at <%s>",
                        TfEnum::GetName(specType).c_str(),
                        childPath.GetText());
        return false;
    }

    // Add this spec to the parent's list of children. Since _CreateSpec above
    // will fail if a duplicate spec exists, we can simply append the new child
    // to the list.
    const SdfPath parentPath = ChildPolicy::GetParentPath(childPath);
    const TfToken childrenKey = ChildPolicy::GetChildrenToken(parentPath);
    const FieldType childName = ChildPolicy::GetFieldValue(childPath);

    layer->_PrimPushChild(parentPath, childrenKey, childName);

    return true;  
}

template <class ChildPolicy>
static void 
_FilterDuplicatePreexistingChildren(
    std::vector<typename ChildPolicy::ValueType>* filtered,
    const SdfPath& parentPath,
    const std::vector<typename ChildPolicy::ValueType>& original)
{
    typedef typename ChildPolicy::FieldType FieldType;
    std::set<FieldType> keySet;

    TF_FOR_ALL(it, original) {
        if (*it) {
            const FieldType key(ChildPolicy::GetKey(*it));
            if (!keySet.insert(key).second) {
                // Key already exists; filter it out if the value it 
                // corresponds to is already a child of the given parent path.
                if ((*it)->GetPath().GetParentPath() == parentPath) {
                    continue;
                }
            }
        }

        filtered->push_back(*it);
    }
}

template<class ChildPolicy>
bool Sdf_ChildrenUtils<ChildPolicy>::SetChildren(
    const SdfLayerHandle &layer,
    const SdfPath &path,
    const std::vector<typename ChildPolicy::ValueType> &origValues)
{
    typedef typename ChildPolicy::FieldType FieldType;
    const TfToken childrenKey = ChildPolicy::GetChildrenToken(path);

    // XXX:
    // This is a hack to preserve some pre-existing behavior. Essentially,
    // operations involving values that are already children of the given
    // path are treated as no-ops; they never generate any errors. 
    //
    // One tricky case is when the given vector contains duplicates of the same
    // child, which is tested explicitly in testSdfPrim. We need to ignore
    // these duplicates; however, much of the code below relies on no duplicates
    // existing in the values being set. The simplest way to deal with this is
    // to just filter these duplicates out here.
    //
    // This behavior -- "duplicate values are OK if they're already child of
    // "the given path" -- is questionable and is just maintained for 
    // compatibility with the Gd-based implementation of Sdf. We could possibly
    // remove it in the future.
    std::vector<typename ChildPolicy::ValueType> values;
    _FilterDuplicatePreexistingChildren<ChildPolicy>(&values, path, origValues);

    std::vector<FieldType> childNames =
        layer->GetFieldAs<std::vector<FieldType> >(path, childrenKey);

    std::set<FieldType> newNamesSet;
    std::vector<FieldType> newNames;
    newNames.reserve(values.size());

    // Build up the new vector of names and check for duplicates or
    // other error conditions.
    TF_FOR_ALL(i, values) {
        if (!*i) {
            TF_CODING_ERROR("Invalid child");
            return false;
        }

        const FieldType key(ChildPolicy::GetKey(*i));
        newNames.push_back(key);
        if (!newNamesSet.insert(key).second) {
            TF_CODING_ERROR("Duplicate child");
            return false;
        }

        if ((*i)->GetLayer() != layer) {
            TF_CODING_ERROR("Cannot reparent to another layer");
            return false;
        }

        // Attempting to insert a value that is a parent of the desired spec path
        // (e.g., attempting to insert /A/B into /A/B/C's children) is an error.
        // However, if this value is already a child of the given path, that's
        // a no-op, not an error.
        if (path != (*i)->GetPath().GetParentPath() &&
            path.HasPrefix((*i)->GetPath())) {
            TF_CODING_ERROR("Cannot reparent child under itself");
            return false;
        }
    }

    // Use a change block to ensure all layer data manipulations below are
    // treated atomically.
    SdfChangeBlock block;

    // Delete Specs that aren't in the new set.
    TF_FOR_ALL(i, childNames) {
        if (newNamesSet.find(*i) == newNamesSet.end()) {
            SdfPath childPath = ChildPolicy::GetChildPath(path, *i);
            layer->_DeleteSpec(childPath);
        }
    }

    // Create a set that contains all of the old names.
    std::set<FieldType> oldKeys(childNames.begin(), childNames.end());

    // Perform the edits
    TF_FOR_ALL(i, values) {
        // Get the key and construct the new path
        const FieldType key(ChildPolicy::GetKey(*i));
        const SdfPath newPath = ChildPolicy::GetChildPath(path, key);

        // If this is already a child, then there's nothing to do.
        const SdfPath oldParentPath = ChildPolicy::GetParentPath((*i)->GetPath());
        if (oldParentPath == path) {
            continue;
        }

        // If there was previously a spec at that path then first delete it.
        if (oldKeys.find(key) != oldKeys.end()) {
            layer->_DeleteSpec(newPath);
        }

        // Move the spec to the new path. We know this spec is currently
        // parented to something else, thanks to the check above, so make sure
        // we remove this spec from its old parent.
        const TfToken oldChildrenKey = 
            ChildPolicy::GetChildrenToken(oldParentPath);

        std::vector<FieldType> oldSiblings =
            layer->GetFieldAs<std::vector<FieldType> >(
                oldParentPath, oldChildrenKey);

        typename std::vector<FieldType>::iterator oldNameIter = 
            std::find(oldSiblings.begin(), oldSiblings.end(), key);
        if (oldNameIter == oldSiblings.end()) {
            TF_CODING_ERROR("An object was not in its parent's list of " 
                            "children");
            return false;
        }

        oldSiblings.erase(oldNameIter);
        if (oldSiblings.empty()) {
            layer->EraseField(oldParentPath, oldChildrenKey);
        }
        else {
            layer->SetField(oldParentPath, oldChildrenKey, oldSiblings);
        }

        layer->_MoveSpec((*i)->GetPath(), newPath);
    }
            
    // Store the new vector of keys and update this object's internal state.
    if (newNames.empty()) {
        layer->EraseField(path, childrenKey);
    } else {
        layer->SetField(path, childrenKey, newNames);
    }

    return true;
}

template<class ChildPolicy>
bool Sdf_ChildrenUtils<ChildPolicy>::InsertChild(
    const SdfLayerHandle &layer,
    const SdfPath &path,
    const typename ChildPolicy::ValueType& value,
    size_t index)
{
    typedef typename ChildPolicy::FieldType FieldType;
    const TfToken childrenKey = ChildPolicy::GetChildrenToken(path);

    if (!value) {
        TF_CODING_ERROR("Invalid child");
        return false;
    }

    if (value->GetLayer() != layer) {
        TF_CODING_ERROR("Cannot reparent to another layer");
        return false;
    }

    // Attempting to insert a value that is already a child of the spec at
    // path is considered a no-op, even if the index is different.
    if (ChildPolicy::GetParentPath(value->GetPath()) == path) {
        return true;
    }

    // Determine the key and new child path.
    FieldType key(ChildPolicy::GetKey(value));
    SdfPath newPath = ChildPolicy::GetChildPath(path, key);

    // Attempting to insert a value that is a parent of the desired spec path
    // (e.g., attempting to insert /A/B into /A/B/C's children) is an error.
    if (newPath.HasPrefix(value->GetPath())) {
        TF_CODING_ERROR("Cannot reparent child under itself");
        return false;
    }

    std::vector<FieldType> childNames =
        layer->GetFieldAs<std::vector<FieldType> >(path, childrenKey);

    // If the index is -1, insert the child at the end.
    if (index == -1) {
        index = childNames.size();
    }

    if (index > childNames.size()) {
        TF_CODING_ERROR("Attempt to insert spec %s at an invalid index %zd",
            newPath.GetText(), index);
        return false;
    }

    // Check to make sure there's not already a spec with the new key
    TF_FOR_ALL(i, childNames) {
        if (*i == key) {
            TF_CODING_ERROR("Attempt to insert duplicate spec %s",
                newPath.GetText());
            return false;
        }
    }
        
    // Get the path of the parent that value is currently a child of
    SdfPath oldParentPath = ChildPolicy::GetParentPath(value->GetPath());

    // Find the child in the old parent's list of children
    const TfToken oldChildrenKey = ChildPolicy::GetChildrenToken(oldParentPath);

    std::vector<FieldType> oldSiblingNames =
        layer->GetFieldAs<std::vector<FieldType> >(oldParentPath, oldChildrenKey);

    typename std::vector<FieldType>::iterator oldNameIter = std::find(
        oldSiblingNames.begin(), oldSiblingNames.end(), key);

    if (oldNameIter == oldSiblingNames.end()) {
        TF_CODING_ERROR("An object was not in its parent's list of children");
        return false;
    }

    // Use a change block to ensure all layer data manipulations below are
    // treated atomically.
    SdfChangeBlock block;

    // Remove the prim from the old parent's list
    oldSiblingNames.erase(oldNameIter);
    if (oldSiblingNames.empty()) {
        layer->EraseField(oldParentPath, oldChildrenKey);
    } else {
        layer->SetField(oldParentPath, oldChildrenKey, oldSiblingNames);
    }

    // Move the actual spec data
    layer->_MoveSpec(value->GetPath(), newPath);

    // Update and set the _childNames vector.
    childNames.insert(childNames.begin()+index, key);
    layer->SetField(path, childrenKey, childNames);

    // Notify the CleanupTracker that a spec was removed from the old parent 
    // path so the old parent can be cleaned up if it is left inert
    if (SdfSpecHandle spec = layer->GetObjectAtPath(oldParentPath)) {
        Sdf_CleanupTracker::GetInstance().AddSpecIfTracking(spec);
    }

        
    return true;
}

template <class ChildPolicy>
bool Sdf_ChildrenUtils<ChildPolicy>::RemoveChild(
    const SdfLayerHandle &layer,
    const SdfPath &path,
    const typename ChildPolicy::KeyType& key)
{
    typedef typename ChildPolicy::FieldType FieldType;
    const TfToken childrenKey = ChildPolicy::GetChildrenToken(path);

    std::vector<FieldType> childNames =
        layer->GetFieldAs<std::vector<FieldType> >(path, childrenKey);

    // Use a change block to ensure all layer data manipulations below are
    // treated atomically.
    SdfChangeBlock block;

    FieldType fieldKey(key);
    for (auto i = childNames.begin(), e = childNames.end(); i != e; ++i) {
        if (*i == fieldKey) {
            SdfPath childPath = ChildPolicy::GetChildPath(path, fieldKey);
            layer->_DeleteSpec(childPath);
            childNames.erase(i);
            if (childNames.empty()) {
                layer->EraseField(path, childrenKey);
            } else {
                layer->SetField(path, childrenKey, childNames);
            }

            // Notify the CleanupTracker that a child spec was removed so that 
            // this spec can be cleaned up if it is left inert
            if (SdfSpecHandle spec = layer->GetObjectAtPath(path)) {
                Sdf_CleanupTracker::GetInstance().AddSpecIfTracking(spec);
            }

            return true;
        }
    }
    return false;
}

template <class ChildPolicy>
bool
Sdf_ChildrenUtils<ChildPolicy>::MoveChildForBatchNamespaceEdit(
    const SdfLayerHandle &layer,
    const SdfPath &path,
    const typename ChildPolicy::ValueType& value,
    const typename ChildPolicy::FieldType& newName,
    size_t index)
{
    typedef typename ChildPolicy::FieldType FieldType;
    const TfToken childrenKey = ChildPolicy::GetChildrenToken(path);

    // Get the new path.
    SdfPath newPath = _ComputeMovedPath<ChildPolicy>(path, newName);

    // Just return if nothing is changing.
    if (newPath == value->GetPath() && index == SdfNamespaceEdit::Same) {
        return true;
    }

    // Get the new sibling names.
    std::vector<FieldType> childNames =
        layer->GetFieldAs<std::vector<FieldType> >(path, childrenKey);

    // Fix up the index.
    FieldType oldKey((ChildPolicy::GetKey(value)));
    SdfPath oldParentPath = ChildPolicy::GetParentPath(value->GetPath());
    if (index == SdfNamespaceEdit::Same && oldParentPath == path) {
        index = std::find(childNames.begin(), childNames.end(), oldKey) -
                childNames.begin();
    }
    else if (index > childNames.size()) {
        // This catches all negative indexes.
        index = childNames.size();
    }

    // Get the old sibling names and find the value.
    const TfToken oldChildrenKey = ChildPolicy::GetChildrenToken(oldParentPath);
    std::vector<FieldType> oldSiblingNames =
        layer->GetFieldAs<std::vector<FieldType> >(oldParentPath, oldChildrenKey);
    typename std::vector<FieldType>::iterator oldNameIter =
        std::find(oldSiblingNames.begin(), oldSiblingNames.end(), oldKey);

    // Use a change block to ensure all layer data manipulations below are
    // treated atomically.
    SdfChangeBlock block;

    // Remove the prim from the old parent's child name list or, if we're
    // reordering, from the prim's child name list.
    if (oldParentPath == path) {
        // If the name isn't changing then we can bail early if the
        // child isn't going to move.
        if (oldKey == newName) {
            int oldIndex = oldNameIter - oldSiblingNames.begin();
            if (oldIndex == index || oldIndex + 1 == index) {
                return true;
            }
        }

        typedef typename std::vector<FieldType>::difference_type Diff;
        if (oldNameIter - oldSiblingNames.begin() < static_cast<Diff>(index)) {
            // Index must be shifted down because we're removing an
            // earlier name.
            --index;
        }

        // Erase the old name.
        childNames.erase(
            std::find(childNames.begin(), childNames.end(), oldKey));
    }
    else {
        oldSiblingNames.erase(oldNameIter);
        if (oldSiblingNames.empty()) {
            layer->EraseField(oldParentPath, oldChildrenKey);

            // Notify the CleanupTracker that a spec was removed from the old
            // parent path so the old parent can be cleaned up if it is left
            // inert.
            if (SdfSpecHandle spec = layer->GetObjectAtPath(oldParentPath)) {
                Sdf_CleanupTracker::GetInstance().AddSpecIfTracking(spec);
            }
        }
        else {
            layer->SetField(oldParentPath, oldChildrenKey, oldSiblingNames);
        }
    }

    // Move the actual spec data
    layer->_MoveSpec(value->GetPath(), newPath);

    // Update and set the _childNames vector.
    childNames.insert(childNames.begin() + index, newName);
    layer->SetField(path, childrenKey, childNames);

    return true;
}

template <class ChildPolicy>
bool
Sdf_ChildrenUtils<ChildPolicy>::CanMoveChildForBatchNamespaceEdit(
    const SdfLayerHandle &layer,
    const SdfPath &path,
    const typename ChildPolicy::ValueType& value,
    const typename ChildPolicy::FieldType& newName,
    size_t index,
    std::string* whyNot)
{
    typedef typename ChildPolicy::FieldType FieldType;
    const TfToken childrenKey = ChildPolicy::GetChildrenToken(path);

    if (!layer->PermissionToEdit()) {
        if (whyNot) {
            *whyNot = "Layer is not editable";
        }
        return false;
    }

    if (!value) {
        if (whyNot) {
            *whyNot = "Object does not exist";
        }
        return false;
    }
    if (value->GetLayer() != layer) {
        if (whyNot) {
            *whyNot = "Cannot reparent to another layer";
        }
        return false;
    }

    SdfPath newPath = _ComputeMovedPath<ChildPolicy>(path, newName);
    if (newPath.IsEmpty()) {
        if (whyNot) {
            *whyNot = "Invalid name";
        }
        return false;
    }
    /* We specifically don't check this for batch namespace edits.
    if (layer->GetObjectAtPath(newPath)) {
        if (whyNot) {
            *whyNot = "Object already exists";
        }
        return false;
    }
    */

    // Renaming to the same name or reordering will work.
    if (ChildPolicy::GetParentPath(value->GetPath()) == path) {
        return true;
    }

    // Attempting to insert a value that is a parent of the desired spec path
    // (e.g., attempting to insert /A/B into /A/B/C's children) is an error.
    if (newPath.HasPrefix(value->GetPath())) {
        if (whyNot) {
            *whyNot = "Cannot reparent object under itself";
        }
        return false;
    }

    std::vector<FieldType> childNames =
        layer->GetFieldAs<std::vector<FieldType> >(path, childrenKey);

    // If the index is AtEnd, insert the child at the end.
    if (index == SdfNamespaceEdit::AtEnd) {
        index = childNames.size();
    }

    // Any index not in the child name range other than Same is invalid.
    if (index != SdfNamespaceEdit::Same && index > childNames.size()) {
        if (whyNot) {
            *whyNot = "Invalid index";
        }
        return false;
    }

    // Check the invariant that a parent has its children.
    FieldType oldKey((ChildPolicy::GetKey(value)));
    SdfPath oldParentPath = ChildPolicy::GetParentPath(value->GetPath());
    const TfToken oldChildrenKey = ChildPolicy::GetChildrenToken(oldParentPath);
    std::vector<FieldType> oldSiblingNames =
        layer->GetFieldAs<std::vector<FieldType> >(oldParentPath,
                                                   oldChildrenKey);
    typename std::vector<FieldType>::iterator oldNameIter =
        std::find(oldSiblingNames.begin(), oldSiblingNames.end(), oldKey);
    if (oldNameIter == oldSiblingNames.end()) {
        if (whyNot) {
            *whyNot = "Coding error: Object is not in its parent's children";
        }
        return false;
    }

    return true;
}

template <class ChildPolicy>
bool
Sdf_ChildrenUtils<ChildPolicy>::CanRemoveChildForBatchNamespaceEdit(
    const SdfLayerHandle &layer,
    const SdfPath &path,
    const typename ChildPolicy::FieldType& key,
    std::string* whyNot)
{
    typedef typename ChildPolicy::FieldType FieldType;
    const TfToken childrenKey = ChildPolicy::GetChildrenToken(path);

    if (!layer->PermissionToEdit()) {
        if (whyNot) {
            *whyNot = "Layer is not editable";
        }
        return false;
    }

    std::vector<FieldType> childNames =
        layer->GetFieldAs<std::vector<FieldType> >(path, childrenKey);
    typename std::vector<FieldType>::iterator i =
        std::find(childNames.begin(), childNames.end(), key);
    if (i == childNames.end()) {
        if (whyNot) {
            *whyNot = "Object does not exist";
        }
        return false;
    }

    return true;
}

template <class ChildPolicy>
bool
Sdf_ChildrenUtils<ChildPolicy>::IsValidName(const FieldType &newName)
{
    return ChildPolicy::IsValidIdentifier(newName);
}

template <class ChildPolicy>
bool
Sdf_ChildrenUtils<ChildPolicy>::IsValidName(const std::string &newName)
{
    return ChildPolicy::IsValidIdentifier(newName);
}

template <class ChildPolicy>
SdfAllowed Sdf_ChildrenUtils<ChildPolicy>::CanRename(
    const SdfSpec &spec,
    const typename ChildPolicy::FieldType &newName)
{
    if (!spec.GetLayer()->PermissionToEdit()) {
        return "Layer is not editable";
    }
    if (!IsValidName(newName)) {
        return TfStringPrintf("Cannot rename %s to invalid name '%s'",
                              spec.GetPath().GetText(), newName.GetText());
    }

    SdfPath newPath = _ComputeRenamedPath<ChildPolicy>(spec.GetPath(), newName);
    if (newPath == spec.GetPath()) {
        // Allow renaming to the same name.
        return true;
    }
    if (newPath.IsEmpty() || spec.GetLayer()->GetObjectAtPath(newPath)) {
        return SdfAllowed("An object with that name already exists");
    }
    return true;
}

template <class ChildPolicy>
bool Sdf_ChildrenUtils<ChildPolicy>::Rename(
    const SdfSpec &spec,
    const typename ChildPolicy::FieldType &newName)
{
    SdfPath oldPath = spec.GetPath();

    if (!IsValidName(newName)) {
        TF_CODING_ERROR("Cannot rename %s to invalid name '%s'",
            oldPath.GetText(), newName.GetText());
        return false;
    }

    SdfPath newPath = _ComputeRenamedPath<ChildPolicy>(oldPath, newName);
    if (newPath.IsEmpty()) {
        return false;
    }

    if (newPath == spec.GetPath()) {
        // Attempting to rename to the same name is considered a no-op.
        return true;
    }

    SdfLayerHandle layer = spec.GetLayer();

    // Determine the key for the children vector.
    const SdfPath parentPath = ChildPolicy::GetParentPath(oldPath);
    const TfToken childrenKey = ChildPolicy::GetChildrenToken(parentPath);

    std::vector<FieldType> childNames = 
        layer->GetFieldAs<std::vector<FieldType> >(parentPath, childrenKey);

    TF_FOR_ALL(i, childNames) {
        if (*i == newName) {
            TF_CODING_ERROR("Cannot rename %s to %s because a sibling "
                "with that name already exists",
                oldPath.GetText(), newPath.GetText());
            return false;
        }
    }

    // Use a change block to ensure all layer data manipulations below are
    // treated atomically.
    SdfChangeBlock block;

    // First move the spec and all the fields under it.
    if (!layer->_MoveSpec(oldPath, newPath)) {
        return false;
    }
        
    // Now update the parent's children list
    TF_FOR_ALL(i, childNames) {
        if (*i == oldPath.GetNameToken()) {
            *i = TfToken(newName);
            break;
        }
    }

    layer->SetField(parentPath, childrenKey, childNames);

    return true;
}

/// We provide specializations of Rename and CanRename for mappers since
/// mappers use a path as a key and that makes them not renamable in the way
/// that other specs are.

template <>
SdfAllowed Sdf_ChildrenUtils<Sdf_MapperChildPolicy>::CanRename(
    const SdfSpec &spec,
    const SdfPath &newName)
{
    TF_CODING_ERROR("Cannot rename mappers");
    return SdfAllowed("Cannot rename mappers");
}

template <>
bool Sdf_ChildrenUtils<Sdf_MapperChildPolicy>::Rename(
    const SdfSpec &spec,
    const SdfPath &newName)
{
    TF_CODING_ERROR("Cannot rename mappers");
    return false;
}

template <>
SdfAllowed Sdf_ChildrenUtils<Sdf_AttributeConnectionChildPolicy>::CanRename(
    const SdfSpec &spec,
    const SdfPath &newName)
{
    TF_CODING_ERROR("Cannot rename attribute connections");
    return SdfAllowed("Cannot rename attribute connections");
}

template <>
bool Sdf_ChildrenUtils<Sdf_AttributeConnectionChildPolicy>::Rename(
    const SdfSpec &spec,
    const SdfPath &newName)
{
    TF_CODING_ERROR("Cannot rename attribute connections");
    return false;
}

template <>
SdfAllowed Sdf_ChildrenUtils<Sdf_RelationshipTargetChildPolicy>::CanRename(
    const SdfSpec &spec,
    const SdfPath &newName)
{
    TF_CODING_ERROR("Cannot rename relationship targets");
    return SdfAllowed("Cannot rename relationship targets");
}

template <>
bool Sdf_ChildrenUtils<Sdf_RelationshipTargetChildPolicy>::Rename(
    const SdfSpec &spec,
    const SdfPath &newName)
{
    TF_CODING_ERROR("Cannot rename relationship targets");
    return false;
}

// Explicitly instantiate Sdf_ChildrenUtils for each our children policy types.
template class Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>;
template class Sdf_ChildrenUtils<Sdf_MapperChildPolicy>;
template class Sdf_ChildrenUtils<Sdf_MapperArgChildPolicy>;
template class Sdf_ChildrenUtils<Sdf_ExpressionChildPolicy>;
template class Sdf_ChildrenUtils<Sdf_PrimChildPolicy>;
template class Sdf_ChildrenUtils<Sdf_PropertyChildPolicy>;
template class Sdf_ChildrenUtils<Sdf_RelationshipChildPolicy>;
template class Sdf_ChildrenUtils<Sdf_VariantChildPolicy>;
template class Sdf_ChildrenUtils<Sdf_VariantSetChildPolicy>;
template class Sdf_ChildrenUtils<Sdf_RelationshipTargetChildPolicy>;
template class Sdf_ChildrenUtils<Sdf_AttributeConnectionChildPolicy>;

PXR_NAMESPACE_CLOSE_SCOPE
