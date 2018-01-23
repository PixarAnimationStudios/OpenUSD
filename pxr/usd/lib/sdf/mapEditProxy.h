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
#ifndef SDF_MAPEDITPROXY_H
#define SDF_MAPEDITPROXY_H

/// \file sdf/mapEditProxy.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/allowed.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/mapEditor.h"
#include "pxr/usd/sdf/spec.h"

#include "pxr/base/vt/value.h"  // for Vt_DefaultValueFactory
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/mallocTag.h"
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/operators.hpp>
#include <iterator>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;

SDF_DECLARE_HANDLES(SdfSpec);

/// \class SdfIdentityMapEditProxyValuePolicy
///
/// A value policy for \c SdfMapEditProxy that does nothing.
///
/// A \c SdfMapEditProxy value policy converts incoming keys and values
/// into a canonical form used for storage.  This is useful if you have
/// a key where multiple values are equivalent for the purposes of the
/// map but don't compare equal and you can store any one of the
/// equivalent values.  Note that the policy is only used on inputs to
/// the map proxy;  it's never used when returning keys or values from
/// the proxy.
///
template <class T>
class SdfIdentityMapEditProxyValuePolicy {
public:
    typedef T Type;
    typedef typename Type::key_type key_type;
    typedef typename Type::mapped_type mapped_type;
    typedef typename Type::value_type value_type;

    /// Canonicalize an entire \c Type object.  \c Type must be convertible
    /// to the type of \p x.  The return value must be convertible to a
    /// \c Type.
    static const Type& CanonicalizeType(const SdfSpecHandle&, const Type& x)
    {
        return x;
    }

    /// Canonicalize a key.  \c key_type must be convertible to the type of
    /// \p x.  The return value must be convertible to a \c key_type.
    static const key_type& CanonicalizeKey(const SdfSpecHandle&,
                                           const key_type& x)
    {
        return x;
    }

    /// Canonicalize a value.  \c mapped_type must be convertible to the type
    /// of \p x.  The return value must be convertible to a \c mapped_type.
    static const mapped_type& CanonicalizeValue(const SdfSpecHandle&,
                                                const mapped_type& x)
    {
        return x;
    }

    /// Canonicalize a key/value pair.  \c value_type must be convertible
    /// to the type of \p x.  The return value must be convertible to a
    /// \c value_type.
    static const value_type& CanonicalizePair(const SdfSpecHandle&,
                                              const value_type& x)
    {
        return x;
    }
};

/// \class SdfMapEditProxy
///
/// A proxy for editing map-like values.
///
/// A \c SdfMapEditProxy provides an interface for editing fields
/// containing map-like values. The proxy allows consumers to
/// interact with these values like a typical std::map while
/// taking into account additional editing and validation policies.
///
/// The \c _ValuePolicy is used to canonicalize keys and values before
/// storage or comparison.
///
/// \sa SdfIdentityMapEditProxyValuePolicy
///
template <class T, class _ValuePolicy = SdfIdentityMapEditProxyValuePolicy<T> >
class SdfMapEditProxy :
    boost::totally_ordered<SdfMapEditProxy<T, _ValuePolicy>, T> {
public:
    typedef T Type;
    typedef _ValuePolicy ValuePolicy;
    typedef SdfMapEditProxy<Type, ValuePolicy> This;
    typedef typename Type::key_type key_type;
    typedef typename Type::mapped_type mapped_type;
    typedef typename Type::value_type value_type;

private:
    // Note:  We're playing a dangerous game with copy-on-write and
    //        iterators.  Our iterators wrap iterators on the proxied
    //        Type.  When and if we copy-on-write then all of our
    //        existing iterators use the old proxied object's iterators
    //        and any new iterators will use the new proxied object's
    //        iterators.  Therefore old and new iterators are no longer
    //        compatible and an old and new iterator that refer to the
    //        same key will not compare equal.
    //
    //        It turns out that this is okay because we don't promise
    //        to keep iterators valid across an edit.  However, we'd
    //        like dereferencing an old iterator to either report an
    //        error or yield the current state.  We can do that by
    //        storing the Type* in use when the iterator was created
    //        and comparing it to the current Type*.  If they're
    //        different we need to report an error or use the key from
    //        the old iterator and lookup that key in the new Type*.
    //        We currently choose to return the current state.

    typedef typename Type::iterator inner_iterator;
    typedef typename Type::const_iterator const_inner_iterator;

    class _ValueProxy {
    public:
        _ValueProxy(This* owner, const Type* data, inner_iterator i) :
            _owner(owner), _data(data), _pos(i)
        {
            // Do nothing
        }

        template <class U>
        _ValueProxy& operator=(const U& other)
        {
            if (!_owner) {
                TF_CODING_ERROR("Assignment to invalid map proxy");
            } else {
                _owner->_Set(_data, _pos, other);
            }
            return *this;
        }

        operator mapped_type() const
        {
            return Get();
        }

        // Required for _PairProxy::operator value_type().
        mapped_type Get() const
        {
            if (!_owner) {
                TF_CODING_ERROR("Read from invalid map proxy");
                return mapped_type();
            }
            return _owner->_Get(_data, _pos);
        }

    private:
        This* _owner;
        const Type* _data;
        inner_iterator _pos;
    };

    class _PairProxy : boost::totally_ordered<_PairProxy> {
    public:
        explicit _PairProxy(This* owner, const Type* data, inner_iterator i) :
                        first(i->first), second(_ValueProxy(owner, data, i)) { }

        const key_type first;
        _ValueProxy second;

        operator value_type() const
        {
            // Note that we cannot simply use 'second' or we'll use the
            // mapped_type c'tor instead of the _ValueProxy implicit
            // conversion if one is available.  If mapped_type is VtValue
            // then we'll type erase _ValueProxy instead of just getting
            // the VtValue out of the _ValueProxy.
            return value_type(first, second.Get());
        }
    };

    class Traits {
    public:
        static _PairProxy Dereference(This* owner,
                                      const Type* data, inner_iterator i)
        {
            if (!owner) {
                TF_FATAL_ERROR("Dereferenced an invalid map proxy iterator");
            }
            return _PairProxy(owner, data, i);
        }

        static const value_type& Dereference(const This* owner,
                                             const Type* data,
                                             const_inner_iterator i)
        {
            if (!owner) {
                TF_FATAL_ERROR("Dereferenced an invalid map proxy iterator");
            }
            return owner->_Get(data, i);
        }
    };

    template <class Owner, class I, class R>
    class _Iterator :
        public boost::iterator_facade<_Iterator<Owner, I, R>, R,
                                      std::bidirectional_iterator_tag, R> {
    public:
        _Iterator() :
            _owner(NULL), _data(NULL) { }

        _Iterator(Owner owner, const Type* data, I i) :
            _owner(owner), _data(data), _pos(i)
        {
            // Do nothing
        }

        template <class Owner2, class I2, class R2>
        _Iterator(const _Iterator<Owner2, I2, R2>& other) :
            _owner(other._owner), _data(other._data), _pos(other._pos)
        {
            // Do nothing
        }

        const I& base() const
        {
            return _pos;
        }

    private:
        R dereference() const
        {
            return Traits::Dereference(_owner, _data, _pos);
        }

        template <class Owner2, class I2, class R2>
        bool equal(const _Iterator<Owner2, I2, R2>& other) const
        {
            if (_owner == other._owner && _pos == other._pos) {
                return true;
            }
            else {
                // All iterators at the end compare equal.
                return atEnd() && other.atEnd();
            }
        }

        void increment() {
            ++_pos;
        }

        void decrement() {
            --_pos;
        }

        bool atEnd() const {
            // We consider an iterator with no owner to be at the end.
            return !_owner || _pos == _owner->_ConstData()->end();
        }

    private:
        Owner _owner;
        const Type* _data;
        I _pos;

        friend class boost::iterator_core_access;
        template <class Owner2, class I2, class R2> friend class _Iterator;
    };

public:
    typedef _ValueProxy reference;
    typedef const value_type& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef _Iterator<This*, inner_iterator, _PairProxy> iterator;
    typedef _Iterator<const This*, const_inner_iterator,
                                   const value_type&> const_iterator;
    typedef boost::reverse_iterator<iterator> reverse_iterator;
    typedef boost::reverse_iterator<const_iterator> const_reverse_iterator;

    explicit SdfMapEditProxy(const SdfSpecHandle& owner, const TfToken& field) :
        _editor(Sdf_CreateMapEditor<T>(owner, field))
    {
        // Do nothing
    }

    SdfMapEditProxy()
    {
        // Do nothing
    }

    This& operator=(const This& other)
    {
        if (other._Validate()) {
            _Copy(*other._ConstData());
        }
        return *this;
    }

    template <class U, class UVP>
    This& operator=(const SdfMapEditProxy<U, UVP>& other)
    {
        if (other._Validate()) {
            _Copy(Type(other._ConstData()->begin(), other._ConstData()->end()));
        }
        return *this;
    }

    This& operator=(const Type& data)
    {
        _Copy(data);
        return *this;
    }

    /// Returns a copy of the value.
    operator Type() const
    {
        return _Validate() ? *_ConstData() : Type();
    }

    iterator begin()
    {
        return _Validate() ? iterator(this, _Data(), _Data()->begin()) :
                             iterator();
    }
    iterator end()
    {
        return _Validate() ? iterator(this, _Data(), _Data()->end()) :
                             iterator();
    }
    const_iterator begin() const
    {
        return _Validate() ?
                    const_iterator(this, _ConstData(), _ConstData()->begin()) :
                    const_iterator();
    }
    const_iterator end() const
    {
        return _Validate() ?
                    const_iterator(this, _ConstData(), _ConstData()->end()) :
                    const_iterator();
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }
    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }
    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(begin());
    }

    size_type size() const
    {
        return _Validate() ? _ConstData()->size() : 0;
    }

    size_type max_size() const
    {
        return _Validate() ? _ConstData()->max_size() : 0;
    }

    bool empty() const
    {
        return _Validate() ? _ConstData()->empty() : true;
    }

    std::pair<iterator, bool> insert(const value_type& value)
    {
        return _Insert(value);
    }

    iterator insert(iterator pos, const value_type& value)
    {
        return _Insert(value).first;
    }

    template <class InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        if (_Validate()) {
            SdfChangeBlock block;
            for (; first != last; ++first) {
                const value_type& v = 
                    ValuePolicy::CanonicalizePair(_Owner(), *first);

                if (_ValidateInsert(v)) {
                    _editor->Insert(v);
                }
            }
        }
    }

    void erase(iterator pos)
    {
        if (_Validate() && _ValidateErase(pos->first)) {
            _Erase(pos->first);
        }
    }

    size_type erase(const key_type& key)
    {
        if (_Validate()) {
            const key_type& k = ValuePolicy::CanonicalizeKey(_Owner(), key);
            if (_ValidateErase(k)) {
                return _editor->Erase(k) ? 1 : 0;
            }
        }
        return 0;
    }

    void erase(iterator first, iterator last)
    {
        if (_Validate()) {
            SdfChangeBlock block;
            while (first != last) {
                const key_type& key = first->first;
                ++first;
                if (_ValidateErase(key)) {
                    _editor->Erase(key);
                }
            }
        }
    }

    void clear()
    {
        _Copy(Type());
    }

    iterator find(const key_type& key)
    {
        return
            _Validate() ?
                iterator(this, _Data(),
                    _Data()->find(ValuePolicy::CanonicalizeKey(_Owner(), key))) :
                iterator();
    }

    const_iterator find(const key_type& key) const
    {
        return
            _Validate() ?
                const_iterator(this, _ConstData(),
                    _ConstData()->find(
                        ValuePolicy::CanonicalizeKey(_Owner(), key))) :
                const_iterator();
    }

    size_type count(const key_type& key) const
    {
        return
            _Validate() ?
                _ConstData()->count(
                    ValuePolicy::CanonicalizeKey(_Owner(), key)) :
                0;
    }

    iterator lower_bound(const key_type& key)
    {
        return
            _Validate() ?
                iterator(this, _Data(),
                    _Data()->lower_bound(
                        ValuePolicy::CanonicalizeKey(_Owner(), key))) :
                iterator();
    }

    const_iterator lower_bound(const key_type& key) const
    {
        return
            _Validate() ?
                const_iterator(this, _ConstData(),
                    _ConstData()->lower_bound(
                        ValuePolicy::CanonicalizeKey(_Owner(), key))) :
                const_iterator();
    }

    iterator upper_bound(const key_type& key)
    {
        return
            _Validate() ?
                iterator(this, _Data(),
                    _Data()->upper_bound(
                        ValuePolicy::CanonicalizeKey(_Owner(), key))) :
                iterator();
    }

    const_iterator upper_bound(const key_type& key) const
    {
        return
            _Validate() ?
                const_iterator(this, _ConstData(),
                    _ConstData()->upper_bound(
                        ValuePolicy::CanonicalizeKey(_Owner(), key))) :
                const_iterator();
    }

    std::pair<iterator, iterator> equal_range(const key_type& key)
    {
        if (_Validate()) {
            std::pair<inner_iterator, inner_iterator> result =
                _Data()->equal_range(
                    ValuePolicy::CanonicalizeKey(_Owner(), key));
            return std::make_pair(iterator(this, _Data(), result.first),
                                  iterator(this, _Data(), result.second));
        }
        else {
            return std::make_pair(iterator(), iterator());
        }
    }

    std::pair<const_iterator,const_iterator>
    equal_range(const key_type& key) const
    {
        if (_Validate()) {
            std::pair<const_inner_iterator, const_inner_iterator> result =
                _ConstData()->equal_range(
                    ValuePolicy::CanonicalizeKey(_Owner(), key));
            return std::make_pair(
                        const_iterator(this, _ConstData(), result.first),
                        const_iterator(this, _ConstData(), result.second));
        }
        else {
            return std::make_pair(const_iterator(), const_iterator());
        }
    }

    reference operator[](const key_type& key)
    {
        auto iter = _Insert(value_type(key, mapped_type())).first;
        bool failed = iter == iterator();
        return reference(failed ? nullptr : this,
                         failed ? nullptr : _Data(),
                         iter.base());
    }

    bool operator==(const Type& other) const
    {
        return _Validate() ? _CompareEqual(other) : false;
    }

    bool operator<(const Type& other) const
    {
        return _Validate() ? _Compare(other) < 0 : false;
    }

    bool operator>(const Type& other) const
    {
        return _Validate() ? _Compare(other) > 0 : false;
    }

    template <class U, class UVP>
    bool operator==(const SdfMapEditProxy<U, UVP>& other) const
    {
        return _Validate() && other._Validate() ?
                    _CompareEqual(*other._ConstData()) : false;
    }

    template <class U, class UVP>
    bool operator!=(const SdfMapEditProxy<U, UVP>& other) const
    {
        return !(*this == other);
    }

    template <class U, class UVP>
    bool operator<(const SdfMapEditProxy<U, UVP>& other) const
    {
        return _Validate() && other._Validate() ?
                    _Compare(*other._ConstData()) < 0 : false;
    }

    template <class U, class UVP>
    bool operator<=(const SdfMapEditProxy<U, UVP>& other) const
    {
        return _Validate() && other._Validate() ?
                    _Compare(*other._ConstData()) <= 0 : false;
    }

    template <class U, class UVP>
    bool operator>(const SdfMapEditProxy<U, UVP>& other) const
    {
        return !(*this <= other);
    }

    template <class U, class UVP>
    bool operator>=(const SdfMapEditProxy<U, UVP>& other) const
    {
        return !(*this < other);
    }

    /// Returns true if the value is expired. Note this a default-constructed
    /// MapEditProxy is considered to be invalid but *not* expired.
    bool IsExpired() const
    {
        return _editor && _editor->IsExpired();
    }

#if !defined(doxygen)
    typedef std::shared_ptr<Sdf_MapEditor<Type> > This::*UnspecifiedBoolType;
#endif

    /// Returns \c true in a boolean context if the value is valid,
    /// \c false otherwise.
    operator UnspecifiedBoolType() const
    {
        return _ConstData() && !IsExpired() ? &This::_editor : NULL;
    }
    /// Returns \c false in a boolean context if the value is valid,
    /// \c true otherwise.
    bool operator!() const
    {
        return !_ConstData() || IsExpired();
    }

private:
    bool _Validate()
    {
        if (_ConstData() && !IsExpired()) {
            return true;
        }
        else {
            TF_CODING_ERROR("Editing an invalid map proxy");
            return false;
        }
    }

    bool _Validate() const
    {
        if (_ConstData() && !IsExpired()) {
            return true;
        }
        else {
            TF_CODING_ERROR("Accessing an invalid map proxy");
            return false;
        }
    }

    Type* _Data()
    {
        return _editor ? _editor->GetData() : NULL;
    }

    const Type* _ConstData() const
    {
        return _editor ? _editor->GetData() : NULL;
    }

    SdfSpecHandle _Owner() const
    {
        return _editor ? _editor->GetOwner() : SdfSpecHandle();
    }

    std::string _Location() const
    {
        return _editor ? _editor->GetLocation() : std::string();
    }

    bool _CompareEqual(const Type& other) const
    {
        if (_ConstData()->size() < other.size()) {
            return false;
        }
        if (_ConstData()->size() > other.size()) {
            return false;
        }

        // Same size -- find the first mismatch.
        const Type& x = ValuePolicy::CanonicalizeType(_Owner(), other);
        std::pair<const_inner_iterator, const_inner_iterator> result =
            std::mismatch(_ConstData()->begin(), _ConstData()->end(),
                          x.begin());
        return result.first == _ConstData()->end();
    }

    int _Compare(const Type& other) const
    {
        if (_ConstData()->size() < other.size()) {
            return -1;
        }
        if (_ConstData()->size() > other.size()) {
            return 1;
        }

        // Same size -- find the first mismatch.
        const Type& x = ValuePolicy::CanonicalizeType(_Owner(), other);
        std::pair<const_inner_iterator, const_inner_iterator> result =
            std::mismatch(_ConstData()->begin(), _ConstData()->end(),
                          x.begin());
        if (*result.first < *result.second) {
            return -1;
        }
        else if (*result.first > *result.second) {
            return 1;
        }
        else {
            return 0;
        }
    }

    template <class D>
    bool _CompareEqual(const D& other) const
    {
        // This is expensive but yields reliable results.
        return _CompareEqual(Type(other.begin(), other.end()));
    }

    template <class D>
    int _Compare(const D& other) const
    {
        // This is expensive but yields reliable ordering.
        return _Compare(Type(other.begin(), other.end()));
    }

    mapped_type _Get(const Type* data, const inner_iterator& i)
    {
        if (_Validate()) {
            if (data == _ConstData()) {
                return i->second;
            }
            else {
                // Data has changed since we created the iterator.
                // Look up same key in new data.
                return _ConstData()->find(i->first)->second;
            }
        }
        return mapped_type();
    }

    const value_type& _Get(const Type* data,
                           const const_inner_iterator& i) const
    {
        // If data has changed since we created the iterator then look up
        // the same key in the new data.
        return (data == _ConstData()) ? *i : *_ConstData()->find(i->first);
    }

    void _Copy(const Type& other)
    {
        if (_Validate()) {
            // Canonicalize the given map before copying it into ourselves.
            // If multiple keys in the given map would conflict with each
            // other in the canonicalized map, we consider this an error.
            // This is primarily to avoid confusing the consumer, who would
            // otherwise observe a key/value pair to be missing entirely.
            Type canonicalOther;
            TF_FOR_ALL(it, other) {
                const value_type canonicalValue = 
                    ValuePolicy::CanonicalizePair(_Owner(), *it);
                if (!canonicalOther.insert(canonicalValue).second) {
                    TF_CODING_ERROR("Can't copy to %s: Duplicate key '%s' "
                                    "exists in map.",
                                    _Location().c_str(),
                                    TfStringify(canonicalValue.first).c_str());
                    return;
                }
            }

            if (_ValidateCopy(canonicalOther)) {
                _editor->Copy(canonicalOther);
            }
        }
    }

    bool _ValidateCopy(const Type& other)
    {
        SdfSpecHandle owner = _Owner();
        if (owner && !owner->PermissionToEdit()) {
            TF_CODING_ERROR("Can't copy to %s: Permission denied.",
                            _Location().c_str());
            return false;
        }

        if (other.empty()) {
            return true;
        }

        TF_FOR_ALL(it, other) {
            if (!_ValidateInsert(*it)) {
                return false;
            }
        }

        return true;
    }

    template <class U>
    void _Set(const Type* data, const inner_iterator& i, const U& value)
    {
        if (_Validate()) {
            const mapped_type& x =
                ValuePolicy::CanonicalizeValue(_Owner(), value);
            if (_ValidateSet(i->first, x)) {
                _editor->Set(i->first, x);
            }
        }
    }

    bool _ValidateSet(const key_type& key, const mapped_type& value)
    {
        SdfSpecHandle owner = _Owner();
        if (owner && !owner->PermissionToEdit()) {
            TF_CODING_ERROR("Can't set value in %s: Permission denied.",
                            _Location().c_str());
            return false;
        }

        if (SdfAllowed allowed = _editor->IsValidValue(value)) {
            // Do nothing
        }
        else {
            TF_CODING_ERROR("Can't set value in %s: %s",
                            _Location().c_str(),
                            allowed.GetWhyNot().c_str());
            return false;
        }

        return true;
    }

    std::pair<iterator, bool> _Insert(const value_type& value)
    {
        if (_Validate()) {
            const value_type& v = ValuePolicy::CanonicalizePair(_Owner(), value);
            if (_ValidateInsert(v)) {
                std::pair<inner_iterator, bool> status = _editor->Insert(v);
                return std::make_pair(iterator(this, _Data(), status.first),
                                      status.second);
            }
            else {
                return std::make_pair(iterator(), false);
            }
        }
        return std::make_pair(iterator(), false);
    }

    bool _ValidateInsert(const value_type& value)
    {
        SdfSpecHandle owner = _Owner();
        if (owner && !owner->PermissionToEdit()) {
            TF_CODING_ERROR("Can't insert value in %s: Permission denied.",
                            _Location().c_str());
            return false;
        }

        if (SdfAllowed allowed = _editor->IsValidKey(value.first)) {
            // Do nothing
        }
        else {
            TF_CODING_ERROR("Can't insert key in %s: %s",
                            _Location().c_str(),
                            allowed.GetWhyNot().c_str());
            return false;
        }

        if (SdfAllowed allowed = _editor->IsValidValue(value.second)) {
            // Do nothing
        }
        else {
            TF_CODING_ERROR("Can't insert value in %s: %s",
                            _Location().c_str(),
                            allowed.GetWhyNot().c_str());
            return false;
        }

        return true;
    }

    void _Erase(const key_type& key)
    {
        if (_Validate() && _ValidateErase(key)) {
            _editor->Erase(key);
        }
    }

    bool _ValidateErase(const key_type& key)
    {
        SdfSpecHandle owner = _Owner();
        if (owner && !owner->PermissionToEdit()) {
            TF_CODING_ERROR("Can't erase value from %s: Permission denied.",
                            _Location().c_str());
            return false;
        }

        return true;
    }

private:
    template <class ProxyT> friend class SdfPyWrapMapEditProxy;

    std::shared_ptr<Sdf_MapEditor<T> > _editor;
};

// Cannot get from a VtValue except as the correct type.
template <class T, class _ValuePolicy>
struct Vt_DefaultValueFactory<SdfMapEditProxy<T, _ValuePolicy> > {
    static Vt_DefaultValueHolder Invoke() {
        TF_AXIOM(false && "Failed VtValue::Get<SdfMapEditProxy> not allowed");
        return Vt_DefaultValueHolder::Create((void*)0);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_MAPEDITPROXY_H
