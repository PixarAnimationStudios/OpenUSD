/*
    nanobind/nb_accessor.h: Accessor helper class for .attr(), operator[]

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

#define NB_DECL_ACCESSOR_OP_I(name)                                            \
    template <typename T> accessor& name(const api<T> &o);

#define NB_IMPL_ACCESSOR_OP_I(name, op)                                        \
    template <typename Impl> template <typename T>                             \
    accessor<Impl>& accessor<Impl>::name(const api<T> &o) {                    \
        object tmp = steal(obj_op_2(ptr(), o.derived().ptr(), op));            \
        Impl::set(m_base, m_key, tmp.ptr());                                   \
        return *this;                                                          \
    }

template <typename Impl> class accessor : public api<accessor<Impl>> {
    template <typename T> friend void nanobind::del(accessor<T> &);
    template <typename T> friend void nanobind::del(accessor<T> &&);
public:
    static constexpr auto Name = const_name("object");

    template <typename Key>
    accessor(handle obj, Key &&key)
        : m_base(obj.ptr()), m_key(std::move(key)) { }
    accessor(const accessor &) = delete;
    accessor(accessor &&) = delete;
    ~accessor() {
        if constexpr (Impl::cache_dec_ref)
            Py_XDECREF(m_cache);
    }

    template <typename T> accessor& operator=(T &&value);

    template <typename T, enable_if_t<std::is_base_of_v<object, T>> = 0>
    operator T() const & { return borrow<T>(ptr()); }

    // Temporary accessors hand out the cached reference instead of copying it
    template <typename T, enable_if_t<std::is_base_of_v<object, T>> = 0>
    operator T() && {
        if constexpr (Impl::cache_dec_ref) {
            PyObject *value = ptr();
            m_cache = nullptr;
            return steal<T>(value);
        } else {
            return borrow<T>(ptr());
        }
    }
    NB_INLINE PyObject *ptr() const {
        // Fetch once and cache an owned reference. Impls with cache_dec_ref
        // == false hand out borrowed references and refetch on every access.
        if constexpr (Impl::cache_dec_ref) {
            if (!m_cache)
                m_cache = Impl::get(m_base, m_key);
        } else {
            m_cache = Impl::get(m_base, m_key);
        }
        return m_cache;
    }
    NB_INLINE handle base() const { return m_base; }

    /// Python key of an attribute accessor. Borrowed unless 'owned' is set.
    NB_INLINE PyObject *key(bool &owned) const {
        return Impl::key(m_key, owned);
    }

    NB_DECL_ACCESSOR_OP_I(operator+=)
    NB_DECL_ACCESSOR_OP_I(operator-=)
    NB_DECL_ACCESSOR_OP_I(operator*=)
    NB_DECL_ACCESSOR_OP_I(operator/=)
    NB_DECL_ACCESSOR_OP_I(operator%=)
    NB_DECL_ACCESSOR_OP_I(operator|=)
    NB_DECL_ACCESSOR_OP_I(operator&=)
    NB_DECL_ACCESSOR_OP_I(operator^=)
    NB_DECL_ACCESSOR_OP_I(operator<<=)
    NB_DECL_ACCESSOR_OP_I(operator>>=)

private:
    NB_INLINE void del () { Impl::del(m_base, m_key); }

private:
    PyObject *m_base;
    mutable PyObject *m_cache{nullptr};
    typename Impl::key_type m_key;
};

struct str_attr {
    static constexpr bool cache_dec_ref = true;
    using key_type = str_key;

    NB_INLINE static PyObject *get(PyObject *obj, str_key k) {
        return NB_CALL(getattr_str)(NB_CTX, obj, k.str, k.bound);
    }

    NB_INLINE static void set(PyObject *obj, str_key k, PyObject *v) {
        NB_CALL(setattr_str)(NB_CTX, obj, k.str, k.bound, v);
    }

    NB_INLINE static PyObject *key(str_key k, bool &owned) {
        return NB_CALL(cached_string)(NB_CTX, k.str, k.bound, &owned);
    }
};

template <typename Key> struct obj_attr_t {
    static constexpr bool cache_dec_ref = true;
    using key_type = Key;

    NB_INLINE static PyObject *get(PyObject *obj, handle key) {
        return detail::getattr(obj, key.ptr());
    }

    NB_INLINE static void set(PyObject *obj, handle key, PyObject *v) {
        setattr(obj, key.ptr(), v);
    }

    NB_INLINE static PyObject *key(handle key, bool &owned) {
        owned = false;
        return key.ptr();
    }
};

struct str_item {
    static constexpr bool cache_dec_ref = true;
    using key_type = str_key;

    NB_INLINE static PyObject *get(PyObject *obj, str_key k) {
        return NB_CALL(getitem_str)(NB_CTX, obj, k.str, k.bound);
    }

    NB_INLINE static void set(PyObject *obj, str_key k, PyObject *v) {
        NB_CALL(setitem_str)(NB_CTX, obj, k.str, k.bound, v);
    }

    NB_INLINE static void del(PyObject *obj, str_key k) {
        NB_CALL(delitem_str)(NB_CTX, obj, k.str, k.bound);
    }
};

template <typename Key> struct obj_item_t {
    static constexpr bool cache_dec_ref = true;
    using key_type = Key;

    NB_INLINE static PyObject *get(PyObject *obj, handle key) {
        return raise_if_null(PyObject_GetItem(obj, key.ptr()));
    }

    NB_INLINE static void set(PyObject *obj, handle key, PyObject *v) {
        setitem(obj, key.ptr(), v);
    }

    NB_INLINE static void del(PyObject *obj, handle key) {
        delitem(obj, key.ptr());
    }
};

// Item access on an 'nb::dict', which always uses the PyDict_* API. This also
// covers dictionary subclasses, whose item hooks therefore have no effect.
template <typename Key> struct dict_item_t {
    static constexpr bool cache_dec_ref = true;
    using key_type = Key;

    NB_INLINE static PyObject *get(PyObject *obj, handle key) {
        bool error;
        PyObject *value = dict_getitem_ref(obj, key.ptr(), &error);
        if (NB_UNLIKELY(!value))
            error ? raise_python_error() : raise_key_error(key.ptr());
        return value;
    }

    NB_INLINE static void set(PyObject *obj, handle key, PyObject *v) {
        raise_if_nonzero(PyDict_SetItem(obj, key.ptr(), v));
    }

    NB_INLINE static void del(PyObject *obj, handle key) {
        raise_if_nonzero(PyDict_DelItem(obj, key.ptr()));
    }
};

struct dict_str_item {
    static constexpr bool cache_dec_ref = true;
    using key_type = str_key;

    NB_INLINE static PyObject *get(PyObject *obj, str_key k) {
        return NB_CALL(dict_getitem_str)(NB_CTX, obj, k.str, k.bound);
    }

    NB_INLINE static void set(PyObject *obj, str_key k, PyObject *v) {
        NB_CALL(dict_setitem_str)(NB_CTX, obj, k.str, k.bound, v);
    }

    NB_INLINE static void del(PyObject *obj, str_key k) {
        NB_CALL(dict_delitem_str)(NB_CTX, obj, k.str, k.bound);
    }
};

struct num_item {
    static constexpr bool cache_dec_ref = true;
    using key_type = Py_ssize_t;

    NB_INLINE static PyObject *get(PyObject *obj, Py_ssize_t index) {
        return raise_if_null(PySequence_GetItem(obj, index));
    }

    NB_INLINE static void set(PyObject *obj, Py_ssize_t index, PyObject *v) {
        setitem(obj, index, v);
    }

    NB_INLINE static void del(PyObject *obj, Py_ssize_t index) {
        delitem(obj, index);
    }
};

struct num_item_list {
    #if defined(NB_FREE_THREADED)
          static constexpr bool cache_dec_ref = true;
    #else
          static constexpr bool cache_dec_ref = false;
    #endif

    using key_type = Py_ssize_t;

    NB_INLINE static PyObject *get(PyObject *obj, Py_ssize_t index) {
        #if !defined(NB_FREE_THREADED)
            return NB_LIST_GET_ITEM(obj, index);
        #elif NB_PYTHON_VERSION >= 0x030D0000
            return PyList_GetItemRef(obj, index);
        #else
            return PySequence_GetItem(obj, index);
        #endif
    }

    NB_INLINE static void set(PyObject *obj, Py_ssize_t index, PyObject *v) {
#if defined(Py_LIMITED_API) || defined(NB_FREE_THREADED)
        PyList_SetItem(obj, index, Py_NewRef(v));
#else
        PyObject *old = NB_LIST_GET_ITEM(obj, index);
        NB_LIST_SET_ITEM(obj, index, Py_NewRef(v));
        Py_DECREF(old);
#endif
    }

    NB_INLINE static void del(PyObject *obj, Py_ssize_t index) {
        delitem(obj, index);
    }
};

struct num_item_tuple {
    static constexpr bool cache_dec_ref = false;
    using key_type = Py_ssize_t;

    NB_INLINE static PyObject *get(PyObject *obj, Py_ssize_t index) {
        return NB_TUPLE_GET_ITEM(obj, index);
    }

    template <typename...Ts> static void set(Ts...) {
        static_assert(false_v<Ts...>, "tuples are immutable!");
    }
};

template <typename D> accessor<obj_attr> api<D>::attr(handle key) const {
    return { derived(), key };
}

template <typename D>
template <typename T, enable_if_t<is_owned_key_v<T>>>
accessor<obj_attr_own> api<D>::attr(T &&key) const {
    return { derived(), (forward_t<T>) key };
}

template <typename D> accessor<str_attr> api<D>::attr(str_key key) const {
    return { derived(), key };
}

template <typename D> accessor<str_attr> api<D>::doc() const {
    return { derived(), str_key("__doc__", sizeof("__doc__")) };
}

template <typename D> accessor<obj_item> api<D>::operator[](handle key) const {
    return { derived(), key };
}

template <typename D>
template <typename T, enable_if_t<is_owned_key_v<T>>>
accessor<obj_item_own> api<D>::operator[](T &&key) const {
    return { derived(), (forward_t<T>) key };
}

template <typename D> accessor<str_item> api<D>::operator[](str_key key) const {
    return { derived(), key };
}

template <typename D>
template <typename T, enable_if_t<std::is_arithmetic_v<T>>>
accessor<num_item> api<D>::operator[](T index) const {
    return { derived(), (Py_ssize_t) index };
}

NB_IMPL_ACCESSOR_OP_I(operator+=, PyNumber_InPlaceAdd)
NB_IMPL_ACCESSOR_OP_I(operator%=, PyNumber_InPlaceRemainder)
NB_IMPL_ACCESSOR_OP_I(operator-=, PyNumber_InPlaceSubtract)
NB_IMPL_ACCESSOR_OP_I(operator*=, PyNumber_InPlaceMultiply)
NB_IMPL_ACCESSOR_OP_I(operator/=, PyNumber_InPlaceTrueDivide)
NB_IMPL_ACCESSOR_OP_I(operator|=, PyNumber_InPlaceOr)
NB_IMPL_ACCESSOR_OP_I(operator&=, PyNumber_InPlaceAnd)
NB_IMPL_ACCESSOR_OP_I(operator^=, PyNumber_InPlaceXor)
NB_IMPL_ACCESSOR_OP_I(operator<<=,PyNumber_InPlaceLshift)
NB_IMPL_ACCESSOR_OP_I(operator>>=,PyNumber_InPlaceRshift)

NAMESPACE_END(detail)

template <typename T, detail::enable_if_t<std::is_arithmetic_v<T>>>
detail::accessor<detail::num_item_list> list::operator[](T index) const {
    return { derived(), (Py_ssize_t) index };
}

template <typename T, detail::enable_if_t<std::is_arithmetic_v<T>>>
detail::accessor<detail::num_item_tuple> tuple::operator[](T index) const {
    return { derived(), (Py_ssize_t) index };
}

inline detail::accessor<detail::dict_item> dict::operator[](handle key) const {
    return { *this, key };
}

template <typename T, detail::enable_if_t<detail::is_owned_key_v<T>>>
detail::accessor<detail::dict_item_own> dict::operator[](T &&key) const {
    return { *this, (detail::forward_t<T>) key };
}

inline detail::accessor<detail::dict_str_item>
dict::operator[](detail::str_key key) const {
    return { *this, key };
}

template <typename... Args> str str::format(Args&&... args) const {
    return steal<str>(
        derived().attr("format")((detail::forward_t<Args>) args...).release());
}

NAMESPACE_END(NB_NAMESPACE)
