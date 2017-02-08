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
#ifndef SDF_LIST_OP_LIST_EDITOR_H
#define SDF_LIST_OP_LIST_EDITOR_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/listEditor.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/listOp.h"

#include <boost/array.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/// \class Sdf_ListOpListEditor
///
/// List editor implementation for list editing operations stored in an
/// SdfListOp object. 
///
template <class TypePolicy>
class Sdf_ListOpListEditor
    : public Sdf_ListEditor<TypePolicy>
{
private:
    typedef Sdf_ListOpListEditor<TypePolicy> This;
    typedef Sdf_ListEditor<TypePolicy>       Parent;

    typedef SdfListOp<typename Parent::value_type> ListOpType;

public:
    typedef typename Parent::value_type        value_type;
    typedef typename Parent::value_vector_type value_vector_type;

    Sdf_ListOpListEditor(const SdfSpecHandle& owner, const TfToken& listField,
                         const TypePolicy& typePolicy = TypePolicy());

    virtual ~Sdf_ListOpListEditor() = default;

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
        SdfListOpType opType, const Sdf_ListEditor<TypePolicy>& rhs);

protected:
    // Pull in protected functions from base class; this is needed because 
    // these are non-dependent names and the compiler won't look in 
    // dependent base-classes for these.
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

    static bool _ListDiffers(SdfListOpType op, 
                             const ListOpType& x, const ListOpType& y)
    {
        return x.GetItems(op) != y.GetItems(op);
    }

    void _UpdateListOp(const ListOpType& newListOp,
                       const SdfListOpType* updatedListOpType = NULL)
    {
        if (!_GetOwner()) {
            TF_CODING_ERROR("Invalid owner.");
            return;
        }

        if (!_GetOwner()->GetLayer()->PermissionToEdit()) {
            TF_CODING_ERROR("Layer is not editable.");
            return;
        }

        const boost::array<SdfListOpType, 4> opTypes = {
            SdfListOpTypeExplicit,
            SdfListOpTypeAdded,
            SdfListOpTypeDeleted,
            SdfListOpTypeOrdered
        };

        // Check if any of the list operation vectors have changed and validate
        // their new contents.
        bool anyChanged = false;
        boost::array<bool, 4> opListChanged = { 
            false, false, false, false 
        };

        for (int i = 0; i < opTypes.size(); ++i) {
            // If the consumer has specified that only a single op type has
            // changed, ignore all others.
            if (updatedListOpType && *updatedListOpType != opTypes[i]) {
                continue;
            }

            opListChanged[i] = _ListDiffers(opTypes[i], newListOp, _listOp);
            if (opListChanged[i]) {
                if (!_ValidateEdit(opTypes[i], 
                                      _listOp.GetItems(opTypes[i]), 
                                      newListOp.GetItems(opTypes[i]))) {
                    return;
                }
                anyChanged = true;
            }
        }

        if (!anyChanged && 
            (newListOp.IsExplicit() == _listOp.IsExplicit())) {
            return;
        }

        // Save away the old SdfListOp and set in our new one.
        SdfChangeBlock block;

        ListOpType oldListOp = newListOp;
        _listOp.Swap(oldListOp);

        if (newListOp.HasKeys()) {
            _GetOwner()->SetField(_GetField(), VtValue(newListOp));
        } else {
            _GetOwner()->ClearField(_GetField());
        }

        // For each operation list that changed, call helper function to allow
        // subclasses to execute additional behavior in response to changes.
        for (int i = 0; i < opTypes.size(); ++i) {
            if (opListChanged[i]) {
                this->_OnEdit(opTypes[i], 
                        oldListOp.GetItems(opTypes[i]),
                        newListOp.GetItems(opTypes[i]));
            }
        }
    }

private:
    ListOpType _listOp;
};

////////////////////////////////////////

template <class TP>
Sdf_ListOpListEditor<TP>::Sdf_ListOpListEditor(
    const SdfSpecHandle& owner, 
    const TfToken& listField,
    const TP& typePolicy)
    : Parent(owner, listField, typePolicy)
{
    if (owner) {
        _listOp = owner->GetFieldAs<ListOpType>(_GetField());
    }
}

template <class TP>
bool 
Sdf_ListOpListEditor<TP>::IsExplicit() const
{
    return _listOp.IsExplicit();
}

template <class TP>
bool 
Sdf_ListOpListEditor<TP>::IsOrderedOnly() const
{
    return false;
}

template <class TP>
bool 
Sdf_ListOpListEditor<TP>::CopyEdits(const Sdf_ListEditor<TP>& rhs)
{
    const This* rhsEdit = dynamic_cast<const This*>(&rhs);
    if (!rhsEdit) {
        TF_CODING_ERROR("Could not copy from list editor of different type");
        return false;
    }
    
    _UpdateListOp(rhsEdit->_listOp);
    return true;
}

template <class TP>
bool 
Sdf_ListOpListEditor<TP>::ClearEdits()
{
    ListOpType emptyAndNotExplicit;
    
    _UpdateListOp(emptyAndNotExplicit);
    return true;
}

template <class TP>
bool 
Sdf_ListOpListEditor<TP>::ClearEditsAndMakeExplicit()
{
    ListOpType emptyAndExplicit;
    emptyAndExplicit.ClearAndMakeExplicit();
    
    _UpdateListOp(emptyAndExplicit);
    return true;
}

template <class TP>
void 
Sdf_ListOpListEditor<TP>::ModifyItemEdits(const ModifyCallback& cb)
{
    ListOpType modifiedListOp = _listOp;
    modifiedListOp.ModifyOperations(
        boost::bind(_ModifyCallbackHelper, cb, _GetTypePolicy(), _1));

    _UpdateListOp(modifiedListOp);
}

template <class TP>
void 
Sdf_ListOpListEditor<TP>::ApplyEdits(
    value_vector_type* vec, const ApplyCallback& cb)
{
    _listOp.ApplyOperations(vec, cb);
}

template <class TP>
bool 
Sdf_ListOpListEditor<TP>::ReplaceEdits(
    SdfListOpType opType, size_t index, size_t n, 
    const value_vector_type& newItems)
{
    ListOpType editedListOp = _listOp;
    if (!editedListOp.ReplaceOperations(
           opType, index, n, 
           _GetTypePolicy().Canonicalize(newItems))) {
        return false;
    }
        
    _UpdateListOp(editedListOp, &opType);
    return true;
}

template <class TP>
void 
Sdf_ListOpListEditor<TP>::ApplyList(
    SdfListOpType opType, const Sdf_ListEditor<TP>& rhs)
                                   
{
    const This* rhsEdit = dynamic_cast<const This*>(&rhs);
    if (!rhsEdit) {
        TF_CODING_ERROR("Cannot apply from list editor of different type");
        return;
    }
    
    ListOpType composedListOp = _listOp;
    composedListOp.ComposeOperations(rhsEdit->_listOp, opType);
    
    _UpdateListOp(composedListOp, &opType);
}

template <class TP>
const typename Sdf_ListOpListEditor<TP>::value_vector_type& 
Sdf_ListOpListEditor<TP>::_GetOperations(SdfListOpType op) const
{
    return _listOp.GetItems(op);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_LIST_OP_LIST_EDITOR_H
