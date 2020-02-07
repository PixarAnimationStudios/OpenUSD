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
#ifndef PXR_USD_SDF_CHILDREN_UTILS_H
#define PXR_USD_SDF_CHILDREN_UTILS_H

/// \file sdf/childrenUtils.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/allowed.h"
#include "pxr/usd/sdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class Sdf_ChildrenUtils
///
/// Helper functions for creating and manipulating the children
/// of a spec. A ChildPolicy must be provided that specifies which type
/// of children to edit. (See childrenPolicies.h for details).
///
template<class ChildPolicy>
class Sdf_ChildrenUtils
{
public:
    /// The type of the key that identifies a child. This is usually
    /// a std::string or an SdfPath.
    typedef typename ChildPolicy::KeyType KeyType;

    /// The type of the child identifier as it's stored in the layer's data.
    /// This is usually a TfToken.
    typedef typename ChildPolicy::FieldType FieldType;

    /// Create a new spec in \a layer at \childPath and add it to its parent's
    /// field named \childrenKey. Emit an error and return false if the new spec
    /// couldn't be created.
    static bool CreateSpec(
        const SdfLayerHandle &layer,
        const SdfPath &childPath,
        SdfSpecType specType,
        bool inert=true) {
        return CreateSpec(get_pointer(layer), childPath, specType, inert);
    }

    // This overload is intended primarily for internal use.
    SDF_API
    static bool CreateSpec(
        SdfLayer *layer,
        const SdfPath &childPath,
        SdfSpecType specType,
        bool inert=true);

    /// \name Rename API
    /// @{

    /// Return whether \a newName is a valid name for a child.
    SDF_API
    static bool IsValidName(const FieldType &newName);

    /// Return whether \a newName is a valid name for a child.
    SDF_API
    static bool IsValidName(const std::string &newName);

    /// Return whether \a spec can be renamed to \a newName.
    static SdfAllowed CanRename(
        const SdfSpec &spec,
        const FieldType &newName);

    /// Rename \a spec to \a newName. If \a fixPrimListEdits is true,
    /// then also fix up the name children order. It's an error for
    /// \a fixPrimListEdits to be true if spec is not a PrimSpec.
    SDF_API
    static bool Rename(
        const SdfSpec &spec,
        const FieldType &newName);

    /// @}

    /// \name Children List API
    /// @{

    /// Replace the children of the spec at \a path with the specs in \a
    /// values. This will delete existing children that aren't in \a values and
    /// reparent children from other locations in the layer.
    SDF_API
    static bool SetChildren(
        const SdfLayerHandle &layer,
        const SdfPath &path,
        const std::vector<typename ChildPolicy::ValueType> &values);

    /// Insert \a value as a child of \a path at the specified index.
    SDF_API
    static bool InsertChild(
        const SdfLayerHandle &layer,
        const SdfPath &path,
        const typename ChildPolicy::ValueType& value,
        int index);

    /// Remove the child identified by \a key.
    SDF_API
    static bool RemoveChild(
        const SdfLayerHandle &layer,
        const SdfPath &path,
        const typename ChildPolicy::KeyType& key);

    /// @}
    /// \name Batch editing API
    /// @{


    /// Insert \a value as a child of \a path at the specified index with
    /// the new name \p newName.
    SDF_API
    static bool MoveChildForBatchNamespaceEdit(
        const SdfLayerHandle &layer,
        const SdfPath &path,
        const typename ChildPolicy::ValueType& value,
        const typename ChildPolicy::FieldType& newName,
        int index);

    /// Remove the child identified by \a key.
    SDF_API
    static bool RemoveChildForBatchNamespaceEdit(
        const SdfLayerHandle &layer,
        const SdfPath &path,
        const typename ChildPolicy::KeyType& key)
    {
        return RemoveChild(layer, path, key);
    }

    /// Returns \c true if \p value can be inserted as a child of \p path
    /// with the new name \p newName at the index \p index, otherwise
    /// returns \c false and sets \p whyNot.
    SDF_API
    static bool CanMoveChildForBatchNamespaceEdit(
        const SdfLayerHandle &layer,
        const SdfPath &path,
        const typename ChildPolicy::ValueType& value,
        const typename ChildPolicy::FieldType& newName,
        int index,
        std::string* whyNot);

    /// Returns \c true if the child of \p path identified by \p key can
    /// be removed, otherwise returns \c false and sets \p whyNot.
    SDF_API
    static bool CanRemoveChildForBatchNamespaceEdit(
        const SdfLayerHandle &layer,
        const SdfPath &path,
        const typename ChildPolicy::FieldType& key,
        std::string* whyNot);

    /// @}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_CHILDREN_UTILS_H
