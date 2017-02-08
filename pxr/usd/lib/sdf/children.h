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
#ifndef SDF_CHILDREN_H
#define SDF_CHILDREN_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"

#include <boost/iterator/iterator_facade.hpp>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);
class SdfSpec;

/// \class Sdf_Children
///
/// Sdf_Children generically represents the children of an object. For
/// instance, it might be used for the name children of a prim or the
/// relationship targets of a relationship.
///
/// The provided ChildPolicy class determines how this object behaves.
/// It primarily specifies how to map between keys (such as the name of
/// an object) and values (such as SpecHandles for those objects).
///
template<class ChildPolicy>
class Sdf_Children
{
public:
    typedef typename ChildPolicy::KeyPolicy KeyPolicy;
    typedef typename ChildPolicy::KeyType KeyType;
    typedef typename ChildPolicy::ValueType ValueType;
    typedef typename ChildPolicy::FieldType FieldType;
    typedef Sdf_Children<ChildPolicy> This;

    Sdf_Children();

    Sdf_Children(const Sdf_Children<ChildPolicy> &other);

    Sdf_Children(const SdfLayerHandle &layer,
        const SdfPath &parentPath, const TfToken &childrenKey,
        const KeyPolicy& keyPolicy = KeyPolicy());

    /// Return whether this object is valid.
    bool IsValid() const;

    /// Return the number of children that this object contains.
    size_t GetSize() const;

    /// Return the child at the specified index.
    ValueType GetChild(size_t index) const;

    /// Find the index of the specified key, or return the size if it's not found.
    size_t Find(const KeyType &key) const;
    
    /// Find the key that corresponds to \a value, or return a default
    /// constructed key if it's not found.
    KeyType FindKey(const ValueType &value) const;

    /// Return true if this object and \a other are equivalent.
    bool IsEqualTo(const This &other) const;

    /// Replace this object's children with the ones in \a values.
    bool Copy(const std::vector<ValueType> & values, const std::string &type);
    
    /// Insert a new child at the specified \a index.
    bool Insert(const ValueType& value, size_t index, const std::string &type);

    /// Erase the child with the specified key.
    bool Erase(const KeyType& key, const std::string &type);

private:
    void _UpdateChildNames() const;
    
private:
    SdfLayerHandle _layer;
    SdfPath _parentPath;
    TfToken _childrenKey;
    KeyPolicy _keyPolicy;
    
    mutable std::vector<FieldType> _childNames;
    mutable bool _childNamesValid;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_CHILDREN_H
