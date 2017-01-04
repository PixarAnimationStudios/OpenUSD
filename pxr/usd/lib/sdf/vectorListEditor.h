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
#ifndef SDF_VECTOR_LIST_EDITOR_H
#define SDF_VECTOR_LIST_EDITOR_H

#include "pxr/usd/sdf/listEditor.h"

#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/listOp.h"

#include "pxr/base/tf/ostreamMethods.h"
#include <vector>

// Various adapter classes used by Sdf_VectorListEditor to allow for
// conversions between the publicly exposed value type and the underlying 
// field data type.
template <class To, class From>
struct Sdf_VectorFieldAdapter;

template <class T>
struct Sdf_VectorFieldAdapter<T, T>
{
    static const std::vector<T>& Convert(const std::vector<T>& from)
    {
        return from;
    }
};

template <>
struct Sdf_VectorFieldAdapter<TfToken, std::string>
{
    static std::vector<TfToken> Convert(const std::vector<std::string>& from)
    {
        return TfToTokenVector(from);
    }
};

template <>
struct Sdf_VectorFieldAdapter<std::string, TfToken>
{
    static std::vector<std::string> Convert(const std::vector<TfToken>& from)
    {
        return TfToStringVector(from);
    }
};

/// \class Sdf_VectorListEditor
///
/// An Sdf_ListEditor implementation that represents a single type of list 
/// editing operation stored in a vector-typed field. 
///
/// TypePolicy determines the externally visible value type of this
/// list editor. By default, it's assumed this value type is also stored in
/// the underlying field data. This may be overridden by explicitly specifying
/// the FieldStorageType.
///
template <class TypePolicy, 
          class FieldStorageType = typename TypePolicy::value_type>
class Sdf_VectorListEditor
    : public Sdf_ListEditor<TypePolicy>
{
private:
    typedef Sdf_VectorListEditor<
                TypePolicy, FieldStorageType> This;
    typedef Sdf_ListEditor<TypePolicy>        Parent;

public:
    typedef typename Parent::value_type        value_type;
    typedef typename Parent::value_vector_type value_vector_type;

    virtual ~Sdf_VectorListEditor() = default;

    Sdf_VectorListEditor(const SdfSpecHandle& owner, 
                         const TfToken& field, SdfListOpType op,
                         const TypePolicy& typePolicy = TypePolicy());

    virtual bool IsExplicit() const;
    virtual bool IsOrderedOnly() const;

    virtual bool CopyEdits(const Sdf_ListEditor<TypePolicy>& rhs);
    virtual bool ClearEdits();
    virtual bool ClearEditsAndMakeExplicit();

    typedef typename Parent::ModifyCallback ModifyCallback;
    virtual void ModifyItemEdits(const ModifyCallback& cb);

    typedef typename Parent::ApplyCallback ApplyCallback;
    virtual void ApplyEdits(value_vector_type* vec, const ApplyCallback& cb);

    virtual bool ReplaceEdits(
        SdfListOpType op, size_t index, size_t n, const value_vector_type& elems);

    virtual void ApplyList(
        SdfListOpType op, const Sdf_ListEditor<TypePolicy>& rhs);

protected:
    using Parent::_GetField;
    using Parent::_GetOwner;
    using Parent::_GetTypePolicy;
    using Parent::_ValidateEdit;

    virtual const value_vector_type& _GetOperations(SdfListOpType op) const;

private:
    static 
    boost::optional<value_type> 
    _ModifyCallbackHelper(const ModifyCallback& cb,
                          const TypePolicy& typePolicy,
                          const value_type& v)
    {
        boost::optional<value_type> value = cb(v);
        return value ? typePolicy.Canonicalize(*value) : value;
    }

    void _UpdateFieldData(const value_vector_type& newData)
    {
        if (!_GetOwner()) {
            TF_CODING_ERROR("Invalid owner.");
            return;
        }

        if (!_GetOwner()->GetLayer()->PermissionToEdit()) {
            TF_CODING_ERROR("Layer is not editable.");
            return;
        }

        if (newData == _data || !_ValidateEdit(_op, _data, newData)) {
            return;
        }

        SdfChangeBlock changeBlock;

        value_vector_type oldData = newData;
        _data.swap(oldData);

        if (newData.empty()) {
            _GetOwner()->ClearField(_GetField());
        }
        else {
            typedef Sdf_VectorFieldAdapter<FieldStorageType, value_type> 
                ToFieldType;
            const std::vector<FieldStorageType> newFieldData = 
                ToFieldType::Convert(newData);
            _GetOwner()->SetField(_GetField(), newFieldData);
        }

        this->_OnEdit(_op, oldData, newData);
    }

private:
    TfToken _field;
    SdfListOpType _op;
    value_vector_type _data;
};

////////////////////////////////////////

template <class TP, class FST>
Sdf_VectorListEditor<TP, FST>::Sdf_VectorListEditor(
    const SdfSpecHandle& owner, 
    const TfToken& field, SdfListOpType op,
    const TP& typePolicy)
    : Parent(owner, field, typePolicy),
      _op(op)
{
    if (owner) {
        typedef std::vector<FST> FieldVectorType;
        typedef Sdf_VectorFieldAdapter<value_type, FST> ToValueType;

        _data = ToValueType::Convert(owner->GetFieldAs<FieldVectorType>(field));
    }
}

template <class TP, class FST>
bool 
Sdf_VectorListEditor<TP, FST>::IsExplicit() const
{
    return _op == SdfListOpTypeExplicit;
}

template <class TP, class FST>
bool 
Sdf_VectorListEditor<TP, FST>::IsOrderedOnly() const
{
    return _op == SdfListOpTypeOrdered;
}

template <class TP, class FST>
bool 
Sdf_VectorListEditor<TP, FST>::CopyEdits(const Sdf_ListEditor<TP>& rhs)
{
    const This* rhsEdit = dynamic_cast<const This*>(&rhs);
    if (!rhsEdit) {
        TF_CODING_ERROR("Cannot copy from list editor of different type");
        return false;
    }

    if (_op != rhsEdit->_op) {
        TF_CODING_ERROR("Cannot copy from list editor in different mode");
        return false;
    }

    _UpdateFieldData(rhsEdit->_data);
    return true;
}

template <class TP, class FST>
bool 
Sdf_VectorListEditor<TP, FST>::ClearEdits()
{
    // Per specification of ClearEdits, we need to return false if we're
    // not able to switch to non-explicit mode. This list editor doesn't
    // allow any mode switching at all, so we need to return false if we're
    // explicit.
    if (_op == SdfListOpTypeExplicit) {
        return false;
    }

    _UpdateFieldData(value_vector_type());
    return true;
}

template <class TP, class FST>
bool 
Sdf_VectorListEditor<TP, FST>::ClearEditsAndMakeExplicit()
{
    // Per specification of ClearEditsAndMakeExplicit, we need to return 
    // false if we're not able to switch to explicit mode. This list editor 
    // doesn't allow any mode switching at all, so we need to return false 
    // if we're not already explicit.
    if (_op != SdfListOpTypeExplicit) {
        return false;
    }
    
    _UpdateFieldData(value_vector_type());
    return true;
}

template <class TP, class FST>
void 
Sdf_VectorListEditor<TP, FST>::ModifyItemEdits(const ModifyCallback& cb)
{
    if (_data.empty()) {
        return;
    }

    SdfListOp<value_type> valueListOp;
    valueListOp.SetItems(_data, _op);
    valueListOp.ModifyOperations(
        boost::bind(_ModifyCallbackHelper, cb, _GetTypePolicy(), _1));

    _UpdateFieldData(valueListOp.GetItems(_op));
}

template <class TP, class FST>
void 
Sdf_VectorListEditor<TP, FST>::ApplyEdits(
    value_vector_type* vec, const ApplyCallback& cb)
{
    if (_data.empty()) {
        return;
    }

    SdfListOp<value_type> valueListOp;
    valueListOp.SetItems(_data, _op);
    valueListOp.ApplyOperations(vec, cb);
}

template <class TP, class FST>
bool 
Sdf_VectorListEditor<TP, FST>::ReplaceEdits(
    SdfListOpType op, size_t index, size_t n, const value_vector_type& elems)
{
    if (op != _op) {
        return false;
    }

    SdfListOp<value_type> fieldListOp;
    fieldListOp.SetItems(_data, op);
    if (!fieldListOp.ReplaceOperations(
            op, index, n, _GetTypePolicy().Canonicalize(elems))) {
        return false;
    }

    _UpdateFieldData(fieldListOp.GetItems(op));
    return true;
}

template <class TP, class FST>
void 
Sdf_VectorListEditor<TP, FST>::ApplyList(
    SdfListOpType op, const Sdf_ListEditor<TP>& rhs)
{
    const This* rhsEdit = dynamic_cast<const This*>(&rhs);
    if (!rhsEdit) {
        TF_CODING_ERROR("Cannot apply from list editor of different type");
        return;
    }
    
    if (op != _op && op != rhsEdit->_op) {
        return;
    }

    SdfListOp<value_type> self;
    self.SetItems(_data, op);

    SdfListOp<value_type> stronger;
    stronger.SetItems(rhsEdit->_data, op);

    self.ComposeOperations(stronger, op);

    _UpdateFieldData(self.GetItems(op));
}

template <class TP, class FST>
const typename Sdf_VectorListEditor<TP, FST>::value_vector_type& 
Sdf_VectorListEditor<TP, FST>::_GetOperations(SdfListOpType op) const
{
    if (op != _op) {
        static const value_vector_type empty;
        return empty;
    }
    
    return _data;
}

#endif // SDF_VECTOR_LIST_EDITOR_H
