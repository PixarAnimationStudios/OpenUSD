/*
    nanobind/nb_types.h: nb::dict/str/list/..: C++ wrappers for Python types

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)

inline namespace NB_BACKEND_ABI_NS { class NB_EXPORT python_error; }

/// Macro defining functions/constructors for nanobind::handle subclasses
#define NB_OBJECT(Type, Parent, Str, Check)                                    \
public:                                                                        \
    static constexpr auto Name = ::nanobind::detail::const_name(Str);          \
    NB_INLINE Type(handle h, ::nanobind::detail::borrow_t)                     \
        : Parent(h, ::nanobind::detail::borrow_t{}) { }                        \
    NB_INLINE Type(handle h, ::nanobind::detail::steal_t)                      \
        : Parent(h, ::nanobind::detail::steal_t{}) { }                         \
    NB_INLINE static bool check_(handle h) {                                   \
        return Check(h.ptr());                                                 \
    }

/// Like NB_OBJECT but allow null-initialization
#define NB_OBJECT_DEFAULT(Type, Parent, Str, Check)                            \
    NB_OBJECT(Type, Parent, Str, Check)                                        \
    NB_INLINE Type() : Parent() {}

/// Helper macro to create detail::api comparison functions
#define NB_DECL_COMP(name)                                                     \
    template <typename T2> NB_INLINE bool name(const api<T2> &o) const;

#define NB_IMPL_COMP(name, op)                                                 \
    template <typename T1> template <typename T2>                              \
    NB_INLINE bool api<T1>::name(const api<T2> &o) const {                     \
        int rv = PyObject_RichCompareBool(derived().ptr(),                     \
                                          o.derived().ptr(), op);              \
        if (NB_UNLIKELY(rv < 0))                                               \
            detail::raise_python_error();                                      \
        return rv == 1;                                                        \
    }

/// Helper macros to create detail::api unary operators
#define NB_DECL_OP_1(name)                                                     \
    NB_INLINE object name() const;

#define NB_IMPL_OP_1(name, op)                                                 \
    template <typename T> NB_INLINE object api<T>::name() const {              \
        return steal(detail::obj_op_1(derived().ptr(), op));                   \
    }

/// Helper macros to create detail::api binary operators
#define NB_DECL_OP_2(name)                                                     \
    template <typename T2> NB_INLINE object name(const api<T2> &o) const;

#define NB_IMPL_OP_2(name, op)                                                 \
    template <typename T1> template <typename T2>                              \
    NB_INLINE object api<T1>::name(const api<T2> &o) const {                   \
        return steal(                                                          \
            detail::obj_op_2(derived().ptr(), o.derived().ptr(), op));         \
    }

#define NB_DECL_OP_2_I(name)                                                   \
    template <typename T2> NB_INLINE object name(const api<T2> &o);

#define NB_IMPL_OP_2_I(name, op)                                               \
    template <typename T1> template <typename T2>                              \
    NB_INLINE object api<T1>::name(const api<T2> &o) {                         \
        return steal(                                                          \
            detail::obj_op_2(derived().ptr(), o.derived().ptr(), op));         \
    }

#define NB_IMPL_OP_2_IO(name)                                                  \
    template <typename T> NB_INLINE decltype(auto) name(const api<T> &o) {     \
        return operator=(handle::name(o));                                     \
    }

// A few forward declarations
class object;
class handle;
class iterator;

template <typename T = object> NB_INLINE T borrow(handle h);
template <typename T = object> NB_INLINE T steal(handle h);

NAMESPACE_BEGIN(detail)

/// Raise a runtime error with the given message
#if defined(__GNUC__)
    __attribute__((noreturn, noinline, __format__ (__printf__, 1, 2)))
#else
    [[noreturn]] NB_NOINLINE
#endif
inline void raise(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    NB_CALL(raise_v)(exception_type::runtime_error, fmt, args);
    va_end(args);
    NB_UNREACHABLE();
}

/// Raise a type error with the given message
#if defined(__GNUC__)
    __attribute__((noreturn, noinline, __format__ (__printf__, 1, 2)))
#else
    [[noreturn]] NB_NOINLINE
#endif
inline void raise_type_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    NB_CALL(raise_v)(exception_type::type_error, fmt, args);
    va_end(args);
    NB_UNREACHABLE();
}

/* Raise nanobind::python_error, resp. nanobind::cast_error when no Python
   error is pending. Defined in nb_error.h, which has the complete
   python_error type. GCC rejects the 'noinline' attribute when it appears
   on a declaration that is already known to be inline, hence these two
   specifiers only occur on the definition. */
[[noreturn]] void raise_python_error();
[[noreturn]] void raise_python_or_cast_error();

/// Raise an exception if 'p' is null (shared cold path of the inline helpers)
NB_INLINE PyObject *raise_if_null(PyObject *p) {
    if (NB_UNLIKELY(!p))
        raise_python_error();
    return p;
}

/// Raise an exception if 'rv' is nonzero (ditto, for int-returning C API)
NB_INLINE void raise_if_nonzero(int rv) {
    if (NB_UNLIKELY(rv))
        raise_python_error();
}

/// FNV-1a string hash; the trampoline macros bind it to a constexpr variable
constexpr uint64_t str_hash(const char *s) {
    uint64_t h = 0xcbf29ce484222325ull;
    while (*s) {
        h ^= (uint64_t) (uint8_t) *s++;
        h *= 0x100000001b3ull;
    }
    return h;
}

/// A C string key together with a size bound.
struct str_key {
    const char *str;
    size_t bound;

    template <typename T, enable_if_t<is_c_string_v<T>> = 0>
    NB_INLINE str_key(T &&s) : str(s), bound(c_string_bound<T>()) { }
    NB_INLINE str_key(const char *s, size_t bound) : str(s), bound(bound) { }
};

/// Python string for a C string key, memoized in the backend's cache when
/// possible (see the cached_string() slot). The instance owns the result
/// only when the cache could not retain it. 'value' is null if the
/// conversion failed (with a Python error set).
struct cached_name {
    PyObject *value;
    bool owned;

    NB_INLINE cached_name(const char *key)
        : value(NB_CALL(cached_string)(NB_CTX, key, strlen(key) + 1, &owned)) { }
    NB_INLINE cached_name(str_key key)
        : value(NB_CALL(cached_string)(NB_CTX, key.str, key.bound, &owned)) { }
    cached_name(const cached_name &) = delete;
    cached_name(cached_name &&) = delete;
    NB_INLINE ~cached_name() {
        if (NB_UNLIKELY(owned))
            Py_DECREF(value);
    }

    /// Transfer ownership to the caller, adding a reference if needed
    NB_INLINE PyObject *release() {
        if (!owned)
            Py_XINCREF(value);
        owned = false;
        return value;
    }
};

/// Try to import a Python extension module, raises an exception upon failure
inline PyObject *module_import(const char *name) {
    return raise_if_null(PyImport_ImportModule(name));
}

/// Try to import a Python extension module, raises an exception upon failure
inline PyObject *module_import(PyObject *name) {
    return raise_if_null(PyImport_Import(name));
}

/// Get an object attribute or raise an exception
inline PyObject *getattr(PyObject *obj, PyObject *key) {
    return raise_if_null(PyObject_GetAttr(obj, key));
}

/// Get an object attribute or return a default value (never raises)
inline PyObject *getattr(PyObject *obj, PyObject *key, PyObject *def) noexcept {
#if defined(PYPY_VERSION) || \
    (defined(Py_LIMITED_API) && NB_PYTHON_VERSION < 0x030D0000)
    if (PyObject_HasAttr(obj, key)) {
        PyObject *res = PyObject_GetAttr(obj, key);
        if (res)
            return res;
        PyErr_Clear();
    }
#else
    PyObject *res;
    int rv;
    #if PY_VERSION_HEX < 0x030D0000
        rv = _PyObject_LookupAttr(obj, key, &res);
    #else
        rv = PyObject_GetOptionalAttr(obj, key, &res);
    #endif
    if (rv == 1)
        return res;
    else if (rv < 0)
        PyErr_Clear();
#endif
    return Py_XNewRef(def);
}

/// Set an object attribute or raise an exception
inline void setattr(PyObject *obj, PyObject *key, PyObject *value) {
    raise_if_nonzero(PyObject_SetAttr(obj, key, value));
}

/// Delete an object attribute or raise an exception
inline void delattr(PyObject *obj, PyObject *key) {
#if defined(Py_LIMITED_API) && NB_PYTHON_VERSION < 0x030D0000
    raise_if_nonzero(PyObject_SetAttr(obj, key, nullptr));
#else
    raise_if_nonzero(PyObject_DelAttr(obj, key));
#endif
}
/// Set an item or raise an exception
inline void setitem(PyObject *obj, Py_ssize_t key, PyObject *value) {
    raise_if_nonzero(PySequence_SetItem(obj, key, value));
}
inline void setitem(PyObject *obj, PyObject *key, PyObject *value) {
    raise_if_nonzero(PyObject_SetItem(obj, key, value));
}

/// Delete an item or raise an exception
inline void delitem(PyObject *obj, Py_ssize_t key) {
    raise_if_nonzero(PySequence_DelItem(obj, key));
}
inline void delitem(PyObject *obj, PyObject *key) {
    raise_if_nonzero(PyObject_DelItem(obj, key));
}

/// Perform an unary operation on a Python object with error handling
inline PyObject *obj_op_1(PyObject *a, PyObject* (*op)(PyObject*)) {
    return raise_if_null(op(a));
}

/// Perform a binary operation on Python objects with error handling
inline PyObject *obj_op_2(PyObject *a, PyObject *b,
                          PyObject *(*op)(PyObject *, PyObject *)) {
    return raise_if_null(op(a, b));
}

// Borrowed references to Python's singletons. The functions below are the only
// place in nanobind that may mention 'Py_None' and friends.
#if NB_CACHE_SINGLETONS
struct singleton_cache {
    PyObject *none, *true_, *false_, *not_implemented, *ellipsis;
};

/// Hidden and inline, so that each DSO fills and uses a private copy
NB_HIDDEN inline singleton_cache singletons { };

inline void init_singletons() noexcept {
    singletons = { Py_None, Py_True, Py_False, Py_NotImplemented, Py_Ellipsis };
}

NB_INLINE PyObject *none_ptr() noexcept { return singletons.none; }
NB_INLINE PyObject *true_ptr() noexcept { return singletons.true_; }
NB_INLINE PyObject *false_ptr() noexcept { return singletons.false_; }
NB_INLINE PyObject *not_implemented_ptr() noexcept { return singletons.not_implemented; }
NB_INLINE PyObject *ellipsis_ptr() noexcept { return singletons.ellipsis; }
#else
NB_INLINE void init_singletons() noexcept { }

NB_INLINE PyObject *none_ptr() noexcept { return Py_None; }
NB_INLINE PyObject *true_ptr() noexcept { return Py_True; }
NB_INLINE PyObject *false_ptr() noexcept { return Py_False; }
NB_INLINE PyObject *not_implemented_ptr() noexcept { return Py_NotImplemented; }
NB_INLINE PyObject *ellipsis_ptr() noexcept { return Py_Ellipsis; }
#endif

// Strong references to the same objects. The Py_RETURN_* macros are broken on
// CPython 3.12, Py_RETURN_NOTIMPLEMENTED is broken until CPython 3.14.
NB_INLINE PyObject *singleton_ref(PyObject *o) noexcept {
#if !NB_IMMORTAL_SINGLETONS
    Py_INCREF(o);
#endif
    return o;
}

NB_INLINE PyObject *none_ref() noexcept { return singleton_ref(none_ptr()); }
NB_INLINE PyObject *true_ref() noexcept { return singleton_ref(true_ptr()); }
NB_INLINE PyObject *false_ref() noexcept { return singleton_ref(false_ptr()); }
NB_INLINE PyObject *not_implemented_ref() noexcept { return singleton_ref(not_implemented_ptr()); }
NB_INLINE PyObject *ellipsis_ref() noexcept { return singleton_ref(ellipsis_ptr()); }

/// Cold path of the GIL assertions in debug builds (reference counting, calls)
[[noreturn]] NB_NOINLINE inline void fail_gil() noexcept {
    fprintf(stderr, "Critical nanobind error: attempted to change the "
                    "reference count of a Python object or to call into "
                    "Python while the GIL was not held!\n");
    abort();
}

/// Convert a Python object into a Python boolean object
inline PyObject *bool_from_obj(PyObject *o) {
    int rv = PyObject_IsTrue(o);
    if (NB_UNLIKELY(rv < 0))
        raise_python_error();
    return rv ? true_ref() : false_ref();
}

/// Advance the iterator 'o', raise an exception in case of errors. A null
/// return without a pending error indicates exhaustion.
inline PyObject *obj_iter_next(PyObject *o) {
    PyObject *result = PyIter_Next(o);
    if (NB_UNLIKELY(!result && PyErr_Occurred()))
        raise_python_error();
    return result;
}

/// Raise a KeyError for the given key (cold path of dict lookups)
[[noreturn]] NB_NOINLINE inline void raise_key_error(PyObject *key) {
    PyErr_SetObject(PyExc_KeyError, key);
    raise_python_error();
}

/// Look up 'k' in the dictionary 'd', returning a *new* reference
inline PyObject *dict_getitem_ref(PyObject *d, PyObject *k, bool *error) noexcept {
    PyObject *value;
#if NB_PYTHON_VERSION >= 0x030D0000
    *error = PyDict_GetItemRef(d, k, &value) == -1;
#else
    // GIL-protected borrowed-reference pattern; free-threaded builds never
    // land here (NB_FREE_THREADED implies 3.13+ headers)
    value = Py_XNewRef(PyDict_GetItemWithError(d, k));
    *error = !value && PyErr_Occurred() != nullptr;
#endif
    return value;
}

/// Dict lookup that returns a default value for missing keys
inline PyObject *dict_getitem_or_default(PyObject *d, PyObject *k, PyObject *def) {
    bool error;
    PyObject *value = dict_getitem_ref(d, k, &error);
    if (NB_UNLIKELY(error))
        raise_python_error();
    if (!value)
        value = Py_XNewRef(def);
    return value;
}

// The limited API routes the type tests below through a PyType_GetFlags()
// call. Testing for the exact type first avoids it in the common case.
#if defined(Py_LIMITED_API)
#  define NB_TYPE_CHECK(name, check, type, flag)                               \
      NB_INLINE bool name(PyObject *o) noexcept {                              \
          PyTypeObject *tp = Py_TYPE(o);                                       \
          return tp == &type || PyType_HasFeature(tp, flag);                   \
      }
#else
#  define NB_TYPE_CHECK(name, check, type, flag)                               \
      NB_INLINE bool name(PyObject *o) noexcept { return check(o); }
#endif

NB_TYPE_CHECK(int_check,     PyLong_Check,    PyLong_Type,    Py_TPFLAGS_LONG_SUBCLASS)
NB_TYPE_CHECK(str_check,     PyUnicode_Check, PyUnicode_Type, Py_TPFLAGS_UNICODE_SUBCLASS)
NB_TYPE_CHECK(bytes_check,   PyBytes_Check,   PyBytes_Type,   Py_TPFLAGS_BYTES_SUBCLASS)
NB_TYPE_CHECK(tuple_check,   PyTuple_Check,   PyTuple_Type,   Py_TPFLAGS_TUPLE_SUBCLASS)
NB_TYPE_CHECK(list_check,    PyList_Check,    PyList_Type,    Py_TPFLAGS_LIST_SUBCLASS)
NB_TYPE_CHECK(dict_check,    PyDict_Check,    PyDict_Type,    Py_TPFLAGS_DICT_SUBCLASS)
NB_TYPE_CHECK(py_type_check, PyType_Check,    PyType_Type,    Py_TPFLAGS_TYPE_SUBCLASS)

#undef NB_TYPE_CHECK

/// Check whether the object can be iterated over (see nb::iterable)
inline bool iterable_check(PyObject *o) noexcept {
    PyTypeObject *tp = Py_TYPE(o);
#if !defined(Py_LIMITED_API)
    bool has_iter = tp->tp_iter != nullptr;
#else
    bool has_iter = PyType_GetSlot(tp, Py_tp_iter) != nullptr;
#endif
    return has_iter || PySequence_Check(o);
}

/// Python issubclass() check with error handling
inline bool issubclass(PyObject *a, PyObject *b) {
    int rv = PyObject_IsSubclass(a, b);
    if (NB_UNLIKELY(rv < 0))
        raise_python_error();
    return bool(rv);
}

template <typename T, typename SFINAE = int> struct type_caster;
template <typename T> using make_caster = type_caster<intrinsic_t<T>>;

template <typename Impl> class accessor;

/// Is 'T' an accessor, whose value is only fetched when it is used?
template <typename T> inline constexpr bool is_accessor_v = false;
template <typename Impl> inline constexpr bool is_accessor_v<accessor<Impl>> = true;

struct str_attr; struct str_item; struct num_item; struct dict_str_item;
struct num_item_list; struct num_item_tuple;
template <typename Key> struct obj_attr_t;
template <typename Key> struct obj_item_t;
template <typename Key> struct dict_item_t;

/* An accessor borrows its key from the caller whenever the latter is known to
   outlive it, and captures it by value otherwise. The second case covers
   temporaries and nested accessors; a bare 'handle' remains borrowed, since
   its lifetime is the caller's responsibility either way. */
using obj_attr = obj_attr_t<handle>;
using obj_item = obj_item_t<handle>;
using dict_item = dict_item_t<handle>;
using obj_attr_own = obj_attr_t<object>;
using obj_item_own = obj_item_t<object>;
using dict_item_own = dict_item_t<object>;

template <typename T>
constexpr bool is_owned_key_v = !std::is_lvalue_reference_v<T> &&
                                std::is_constructible_v<object, forward_t<T>>;
class args_proxy; class kwargs_proxy;
struct borrow_t { };
struct steal_t { };
struct api_tag {
    constexpr static bool nb_typed = false;
};
class dict_iterator;
template <bool IsTuple> struct seq_iterator;

using tuple_iterator = seq_iterator<true>;

#if defined(NB_FREE_THREADED)
struct list_ref_iterator;
using list_iterator = list_ref_iterator;
#else
using list_iterator = seq_iterator<false>;
#endif

// Standard operations provided by every nanobind object
template <typename Derived> class api : public api_tag {
public:
    Derived &derived() { return static_cast<Derived &>(*this); }
    const Derived &derived() const { return static_cast<const Derived &>(*this); }

    NB_INLINE bool is(handle value) const;
    NB_INLINE bool is_none() const { return derived().ptr() == detail::none_ptr(); }
    NB_INLINE bool is_type() const { return py_type_check(derived().ptr()); }
    NB_INLINE bool is_valid() const { return derived().ptr() != nullptr; }
    NB_INLINE handle inc_ref() const &;
    NB_INLINE handle dec_ref() const &;
    iterator begin() const;
    iterator end() const;

    NB_INLINE handle type() const;
    NB_INLINE operator handle() const;

    accessor<obj_attr> attr(handle key) const;
    template <typename T, enable_if_t<is_owned_key_v<T>> = 0>
    accessor<obj_attr_own> attr(T &&key) const;
    accessor<str_attr> attr(str_key key) const;
    accessor<str_attr> doc() const;

    accessor<obj_item> operator[](handle key) const;
    template <typename T, enable_if_t<is_owned_key_v<T>> = 0>
    accessor<obj_item_own> operator[](T &&key) const;
    accessor<str_item> operator[](str_key key) const;
    template <typename T, enable_if_t<std::is_arithmetic_v<T>> = 1>
    accessor<num_item> operator[](T key) const;
    args_proxy operator*() const;

    template <rv_policy::value policy = rv_policy::automatic_reference_v,
              typename... Args>
    object operator()(Args &&...args) const;

    NB_DECL_COMP(equal)
    NB_DECL_COMP(not_equal)
    NB_DECL_COMP(operator<)
    NB_DECL_COMP(operator<=)
    NB_DECL_COMP(operator>)
    NB_DECL_COMP(operator>=)
    NB_DECL_OP_1(operator-)
    NB_DECL_OP_1(operator~)
    NB_DECL_OP_2(operator+)
    NB_DECL_OP_2(operator-)
    NB_DECL_OP_2(operator*)
    NB_DECL_OP_2(operator/)
    NB_DECL_OP_2(operator%)
    NB_DECL_OP_2(operator|)
    NB_DECL_OP_2(operator&)
    NB_DECL_OP_2(operator^)
    NB_DECL_OP_2(operator<<)
    NB_DECL_OP_2(operator>>)
    NB_DECL_OP_2(floor_div)
    NB_DECL_OP_2_I(operator+=)
    NB_DECL_OP_2_I(operator-=)
    NB_DECL_OP_2_I(operator*=)
    NB_DECL_OP_2_I(operator/=)
    NB_DECL_OP_2_I(operator%=)
    NB_DECL_OP_2_I(operator|=)
    NB_DECL_OP_2_I(operator&=)
    NB_DECL_OP_2_I(operator^=)
    NB_DECL_OP_2_I(operator<<=)
    NB_DECL_OP_2_I(operator>>=)
};

NAMESPACE_END(detail)

using detail::raise;
using detail::raise_type_error;
using detail::raise_python_error;

// *WARNING*: nanobind regularly receives requests from users who run it
// through Clang-Tidy, or who compile with increased warnings levels, like
//
//     -Wcast-qual, -Wsign-conversion, etc.
//
// (i.e., beyond -Wall -Wextra and /W4 that are currently already used)
//
// Their next step is to open a big pull request needed to silence all of
// the resulting messages.  This comment is strategically placed here
// because the (PyObject *) casts below cast away the const qualifier and
// will almost certainly be flagged in this process.
//
// My policy on this is as follows: I am always happy to fix issues in the
// codebase.  However, many of the resulting change requests are in the
// "ritual purification" category: things that cause churn, decrease
// readability, and which don't fix actual problems.  It's a never-ending
// cycle because each new revision of such tooling adds further warnings
// and purification rites.
//
// So just to be clear: I do not wish to pepper this codebase with
// "const_cast" and #pragmas/comments to avoid warnings in external
// tooling just so those users can have a "silent" build.  I don't think it
// is reasonable for them to impose their own style on this project.
//
// As a workaround it is likely possible to restrict the scope of style
// checks to particular C++ namespaces or source code locations.

class handle : public detail::api<handle> {
    friend class python_error;
    friend struct detail::str_attr;
    friend struct detail::str_item;
    friend struct detail::num_item;
    template <typename> friend struct detail::obj_attr_t;
    template <typename> friend struct detail::obj_item_t;
public:
    static constexpr auto Name = detail::const_name("object");

    handle() = default;
    handle(const handle &) = default;
    handle(handle &&) noexcept = default;
    handle &operator=(const handle &) = default;
    handle &operator=(handle &&) noexcept = default;
    NB_INLINE handle(std::nullptr_t, detail::steal_t) : m_ptr(nullptr) { }
    NB_INLINE handle(std::nullptr_t) : m_ptr(nullptr) { }
    NB_INLINE handle(const PyObject *ptr) : m_ptr((PyObject *) ptr) { }
    NB_INLINE handle(const PyTypeObject *ptr) : m_ptr((PyObject *) ptr) { }

    const handle& inc_ref() const & noexcept {
#if !defined(NDEBUG)
        if (m_ptr && NB_UNLIKELY(!NB_CALL(gil_check)()))
            detail::fail_gil();
#endif
        Py_XINCREF(m_ptr);
        return *this;
    }

    const handle& dec_ref() const & noexcept {
#if !defined(NDEBUG)
        if (m_ptr && NB_UNLIKELY(!NB_CALL(gil_check)()))
            detail::fail_gil();
#endif
        Py_XDECREF(m_ptr);
        return *this;
    }

    NB_INLINE explicit operator bool() const { return m_ptr != nullptr; }
    NB_INLINE PyObject *ptr() const { return m_ptr; }
    NB_INLINE static bool check_(handle) { return true; }

protected:
    PyObject *m_ptr = nullptr;
};

class object : public handle {
public:
    static constexpr auto Name = detail::const_name("object");

    object() = default;
    object(const object &o) : handle(o) { inc_ref(); }
    object(object &&o) noexcept : handle(o) { o.m_ptr = nullptr; }
    NB_INLINE ~object() {
#if defined(__GNUC__)
        // Fold away the destructor of moved-from instances
        if (!__builtin_constant_p(m_ptr) || m_ptr)
            dec_ref();
#elif !defined(NDEBUG)
        dec_ref(); // Includes the GIL assertion of debug builds
#else
        if (m_ptr)
            Py_DECREF(m_ptr);
#endif
    }
    object(handle h, detail::borrow_t) : handle(h) { inc_ref(); }
    object(handle h, detail::steal_t) : handle(h) { }

    handle release() {
      handle temp(m_ptr);
      m_ptr = nullptr;
      return temp;
    }

    void reset() {
        dec_ref();
        m_ptr = nullptr;
    }

    object& operator=(const object &o) {
        handle temp(m_ptr);
        o.inc_ref();
        m_ptr = o.m_ptr;
        temp.dec_ref();
        return *this;
    }

    object& operator=(object &&o) noexcept {
        handle temp(m_ptr);
        m_ptr = o.m_ptr;
        o.m_ptr = nullptr;
        temp.dec_ref();
        return *this;
    }

    NB_IMPL_OP_2_IO(operator+=)
    NB_IMPL_OP_2_IO(operator%=)
    NB_IMPL_OP_2_IO(operator-=)
    NB_IMPL_OP_2_IO(operator*=)
    NB_IMPL_OP_2_IO(operator/=)
    NB_IMPL_OP_2_IO(operator|=)
    NB_IMPL_OP_2_IO(operator&=)
    NB_IMPL_OP_2_IO(operator^=)
    NB_IMPL_OP_2_IO(operator<<=)
    NB_IMPL_OP_2_IO(operator>>=)
};

template <typename T> NB_INLINE T borrow(handle h) {
    return { h, detail::borrow_t() };
}

template <typename T = object, typename T2,
          std::enable_if_t<std::is_base_of_v<object, T2> && !std::is_lvalue_reference_v<T2>, int> = 0>
NB_INLINE T borrow(T2 &&o) {
    return { o.release(), detail::steal_t() };
}

template <typename T> NB_INLINE T steal(handle h) {
    return { h, detail::steal_t() };
}

inline bool hasattr(handle h, detail::str_key key) noexcept {
    return NB_CALL(hasattr_str)(NB_CTX, h.ptr(), key.str, key.bound);
}

inline bool hasattr(handle h, handle key) noexcept {
    return PyObject_HasAttr(h.ptr(), key.ptr());
}

inline object getattr(handle h, detail::str_key key) {
    return steal(NB_CALL(getattr_str)(NB_CTX, h.ptr(), key.str, key.bound));
}

inline object getattr(handle h, handle key) {
    return steal(detail::getattr(h.ptr(), key.ptr()));
}

inline object getattr(handle h, detail::str_key key, handle def) noexcept {
    return steal(
        NB_CALL(getattr_str_def)(NB_CTX, h.ptr(), key.str, key.bound, def.ptr()));
}

inline object getattr(handle h, handle key, handle def) noexcept {
    return steal(NB_CALL(getattr_def)(h.ptr(), key.ptr(), def.ptr()));
}

inline void setattr(handle h, detail::str_key key, handle value) {
    NB_CALL(setattr_str)(NB_CTX, h.ptr(), key.str, key.bound, value.ptr());
}

inline void setattr(handle h, handle key, handle value) {
    detail::setattr(h.ptr(), key.ptr(), value.ptr());
}

inline void delattr(handle h, detail::str_key key) {
    NB_CALL(delattr_str)(NB_CTX, h.ptr(), key.str, key.bound);
}

inline void delattr(handle h, handle key) {
    detail::delattr(h.ptr(), key.ptr());
}

class module_ : public object {
public:
    NB_OBJECT(module_, object, "types.ModuleType", PyModule_CheckExact)

    template <typename Func, typename... Extra>
    module_ &def(const char *name_, Func &&f, const Extra &...extra);

    static NB_INLINE module_ import_(const char *name) {
        return steal<module_>(detail::module_import(name));
    }

    static NB_INLINE module_ import_(handle name) {
        return steal<module_>(detail::module_import(name.ptr()));
    }

    NB_INLINE module_ def_submodule(const char *name,
                                    const char *doc = nullptr) {
        return steal<module_>(NB_CALL(submodule_new)(NB_CTX, m_ptr, name, doc));
    }
};

class capsule : public object {
    NB_OBJECT_DEFAULT(capsule, object, "typing_extensions.CapsuleType",
                      PyCapsule_CheckExact)

    capsule(const void *ptr, void (*cleanup)(void *) noexcept = nullptr)
        : capsule(ptr, nullptr, cleanup) { }

    capsule(const void *ptr, const char *name,
            void (*cleanup)(void *) noexcept = nullptr) {
        if (!ptr) {
            m_ptr = detail::none_ref();
            return;
        }

        // Capsule destructor that invokes the cleanup callback in the context
        auto capsule_cleanup = [](PyObject *o) {
            auto cleanup_2 = (void (*)(void *)) PyCapsule_GetContext(o);
            if (cleanup_2)
                cleanup_2(PyCapsule_GetPointer(o, PyCapsule_GetName(o)));
        };

        m_ptr = detail::raise_if_null(
            PyCapsule_New((void *) ptr, name, capsule_cleanup));
        detail::raise_if_nonzero(PyCapsule_SetContext(m_ptr, (void *) cleanup));
    }

    const char *name() const {
        return (m_ptr != detail::none_ptr()) ? PyCapsule_GetName(m_ptr) : nullptr;
    }

    void *data() const {
        return (m_ptr != detail::none_ptr()) ? PyCapsule_GetPointer(m_ptr, name()) : nullptr;
    }
    void *data(const char *name) const {
        if (m_ptr == detail::none_ptr()) return nullptr;
        void *p = PyCapsule_GetPointer(m_ptr, name);
        if (!p && PyErr_Occurred())
            detail::raise_python_error();
        return p;
    }
};

class none : public object {
public:
    static constexpr auto Name = detail::const_name("None");
    NB_INLINE none() : object(detail::none_ref(), detail::steal_t{}) { }
    NB_INLINE none(handle h, detail::borrow_t) : object(h, detail::borrow_t{}) { }
    NB_INLINE none(handle h, detail::steal_t) : object(h, detail::steal_t{}) { }
    NB_INLINE static bool check_(handle h) { return h.ptr() == detail::none_ptr(); }
};

class bool_ : public object {
    NB_OBJECT_DEFAULT(bool_, object, "bool", PyBool_Check)

    explicit bool_(handle h)
        : object(detail::bool_from_obj(h.ptr()), detail::steal_t{}) { }

    explicit bool_(bool value)
        : object(value ? detail::true_ref() : detail::false_ref(),
                 detail::steal_t{}) { }

    explicit operator bool() const {
        return m_ptr == detail::true_ptr();
    }
};

class int_ : public object {
    NB_OBJECT_DEFAULT(int_, object, "int", detail::int_check)

    explicit int_(handle h)
        : object(detail::raise_if_null(PyNumber_Long(h.ptr())), detail::steal_t{}) { }

    template <typename T, detail::enable_if_t<std::is_arithmetic_v<T>> = 0>
    explicit int_(T value) {
        if constexpr (std::is_floating_point_v<T>)
            m_ptr = PyLong_FromDouble((double) value);
        else if constexpr (detail::is_std_char_v<T>)
            // Treat character types as integers rather than (single-char) strings
            m_ptr = detail::type_caster<std::make_signed_t<T>>::from_cpp(
                (std::make_signed_t<T>) value, rv_policy::copy, nullptr).ptr();
        else
            m_ptr = detail::type_caster<T>::from_cpp(value, rv_policy::copy, nullptr).ptr();

        if (!m_ptr)
            detail::raise_python_error();
    }

    template <typename T, detail::enable_if_t<std::is_arithmetic_v<T>> = 0>
    explicit operator T() const {
        detail::type_caster<T> tc;
        if (!tc.from_python(m_ptr, 0, nullptr))
            throw std::out_of_range("Conversion of nanobind::int_ failed");
        return tc.value;
    }
};

class float_ : public object {
    NB_OBJECT_DEFAULT(float_, object, "float", PyFloat_Check)

    explicit float_(handle h)
        : object(detail::raise_if_null(PyNumber_Float(h.ptr())), detail::steal_t{}) { }

    explicit float_(double value)
        : object(PyFloat_FromDouble(value), detail::steal_t{}) {
        if (!m_ptr)
            detail::raise_python_error();
    }

#if !defined(Py_LIMITED_API)
    explicit operator double() const { return PyFloat_AS_DOUBLE(m_ptr); }
#else
    explicit operator double() const { return PyFloat_AsDouble(m_ptr); }
#endif
};

class str : public object {
    NB_OBJECT_DEFAULT(str, object, "str", detail::str_check)

    explicit str(handle h)
        : object(detail::raise_if_null(PyObject_Str(h.ptr())), detail::steal_t{}) { }

    explicit str(const char *s)
        : object(PyUnicode_FromString(s), detail::steal_t{}) {
        if (!m_ptr)
            detail::raise("nanobind::str(): conversion error!");
    }

    explicit str(const char *s, size_t n)
        : object(PyUnicode_FromStringAndSize(s, (Py_ssize_t) n), detail::steal_t{}) {
        if (!m_ptr)
            detail::raise("nanobind::str(): conversion error!");
    }

    template <typename... Args> str format(Args&&... args) const;

    const char *c_str() const { return PyUnicode_AsUTF8AndSize(m_ptr, nullptr); }
};

class bytes : public object {
    NB_OBJECT_DEFAULT(bytes, object, "bytes", detail::bytes_check)

    explicit bytes(handle h)
        : object(detail::raise_if_null(PyBytes_FromObject(h.ptr())), detail::steal_t{}) { }

    explicit bytes(const char *s)
        : object(detail::raise_if_null(PyBytes_FromString(s)), detail::steal_t{}) { }

    explicit bytes(const void *s, size_t n)
        : object(detail::raise_if_null(PyBytes_FromStringAndSize(
              (const char *) s, (Py_ssize_t) n)), detail::steal_t{}) { }

    const char *c_str() const { return PyBytes_AsString(m_ptr); }

    const void *data() const { return (const void *) PyBytes_AsString(m_ptr); }

    size_t size() const { return (size_t) PyBytes_Size(m_ptr); }
};

NAMESPACE_BEGIN(literals)
inline str operator""_s(const char *s, size_t n) {
    return str(s, n);
}
NAMESPACE_END(literals)

class bytearray : public object {
    NB_OBJECT(bytearray, object, "bytearray", PyByteArray_Check)

    bytearray()
        : object(PyObject_CallNoArgs((PyObject *)&PyByteArray_Type), detail::steal_t{}) { }

    explicit bytearray(handle h)
        : object(detail::raise_if_null(PyByteArray_FromObject(h.ptr())), detail::steal_t{}) { }

    explicit bytearray(const void *s, size_t n)
        : object(detail::raise_if_null(PyByteArray_FromStringAndSize(
              (const char *) s, (Py_ssize_t) n)), detail::steal_t{}) { }

    const char *c_str() const { return PyByteArray_AsString(m_ptr); }

    const void *data() const { return PyByteArray_AsString(m_ptr); }
    void *data() { return PyByteArray_AsString(m_ptr); }

    size_t size() const { return (size_t) PyByteArray_Size(m_ptr); }

    void resize(size_t n) {
        if (PyByteArray_Resize(m_ptr, (Py_ssize_t) n) != 0)
            detail::raise_python_error();
    }
};

class tuple : public object {
    NB_OBJECT(tuple, object, "tuple", detail::tuple_check)
    tuple() : object(PyTuple_New(0), detail::steal_t()) { }
    explicit tuple(handle h)
        : object(detail::raise_if_null(PySequence_Tuple(h.ptr())), detail::steal_t{}) { }
    size_t size() const { return (size_t) NB_TUPLE_GET_SIZE(m_ptr); }
    template <typename T, detail::enable_if_t<std::is_arithmetic_v<T>> = 1>
    detail::accessor<detail::num_item_tuple> operator[](T key) const;

    detail::tuple_iterator begin() const;
    detail::tuple_iterator end() const;
    bool empty() const { return size() == 0; }
};

class type_object : public object {
    NB_OBJECT_DEFAULT(type_object, object, "type", detail::py_type_check)
};

class list : public object {
    NB_OBJECT(list, object, "list", detail::list_check)
    list() : object(PyList_New(0), detail::steal_t()) { }
    explicit list(handle h)
        : object(detail::raise_if_null(PySequence_List(h.ptr())), detail::steal_t{}) { }
    size_t size() const { return (size_t) NB_LIST_GET_SIZE(m_ptr); }

    template <typename T> void append(T &&value);
    template <typename T> void insert(Py_ssize_t index, T &&value);

    template <typename T, detail::enable_if_t<std::is_arithmetic_v<T>> = 1>
    detail::accessor<detail::num_item_list> operator[](T key) const;

    void clear() {
#if PY_VERSION_HEX >= 0x030D0000 && !defined(PYPY_VERSION) && \
    !defined(Py_LIMITED_API)
        if (PyList_Clear(m_ptr))
#else
        if (PyList_SetSlice(m_ptr, 0, PY_SSIZE_T_MAX, nullptr))
#endif
            detail::raise_python_error();
    }

    void extend(handle h) {
#if PY_VERSION_HEX >= 0x030D0000 && !defined(PYPY_VERSION) && \
    !defined(Py_LIMITED_API)
        if (PyList_Extend(m_ptr, h.ptr()))
#else
        if (PyList_SetSlice(m_ptr, PY_SSIZE_T_MAX, PY_SSIZE_T_MAX, h.ptr()))
#endif
            detail::raise_python_error();
    }

    void sort() {
        if (PyList_Sort(m_ptr))
            detail::raise_python_error();
    }

    void reverse() {
        if (PyList_Reverse(m_ptr))
            detail::raise_python_error();
    }

    detail::list_iterator begin() const;
    detail::list_iterator end() const;
    bool empty() const { return size() == 0; }
};

class dict : public object {
    NB_OBJECT(dict, object, "dict", detail::dict_check)
    dict() : object(PyDict_New(), detail::steal_t()) { }
    size_t size() const { return (size_t) NB_DICT_GET_SIZE(m_ptr); }
    detail::dict_iterator begin() const;
    detail::dict_iterator end() const;
    list keys() const { return steal<list>(detail::obj_op_1(m_ptr, PyDict_Keys)); }
    list values() const { return steal<list>(detail::obj_op_1(m_ptr, PyDict_Values)); }
    list items() const { return steal<list>(detail::obj_op_1(m_ptr, PyDict_Items)); }
    object get(handle key, handle def) const {
        return steal(detail::dict_getitem_or_default(m_ptr, key.ptr(), def.ptr()));
    }
    object get(detail::str_key key_, handle def) const {
        detail::cached_name key(key_);
        return steal(detail::dict_getitem_or_default(
            m_ptr, detail::raise_if_null(key.value), def.ptr()));
    }
    template <typename T, detail::enable_if_t<!detail::is_c_string_v<T>> = 0>
    bool contains(T&& key) const;
    bool contains(detail::str_key key) const;
    void clear() { PyDict_Clear(m_ptr); }
    void update(handle h) {
        if (PyDict_Update(m_ptr, h.ptr()))
            detail::raise_python_error();
    }
    bool empty() const { return size() == 0; }

    detail::accessor<detail::dict_item> operator[](handle key) const;
    template <typename T, detail::enable_if_t<detail::is_owned_key_v<T>> = 0>
    detail::accessor<detail::dict_item_own> operator[](T &&key) const;
    detail::accessor<detail::dict_str_item> operator[](detail::str_key key) const;
};

class set : public object {
    NB_OBJECT(set, object, "set", PySet_Check)
    set() : object(PySet_New(nullptr), detail::steal_t()) { }
    explicit set(handle h)
        : object(detail::raise_if_null(PySet_New(h.ptr())), detail::steal_t{}) { }
    size_t size() const { return (size_t) NB_SET_GET_SIZE(m_ptr); }
    template <typename T, detail::enable_if_t<!detail::is_c_string_v<T>> = 0>
    bool contains(T&& key) const;
    bool contains(detail::str_key key) const;
    template <typename T> void add(T &&value);
    void clear() {
        if (PySet_Clear(m_ptr))
            detail::raise_python_error();
    }
    template <typename T> bool discard(T &&value);
    bool empty() const { return size() == 0; }
};

class frozenset : public object {
    NB_OBJECT(frozenset, object, "frozenset", PyFrozenSet_Check)
    frozenset() : object(PyFrozenSet_New(nullptr), detail::steal_t()) { }
    explicit frozenset(handle h)
        : object(detail::raise_if_null(PyFrozenSet_New(h.ptr())), detail::steal_t{}) { }
    size_t size() const { return (size_t) NB_SET_GET_SIZE(m_ptr); }
    template <typename T, detail::enable_if_t<!detail::is_c_string_v<T>> = 0>
    bool contains(T&& key) const;
    bool contains(detail::str_key key) const;
    bool empty() const { return size() == 0; }
};

class sequence : public object {
    NB_OBJECT_DEFAULT(sequence, object, "collections.abc.Sequence", PySequence_Check)
};

class mapping : public object {
    NB_OBJECT_DEFAULT(mapping, object, "collections.abc.Mapping", PyMapping_Check)
    list keys() const { return steal<list>(detail::obj_op_1(m_ptr, PyMapping_Keys)); }
    list values() const { return steal<list>(detail::obj_op_1(m_ptr, PyMapping_Values)); }
    list items() const { return steal<list>(detail::obj_op_1(m_ptr, PyMapping_Items)); }
    template <typename T, detail::enable_if_t<!detail::is_c_string_v<T>> = 0>
    bool contains(T&& key) const;
    bool contains(detail::str_key key) const;
};

class args : public tuple {
    NB_OBJECT_DEFAULT(args, tuple, "tuple", detail::tuple_check)
};

class kwargs : public dict {
    NB_OBJECT_DEFAULT(kwargs, dict, "dict", detail::dict_check)
};

class iterator : public object {
public:
    using difference_type = Py_ssize_t;
    using value_type = handle;
    using reference = const handle;
    using pointer = const handle *;

    NB_OBJECT_DEFAULT(iterator, object, "collections.abc.Iterator", PyIter_Check)

    iterator& operator++() {
        m_value = steal(detail::obj_iter_next(m_ptr));
        return *this;
    }

    iterator operator++(int) {
        iterator rv = *this;
        m_value = steal(detail::obj_iter_next(m_ptr));
        return rv;
    }

    handle operator*() const {
        if (is_valid() && !m_value.is_valid())
            m_value = steal(detail::obj_iter_next(m_ptr));
        return m_value;
    }

    pointer operator->() const { operator*(); return &m_value; }

    static iterator sentinel() { return {}; }

    friend bool operator==(const iterator &a, const iterator &b) { return a->ptr() == b->ptr(); }
    friend bool operator!=(const iterator &a, const iterator &b) { return a->ptr() != b->ptr(); }

private:
    mutable object m_value;
};

class iterable : public object {
public:
    NB_OBJECT_DEFAULT(iterable, object, "collections.abc.Iterable", detail::iterable_check)
};

/// Retrieve the Python type object associated with a C++ class
template <typename T> handle type() noexcept {
    return NB_CALL(nb_type_lookup)(NB_CTX, &typeid(detail::intrinsic_t<T>));
}

template <typename T>
NB_INLINE bool isinstance(handle h) noexcept {
    if constexpr (std::is_base_of_v<handle, T>)
        return T::check_(h);
    else if constexpr (detail::is_base_caster_v<detail::make_caster<T>>)
        return NB_CALL(nb_type_isinstance)(NB_CTX, h.ptr(), &typeid(detail::intrinsic_t<T>));
    else
        return detail::make_caster<T>().from_python(h, 0, nullptr);
}

NB_INLINE bool issubclass(handle h1, handle h2) {
    return detail::issubclass(h1.ptr(), h2.ptr());
}

NB_INLINE str repr(handle h) {
    return steal<str>(detail::raise_if_null(PyObject_Repr(h.ptr())));
}
NB_INLINE size_t len(handle h) {
    Py_ssize_t res = PyObject_Size(h.ptr());
    if (NB_UNLIKELY(res < 0))
        detail::raise_python_error();
    return (size_t) res;
}
NB_INLINE size_t len_hint(handle h) { return NB_CALL(len_hint)(NB_CTX, h.ptr()); }
NB_INLINE size_t len(const tuple &t) { return (size_t) NB_TUPLE_GET_SIZE(t.ptr()); }
NB_INLINE size_t len(const list &l) { return (size_t) NB_LIST_GET_SIZE(l.ptr()); }
NB_INLINE size_t len(const dict &d) { return (size_t) NB_DICT_GET_SIZE(d.ptr()); }
NB_INLINE size_t len(const set &d) { return (size_t) NB_SET_GET_SIZE(d.ptr()); }

inline void print(handle value, handle end = handle(), handle file = handle()) {
    object file_o;
    if (!file.is_valid()) {
        object sys = steal(detail::raise_if_null(PyImport_ImportModule("sys")));
        file_o = steal(
            detail::raise_if_null(PyObject_GetAttrString(sys.ptr(), "stdout")));
    }

    PyObject *file_p = file.is_valid() ? file.ptr() : file_o.ptr();

    detail::raise_if_nonzero(PyFile_WriteObject(value.ptr(), file_p, Py_PRINT_RAW));

    int rv;
    if (end.is_valid())
        rv = PyFile_WriteObject(end.ptr(), file_p, Py_PRINT_RAW);
    else
        rv = PyFile_WriteString("\n", file_p);
    detail::raise_if_nonzero(rv);
}

inline void print(const char *str, handle end = handle(), handle file = handle()) {
    print(nanobind::str(str), end, file);
}

inline dict builtins() {
#if NB_PYTHON_VERSION >= 0x030D0000
    return steal<dict>(PyEval_GetFrameBuiltins());
#else
    return borrow<dict>(PyEval_GetBuiltins());
#endif
}

inline iterator iter(handle h) {
    return steal<iterator>(detail::raise_if_null(PyObject_GetIter(h.ptr())));
}

class slice : public object {
public:
    NB_OBJECT_DEFAULT(slice, object, "slice", PySlice_Check)
    slice(handle start, handle stop, handle step) {
        m_ptr = PySlice_New(start.ptr(), stop.ptr(), step.ptr());
        if (!m_ptr)
            detail::raise_python_error();
    }

    template <typename T, detail::enable_if_t<std::is_arithmetic_v<T>> = 0>
    explicit slice(T stop)
        : slice(detail::none_ptr(), int_(stop), detail::none_ptr()) {}
    template <typename T, detail::enable_if_t<std::is_arithmetic_v<T>> = 0>
    slice(T start, T stop)
        : slice(int_(start), int_(stop), detail::none_ptr()) {}
    template <typename T, detail::enable_if_t<std::is_arithmetic_v<T>> = 0>
    slice(T start, T stop, T step) : slice(int_(start), int_(stop), int_(step)) {}

    detail::tuple<Py_ssize_t, Py_ssize_t, Py_ssize_t, size_t> compute(size_t size) const {
        Py_ssize_t start, stop, step;
        detail::raise_if_nonzero(PySlice_Unpack(m_ptr, &start, &stop, &step));
        size_t slice_length = (size_t) PySlice_AdjustIndices(
            (Py_ssize_t) size, &start, &stop, step);
        return detail::tuple(start, stop, step, slice_length);
    }
};

class memoryview : public object {
    NB_OBJECT(memoryview, object, "memoryview", PyMemoryView_Check)
    explicit memoryview(handle h)
        : object(detail::raise_if_null(PyMemoryView_FromObject(h.ptr())), detail::steal_t{}) { }
};

class ellipsis : public object {
    static bool is_ellipsis(PyObject *obj) { return obj == detail::ellipsis_ptr(); }

public:
    NB_OBJECT(ellipsis, object, "types.EllipsisType", is_ellipsis)
    ellipsis() : object(detail::ellipsis_ref(), detail::steal_t{}) {}
};

class not_implemented : public object {
    static bool is_not_implemented(PyObject *obj) {
        return obj == detail::not_implemented_ptr();
    }

public:
    NB_OBJECT(not_implemented, object, "types.NotImplementedType", is_not_implemented)
    not_implemented()
        : object(detail::not_implemented_ref(), detail::steal_t{}) { }
};

class callable : public object {
public:
    NB_OBJECT(callable, object, "collections.abc.Callable", PyCallable_Check)
    using object::object;
};

class weakref : public object {
public:
    NB_OBJECT(weakref, object, "weakref.ReferenceType", PyWeakref_Check)

    explicit weakref(handle obj, handle callback = {})
        : object(PyWeakref_NewRef(obj.ptr(), callback.ptr()), detail::steal_t{}) {
        if (!m_ptr)
            detail::raise_python_error();
    }
};

class any : public object {
public:
    using object::object;
    using object::operator=;
    static constexpr auto Name = detail::const_name("typing.Any");
};

template <typename T> class handle_t : public handle {
public:
    static constexpr auto Name = detail::make_caster<T>::Name;

    using handle::handle;
    using handle::operator=;
    handle_t(const handle &h) : handle(h) { }

    static bool check_(handle h) { return isinstance<T>(h); }
};

struct fallback : public handle {
public:
    static constexpr auto Name = detail::const_name("object");

    using handle::handle;
    using handle::operator=;
    fallback(const handle &h) : handle(h) { }
};

template <typename T> class type_object_t : public type_object {
public:
    static constexpr auto Name = detail::const_name("type[") +
                                 detail::make_caster<T>::Name +
                                 detail::const_name("]");

    using type_object::type_object;
    using type_object::operator=;

    static bool check_(handle h) {
        return detail::py_type_check(h.ptr()) &&
               PyType_IsSubtype((PyTypeObject *) h.ptr(),
                                (PyTypeObject *) nanobind::type<T>().ptr());
    }
};

template <typename T, typename...> class typed : public T {
public:
    constexpr static bool nb_typed = true;
    using T::T;
    using T::operator=;
    typed(const T& o) : T(o) {}
    typed(T&& o) : T(std::move(o)) {}
};

template <typename T> struct pointer_and_handle {
    T *p;
    handle h;
};

NAMESPACE_BEGIN(detail)

/// Cold path of the sequence builder assertions in debug builds
[[noreturn]] NB_NOINLINE inline void fail_seq_builder(const char *why) noexcept {
    fprintf(stderr, "Critical nanobind error: sequence builder misuse: %s\n",
            why);
    abort();
}

// Incrementally construct a tuple or list of known size
template <bool IsTuple> struct seq_builder {
public:
    NB_INLINE explicit seq_builder(size_t size) noexcept : m_index(0), m_size(size) {
#if defined(NB_BACKEND_MODULE)
        PyObject **items;
        if constexpr (IsTuple)
            m_builder = NB_CALL(tuple_alloc)(size, &items);
        else
            m_builder = NB_CALL(list_alloc)(size, &items);
        m_items = items;
#else
        if constexpr (IsTuple)
            m_seq = PyTuple_New((Py_ssize_t) size);
        else
            m_seq = PyList_New((Py_ssize_t) size);
#endif
    }

    seq_builder(const seq_builder &) = delete;
    seq_builder &operator=(const seq_builder &) = delete;
    seq_builder &operator=(seq_builder &&) = delete;

    NB_INLINE seq_builder(seq_builder &&b) noexcept
        : m_index(b.m_index), m_size(b.m_size) {
#if defined(NB_BACKEND_MODULE)
        m_builder = b.m_builder;
        m_items = b.m_items;
        b.m_builder = nullptr;
#else
        m_seq = b.m_seq;
        b.m_seq = nullptr;
#endif
    }

    NB_INLINE ~seq_builder() {
#if defined(NB_BACKEND_MODULE)
        if (NB_UNLIKELY(m_builder))
            NB_CALL(seq_commit)(m_builder, SIZE_MAX);
#else
        Py_XDECREF(m_seq);
#endif
    }

    /// Check whether the constructor succeeded
    NB_INLINE bool valid() const noexcept {
#if defined(NB_BACKEND_MODULE)
        return m_builder != nullptr;
#else
        return m_seq != nullptr;
#endif
    }

    /// Check whether every entry was filled
    NB_INLINE bool full() const noexcept { return m_index == m_size; }

    /// Fill the next entry, stealing the given valid reference
    NB_INLINE void put(handle h) noexcept {
#if !defined(NDEBUG)
        if (NB_UNLIKELY(!valid()))
            fail_seq_builder("put(): the builder is inactive");
        if (NB_UNLIKELY(full()))
            fail_seq_builder("put(): capacity exceeded");
        if (NB_UNLIKELY(!h.ptr()))
            fail_seq_builder("put(): null object");
#endif
#if defined(NB_BACKEND_MODULE)
        m_items[m_index] = h.ptr();
#else
        if constexpr (IsTuple)
            NB_TUPLE_SET_ITEM(m_seq, (Py_ssize_t) m_index, h.ptr());
        else
            NB_LIST_SET_ITEM(m_seq, (Py_ssize_t) m_index, h.ptr());
#endif
        m_index++;
    }

    /// Deactivate the builder and return the sequence or null
    NB_INLINE handle commit() noexcept {
#if !defined(NDEBUG)
        if (NB_UNLIKELY(!valid()))
            fail_seq_builder("commit(): the builder is inactive");
#endif
#if defined(NB_BACKEND_MODULE)
        void *builder = m_builder;
        m_builder = nullptr;
        return NB_CALL(seq_commit)(builder, m_index);
#else
        PyObject *seq = m_seq;
        m_seq = nullptr;

        if (NB_LIKELY(m_index == m_size))
            return seq;

        Py_DECREF(seq);
        return handle();
#endif
    }

private:
#if defined(NB_BACKEND_MODULE)
    void *m_builder;
    PyObject **m_items;
#else
    PyObject *m_seq;
#endif
    size_t m_index, m_size;
};

/// Cold error path of builder::commit()
[[noreturn]] NB_NOINLINE inline void raise_incomplete_builder() {
#if !defined(NDEBUG)
    if (!PyErr_Occurred())
        raise("nanobind::builder::commit(): the sequence was not completely "
              "filled with put()");
#endif
    raise_python_or_cast_error();
}

NAMESPACE_END(detail)

template <typename Seq> class builder {
    static_assert(std::is_same_v<Seq, tuple> || std::is_same_v<Seq, list>,
                  "nb::builder<Seq> requires Seq to be nb::tuple or nb::list");

public:
    /// Reserve a builder for a sequence with 'size' entries
    NB_INLINE explicit builder(size_t size) : m_core(size) {
        if (NB_UNLIKELY(!m_core.valid()))
            detail::raise_python_error();
    }

    builder(builder &&) noexcept = default;
    NB_INLINE ~builder() = default;

    /// Cast 'value' to Python and store it in the next entry. Returns false
    /// when the cast fails and stores nothing in that case. The result may
    /// be ignored, since a later commit() raises on an incomplete sequence.
    template <typename T> NB_INLINE bool put(T &&value) noexcept;

    /// Finish the builder and return the sequence
    NB_INLINE Seq commit() {
#if !defined(NDEBUG)
        if (NB_UNLIKELY(!m_core.valid()))
            detail::raise("nanobind::builder::commit(): the builder was "
                          "already committed or moved from");
#endif
        handle h = m_core.commit();
        if (NB_UNLIKELY(!h.ptr()))
            detail::raise_incomplete_builder();
        return steal<Seq>(h);
    }

private:
    detail::seq_builder<std::is_same_v<Seq, tuple>> m_core;
};

using tuple_builder = builder<tuple>;
using list_builder = builder<list>;

NAMESPACE_BEGIN(detail)

/// Module attribute resolved on first use. The constructor runs no code, and
/// the returned handle belongs to the nanobind internals.
struct import_cache {
    constexpr import_cache(const char *module, const char *attr)
        : module(module), attr(attr) { }

    handle get() {
        PyObject *v = load();
        if (NB_UNLIKELY(!v))
            v = raise_if_null(NB_CALL(import_cached)(NB_CTX, this));
        return v;
    }

    PyObject *load() const {
#if defined(NB_FREE_THREADED)
        return value.load(std::memory_order_acquire);
#else
        return value;
#endif
    }

    void store(PyObject *v) {
#if defined(NB_FREE_THREADED)
        value.store(v, std::memory_order_release);
#else
        value = v;
#endif
    }

    const char *module;
    const char *attr;
#if defined(NB_FREE_THREADED)
    std::atomic<PyObject *> value { nullptr };
#else
    PyObject *value = nullptr;
#endif
};

template <typename Derived> NB_INLINE api<Derived>::operator handle() const {
    return derived().ptr();
}

template <typename Derived> NB_INLINE handle api<Derived>::type() const {
    return (PyObject *) Py_TYPE(derived().ptr());
}

template <typename Derived> NB_INLINE handle api<Derived>::inc_ref() const & {
    return operator handle().inc_ref();
}

template <typename Derived> NB_INLINE handle api<Derived>::dec_ref() const & {
    return operator handle().dec_ref();
}

template <typename Derived>
NB_INLINE bool api<Derived>::is(handle value) const {
    return derived().ptr() == value.ptr();
}

template <typename Derived> iterator api<Derived>::begin() const {
    return iter(*this);
}

template <typename Derived> iterator api<Derived>::end() const {
    return iterator::sentinel();
}

/// Tag type to construct the end sentinel of a sequence iterator
struct seq_end_t { };

/// Iterator over a tuple or list. Dereferencing produces a borrowed
/// reference to an entry, which the sequence keeps alive.
template <bool IsTuple> struct seq_iterator {
    using value_type = handle;
    using reference = const value_type;
    using difference_type = std::ptrdiff_t;

    seq_iterator() = default;
    seq_iterator(PyObject *seq) : seq(seq), index(0) { }
    seq_iterator(PyObject *seq, seq_end_t) : seq(seq) {
        if constexpr (IsTuple)
            index = NB_TUPLE_GET_SIZE(seq);
        else
            index = NB_LIST_GET_SIZE(seq);
    }

    seq_iterator& operator++() { index++; return *this; }
    seq_iterator operator++(int) { seq_iterator rv = *this; index++; return rv; }
    friend bool operator==(const seq_iterator &a, const seq_iterator &b) { return a.index == b.index; }
    friend bool operator!=(const seq_iterator &a, const seq_iterator &b) { return a.index != b.index; }

    handle operator*() const {
        if constexpr (IsTuple)
            return NB_TUPLE_GET_ITEM(seq, index);
        else
            return NB_LIST_GET_ITEM(seq, index);
    }

    static Py_ssize_t size(PyObject *seq) {
        if constexpr (IsTuple)
            return NB_TUPLE_GET_SIZE(seq);
        else
            return NB_LIST_GET_SIZE(seq);
    }

    PyObject *seq = nullptr;
    Py_ssize_t index = 0;
};

#if defined(NB_FREE_THREADED)
/// Iterator over a list in free-threaded builds. Another thread may drop an
/// entry at any time, hence the iterator holds a reference to the current one
/// instead of handing out a borrowed reference. Truncation ends the iteration,
/// which matches the behavior of Python's own list iterator.
struct list_ref_iterator {
    using value_type = handle;
    using reference = const value_type;
    using difference_type = std::ptrdiff_t;

    list_ref_iterator() = default;
    list_ref_iterator(PyObject *seq) : seq(seq), index(0) { fetch(); }
    list_ref_iterator(PyObject *, seq_end_t) { }

    list_ref_iterator& operator++() { index++; fetch(); return *this; }
    list_ref_iterator operator++(int) { list_ref_iterator rv = *this; index++; fetch(); return rv; }
    friend bool operator==(const list_ref_iterator &a, const list_ref_iterator &b) { return a.index == b.index; }
    friend bool operator!=(const list_ref_iterator &a, const list_ref_iterator &b) { return a.index != b.index; }

    handle operator*() const { return value; }

    void fetch() {
        if (index < NB_LIST_GET_SIZE(seq)) {
            value = steal(PyList_GetItemRef(seq, index));
            if (value.is_valid())
                return;
            PyErr_Clear(); // Shrunk by another thread
        }

        value.reset();
        index = -1;
    }

    PyObject *seq = nullptr;
    Py_ssize_t index = -1;
    object value;
};
#endif

class dict_iterator {
public:
    NB_NONCOPYABLE(dict_iterator)

    using value_type = std::pair<handle, handle>;
    using reference = const value_type;

    dict_iterator() = default;
    dict_iterator(handle h) : h(h), pos(0) {
#if defined(NB_FREE_THREADED)
        PyCriticalSection_Begin(&cs, h.ptr());
#endif
        increment();
    }

#if defined(NB_FREE_THREADED)
    ~dict_iterator() {
        if (h.ptr())
            PyCriticalSection_End(&cs);
    }
#endif

    dict_iterator& operator++() {
        increment();
        return *this;
    }

    void increment() {
        if (PyDict_Next(h.ptr(), &pos, &key, &value) == 0)
            pos = -1;
    }

    value_type operator*() const { return { key, value }; }

    friend bool operator==(const dict_iterator &a, const dict_iterator &b) { return a.pos == b.pos; }
    friend bool operator!=(const dict_iterator &a, const dict_iterator &b) { return a.pos != b.pos; }

private:
    handle h;
    Py_ssize_t pos = -1;
    PyObject *key = nullptr;
    PyObject *value = nullptr;
#if defined(NB_FREE_THREADED)
    PyCriticalSection cs { };
#endif
};

NB_IMPL_COMP(equal,      Py_EQ)
NB_IMPL_COMP(not_equal,  Py_NE)
NB_IMPL_COMP(operator<,  Py_LT)
NB_IMPL_COMP(operator<=, Py_LE)
NB_IMPL_COMP(operator>,  Py_GT)
NB_IMPL_COMP(operator>=, Py_GE)
NB_IMPL_OP_1(operator-,  PyNumber_Negative)
NB_IMPL_OP_1(operator~,  PyNumber_Invert)
NB_IMPL_OP_2(operator+,  PyNumber_Add)
NB_IMPL_OP_2(operator-,  PyNumber_Subtract)
NB_IMPL_OP_2(operator*,  PyNumber_Multiply)
NB_IMPL_OP_2(operator/,  PyNumber_TrueDivide)
NB_IMPL_OP_2(operator%,  PyNumber_Remainder)
NB_IMPL_OP_2(operator|,  PyNumber_Or)
NB_IMPL_OP_2(operator&,  PyNumber_And)
NB_IMPL_OP_2(operator^,  PyNumber_Xor)
NB_IMPL_OP_2(operator<<, PyNumber_Lshift)
NB_IMPL_OP_2(operator>>, PyNumber_Rshift)
NB_IMPL_OP_2(floor_div,  PyNumber_FloorDivide)
NB_IMPL_OP_2_I(operator+=, PyNumber_InPlaceAdd)
NB_IMPL_OP_2_I(operator%=, PyNumber_InPlaceRemainder)
NB_IMPL_OP_2_I(operator-=, PyNumber_InPlaceSubtract)
NB_IMPL_OP_2_I(operator*=, PyNumber_InPlaceMultiply)
NB_IMPL_OP_2_I(operator/=, PyNumber_InPlaceTrueDivide)
NB_IMPL_OP_2_I(operator|=, PyNumber_InPlaceOr)
NB_IMPL_OP_2_I(operator&=, PyNumber_InPlaceAnd)
NB_IMPL_OP_2_I(operator^=, PyNumber_InPlaceXor)
NB_IMPL_OP_2_I(operator<<=,PyNumber_InPlaceLshift)
NB_IMPL_OP_2_I(operator>>=,PyNumber_InPlaceRshift)

#undef NB_DECL_COMP
#undef NB_IMPL_COMP
#undef NB_DECL_OP_1
#undef NB_IMPL_OP_1
#undef NB_DECL_OP_2
#undef NB_IMPL_OP_2
#undef NB_DECL_OP_2_I
#undef NB_IMPL_OP_2_I
#undef NB_IMPL_OP_2_IO

NAMESPACE_END(detail)

inline detail::dict_iterator dict::begin() const { return { *this }; }
inline detail::dict_iterator dict::end() const { return { }; }

inline detail::tuple_iterator tuple::begin() const { return { m_ptr }; }
inline detail::tuple_iterator tuple::end() const { return { m_ptr, detail::seq_end_t() }; }
inline detail::list_iterator list::begin() const { return { m_ptr }; }
inline detail::list_iterator list::end() const { return { m_ptr, detail::seq_end_t() }; }

template <typename T> void del(detail::accessor<T> &a) { a.del(); }
template <typename T> void del(detail::accessor<T> &&a) { a.del(); }

NAMESPACE_END(NB_NAMESPACE)
