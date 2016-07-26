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
#include "pxr/usd/sdf/children.h"

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/childrenPolicies.h"
#include "pxr/usd/sdf/childrenUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/mapperSpec.h"
#include "pxr/usd/sdf/mapperArgSpec.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"
#include "pxr/base/tf/ostreamMethods.h"

template<class ChildPolicy>
Sdf_Children<ChildPolicy>::Sdf_Children() :
    _childNamesValid(false)
{
}

template<class ChildPolicy>
Sdf_Children<ChildPolicy>::Sdf_Children(const Sdf_Children<ChildPolicy> &other) :
    _layer(other._layer),
    _parentPath(other._parentPath),
    _childrenKey(other._childrenKey),
    _keyPolicy(other._keyPolicy),
    _childNamesValid(false)
{
}

template<class ChildPolicy>
Sdf_Children<ChildPolicy>::Sdf_Children(const SdfLayerHandle &layer,
    const SdfPath &parentPath, const TfToken &childrenKey,
    const KeyPolicy& keyPolicy) : 
    _layer(layer),
    _parentPath(parentPath),
    _childrenKey(childrenKey),
    _keyPolicy(keyPolicy),
    _childNamesValid(false)
{
}

template<class ChildPolicy>
size_t
Sdf_Children<ChildPolicy>::GetSize() const
{
    _UpdateChildNames();
        
    return _childNames.size();
}

template<class ChildPolicy>
typename Sdf_Children<ChildPolicy>::ValueType
Sdf_Children<ChildPolicy>::GetChild(size_t index) const
{
    if (not TF_VERIFY(IsValid())) {
        return ValueType();
    }

    _UpdateChildNames();
        
    // XXX: Would like to avoid unnecessary dynamic_casts...
    SdfPath childPath = 
        ChildPolicy::GetChildPath(_parentPath, _childNames[index]);
    return TfDynamic_cast<ValueType>(_layer->GetObjectAtPath(childPath));
}

template<class ChildPolicy>
bool
Sdf_Children<ChildPolicy>::IsValid() const
{
    // XXX: Should we also check for the existence of the spec?
    return _layer and not _parentPath.IsEmpty();
}

template<class ChildPolicy>
size_t
Sdf_Children<ChildPolicy>::Find(const KeyType &key) const
{
    if (not TF_VERIFY(IsValid())) {
        return 0;
    }

    _UpdateChildNames();

    const FieldType expectedKey(_keyPolicy.Canonicalize(key));
    size_t i = 0;
    for (i=0; i < _childNames.size(); i++) {
        if (_childNames[i] == expectedKey) {
            break;
        }
    }
    return i;
}

template<class ChildPolicy>
typename Sdf_Children<ChildPolicy>::KeyType
Sdf_Children<ChildPolicy>::FindKey(const ValueType &x) const
{
    if (not TF_VERIFY(IsValid())) {
        return KeyType();
    }

    // If the value is invalid or does not belong to this layer,
    // then return a default-constructed key.
    if (not x or x->GetLayer() != _layer) {
        return KeyType();
    }

    // If the value's path is not a child path of the parent path,
    // then return a default-constructed key.
    if (ChildPolicy::GetParentPath(x->GetPath()) != _parentPath) {
        return KeyType();
    }
        
    return ChildPolicy::GetKey(x);
}

template<class ChildPolicy>
bool
Sdf_Children<ChildPolicy>::IsEqualTo(const Sdf_Children<ChildPolicy> &other) const
{
    // Return true if this and other refer to the same set of children
    // on the same object in the same layer.
    return (_layer == other._layer and _parentPath == other._parentPath and
        _childrenKey == other._childrenKey);
}

template<class ChildPolicy>
bool
Sdf_Children<ChildPolicy>::Copy(
    const std::vector<ValueType> & values,
    const std::string &type)
{
    _childNamesValid = false;

    if (not TF_VERIFY(IsValid())) {
        return false;
    }

    return Sdf_ChildrenUtils<ChildPolicy>::SetChildren(
        _layer, _parentPath, values);
}

template<class ChildPolicy>
bool
Sdf_Children<ChildPolicy>::Insert(const ValueType& value, size_t index, const std::string &type)
{
    _childNamesValid = false;

    if (not TF_VERIFY(IsValid())) {
        return false;
    }

    return Sdf_ChildrenUtils<ChildPolicy>::InsertChild(
        _layer, _parentPath, value, index);
}

template<class ChildPolicy>
bool
Sdf_Children<ChildPolicy>::Erase(const KeyType& key, const std::string &type)
{
    _childNamesValid = false;

    if (not TF_VERIFY(IsValid())) {
        return false;
    }

    const FieldType expectedKey(_keyPolicy.Canonicalize(key));

    return Sdf_ChildrenUtils<ChildPolicy>::RemoveChild(
        _layer, _parentPath, expectedKey);
}

template<class ChildPolicy>
void
Sdf_Children<ChildPolicy>::_UpdateChildNames() const
{
    if (_childNamesValid) {
        return;
    }
    _childNamesValid = true;

    if (_layer) {
        _childNames = _layer->GetFieldAs<std::vector<FieldType> >(
            _parentPath, _childrenKey);
    } else {
        _childNames.clear();
    }
}

template class Sdf_Children<Sdf_AttributeChildPolicy>;
template class Sdf_Children<Sdf_MapperChildPolicy>;
template class Sdf_Children<Sdf_MapperArgChildPolicy>;
template class Sdf_Children<Sdf_PrimChildPolicy>;
template class Sdf_Children<Sdf_PropertyChildPolicy>;
template class Sdf_Children<Sdf_RelationshipChildPolicy>;
template class Sdf_Children<Sdf_VariantChildPolicy>;
template class Sdf_Children<Sdf_VariantSetChildPolicy>;


