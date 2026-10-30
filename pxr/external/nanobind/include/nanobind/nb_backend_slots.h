/*
    nanobind/nb_backend_slots.h: function slots of the backend ABI. A consumer
    defines NB_SLOT(return type, name, argument list) and includes this file
    (which has no include guards and undefines the macro again at its end).

    Slots declared with NB_SLOT_ALIAS name a CPython function that the backend
    stores verbatim. Consumers may leave NB_SLOT_ALIAS undefined to get NB_SLOT.

    The entries in this file are frozen and part of the versioned backend ABI.
    Any modifications to function order/signatures require bumping the major
    version, which is generally not acceptable. Appending entries is possible
    but requires careful planning/discussion and a minor ABI bump.

    The main characteristic of functions declared here is that they require
    access to backend state (``nb_internals``, etc.) or are unacceptably slow
    in the Python 3.10 limited ABI.

    Slots may require access to nanobind's internal data structures, and
    we conservatively assume they always do. Three rules determine whether
    it must be explicitly supplied as first argument:

    1. Operations defined by the *caller's context* (type lookup and creation,
       string/import caches, etc.) should get it explicitly.

    2. Operations involving an existing *nanobind object or type* can derive
       the internals pointer internally and do not need it.

    3. Exception object-related slots (``python_error``, ``builtin_exception``)
       should not depend on internals and do not receive the argument.

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#if !defined(NB_SLOT_ALIAS)
#  define NB_SLOT_ALIAS(ret, name, args, target) NB_SLOT(ret, name, args)
#endif

// --------------------------------------------------------------------------
// Error handling
// --------------------------------------------------------------------------

/// Raise a builtin_exception of the given kind (va_list core)
NB_SLOT(void, raise_v, (exception_type type, const char *fmt, va_list args))

/// Chain a new error of the given type onto the currently pending one
NB_SLOT(void, chain_v, (PyObject *type, const char *fmt, va_list args) noexcept)

// --------------------------------------------------------------------------
// Backend implementation of the 'python_error' exception class
// --------------------------------------------------------------------------

/// Fetch the pending Python error as a normalized exception object and
/// initialize 'p' with it
NB_SLOT(void, error_fetch, (error_payload *p) noexcept)

/// Initialize 'dst' with a copy of the exception state held by 'src'
NB_SLOT(void, error_copy,
        (const error_payload *src, error_payload *dst) noexcept)

/// Release a python_error's owned reference and what() buffer
NB_SLOT(void, error_release, (error_payload *p) noexcept)

/// Restore the exception as the pending Python error
NB_SLOT(void, error_restore, (PyObject *value) noexcept)

/// Render and cache a human-readable description incl. the traceback
NB_SLOT(const char *, error_what, (error_payload *p) noexcept)

// --------------------------------------------------------------------------
// Custom exception types and translators
// --------------------------------------------------------------------------

/// Create a new Python exception type (see nb::exception<T>)
NB_SLOT(PyObject *, exception_new,
        (nb_internals *p, PyObject *mod, const char *name, PyObject *base))

/// Prepend a C++ -> Python exception translation callback
NB_SLOT(void, register_exception_translator,
        (nb_internals *p, exception_translator translator, void *payload))

// --------------------------------------------------------------------------
// Module bootstrap
// --------------------------------------------------------------------------

/// Resolve the backend state of the given domain, creating it if needed, and
/// register the module 'm' (created by 'module_new') as one of its users.
/// Returns nullptr with a Python error set on failure.
NB_SLOT(nb_internals *, nb_module_init,
        (const char *domain, PyObject *m) noexcept)

/// Build a module definition and slot array on the backend's heap and return
/// the result of PyModuleDef_Init(). 'exec' is a 'int (*)(PyObject *)'
/// callback. 'flags' holds the ABI tag and is otherwise zero.
NB_SLOT(PyObject *, module_new,
        (const char *name, const char *doc, void *exec,
         uint32_t flags) noexcept)

/// Create a submodule of an existing module
NB_SLOT(PyObject *, submodule_new,
        (nb_internals *p, PyObject *base, const char *name,
         const char *doc) noexcept)

// --------------------------------------------------------------------------
// Object protocol helpers
// --------------------------------------------------------------------------

/// Try to roughly determine the length of a Python object
NB_SLOT(size_t, len_hint, (nb_internals *p, PyObject *o) noexcept)

/// Return a Python string for 'str' of size <= 'bound'. 0 indicates a
/// potentially non-literal string. The implementation tries to memoize
/// literals in a builtin cache. 'owned' returns whether the caller receives an
/// owned non-interned string. Returns nullptr with an error set on failure.
NB_SLOT(PyObject *, cached_string,
        (nb_internals *p, const char *str, size_t bound, bool *owned) noexcept)

/// PyObject_GetAttr() returning a new reference to 'def' instead of raising.
/// The limited API only has a single-lookup entry point from Python 3.13 on.
NB_SLOT(PyObject *, getattr_def,
        (PyObject *obj, PyObject *key, PyObject *def) noexcept)

// The operations below resolve their key via cached_string() ('str' and
// 'bound' follow its contract) and raise an exception when the underlying
// Python operation fails, except where noted.

/// PyObject_GetAttr() with a C string key
NB_SLOT(PyObject *, getattr_str,
        (nb_internals *p, PyObject *obj, const char *str, size_t bound))

/// Variant of the above that returns a new reference to 'def' instead of
/// raising when the attribute cannot be retrieved
NB_SLOT(PyObject *, getattr_str_def,
        (nb_internals *p, PyObject *obj, const char *str, size_t bound,
         PyObject *def) noexcept)

/// PyObject_SetAttr() with a C string key
NB_SLOT(void, setattr_str,
        (nb_internals *p, PyObject *obj, const char *str, size_t bound,
         PyObject *value))

/// PyObject_DelAttr() with a C string key
NB_SLOT(void, delattr_str,
        (nb_internals *p, PyObject *obj, const char *str, size_t bound))

/// PyObject_HasAttr() with a C string key (never raises)
NB_SLOT(bool, hasattr_str,
        (nb_internals *p, PyObject *obj, const char *str,
         size_t bound) noexcept)

/// Perform a vector call with positional arguments. 'base' is the callable
/// or, with 'call_flags::method', a method name looked up on 'args[0]'.
/// The bits of 'owned' mark arguments that the backend releases; the caller
/// keeps the others alive across the call. Arguments beyond the 64th are
/// always released. The slot in front of the first argument ('args[-1]', or
/// the 'self' entry of a method call) may be overwritten during the call.
NB_SLOT(PyObject *, obj_vectorcall,
        (nb_internals *p, PyObject *base, PyObject *const *args, size_t nargsf,
         uint64_t owned, uint32_t flags))

/// Perform a call with keyword arguments and/or '*'/'**' expansions,
/// described by 'nargs' entries of 'args'. With 'call_flags::method', the
/// first entry is the 'self' object. 'flags' also carries the ABI tag.
NB_SLOT(PyObject *, obj_vectorcall_ex,
        (nb_internals *p, PyObject *base, call_arg *args, size_t nargs,
         uint32_t flags))

/// PyObject_GetItem() with a C string key
NB_SLOT(PyObject *, getitem_str,
        (nb_internals *p, PyObject *obj, const char *str, size_t bound))

/// PyObject_SetItem() with a C string key
NB_SLOT(void, setitem_str,
        (nb_internals *p, PyObject *obj, const char *str, size_t bound,
         PyObject *value))

/// PyObject_DelItem() with a C string key
NB_SLOT(void, delitem_str,
        (nb_internals *p, PyObject *obj, const char *str, size_t bound))

/// Dictionary lookup with a C string key, raises KeyError when it is absent
NB_SLOT(PyObject *, dict_getitem_str,
        (nb_internals *p, PyObject *obj, const char *str, size_t bound))

/// PyDict_SetItem() with a C string key
NB_SLOT(void, dict_setitem_str,
        (nb_internals *p, PyObject *obj, const char *str, size_t bound,
         PyObject *value))

/// PyDict_DelItem() with a C string key
NB_SLOT(void, dict_delitem_str,
        (nb_internals *p, PyObject *obj, const char *str, size_t bound))

/// Membership test ('key in obj') with a C string key
NB_SLOT(bool, contains_str,
        (nb_internals *p, PyObject *obj, const char *str, size_t bound))

// --------------------------------------------------------------------------
// Sequence helpers
// --------------------------------------------------------------------------

/// Take ownership of 'n' objects and move them into a new tuple. When an
/// item is null or the allocation fails, release the items and return null
NB_SLOT(PyObject *, tuple_new, (PyObject **items, size_t n) noexcept)

/// Equivalent of 'tuple_new' that constructs a list
NB_SLOT(PyObject *, list_new, (PyObject **items, size_t n) noexcept)


/// Allocate a tuple builder for 'n' entries and return its null-initialized
/// item storage in 'items'. The caller fills a prefix with strong references
/// and finishes via 'seq_commit'. Returns null with an error set on failure
NB_SLOT(void *, tuple_alloc, (size_t n, PyObject ***items) noexcept)

/// Equivalent of 'tuple_alloc', but build a list
NB_SLOT(void *, list_alloc, (size_t n, PyObject ***items) noexcept)

/// Release a builder. Returns the sequence when 'n_valid' equals its size.
/// Any other value (e.g. SIZE_MAX) releases the stored references and
/// returns null, without allocating or setting an error
NB_SLOT(PyObject *, seq_commit, (void *builder, size_t n_valid) noexcept)

/// If the given sequence has the size 'size', return a pointer to its contents.
/// May produce a temporary.
NB_SLOT(PyObject **, seq_get_with_size,
        (PyObject *seq, size_t size, PyObject **temp) noexcept)

/// Like the above, but return the size instead of checking it.
NB_SLOT(PyObject **, seq_get,
        (PyObject *seq, size_t *size, PyObject **temp) noexcept)

// --------------------------------------------------------------------------
// Mapping helpers
// --------------------------------------------------------------------------

/// Snapshot a mapping into 2*'size' pointers alternating between keys and
/// values, owned by 'temp'. Null when the entries cannot be retrieved.
NB_SLOT(PyObject **, mapping_get,
        (PyObject *o, size_t *size, PyObject **temp) noexcept)

// --------------------------------------------------------------------------
// Function objects
// --------------------------------------------------------------------------

/// Create a Python function object for the given function record
NB_SLOT(PyObject *, nb_func_new,
        (nb_internals *p, const func_data_init_base *f) noexcept)

// --------------------------------------------------------------------------
// Generic type objects and instances
// --------------------------------------------------------------------------

/// Return a strong reference to ``t.__dict__``, where ``t`` is a heap type
NB_SLOT(PyObject *, type_dict, (PyObject *t) noexcept)

/// Return a strong reference to the attribute dictionary of the object 'o',
/// creating it if needed. Returns NULL when 'o' has no such dictionary.
NB_SLOT(PyObject *, inst_dict, (PyObject *o) noexcept)

/// Raw MRO lookup of the key 'key'. (wraps or emulates ``_PyType_LookupRef()``)
NB_SLOT(PyObject *, type_lookup,
        (nb_internals *p, PyObject *t, PyObject *key) noexcept)

/// Variant of the above taking a C string, which it memoizes.
NB_SLOT(PyObject *, type_lookup_str,
        (nb_internals *p, PyObject *t, const char *str, size_t bound) noexcept)

/// Make the type 't' immutable (PyType_Freeze, Python 3.15 and newer).
/// Returns false when the backend does not implement this operation.
NB_SLOT(bool, type_freeze, (PyObject *t))

// --------------------------------------------------------------------------
// Type objects and instances
// --------------------------------------------------------------------------

/// Create a Python type object for the given type record
NB_SLOT(PyObject *, nb_type_new,
        (nb_internals *p, const type_data_init *c) noexcept)

/// Extract a pointer to a C++ type underlying a Python object, if possible
NB_SLOT(bool, nb_type_get,
        (nb_internals *p, const std::type_info *t, PyObject *o, uint32_t flags,
         cleanup_list *cleanup, void **out) noexcept)

/// Cast a C++ type instance into a Python object. 'cpp_type_p' optionally
/// names the dynamic (most derived) type of polymorphic instances
NB_SLOT(PyObject *, nb_type_put,
        (nb_internals *p, const std::type_info *cpp_type,
         const std::type_info *cpp_type_p, void *value, rv_policy rvp,
         cleanup_list *cleanup, bool *is_new) noexcept)

/// Special version of 'nb_type_put' for unique pointers and ownership transfer
NB_SLOT(PyObject *, nb_type_put_unique,
        (nb_internals *p, const std::type_info *cpp_type,
         const std::type_info *cpp_type_p, void *value, cleanup_list *cleanup,
         bool cpp_delete) noexcept)

/// Try to relinquish ownership from Python object to a unique_ptr;
/// return true if successful, false if not. (Failure is only
/// possible if `cpp_delete` is true.)
NB_SLOT(bool, nb_type_relinquish_ownership,
        (PyObject *o, bool cpp_delete) noexcept)

/// Reverse the effects of nb_type_relinquish_ownership().
NB_SLOT(void, nb_type_restore_ownership,
        (PyObject *o, bool cpp_delete) noexcept)

/// Get a pointer to a user-defined 'extra' value associated with the nb_type t.
NB_SLOT(void *, nb_type_supplement, (PyObject *t) noexcept)

/// Check if the given python object represents a nanobind type of the
/// caller's domain
NB_SLOT(bool, nb_type_check, (nb_internals *p, PyObject *t) noexcept)

/// Return the size of the type wrapped by the given nanobind type object
NB_SLOT(size_t, nb_type_size, (PyObject *t) noexcept)

/// Return the alignment of the type wrapped by the given nanobind type object
NB_SLOT(size_t, nb_type_align, (PyObject *t) noexcept)

/// Return a unicode string representing the long-form name of the given type
NB_SLOT(PyObject *, nb_type_name, (PyObject *t) noexcept)

/// Return a unicode string representing the long-form name of object's type
NB_SLOT(PyObject *, nb_inst_name, (PyObject *o) noexcept)

/// Return the C++ type_info wrapped by the given nanobind type object
NB_SLOT(const std::type_info *, nb_type_info, (PyObject *t) noexcept)

/// Get a pointer to the instance data of a nanobind instance (nb_inst)
NB_SLOT(void *, nb_inst_ptr, (PyObject *o) noexcept)

/// Check if a Python type object wraps an instance of a specific C++ type
NB_SLOT(bool, nb_type_isinstance,
        (nb_internals *p, PyObject *obj, const std::type_info *t) noexcept)

/// Search for the Python type object associated with a C++ type
NB_SLOT(PyObject *, nb_type_lookup,
        (nb_internals *p, const std::type_info *t) noexcept)

/// Allocate an instance of type 't'
NB_SLOT(PyObject *, nb_inst_alloc, (PyTypeObject *t))

/// Allocate an zero-initialized instance of type 't'
NB_SLOT(PyObject *, nb_inst_alloc_zero, (PyTypeObject *t))

/// Allocate an instance of type 't' referencing the existing 'ptr'
NB_SLOT(PyObject *, nb_inst_reference,
        (PyTypeObject *t, void *ptr, PyObject *parent))

/// Allocate an instance of type 't' taking ownership of the existing 'ptr'
NB_SLOT(PyObject *, nb_inst_take_ownership, (PyTypeObject *t, void *ptr))

/// Call the destructor of the given python object
NB_SLOT(void, nb_inst_destruct, (PyObject *o) noexcept)

/// Zero-initialize a POD type and mark it as ready + to be destructed upon GC
NB_SLOT(void, nb_inst_zero, (PyObject *o) noexcept)

/// Copy-construct 'dst' from 'src' and mark it as ready (both must share
/// one nb_type). An uninitialized 'dst' afterwards has its 'destruct' flag
/// set; a live 'dst' is destructed first and keeps its previous flag value
NB_SLOT(void, nb_inst_copy, (PyObject *dst, const PyObject *src) noexcept)

/// Analogous to 'nb_inst_copy', using the move constructor
NB_SLOT(void, nb_inst_move, (PyObject *dst, const PyObject *src) noexcept)

/// Check if a particular instance uses a Python-derived type
NB_SLOT(bool, nb_inst_python_derived, (PyObject *o) noexcept)

/// Query the instance's ready (bit 0) and destruct (bit 1) flags
NB_SLOT(uint32_t, nb_inst_state_read, (PyObject *o) noexcept)

/// Overwrite the instance's ready (bit 0) and destruct (bit 1) flags
NB_SLOT(void, nb_inst_state_write, (PyObject *o, uint32_t state) noexcept)

// --------------------------------------------------------------------------
// Properties
// --------------------------------------------------------------------------

/// Create and install a Python property object
NB_SLOT(void, property_install,
        (nb_internals *p, PyObject *scope, const char *name, PyObject *getter,
         PyObject *setter, bool is_static) noexcept)

// --------------------------------------------------------------------------
// Trampolines (Python overrides of C++ virtual methods)
// --------------------------------------------------------------------------

/// Return the Python object that owns the C++ instance 'ptr' (borrowed)
NB_SLOT(PyObject *, trampoline_new, (nb_internals *p, void *ptr) noexcept)

/// Look up the Python override of a virtual method, filling 'ticket'
NB_SLOT(void, trampoline_enter,
        (PyObject *self, const char *name, uint64_t hash, bool pure,
         ticket *ticket))

/// Release the resources held by a method override 'ticket'
NB_SLOT(void, trampoline_leave, (ticket *ticket) noexcept)

// --------------------------------------------------------------------------
// Keep-alive relationships
// --------------------------------------------------------------------------

/// Ensure that 'patient' cannot be GCed while 'nurse' is alive
NB_SLOT(void, keep_alive_py,
        (nb_internals *p, PyObject *nurse, PyObject *patient))

/// Keep 'payload' alive until 'nurse' is GCed
NB_SLOT(void, keep_alive_ptr,
        (nb_internals *p, PyObject *nurse, void *payload,
         void (*deleter)(void *) noexcept) noexcept)

// --------------------------------------------------------------------------
// Implicit conversions
// --------------------------------------------------------------------------

/// Register an implicit conversion to 'dst'. 'src' is either a
/// 'const std::type_info *' naming the source type (is_predicate == false)
/// or a 'bool (*)(PyTypeObject *, PyObject *, cleanup_list *)' callback
/// that decides convertibility at runtime (is_predicate == true)
NB_SLOT(void, implicitly_convertible,
        (nb_internals *p, const std::type_info *dst, void *src,
         bool is_predicate) noexcept)

// --------------------------------------------------------------------------
// Enumerations
// --------------------------------------------------------------------------

/// Create a new enumeration type
NB_SLOT(PyObject *, enum_create,
        (nb_internals *p, const enum_data_init *e) noexcept)

/// Append an entry to an enumeration. For StrEnum members, 'str_value' carries
/// the string value; for all other enumerations it must be nullptr.
NB_SLOT(void, enum_append,
        (PyObject *tp, const char *name, int64_t value, const char *str_value,
         const char *doc) noexcept)

/// Query an enumeration's Python object -> integer value map
NB_SLOT(bool, enum_from_python,
        (nb_internals *p, const std::type_info *type, PyObject *o,
         int64_t *out, uint32_t flags) noexcept)

/// Query an enumeration's integer value -> Python object map
NB_SLOT(PyObject *, enum_from_cpp,
        (nb_internals *p, const std::type_info *type, int64_t value) noexcept)

/// Export enum entries to the parent scope
NB_SLOT(void, enum_export, (PyObject *tp))

// --------------------------------------------------------------------------
// ndarrays
// --------------------------------------------------------------------------

/// Try to import a reference-counted ndarray object via DLPack.
NB_SLOT(ndarray_handle *, ndarray_import,
        (nb_internals *p, PyObject *o, const ndarray_config *c,
         bool convert, cleanup_list *cleanup) noexcept)

/// Describe a local ndarray object using a DLPack capsule.
NB_SLOT(ndarray_handle *, ndarray_create,
        (nb_internals *p, const ndarray_create_args *a))

/// Increase the reference count of the given ndarray object
NB_SLOT(dlpack::dltensor *, ndarray_inc_ref, (ndarray_handle *h) noexcept)

/// Decrease the reference count of the given ndarray object
NB_SLOT(void, ndarray_dec_ref, (ndarray_handle *h) noexcept)

/// Wrap a ndarray_handle* into a PyCapsule
NB_SLOT(PyObject *, ndarray_export,
        (nb_internals *p, ndarray_handle *h, int framework, rv_policy policy,
         cleanup_list *cleanup) noexcept)

/// Check if an object represents an ndarray
NB_SLOT(bool, ndarray_check, (nb_internals *p, PyObject *o) noexcept)

// --------------------------------------------------------------------------
// Scalar conversions used by the arithmetic type casters
// --------------------------------------------------------------------------

/// Type-checked conversions of a Python object into a C++ scalar value;
/// 'flags' carries the cast_flags of the current conversion
NB_SLOT(bool, load_i8,
        (nb_internals *p, PyObject *o, uint32_t flags, int8_t *out) noexcept)
NB_SLOT(bool, load_u8,
        (nb_internals *p, PyObject *o, uint32_t flags, uint8_t *out) noexcept)
NB_SLOT(bool, load_i16,
        (nb_internals *p, PyObject *o, uint32_t flags, int16_t *out) noexcept)
NB_SLOT(bool, load_u16,
        (nb_internals *p, PyObject *o, uint32_t flags, uint16_t *out) noexcept)
NB_SLOT(bool, load_i32,
        (nb_internals *p, PyObject *o, uint32_t flags, int32_t *out) noexcept)
NB_SLOT(bool, load_u32,
        (nb_internals *p, PyObject *o, uint32_t flags, uint32_t *out) noexcept)
NB_SLOT(bool, load_i64,
        (nb_internals *p, PyObject *o, uint32_t flags, int64_t *out) noexcept)
NB_SLOT(bool, load_u64,
        (nb_internals *p, PyObject *o, uint32_t flags, uint64_t *out) noexcept)
NB_SLOT(bool, load_f32,
        (nb_internals *p, PyObject *o, uint32_t flags, float *out) noexcept)
NB_SLOT(bool, load_f64,
        (nb_internals *p, PyObject *o, uint32_t flags, double *out) noexcept)

/// Load a complex number; 'out' points to two doubles (real, imaginary).
/// std::complex<double> is layout-compatible; the spelling keeps <complex>
/// out of the core headers. Used by <nanobind/stl/complex.h>.
NB_SLOT(bool, load_cmplx,
        (nb_internals *p, PyObject *o, uint32_t flags, double *out) noexcept)

// --------------------------------------------------------------------------
// Datetime conversions
// --------------------------------------------------------------------------

// The two slots below transport the fields of Python datetime objects using
// the per-kind layouts documented in the 'datetime_kind' enumeration.

/// Unpack the fields of 'o' into 'parts' and return its type's entry in the
/// 'accept' bit mask. Subtype checks follow ascending kind order, and
/// 'parts' must fit the largest accepted layout. Returns 0 when nothing
/// matches, or -1 with a Python error set on failure.
NB_SLOT(int, datetime_unpack,
        (nb_internals *p, PyObject *o, uint32_t accept,
         int32_t *parts) noexcept)

/// Construct an instance of the datetime module type named by 'kind' from
/// 'parts'. Returns a new reference, or nullptr with a Python error set on
/// failure.
NB_SLOT(PyObject *, datetime_pack,
        (nb_internals *p, uint32_t kind, const int32_t *parts) noexcept)

// --------------------------------------------------------------------------
// Free-threading
// --------------------------------------------------------------------------

/// Lock/unlock a one-byte PyMutex on behalf of 'abi3t' extensions, whose
/// stable ABI lacks the mutex API (see nb::ft_mutex)
NB_SLOT(void, ft_mutex_lock, (void *m) noexcept)
NB_SLOT(void, ft_mutex_unlock, (void *m) noexcept)

// --------------------------------------------------------------------------
// Interpreter-state queries and backend configuration
// --------------------------------------------------------------------------

/// Attach a Python thread state to the calling thread, which acquires the GIL
/// in non-free-threaded builds. Nested calls are allowed. The return value is
/// an opaque token for 'tstate_release', or nullptr when the interpreter is
/// shutting down and can no longer be entered (see nb::gil_scoped_acquire).
NB_SLOT(void *, tstate_ensure, () noexcept)

/// Undo a 'tstate_ensure' call. A null token (i.e. a failed one) is ignored.
NB_SLOT(void, tstate_release, (void *token) noexcept)

/// PyGILState_Check() for TUs that cannot call it (limited API)
NB_SLOT(bool, gil_check, () noexcept)

/// Read a backend configuration flag (see the 'nb_flag' enumeration)
NB_SLOT(uint32_t, read_flag, (nb_internals *p, nb_flag f) noexcept)

/// Write a backend configuration flag
NB_SLOT(void, write_flag, (nb_internals *p, nb_flag f, uint32_t value))

/// Check whether the Python interpreter is still running (nb::is_alive)
NB_SLOT(bool, is_alive, () noexcept)

/// Resolve and cache 'c->module.c->attr' (cold path of import_cache::get())
NB_SLOT(PyObject *, import_cached,
        (nb_internals *p, import_cache *c) noexcept)

// --------------------------------------------------------------------------
// Raw vector calls
// --------------------------------------------------------------------------

// Python < 3.12 does not expose vector calls in the stable ABI. The
// following pipes them through to frontends that need them

/// PyObject_Vectorcall()
NB_SLOT_ALIAS(PyObject *, vectorcall,
              (PyObject *callable, PyObject *const *args, size_t nargsf,
               PyObject *kwnames),
              PyObject_Vectorcall)

/// PyObject_VectorcallMethod().
NB_SLOT_ALIAS(PyObject *, vectorcall_method,
              (PyObject *name, PyObject *const *args, size_t nargsf,
               PyObject *kwnames),
              PyObject_VectorcallMethod)

#undef NB_SLOT
#undef NB_SLOT_ALIAS
