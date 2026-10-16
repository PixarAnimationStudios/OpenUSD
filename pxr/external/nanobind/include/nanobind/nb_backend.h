/*
    nanobind/nb_backend.h: ABI contract between nanobind extensions and the backend.

    Three separate contracts govern binary compatibility in nanobind, and
    they must not be confused:

    - The *platform ABI* (NB_PLATFORM_ABI_TAG in nb_platform.h, a string)
      answers whether two binaries may call each other at all. It captures
      facts about the compilation environment such as the compiler family, C++
      standard library and its ABI variant, MSVC CRT flavor, debug mode,
      free-threading, and the pre-release number (dev snapshots may change the
      contracts below without moving their versions, so each snapshot counts
      as its own platform).

    - The *backend ABI* is declared in this file and describes the contract
      between nanobind *headers* compiled into an extension and a compiled
      *backend*. A test ``tests/test_abi_layout.cpp`` asserts the memory
      layout of the data structures declared here.

      Records with flag fields pack the ``NB_BACKEND_ABI_MINOR`` into
      the top 8 bits, which allows a single backend to service requests
      by callers with different ABI minor versions.

    - The *internals ABI* (NB_INTERNALS_VERSION in src/nb_internals.h) governs
      whether two *backends* may share a state. Packages with an
      incompatible internals ABI are isolated from each other.

    A few additional aspects belong to the backend ABI but are not in this file
    for technical reasons:

    - exceptions: ``python_error``, ``builtin_exception``.
    - trampolines: ``trampoline``, ``ticket``.
    - ND-arrays: ``ndarray_config``, ``ndarray_create_args``.

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

/// Major version of the backend ABI. Advances after ABI-breaking backend
/// changes. Such changes are to be avoided at all costs.
#define NB_BACKEND_ABI_MAJOR 1

/// Minor version of the backend ABI. Advances after ABI-compatible changes
/// (appending flag-gated fields, adding enum bits, etc.).
#define NB_BACKEND_ABI_MINOR 0

/// Patch revision signaling internal improvements without effect on the ABI
/// contract. Together with the ABI macros above, it forms the version of the
/// ``nanobind-backend`` package (maintained by ``python src/version.py -b``).
#define NB_BACKEND_REVISION 0

/// Inline namespace holding weakly exported symbols like exceptions.
#define NB_BACKEND_ABI_NS NB_CONCAT(abi, NB_BACKEND_ABI_MAJOR)

NAMESPACE_BEGIN(NB_NAMESPACE)

/// Strategies to cast an unknown C++ object to Python.
class rv_policy {
public:
    enum value : uint8_t {
        automatic_v,
        automatic_reference_v,
        take_ownership_v,
        copy_v,
        move_v,
        reference_v,
        reference_internal_v,
        none_v
    };

    /// Helper type, which tags policies at compile time, while implicitly
    /// converting to the runtime ``rv_policy::value``.
    template <value V> struct policy_tag {
        static constexpr value policy = V;
        constexpr operator rv_policy() const noexcept { return rv_policy(V); }
        constexpr operator value() const noexcept { return V; }
    };

    constexpr rv_policy(value v) noexcept : m_value(v) {}
    template <value V>
    constexpr rv_policy(policy_tag<V>) noexcept : m_value(V) {}
    constexpr operator value() const noexcept { return m_value; }

    static constexpr policy_tag<automatic_v> automatic {};
    static constexpr policy_tag<automatic_reference_v> automatic_reference {};
    static constexpr policy_tag<take_ownership_v> take_ownership {};
    static constexpr policy_tag<copy_v> copy {};
    static constexpr policy_tag<move_v> move {};
    static constexpr policy_tag<reference_v> reference {};
    static constexpr policy_tag<reference_internal_v> reference_internal {};
    static constexpr policy_tag<none_v> none {};

private:
    value m_value;
};

/// Tag distinguishing the exception kinds for 'builtin_exception' and 'raise_v'
enum class exception_type {
    runtime_error, stop_iteration, index_error, key_error, value_error,
    type_error, buffer_error, import_error, attribute_error, next_overload
};

NAMESPACE_BEGIN(dlpack)
struct dltensor;
struct dtype;
NAMESPACE_END(dlpack)

NAMESPACE_BEGIN(detail)

/// The caller's NB_BACKEND_ABI_MINOR shifted to the top 8 bits. The frontend
/// ORs it into the following flags words. The backend never sees a tag newer
/// than its own, since nb_backend_fill() rejects such callers:
///
/// - func_data_init_base::flags, type_data_init::flags, enum_data_init::flags
/// - ndarray_config::flags, ndarray_create_args::flags
/// - the 'flags' arguments of the obj_vectorcall_ex and module_new slots
#define NB_ABI_MINOR_TAG ((uint32_t) NB_BACKEND_ABI_MINOR << 24)

struct ndarray_handle;
struct ndarray_config;
struct ndarray_create_args;
struct ticket;
struct import_cache;

/// Opaque record holding the backend state of one domain
struct nb_internals;

#if !defined(NB_BUILD)
/// This extension's backend state, set during module initialization
NB_HIDDEN inline nb_internals *internals = nullptr;
#  define NB_CTX ::nanobind::detail::internals
/// Variant for callers that hold a cleanup_list (which is still unused atm.)
#  define NB_CTX_C(cleanup) ((void) (cleanup), ::nanobind::detail::internals)
#else
/// Backend code threads its state explicitly and must not use API entry
/// points that inject the extension-side pointer. This function is never
/// defined; any use fails at link time.
extern nb_internals *nb_ctx_unavailable() noexcept;
#  define NB_CTX ::nanobind::detail::nb_ctx_unavailable()
#  define NB_CTX_C(cleanup)                                                    \
    ((void) (cleanup), ::nanobind::detail::nb_ctx_unavailable())
#endif

/// Backend configuration flags accessed via read_flag/write_flag.
enum class nb_flag : uint32_t {
    leak_warnings = 0,
    implicit_cast_warnings = 1
};

/// Types of the Python 'datetime' module handled by the 'datetime_unpack'
/// and 'datetime_pack' slots, along with their integer field layouts.
/// 'datetime_unpack' accepts bitwise combinations.
enum class datetime_kind : uint32_t {
    none      = 0,
    datetime  = 1, // year, month, day, hour, minute, second, microsecond, fold
    date      = 2, // year, month, day
    time      = 4, // hour, minute, second, microsecond, fold
    timedelta = 8  // days, seconds, microseconds
};

/// Flags that can be passed to type casters
enum cast_flags : uint32_t {
    // Enable implicit conversions
    convert = (1 << 0),

    // Passed to the 'self' argument in a constructor call (__init__)
    construct = (1 << 1),

    // Indicates that the function dispatcher should accept 'None' arguments
    accepts_none = (1 << 2),

    /// The target binds the value by reference or value (not as a pointer), so
    /// a 'None' argument has no valid mapping.
    none_disallowed = (1 << 3),

    // Indicates that this cast is performed by nb::cast or nb::try_cast.
    // This implies that objects added to the cleanup list may be
    // released immediately after the caster's final output value is
    // obtained, i.e., before it is used.
    manual = (1 << 4),

    /// Indicate that a type is being constructed by nb_type_vectorcall. The
    /// call dispatcher uses this hint to avoid type-checking ``self``
    trusted = (1 << 5)
};

/// Flags passed to the function binding API (\ref func_new)
enum class func_flags : uint32_t {
    /// Did the user specify a name for this function, or is it anonymous?
    has_name = (1 << 0),

    /// Did the user specify a scope in which this function should be installed?
    has_scope = (1 << 1),

    /// Did the user specify a docstring?
    has_doc = (1 << 2),

    /// Did the user specify nb::arg/arg_v annotations for all arguments?
    has_args = (1 << 3),

    /// Does the function signature contain an *args-style argument?
    has_var_args = (1 << 4),

    /// Does the function signature contain an *kwargs-style argument?
    has_var_kwargs = (1 << 5),

    /// Is this function a method of a class?
    is_method = (1 << 6),

    /// Is this function a method called __init__? (automatically generated)
    is_constructor = (1 << 7),

    /// Can this constructor be used to perform an implicit conversion?
    is_implicit = (1 << 8),

    /// Is this function an arithmetic operator?
    is_operator = (1 << 9),

    /// When the function is GCed, do we need to call func_data_init::free_capture?
    has_free = (1 << 10),

    /// Should the func_new() call return a new reference?
    return_ref = (1 << 11),

    /// Does this overload specify a custom function signature (for docstrings, typing)
    has_signature = (1 << 12),

    /// Does this function potentially modify the elements of the PyObject*[] array
    /// representing its arguments? (nb::keep_alive() or call_policy annotations)
    can_mutate_args = (1 << 13),

    /// Is this overload a copy constructor? The dispatcher then never
    /// raises the call-wide 'convert' flag: implicit conversion of the
    /// source argument would recurse infinitely
    is_copy_constructor = (1 << 14)
};

/// Public flags characterizing type objects. Their values are frozen by the
/// ABI contract. Bits 14..18 are free, bits 19..23 belong to ``type_init_flags``
/// below, and bits 24..31 hold the ABI tag.
enum class type_flags : uint32_t {
    /// Does the type provide a C++ destructor?
    is_destructible          = (1 << 0),

    /// Does the type provide a C++ copy constructor?
    is_copy_constructible    = (1 << 1),

    /// Does the type provide a C++ move constructor?
    is_move_constructible    = (1 << 2),

    /// Is the 'destruct' field of the type_data_init structure set?
    has_destruct             = (1 << 3),

    /// Is the 'copy' field of the type_data_init structure set?
    has_copy                 = (1 << 4),

    /// Is the 'move' field of the type_data_init structure set?
    has_move                 = (1 << 5),

    /// This type does not permit subclassing from Python
    is_final                 = (1 << 6),

    /// Instances of this type support dynamic attribute assignment
    has_dynamic_attr         = (1 << 7),

    /// The class uses an intrusive reference counting approach
    intrusive_ptr            = (1 << 8),

    /// Is this a class that inherits from enable_shared_from_this?
    /// If so, type_data_init::keep_shared_from_this_alive is also set.
    has_shared_from_this     = (1 << 9),

    /// Instances of this type can be referenced by 'weakref'
    is_weak_referenceable    = (1 << 10),

    /// A custom signature override was specified
    has_signature            = (1 << 11),

    /// The class implements __class_getitem__ similar to typing.Generic
    is_generic               = (1 << 12),

    /// Does the type opt into instance pooling? (nb::pooled)
    pooled                   = (1 << 13)
};

/// Flags about a type that are only relevant when it is being created.
/// May not overlap with type_flags.
enum class type_init_flags : uint32_t {
    /// Is the 'supplement_size' field of the type_data_init structure set?
    has_supplement           = (1 << 19),

    /// Is the 'doc' field of the type_data_init structure set?
    has_doc                  = (1 << 20),

    /// Is the 'base' field of the type_data_init structure set?
    has_base                 = (1 << 21),

    /// Is the 'base_py' field of the type_data_init structure set?
    has_base_py              = (1 << 22),

    /// This type provides extra PyType_Slot fields
    has_type_slots           = (1 << 23),

    all_init_flags           = (0x1f << 19)
};

/// Flags characterizing enumeration bindings. Bits 24..31 hold the ABI tag.
enum class enum_flags : uint32_t {
    /// Is this an arithmetic enumeration?
    is_arithmetic            = (1 << 1),

    /// Is the number type underlying the enumeration signed?
    is_signed                = (1 << 2),

    /// Is the underlying enumeration type Flag?
    is_flag                = (1 << 3),

    /// Is this a string-valued enumeration (StrEnum)?
    is_str                 = (1 << 4)
};

/**
 * Helper class to clean temporaries created by function dispatch. Entry 0
 * stores the 'self' object of method calls for rv_policy::reference_internal.
 */
struct cleanup_list {
public:
    static constexpr uint32_t Small = 5;

    cleanup_list(PyObject *self, nb_internals *ctx = nullptr) :
        m_size{1},
        m_capacity{Small},
        m_data{m_local},
        m_ctx{ctx} {
        m_local[0] = self;
    }

    ~cleanup_list() = default;

    /// Append a single PyObject to the cleanup stack
    NB_INLINE void append(PyObject *value) noexcept {
        if (NB_UNLIKELY(m_size >= m_capacity))
            expand();
        m_data[m_size++] = value;
    }

    NB_INLINE PyObject *self() const {
        return m_local[0];
    }

    /// Backend state of the domain that owns the dispatched function, or
    /// nullptr when the creator of the list had no such context
    NB_INLINE nb_internals *ctx() const { return m_ctx; }

    /// Decrease the reference count of all appended objects
    void release() noexcept;

    /// Does the list contain any entries? (besides the 'self' argument)
    bool used() { return m_size != 1; }

    /// Return the size of the cleanup stack
    size_t size() const { return m_size; }

    /// Subscript operator
    PyObject *operator[](size_t index) const { return m_data[index]; }

protected:
    /// Out of memory, expand..
    void expand() noexcept;

protected:
    uint32_t m_size;
    uint32_t m_capacity;
    PyObject **m_data;
    nb_internals *m_ctx;
    PyObject *m_local[Small];
};

inline void cleanup_list::release() noexcept {
    // Don't decrease the reference count of the first element,
    // it stores the 'self' element.
    for (size_t i = 1; i < m_size; ++i)
        Py_DECREF(m_data[i]);
    if (m_capacity != Small)
        PyMem_Free(m_data);
    m_data = nullptr;
}

NB_NOINLINE inline void cleanup_list::expand() noexcept {
    uint32_t new_capacity = m_capacity * 2;
    PyObject **new_data = (PyObject **) PyMem_Malloc(new_capacity * sizeof(PyObject *));
    if (NB_UNLIKELY(!new_data)) {
        fprintf(stderr, "Critical nanobind error: "
                        "cleanup_list::expand(): out of memory!\n");
        abort();
    }
    memcpy(new_data, m_data, m_size * sizeof(PyObject *));
    if (m_capacity != Small)
        PyMem_Free(m_data);
    m_data = new_data;
    m_capacity = new_capacity;
}

/// Describes a function argument binding
struct arg_data_init {
    /// Argument name (nullptr if unnamed)
    const char *name;

    /// Overrides the argument type in docstrings and stubs (or nullptr)
    const char *signature;

    /// Default argument value (or nullptr)
    PyObject *value;

    /// Argument-specific cast flags (see the 'cast_flags' enumeration)
    uint32_t flag;

    /// Unused, claimable by a future minor ABI revision
    uint32_t unused;
};

/// Flags of the 'obj_vectorcall' and 'obj_vectorcall_ex' backend slots. The
/// latter also carries the ABI tag in bits 24..31.
enum class call_flags : uint32_t {
    /// 'base' is a method name to be looked up on the first argument
    method = (1 << 0),

    /// 'base' is a strong reference that the backend releases
    base_owned = (1 << 1)
};

/// Role of a 'call_arg' entry
enum class call_arg_kind : uint32_t {
    positional = 0,
    keyword = 1,
    /// Positional expansion of an iterable ('*args')
    args = 2,
    /// Keyword expansion of a mapping ('**kwargs')
    kwargs = 3
};

/// One argument of a call with keyword arguments or '*'/'**' expansion.
/// Both fields hold strong references that the backend releases.
struct call_arg {
    /// Argument value, or the operand of an expansion
    PyObject *value;

    /// Keyword name (only for 'call_arg_kind::keyword', otherwise null)
    PyObject *name;

    call_arg_kind kind;

    /// Unused, claimable by a future minor ABI revision
    uint32_t unused;
};

/// Describes a function binding
struct func_data_init_base {
    // A small amount of space to capture data used by the function/closure
    void *capture[3];

    // Callback to clean up the 'capture' field
    void (*free_capture)(void *);

    /// Type-erased trampoline implementing the function call
    PyObject *(*impl)(void *, PyObject **, uint32_t, cleanup_list *);

    /// Function signature description
    const char *descr;

    /// C++ types referenced by 'descr'
    const std::type_info **descr_types;

    /// Supplementary flags
    uint32_t flags;

    /// Total number of parameters accepted by the C++ function; nb::args
    /// and nb::kwargs parameters are counted as one each. If the
    /// 'has_args' flag is set, then there is one arg_data_init structure
    /// for each of these.
    uint16_t nargs;

    /// Number of parameters to the C++ function that may be filled from
    /// Python positional arguments without additional ceremony.
    /// nb::args and nb::kwargs parameters are not counted in this total, nor
    /// are any parameters after nb::args or after a nb::kw_only annotation.
    /// The parameters counted here may be either named (nb::arg("name")) or
    /// unnamed (nb::arg()).  If unnamed, they are effectively positional-only.
    /// nargs_pos is always <= nargs.
    uint16_t nargs_pos;

    /// Function name
    const char *name;

    /// Docstring
    const char *doc;

    /// Scope (e.g. module) in which the function will be installed
    PyObject *scope;
};

/// Sized version of func_data_init_base
template<size_t Size> struct func_data_init : func_data_init_base {
    arg_data_init args[Size];
};

template<> struct func_data_init<0> : func_data_init_base {};

/// Describes a type binding
struct type_data_init {
    /// Packed size and alignment of a C++ instance (see \ref type_size_align())
    uint32_t size_align;

    /// Combination of ``type_flags``, ``type_init_flags``, and the ABI tag.
    uint32_t flags;

    /// Type name (or a custom signature, see nb::sig)
    const char *name;

    /// C++ RTTI record of the bound type
    const std::type_info *type;

    /// Destruct an instance
    void (*destruct)(void *);

    /// Copy-construct an instance from another one
    void (*copy)(void *, const void *);

    /// Move-construct an instance from another one
    void (*move)(void *, void *) noexcept;

    /// Inform an intrusively reference-counted instance about its Python side
    void (*set_self_py)(void *, PyObject *) noexcept;

    /// Keep-alive callback of types deriving from enable_shared_from_this
    bool (*keep_shared_from_this_alive)(PyObject *) noexcept;

    /// Scope (e.g. module) in which the type will be installed
    PyObject *scope;

    /// C++ RTTI record of the base type, if any
    const std::type_info *base;

    /// Python object of the base type, if specified directly
    PyTypeObject *base_py;

    /// Docstring
    const char *doc;

    /// Custom PyType_Slot entries to install
    const PyType_Slot *type_slots;

    /// Instance pool capacity (see nb::pooled)
    uint32_t pool_capacity;

    /// Size of the nb::supplement<T> storage region
    uint32_t supplement_size;
};

/// Encode the size and alignment of a type into a packed 32-bit word
constexpr uint32_t type_size_align(size_t size, size_t align) {
    uint32_t align_log2 = 0;
    while (((size_t) 1 << align_log2) < align)
        align_log2++;
    return ((uint32_t) (size / align) << 5) | align_log2;
}

/// Describes an enumeration binding
struct enum_data_init {
    /// C++ RTTI record of the enumeration type
    const std::type_info *type;

    /// Python scope (module or type) in which the enumeration is installed
    PyObject *scope;

    /// Name of the enumeration
    const char *name;

    /// Docstring (or nullptr)
    const char *docstr;

    /// Enumeration flags (see the 'enum_flags' enumeration)
    uint32_t flags;

    /// Unused, claimable by a future minor ABI revision
    uint32_t unused;
};

/// Callback that converts a caught C++ exception into a Python error state.
/// The second parameter forwards the payload registered alongside it.
typedef void (*exception_translator)(const std::exception_ptr &, void *);

/**
 * Storage of the ``python_error`` exception class (see ``nb_error.h``). The
 * backend constructs and throws these objects while extensions catch them,
 * sometimes by value, hence every binary in a process must agree on the
 * layout. It is frozen within a major version.
 */
struct error_payload {
    /// Normalized exception object owned by the payload
    PyObject *value;

    /// Backend-private state
    void *internal[2];
};

/// ``nb_backend_slots.h`` specifies the ABI function interface and is potentially
/// included several times here depending on compilation mode.
#if !defined(NB_BACKEND_MODULE) || defined(NB_BUILD)
#define NB_SLOT(ret, name, args) NB_CORE ret name args;
// NB_SLOT_ALIAS resolves to the CPython function itself
#define NB_SLOT_ALIAS(ret, name, args, target)                                 \
    inline ret (*const name) args = &target;
#include "nb_backend_slots.h"
#endif

/// Number of ABI function slots used by this version of nanobind
#define NB_SLOT(ret, name, args) +1
constexpr uint16_t nb_backend_slot_count = (uint16_t) (0
#include "nb_backend_slots.h"
);

/// Backend ABI function table, populated by the backend module's ``fill()``.
struct nb_backend_table {
    uint16_t slot_count;  // Number of slots expected by the caller
    uint16_t abi_minor;   // Caller's NB_BACKEND_ABI_MINOR
    uint8_t reserved[4];  // Unused, zero for now.

#define NB_SLOT(ret, name, args) ret (*name) args;
#include "nb_backend_slots.h"
};

#define NB_SLOT(ret, name, args)                                               \
    static_assert(sizeof(ret (*) args) == sizeof(void *),                      \
                  "boundary function pointers must be pointer-sized");
#include "nb_backend_slots.h"
static_assert(sizeof(nb_backend_table) == 8 + nb_backend_slot_count * sizeof(void *),
              "nb_backend_table contains unexpected padding");

#if defined(NB_BACKEND_MODULE)
/// Create a default-initialized ABI function table.
constexpr nb_backend_table nb_backend_table_init() noexcept {
    nb_backend_table result {};
    result.slot_count = nb_backend_slot_count;
    result.abi_minor = NB_BACKEND_ABI_MINOR;
    return result;
}

/// The function table is declared as hidden and inline so that all versions
/// of it are merged into a single private copy per DSO.
NB_HIDDEN inline nb_backend_table nb_backend = nb_backend_table_init();

/// Has the table been filled by a backend module? (see the NB_MODULE bootstrap)
NB_HIDDEN inline bool nb_backend_ready = false;

#define NB_BACKEND_MODULE_STR NB_TOSTRING(NB_BACKEND_MODULE)

/// Import a backend module and ask it to fill our function table
NB_NOINLINE inline bool nb_backend_init(const char *extension) noexcept {
    if (nb_backend_ready)
        return true;

    PyObject *mod = PyImport_ImportModule(NB_BACKEND_MODULE_STR);
    if (!mod) {
        if (PyErr_ExceptionMatches(PyExc_ModuleNotFoundError))
            PyErr_Format(
                PyExc_ImportError,
                "Importing the extension '%s' failed because the nanobind "
                "backend module '" NB_BACKEND_MODULE_STR "' is not installed."
#if defined(NB_BACKEND_PYPI)
                " Install it via 'pip install " NB_TOSTRING(NB_BACKEND_PYPI) "'."
#endif
                , extension);
        return false;
    }

    // Provide the platform ABI tag both as an explicit string, and as capsule name.
    PyObject *capsule = PyCapsule_New(&nb_backend, NB_PLATFORM_ABI_TAG, nullptr);
    PyObject *result =
        capsule ? PyObject_CallMethod(mod, "fill", "(isO)",
                                      (int) NB_BACKEND_ABI_MAJOR,
                                      NB_PLATFORM_ABI_TAG, capsule)
                : nullptr;
    Py_XDECREF(capsule);
    Py_DECREF(mod);
    if (!result)
        return false; // fill() has already phrased a precise error
    Py_DECREF(result);

    nb_backend_ready = true;
    return true;
}
#else
/// Linked build modes have no bootstrap; see NB_MODULE in nb_defs.h
NB_INLINE bool nb_backend_init(const char *) noexcept { return true; }
#endif

#if defined(NB_BACKEND_MODULE) && !defined(NB_BUILD)
/// Split mode does not have access to the CPython vector call functions and
/// reaches them through the table instead.
NB_INLINE PyObject *vectorcall(PyObject *callable, PyObject *const *args,
                               size_t nargsf, PyObject *kwnames) {
    return nb_backend.vectorcall(callable, args, nargsf, kwnames);
}

NB_INLINE PyObject *vectorcall_method(PyObject *name, PyObject *const *args,
                                      size_t nargsf, PyObject *kwnames) {
    return nb_backend.vectorcall_method(name, args, nargsf, kwnames);
}
#endif

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
