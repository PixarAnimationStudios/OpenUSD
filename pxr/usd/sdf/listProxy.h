//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_LIST_PROXY_H
#define PXR_USD_SDF_LIST_PROXY_H

/// \file sdf/listProxy.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/allowed.h"
#include "pxr/usd/sdf/listEditor.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/iterator.h"

#include <memory>
#include <optional>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfListProxy
///
/// Represents a single list of list editing operations.
///
/// An SdfListProxy represents a single list of list editing operations, making 
/// it look like an STL vector (modeling a random access container and back
/// insertion sequence). 
///
template <class _TypePolicy>
class SdfListProxy {
public:
    typedef _TypePolicy TypePolicy;
    typedef SdfListProxy<TypePolicy> This;
    typedef typename TypePolicy::value_type value_type;
    typedef std::vector<value_type> value_vector_type;

    /// Returned from \ref Find when a value could not be located in the
    /// list of operations. 
    static const size_t invalidIndex = -1;

private:
    // Proxies an item in a list editor list.
    class _ItemProxy {
    public:
        explicit _ItemProxy(This* owner, size_t index) :
            _owner(owner), _index(index)
        {
            // Do nothing
        }

        _ItemProxy& operator=(const _ItemProxy& x) {
            _owner->_Edit(_index, 1, value_vector_type(1, x));
            return *this;
        }

        _ItemProxy& operator=(const value_type& x) {
            _owner->_Edit(_index, 1, value_vector_type(1, x));
            return *this;
        }

        operator value_type() const {
            return _owner->_Get(_index);
        }

        // Operators rely on implicit conversion to value_type
        // for comparing two _ItemProxy instances
        bool operator==(const value_type& x) const {
            return _owner->_Get(_index) == x;
        }

        bool operator!=(const value_type& x) const {
            return !(*this == x);
        }

        bool operator<(const value_type& x) const {
            return _owner->_Get(_index) < x;
        }

        bool operator>(const value_type& x) const {
            return x < value_type(*this);
        }

        bool operator>=(const value_type& x) const {
            return !(*this < x);
        }

        bool operator<=(const value_type& x) const {
            return !(x < value_type(*this));
        }

    private:
        This* _owner;
        size_t _index;
    };
    friend class _ItemProxy;

    class _GetHelper {
    public:
        typedef _ItemProxy result_type;

        result_type operator()(This* owner, size_t index) const {
            return _ItemProxy(owner, index);
        }
    };
    class _ConstGetHelper {
    public:
        typedef value_type result_type;

        result_type operator()(const This* owner, size_t index) const {
            return owner->_Get(index);
        }
    };
    friend class _GetHelper;
    friend class _ConstGetHelper;

    template <class Owner, class GetItem>
    class _Iterator {
        class _PtrProxy {
        public:
            std::add_pointer_t<typename GetItem::result_type> operator->() {
                return std::addressof(_result);
            }
        private:
            friend class _Iterator;
            explicit _PtrProxy(
                std::add_const_t<
                    std::add_lvalue_reference_t<
                        typename GetItem::result_type>
                > result) : _result(result) {}
            typename GetItem::result_type _result;
        };
    public:
        using This = _Iterator<Owner, GetItem>;
        using iterator_category = std::random_access_iterator_tag;
        using value_type = std::remove_cv_t<
                std::remove_reference_t<
                    typename GetItem::result_type
                >
            >;
        using reference = typename GetItem::result_type;
        using pointer = _PtrProxy;
        using difference_type = std::ptrdiff_t;

        static_assert(!std::is_lvalue_reference<reference>::value,
                      "reference is an lvalue_reference and usage of "
                      "this class unnecessarily instantiates a _PtrProxy.");

        _Iterator() = default;

        _Iterator(Owner owner, size_t index) : _owner(owner), _index(index)
        {
            // Do nothing
        }

        reference operator*() const { return dereference(); }
        pointer operator->() const { return pointer(dereference()); }
        reference operator[](const difference_type index) const {
            This advanced(*this);
            advanced.advance(index);
            return advanced.dereference();
        }

        difference_type operator-(const This& other) const {
            return -distance_to(other);
        }

        This& operator++() {
            increment();
            return *this;
        }

        This& operator--() {
            decrement();
            return *this;
        }

        This operator++(int) {
            This result(*this);
            increment();
            return result;
        }

        This operator--(int) {
            This result(*this);
            decrement();
            return result;
        }

        This operator+(const difference_type increment) const {
            This result(*this);
            result.advance(increment);
            return result;
        }

        This operator-(const difference_type decrement) const {
            This result(*this);
            result.advance(-decrement);
            return result;
        }

        This& operator+=(const difference_type increment) {
            advance(increment);
            return *this;
        }

        This& operator-=(const difference_type decrement) {
            advance(-decrement);
            return *this;
        }

        bool operator==(const This& other) const {
            return equal(other);
        }

        bool operator!=(const This& other) const {
            return !equal(other);
        }

        bool operator<(const This& other) const {
            TF_DEV_AXIOM(_owner == other._owner);
            return _index < other._index;
        }

        bool operator<=(const This& other) const {
            TF_DEV_AXIOM(_owner == other._owner);
            return _index <= other._index;
        }

        bool operator>(const This& other) const {
            TF_DEV_AXIOM(_owner == other._owner);
            return _index > other._index;
        }

        bool operator>=(const This& other) const {
            TF_DEV_AXIOM(_owner == other._owner);
            return _index >= other._index;
        }

    private:

        reference dereference() const {
            return _getItem(_owner, _index);
        }

        bool equal(const This& other) const {
            if (_owner != other._owner) {
                TF_CODING_ERROR("Comparing SdfListProxy iterators from "
                                "different proxies!");
                return false;
            }
            return _index == other._index;
        }

        void increment() {
            ++_index;
        }

        void decrement() {
            --_index;
        }

        void advance(difference_type n) {
            _index += n;
        }

        difference_type distance_to(const This& other) const {
            return other._index - _index;
        }

    private:
        GetItem _getItem;
        Owner _owner = nullptr;
        size_t _index = 0;
    };

public:
    typedef _ItemProxy reference;
    typedef _Iterator<This*, _GetHelper> iterator;
    typedef _Iterator<const This*, _ConstGetHelper> const_iterator;
    typedef Tf_ProxyReferenceReverseIterator<iterator> reverse_iterator;
    typedef Tf_ProxyReferenceReverseIterator<const_iterator> const_reverse_iterator;

    /// Creates a default list proxy object for list operation vector specified
    /// \p op. This object evaluates to false in a boolean context and all
    /// operations on this object have no effect.
    SdfListProxy(SdfListOpType op) :
        _op(op)
    {
    }

    /// Create a new proxy wrapping the list operation vector specified by
    /// \p op in the underlying \p listEditor.
    SdfListProxy(const std::shared_ptr<Sdf_ListEditor<TypePolicy> >& editor,
                SdfListOpType op) :
        _listEditor(editor),
        _op(op)
    {
    }

    /// Return an iterator to the start of the sequence.
    iterator begin() {
        return iterator(_GetThis(), 0);
    }
    /// Return an iterator to the end of the sequence.
    iterator end() {
        return iterator(_GetThis(), _GetSize());
    }

    /// Return a reverse iterator to the last item of the sequence.
    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }
    /// Return a reverse iterator past the start item of the sequence.
    reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    /// Return a const iterator to the start of the sequence.
    const_iterator begin() const {
        return const_iterator(_GetThis(), 0);
    }
    /// Return a const iterator to the end of the sequence.
    const_iterator end() const {
        return const_iterator(_GetThis(), _GetSize());
    }

    /// Return a const reverse iterator to the last item of the sequence.
    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }
    /// Return a const reverse iterator past the start item of the
    /// sequence.
    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    /// Return the size of the sequence.
    size_t size() const {
        return _Validate() ? _GetSize() : 0;
    }

    /// Return true if size() == 0.
    bool empty() const { 
        return size() == 0;
    }

    /// Return a \p reference to the item at index \p n.
    reference operator[](size_t n) {
        return reference(_GetThis(), n);
    }

    /// Return a copy of the item at index \p n.
    value_type operator[](size_t n) const {
        return _Get(n);
    }

    /// Return a \p reference to the item at the front of the sequence.
    reference front() {
        return reference(_GetThis(), 0);
    }

    /// Return a \p reference to the item at the back of the sequence.
    reference back() {
        return reference(_GetThis(), _GetSize() - 1);
    }

    /// Return a copy of the item at the front of the sequence.
    value_type front() const {
        return _Get(0);
    }

    /// Return a copy of the item at the back of the sequence.
    value_type back() const {
        return _Get(_GetSize() - 1);
    }

    /// Append \p elem to this sequence.
    void push_back(const value_type& elem) {
        _Edit(_GetSize(), 0, value_vector_type(1, elem));
    }

    /// Remove the last element from this sequence.
    void pop_back() {
        _Edit(_GetSize() - 1, 1, value_vector_type());
    }

    /// Insert \p x into this sequence at position \p pos.
    iterator insert(iterator pos, const value_type& x) {
        _Edit(pos - iterator(this, 0), 0, value_vector_type(1, x));
        return pos;
    }

    /// Insert copies of the elements in [\p f, \p l) into this sequence
    /// starting at position \p pos.
    template <class InputIterator>
    void insert(iterator pos, InputIterator f, InputIterator l) {
        _Edit(pos - iterator(this, 0), 0, value_vector_type(f, l));
    }

    /// Erase the element at \p pos.
    void erase(iterator pos) {
        _Edit(pos - iterator(this, 0), 1, value_vector_type());
    }

    /// Erase all the elements in the range [\p f, \p l).
    void erase(iterator f, iterator l) {
        _Edit(f - iterator(this, 0), l - f, value_vector_type());
    }

    /// Clear the contents of the sequence.
    void clear() {
        _Edit(0, _GetSize(), value_vector_type());
    }

    /// Resize the contents of the sequence. 
    ///
    /// Inserts or erases copies of \p t at the end
    /// such that the size becomes \p n.
    void resize(size_t n, const value_type& t = value_type()) {
        size_t s = _GetSize();
        if (n > s) {
            _Edit(s, 0, value_vector_type(n - s, t));
        }
        else if (n < s) {
            _Edit(n, s - n, value_vector_type());
        }
    }

    /// Produce a copy of the contents of this sequence into a vector.
    operator value_vector_type() const {
        return _listEditor ? _listEditor->GetVector(_op) : value_vector_type();
    }

    /// Replace all elements in this sequence with the elements in
    /// the \p other sequence.
    template <class T2>
    This& operator=(const SdfListProxy<T2>& other) {
        _Edit(0, _GetSize(), static_cast<value_vector_type>(other));
        return *this;
    }

    /// Replace all elements in this sequence with the given vector.
    This& operator=(const value_vector_type& other) {
        _Edit(0, _GetSize(), other);
        return *this;
    }

    /// Replace all elements in this sequence with the given vector.
    template <class Y>
    This& operator=(const std::vector<Y>& v) {
        _Edit(0, _GetSize(), value_vector_type(v.begin(), v.end()));
        return *this;
    }

    /// Equality comparison.
    template <class T2>
    bool operator==(const SdfListProxy<T2>& y) const {
        return value_vector_type(*this) == value_vector_type(y);
    }

    /// Inequality comparison.
    template <class T2>
    bool operator!=(const SdfListProxy<T2>& y) const {
        return !(*this == y);
    }

    /// Less-than comparison.
    template <class T2>
    bool operator<(const SdfListProxy<T2>& y) const {
        return value_vector_type(*this) < value_vector_type(y);
    }

    /// Less-than-or-equal comparison.
    template <class T2>
    bool operator<=(const SdfListProxy<T2>& y) const {
        return value_vector_type(*this) <= value_vector_type(y);
    }

    /// Greater-than comparison.
    template <class T2>
    bool operator>(const SdfListProxy<T2>& y) const {
        return !(*this <= y);
    }

    /// Greater-than-or-equal comparison.
    template <class T2>
    bool operator>=(const SdfListProxy<T2>& y) const {
        return !(*this < y);
    }

    /// Equality comparison.
    bool operator==(const value_vector_type& y) const {
        return value_vector_type(*this) == y;
    }

    /// Equality comparision
    friend bool operator==(const value_vector_type& x, const SdfListProxy& y) {
        return y == x;
    }

    /// Inequality comparison.
    bool operator!=(const value_vector_type& y) const {
        return !(*this == y);
    }

    /// Inequality comparision
    friend bool operator!=(const value_vector_type& x, const SdfListProxy& y) {
        return y != x;
    }

    /// Less-than comparison.
    bool operator<(const value_vector_type& y) const {
        return value_vector_type(*this) < y;
    }

    /// Less-than comparison
    friend bool operator<(const value_vector_type& x, const SdfListProxy& y) {
        return x < value_vector_type(y);
    }

    /// Greater-than comparison.
    bool operator>(const value_vector_type& y) const {
        return value_vector_type(*this) > y;
    }

    /// Greater-than comparison.
    friend bool operator>(const value_vector_type& x, const SdfListProxy& y) {
        return x > value_vector_type(y);
    }

    /// Less-than or equal to comparison.
    bool operator<=(const value_vector_type& y) const {
        return !(*this > y);
    }

    /// Less-than or equal to comparison.
    friend bool operator<=(const value_vector_type& x, const SdfListProxy& y) {
        return x <= value_vector_type(y);
    }

    /// Greater-than or equal to comparison.
    bool operator>=(const value_vector_type& y) const {
        return !(*this < y);
    }

    /// Greater-than or equal to comparison.
    friend bool operator>=(const value_vector_type& x, const SdfListProxy& y) {
        return x >= value_vector_type(y);
    }

    /// Explicit bool conversion operator. The list proxy object converts to 
    /// \c true if the list editor is valid, \c false otherwise.
    explicit operator bool() const
    {
        return _listEditor && _listEditor->IsValid() && _IsRelevant();
    }

    // Extensions

    /// Returns the layer that this list editor belongs to.
    SdfLayerHandle GetLayer() const
    {
        return _listEditor ? _listEditor->GetLayer() : SdfLayerHandle();
    }

    /// Returns the path to this list editor's value.
    SdfPath GetPath() const
    {
        return _listEditor ? _listEditor->GetPath() : SdfPath();
    }

    /// Returns true if the list editor is expired.
    bool IsExpired() const
    {
        return _listEditor && _listEditor->IsExpired();
    }

    size_t Count(const value_type& value) const
    {
        return (_Validate() ? _listEditor->Count(_op, value) : 0);
    }

    /// Returns the index of \p value in the list of operations.  If \p value
    /// is not found, then \ref invalidIndex is returned instead.
    size_t Find(const value_type& value) const
    {
        return (_Validate() ? _listEditor->Find(_op, value) : invalidIndex);
    }

    void Insert(int index, const value_type& value)
    {
        if (index == -1) {
            index = static_cast<int>(_GetSize());
        }
        _Edit(index, 0, value_vector_type(1, value));
    }

    void Remove(const value_type& value)
    {
        size_t index = Find(value);
        if (index != invalidIndex) {
            Erase(index);
        }
        else {
            // Allow policy to raise an error even though we're not
            // doing anything.
            _Edit(_GetSize(), 0, value_vector_type());
        }
    }

    void Replace(const value_type& oldValue, const value_type& newValue)
    {
        size_t index = Find(oldValue);
        if (index != size_t(-1)) {
            _Edit(index, 1, value_vector_type(1, newValue));
        }
        else {
            // Allow policy to raise an error even though we're not
            // doing anything.
            _Edit(_GetSize(), 0, value_vector_type());
        }
    }

    void Erase(size_t index)
    {
        _Edit(index, 1, value_vector_type());
    }

    /// Applies the edits in the given list to this one.
    void ApplyList(const SdfListProxy &list) 
    {
        if (_Validate() && list._Validate()) {
            _listEditor->ApplyList(_op, *list._listEditor);
        }
    }

    /// Apply the edits in this list to the given \p vec.
    void ApplyEditsToList(value_vector_type* vec)
    {
        if (_Validate()) {
            _listEditor->ApplyEditsToList(vec);
        }
    }

    /// Modify all edits in this list. 
    ///
    /// \p callback must be a callable that accepts an argument of type
    /// value_type and returns a std::optional<value_type>.
    ///
    /// \p callback is called with every item in the list. If an invalid
    /// std::optional is returned, the item is removed. Otherwise it's
    /// replaced with the returned item. If a returned item matches an
    /// item that was previously returned, the returned item will be
    /// removed.
    template <class CB>
    void ModifyItemEdits(CB callback)
    {
        if (_Validate()) {
            _listEditor->ModifyItemEdits(std::forward<CB>(callback));
        }
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

    This* _GetThis()
    {
        return _Validate() ? this : NULL;
    }

    const This* _GetThis() const
    {
        return _Validate() ? this : NULL;
    }

    bool _IsRelevant() const
    {
        if (_listEditor->IsExplicit()) {
            return _op == SdfListOpTypeExplicit;
        }
        else if (_listEditor->IsOrderedOnly()) {
            return _op == SdfListOpTypeOrdered;
        }
        else {
            return _op != SdfListOpTypeExplicit;
        }
    }

    size_t _GetSize() const
    {
        return _listEditor ? _listEditor->GetSize(_op) : 0;
    }

    value_type _Get(size_t n) const
    {
        return _Validate() ? _listEditor->Get(_op, n) : value_type();
    }

    void _Edit(size_t index, size_t n, const value_vector_type& elems)
    {
        if (_Validate()) {
            // Allow policy to raise an error even if we're not
            // doing anything.
            if (n == 0 && elems.empty()) {
                SdfAllowed canEdit = _listEditor->PermissionToEdit(_op);
                if (!canEdit) {
                    TF_CODING_ERROR("Editing list: %s", 
                                    canEdit.GetWhyNot().c_str());
                }
                return;
            }

            bool valid = 
                _listEditor->ReplaceEdits(_op, index, n, elems);
            if (!valid) {
                TF_CODING_ERROR("Inserting invalid value into list editor");
            }
        }
    }

private:
    std::shared_ptr<Sdf_ListEditor<TypePolicy> > _listEditor;
    SdfListOpType _op;

    template <class> friend class SdfPyWrapListProxy;
};

// Allow TfIteration over list proxies.
template <typename T>
struct Tf_ShouldIterateOverCopy<SdfListProxy<T> > : std::true_type
{
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_LIST_PROXY_H
