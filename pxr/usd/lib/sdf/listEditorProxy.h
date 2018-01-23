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
#ifndef SDF_LISTEDITORPROXY_H
#define SDF_LISTEDITORPROXY_H

/// \file sdf/listEditorProxy.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/listEditor.h"
#include "pxr/usd/sdf/listProxy.h"
#include "pxr/usd/sdf/changeBlock.h"

#include "pxr/base/vt/value.h"  // for Vt_DefaultValueFactory

#include <boost/mpl/logical.hpp>
#include <boost/type_traits.hpp>
#include <boost/optional.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfListEditorProxy
///
/// Represents a set of list editing operations. 
///
/// An SdfListEditorProxy allows consumers to specify a transformation to be
/// applied to a list via a set of list editing operations. Given a starting 
/// ordered list, it can either replace the result with another ordered list 
/// or apply a sequence of three operations:  deleting keys, then adding keys
/// to the end (if they aren't already in the starting list), then reordering 
/// keys.
///
/// The type policy defines the value type that a particular proxy can operate
/// on.
template <class _TypePolicy>
class SdfListEditorProxy {
public:
    typedef _TypePolicy TypePolicy;
    typedef SdfListEditorProxy<TypePolicy> This;
    typedef SdfListProxy<TypePolicy> ListProxy;
    typedef typename TypePolicy::value_type value_type;
    typedef std::vector<value_type> value_vector_type;

    // ApplyEdits types.
    typedef boost::function<boost::optional<value_type>
                        (SdfListOpType, const value_type&)> ApplyCallback;

    // ModifyEdits types.
    typedef boost::function<boost::optional<value_type>
                        (const value_type&)> ModifyCallback;

    /// Creates a default proxy object. The object evaluates to \c false in a 
    /// boolean context and all operations on this object have no effect.
    SdfListEditorProxy()
    {
    }

    /// Creates a new proxy object backed by the supplied list editor.
    explicit SdfListEditorProxy(
        const std::shared_ptr<Sdf_ListEditor<TypePolicy> >& listEditor)
        : _listEditor(listEditor)
    {
    }

    /// Returns true if the list editor is expired.
    bool IsExpired() const
    {
        if (!_listEditor) {
            return false;
        }

        return _listEditor->IsExpired();
    }

    /// Returns \c true if the editor has an explicit list, \c false if
    /// it has list operations.
    bool IsExplicit() const
    {
        return _Validate() ? _listEditor->IsExplicit() : true;
    }

    /// Returns \c true if the editor is not explicit and allows ordering
    /// only.
    bool IsOrderedOnly() const
    {
        return _Validate() ? _listEditor->IsOrderedOnly() : false;
    }

    /// Returns \c true if the editor has an explicit list (even if it's
    /// empty) or it has any added, prepended, appended, deleted,
    /// or ordered keys.
    bool HasKeys() const
    {
        return _Validate() ? _listEditor->HasKeys() : true;
    }

    /// Apply the edits to \p vec.
    void ApplyEditsToList(value_vector_type* vec) const
    {
        if (_Validate()) {
            _listEditor->ApplyEdits(vec, ApplyCallback());
        }
    }

    /// Apply the edits to \p vec.  If \p callback is valid then it's called
    /// for every key in the editor before applying it to \p vec.  If the
    /// returned key is invalid then the key will not be applied.  Otherwise
    /// the returned key is applied, allowing callbacks to perform key
    /// translation.
    template <class CB>
    void ApplyEditsToList(value_vector_type* vec, CB callback) const
    {
        if (_Validate()) {
            _listEditor->ApplyEdits(vec, ApplyCallback(callback));
        }
    }

    /// Copies the keys from \p other.  This differs from assignment
    /// because assignment just makes two list editors refer to the
    /// same lists.
    ///
    /// Not all list editors support changing their mode.  If the mode
    /// can't be changed to the mode of \p other then this does nothing
    /// and returns \c false, otherwise it returns \c true.
    bool CopyItems(const This& other)
    {
        return _Validate() && other._Validate() ?
            _listEditor->CopyEdits(*other._listEditor) : false;
    }

    /// Removes all keys and changes the editor to have list operations.
    ///
    /// Not all list editors support changing their mode.  If the mode
    /// can't be changed to the mode of \p other then this does nothing
    /// and returns \c false, otherwise it returns \c true.
    bool ClearEdits()
    {
        return _Validate() ? _listEditor->ClearEdits() : false;
    }

    /// Removes all keys and changes the editor to be explicit.
    ///
    /// Not all list editors support changing their mode.  If the mode
    /// can't be changed to the mode of \p other then this does nothing
    /// and returns \c false, otherwise it returns \c true.
    bool ClearEditsAndMakeExplicit()
    {
        return _Validate() ? _listEditor->ClearEditsAndMakeExplicit() : false;
    }

    /// \p callback is called for every key.  If the returned key is
    /// invalid then the key is removed, otherwise it's replaced with the
    /// returned key.
    template <class CB>
    void ModifyItemEdits(CB callback)
    {
        if (_Validate()) {
            _listEditor->ModifyItemEdits(ModifyCallback(callback));
        }
    }

    /// Check if the given item is explicit, added, prepended, appended,
    /// deleted, or ordered by this editor. If \p onlyAddOrExplicit is
    /// \c true we only check the added or explicit items.
    bool ContainsItemEdit(const value_type& item,
                          bool onlyAddOrExplicit = false) const
    {
        if (_Validate()) {
            size_t i;

            i = GetExplicitItems().Find(item);
            if (i != size_t(-1)) {
                return true;
            }

            i = GetAddedItems().Find(item);
            if (i != size_t(-1)) {
                return true;
            }

            i = GetPrependedItems().Find(item);
            if (i != size_t(-1)) {
                return true;
            }

            i = GetAppendedItems().Find(item);
            if (i != size_t(-1)) {
                return true;
            }

            if (!onlyAddOrExplicit) {
                i = GetDeletedItems().Find(item);
                if (i != size_t(-1)) {
                    return true;
                }

                i = GetOrderedItems().Find(item);
                if (i != size_t(-1)) {
                    return true;
                }
            }
        }

        return false;
    }

    /// Remove all occurances of the given item, regardless of whether
    /// the item is explicit, added, prepended, appended, deleted, or ordered.
    void RemoveItemEdits(const value_type& item)
    {
        if (_Validate()) {
            SdfChangeBlock block;
            
            GetExplicitItems().Remove(item);
            GetAddedItems().Remove(item);
            GetPrependedItems().Remove(item);
            GetAppendedItems().Remove(item);
            GetDeletedItems().Remove(item);
            GetOrderedItems().Remove(item);
        }
    }

    /// Replace all occurances of the given item, regardless of
    /// whether the item is explicit, added, prepended, appended,
    /// deleted or ordered.
    void ReplaceItemEdits(const value_type& oldItem, const value_type& newItem)
    {
        if (_Validate()) {
            SdfChangeBlock block;
            
            GetExplicitItems().Replace(oldItem, newItem);
            GetAddedItems().Replace(oldItem, newItem);
            GetPrependedItems().Replace(oldItem, newItem);
            GetAppendedItems().Replace(oldItem, newItem);
            GetDeletedItems().Replace(oldItem, newItem);
            GetOrderedItems().Replace(oldItem, newItem);
        }
    }

    /// Returns the explicitly set items.
    ListProxy GetExplicitItems() const
    {
        return ListProxy(_listEditor, SdfListOpTypeExplicit);
    }

    /// Returns the items added by this list editor
    ListProxy GetAddedItems() const
    {
        return ListProxy(_listEditor, SdfListOpTypeAdded);
    }

    /// Returns the items prepended by this list editor
    ListProxy GetPrependedItems() const
    {
        return ListProxy(_listEditor, SdfListOpTypePrepended);
    }

    /// Returns the items appended by this list editor
    ListProxy GetAppendedItems() const
    {
        return ListProxy(_listEditor, SdfListOpTypeAppended);
    }

    /// Returns the items deleted by this list editor
    ListProxy GetDeletedItems() const
    {
        return ListProxy(_listEditor, SdfListOpTypeDeleted);
    }

    /// Returns the items reordered by this list editor
    ListProxy GetOrderedItems() const
    {
        return ListProxy(_listEditor, SdfListOpTypeOrdered);
    }

    /// Returns the added or explicitly set items.
    value_vector_type GetAddedOrExplicitItems() const
    {
        value_vector_type result;
        if (_Validate()) {
            _listEditor->ApplyEdits(&result);
        }
        return result;
    }

    void Add(const value_type& value)
    {
        if (_Validate()) {
            if (!_listEditor->IsOrderedOnly()) {
                if (_listEditor->IsExplicit()) {
                    _AddOrReplace(SdfListOpTypeExplicit, value);
                }
                else {
                    GetDeletedItems().Remove(value);
                    _AddOrReplace(SdfListOpTypeAdded, value);
                }
            }
        }
    }

    void Prepend(const value_type& value)
    {
        if (_Validate()) {
            if (!_listEditor->IsOrderedOnly()) {
                if (_listEditor->IsExplicit()) {
                    _Prepend(SdfListOpTypeExplicit, value);
                }
                else {
                    GetDeletedItems().Remove(value);
                    _Prepend(SdfListOpTypePrepended, value);
                }
            }
        }
    }

    void Append(const value_type& value)
    {
        if (_Validate()) {
            if (!_listEditor->IsOrderedOnly()) {
                if (_listEditor->IsExplicit()) {
                    _Append(SdfListOpTypeExplicit, value);
                }
                else {
                    GetDeletedItems().Remove(value);
                    _Append(SdfListOpTypeAppended, value);
                }
            }
        }
    }

    void Remove(const value_type& value)
    {
        if (_Validate()) {
            if (_listEditor->IsExplicit()) {
                GetExplicitItems().Remove(value);
            }
            else if (!_listEditor->IsOrderedOnly()) {
                GetAddedItems().Remove(value);
                GetPrependedItems().Remove(value);
                GetAppendedItems().Remove(value);
                _AddIfMissing(SdfListOpTypeDeleted, value);
            }
        }
    }

    void Erase(const value_type& value)
    {
        if (_Validate()) {
            if (!_listEditor->IsOrderedOnly()) {
                if (_listEditor->IsExplicit()) {
                    GetExplicitItems().Remove(value);
                }
                else {
                    GetAddedItems().Remove(value);
                    GetPrependedItems().Remove(value);
                    GetAppendedItems().Remove(value);
                }
            }
        }
    }

#if !defined(doxygen)
    typedef std::shared_ptr<Sdf_ListEditor<TypePolicy> >
        This::*UnspecifiedBoolType;
#endif

    /// Returns \c true in a boolean context if the list editor is valid,
    /// \c false otherwise.
    operator UnspecifiedBoolType() const
    {
        return (_listEditor && _listEditor->IsValid()) ? 
            &This::_listEditor : NULL;
    }

    /// Returns \c false in a boolean context if the list editor is valid,
    /// \c true otherwise.
    bool operator!() const 
    { 
        return (!_listEditor || !_listEditor->IsValid());
    }

private:
    bool _Validate()
    {
        if (!_listEditor) {
            return false;
        }

        if (IsExpired()) {
            TF_CODING_ERROR("Accessing expired list editor");
            return false;
        }
        return true;
    }

    bool _Validate() const
    {
        if (!_listEditor) {
            return false;
        }

        if (IsExpired()) {
            TF_CODING_ERROR("Accessing expired list editor");
            return false;
        }
        return true;
    }

    void _AddIfMissing(SdfListOpType op, const value_type& value)
    {
        ListProxy proxy(_listEditor, op);
        size_t index = proxy.Find(value);
        if (index == size_t(-1)) {
            proxy.push_back(value);
        }
    }

    void _AddOrReplace(SdfListOpType op, const value_type& value)
    {
        ListProxy proxy(_listEditor, op);
        size_t index = proxy.Find(value);
        if (index == size_t(-1)) {
            proxy.push_back(value);
        }
        else if (value != static_cast<value_type>(proxy[index])) {
            proxy[index] = value;
        }
    }

    void _Prepend(SdfListOpType op, const value_type& value)
    {
        ListProxy proxy(_listEditor, op);
        size_t index = proxy.Find(value);
        if (index != 0) {
            if (index != size_t(-1)) {
                proxy.Erase(index);
            }
            proxy.insert(proxy.begin(), value);
        }
    }

    void _Append(SdfListOpType op, const value_type& value)
    {
        ListProxy proxy(_listEditor, op);
        size_t index = proxy.Find(value);
        if (proxy.empty() || (index != proxy.size()-1)) {
            if (index != size_t(-1)) {
                proxy.Erase(index);
            }
            proxy.push_back(value);
        }
    }

private:
    std::shared_ptr<Sdf_ListEditor<TypePolicy> > _listEditor;

    friend class Sdf_ListEditorProxyAccess;
    template <class T> friend class SdfPyWrapListEditorProxy;
};

// Cannot get from a VtValue except as the correct type.
template <class TP>
struct Vt_DefaultValueFactory<SdfListEditorProxy<TP> > {
    static Vt_DefaultValueHolder Invoke() {
        TF_AXIOM(false && "Failed VtValue::Get<SdfListEditorProxy> not allowed");
        return Vt_DefaultValueHolder::Create((void*)0);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_LISTEDITORPROXY_H
