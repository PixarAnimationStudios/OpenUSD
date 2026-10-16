#pragma once

#if defined(__GNUC__)
// Don't warn about missing fields in PyTypeObject declarations
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#elif defined(_MSC_VER)
// Silence warnings that MSVC reports in robin_*.h
#  pragma warning(disable: 4127) // conditional expression is constant
#  pragma warning(disable: 4324) // structure was padded due to alignment specifier
#  pragma warning(disable: 4293) // shift count negative or too big  <-- erroneously raised in a constexpr-disabled block
#  pragma warning(disable: 4310) // cast truncates constant value <-- erroneously raised in a constexpr-disabled block
#endif

#include <nanobind/nanobind.h>
#include <tsl/robin_map.h>
#if defined(NB_FREE_THREADED)
#  include <atomic>
#if !defined(_WIN32)
#  include <pthread.h>
#endif
#endif
#include <cstring>
#include <string_view>
#include <functional>
#include <vector>
#include "hash.h"

#if defined(_AIX) && defined(func_data)
# undef func_data
#endif

#if TSL_RH_VERSION_MAJOR != 1 || TSL_RH_VERSION_MINOR < 3
#  error nanobind depends on tsl::robin_map, in particular version >= 1.3.0, <2.0.0
#endif

#if defined(Py_LIMITED_API) && Py_LIMITED_API < 0x030C0000
#  error "Compiling the nanobind backend under the limited API requires Python >= 3.12"
#endif

/* Can this build use the interpreter views and guards of PEP 788 to attach a
   thread state without risking a hang at interpreter shutdown? They arrived in
   Python 3.15, and only enter the limited API at that version. The backend
   never targets a limited API newer than 3.12, so a linked-mode stable ABI
   build has to fall back to PyGILState_Ensure(). */
#if PY_VERSION_HEX >= 0x030F0000 && !defined(Py_LIMITED_API)
#  define NB_HAVE_INTERP_VIEW 1
#endif

#if PY_VERSION_HEX < 0x030C0000
#  include <structmember.h>
#  define Py_T_PYSSIZET  T_PYSSIZET
#  define Py_T_OBJECT_EX T_OBJECT_EX
#  define Py_READONLY    READONLY
#endif

#if defined(_MSC_VER)
#  define NB_THREAD_LOCAL __declspec(thread)
#else
#  define NB_THREAD_LOCAL __thread
#endif

// When forwarding vector calls between functions that are known to be implemented by
// nanobind, it uses an extended ABI that may set one additional bit to communicate
// that the implicit 'self' argument is trusted and does not need to be type-checked.
#define NB_VECTORCALL_TRUSTED_SELF (NB_VECTORCALL_ARGUMENTS_OFFSET >> 1)

// Decodes the call argument count of this extended ABI. The public
// ``NB_VECTORCALL_NARGS()`` macro leaves the extra bit in place.
#define NB_VECTORCALL_NARGS_EXT(n)                                              \
    (NB_VECTORCALL_NARGS(n) & ~(Py_ssize_t) NB_VECTORCALL_TRUSTED_SELF)

/// Strip the ABI tag (see NB_ABI_MINOR_TAG) from a flags word
#define NB_ABI_FLAGS(flags) ((uint32_t) (flags) & 0xFFFFFF)

/// Version of nanobind's internal data structures. A mismatch isolates
/// backends from each other instead of breaking them: their type universes
/// simply become disjoint.
#ifndef NB_INTERNALS_VERSION
#  define NB_INTERNALS_VERSION 22
#endif

/// Backends compiled under the limited API cache type slots and lay out
/// their internals differently, so they must not share state with others
#if defined(Py_LIMITED_API)
#  define NB_STABLE_ABI "_stable"
#else
#  define NB_STABLE_ABI ""
#endif

/// Prefix of the dictionary key under which a backend stores the
/// ``nb_internals`` record of a domain (see nb_module_init). Backend binaries
/// in one process share the state of a domain exactly when their keys match.
#define NB_INTERNALS_KEY                                                       \
    NB_PLATFORM_ABI_TAG "_a" NB_TOSTRING(NB_BACKEND_ABI_MAJOR)                 \
                        "_v" NB_TOSTRING(NB_INTERNALS_VERSION) NB_STABLE_ABI

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

/// Abort the process with a fatal error
#if defined(__GNUC__)
    __attribute__((noreturn, __format__(__printf__, 1, 2)))
#else
    [[noreturn]]
#endif
extern void fail(const char *fmt, ...) noexcept;

/// Abort the process, describing the exception that is currently handled
[[noreturn]] extern void fail_exception(const char *context,
                                        const char *name) noexcept;

/* Assertion checks. Each check() expands to a branch that the compiler is
   told to expect to succeed, and the message is only assembled once the
   failure is certain. Release builds compile the messages out entirely
   (NB_COMPACT_ASSERTIONS), except in backend modules, whose users have no way
   to rebuild them with a different set of flags. */
#if defined(NB_COMPACT_ASSERTIONS)
[[noreturn]] extern void fail_unspecified() noexcept;
#  define check(cond, ...)                                                     \
      do {                                                                     \
          if (NB_UNLIKELY(!(cond)))                                            \
              nanobind::detail::fail_unspecified();                            \
      } while (0)
#else
/* Out-of-line stub that carries the arguments of a failing check() over to
   fail(). Calling the variadic fail() from the check site would reserve
   outgoing-argument stack space in the frame of every enclosing function,
   which some ABIs charge for on the fast path (Apple arm64 passes variadic
   arguments on the stack). This stub receives them in registers, which leaves
   the enclosing frame identical to a build without messages. */
#  if defined(__GNUC__)
#    pragma GCC diagnostic push
     // 'fmt' arrives as a parameter here; the check() macro below verifies it
#    pragma GCC diagnostic ignored "-Wformat-security"
#    pragma GCC diagnostic ignored "-Wformat-nonliteral"
#  endif
template <typename... Ts>
[[noreturn]] NB_NOINLINE void fail_cold(const char *fmt, Ts... args) noexcept {
    fail(fmt, args...);
}
#  if defined(__GNUC__)
#    pragma GCC diagnostic pop
#  endif

/* A dead direct call to fail() keeps the format attribute checking the format
   string against its arguments, which the stub above cannot do. Both GCC and
   Clang fold it away and generate identical code with and without it. MSVC
   has no such check to begin with. */
#  if defined(__GNUC__)
#    define NB_CHECK_FMT(...) if (false) nanobind::detail::fail(__VA_ARGS__)
#  else
#    define NB_CHECK_FMT(...) ((void) 0)
#  endif

#  define check(cond, ...)                                                     \
      do {                                                                     \
          if (NB_UNLIKELY(!(cond))) {                                          \
              NB_CHECK_FMT(__VA_ARGS__);                                       \
              nanobind::detail::fail_cold(__VA_ARGS__);                        \
          }                                                                    \
      } while (0)
#endif

/// Internal version of a function argument record (see arg_data_init in
/// nb_backend.h)
struct arg_data {
    /// Argument name (nullptr if unnamed)
    const char *name;

    /// Overrides the argument type in docstrings and stubs (or nullptr)
    const char *signature;

    /// Interned Python version of 'name'
    PyObject *name_py;

    /// Default argument value (or nullptr)
    PyObject *value;

    /// Argument-specific cast flags (see the 'cast_flags' enumeration)
    uint32_t flag;
};

/// Internal version of a function binding record (see func_data_init_base in
/// nb_backend.h)
struct func_data {
    /// Space to capture data used by the function/closure
    void *capture[3];

    /// Callback to clean up the 'capture' field
    void (*free_capture)(void *);

    /// Type-erased trampoline implementing the function call
    PyObject *(*impl)(void *, PyObject **, uint32_t, cleanup_list *);

    /// Function signature description
    const char *descr;

    /// C++ types referenced by 'descr'
    const std::type_info **descr_types;

    /// Supplementary flags
    uint32_t flags;

    /// Total number of parameters accepted by the C++ function
    uint16_t nargs;

    /// Number of parameters that can be filled from positional arguments
    uint16_t nargs_pos;

    /// Function name
    const char *name;

    /// Docstring
    const char *doc;

    /// Argument records (nargs entries when func_flags::has_args is set)
    arg_data *args;

    /// Custom signature override (nb::sig), or nullptr
    char *signature;
};

/// Runtime-only type flags maintained by the backend. They occupy bits 24+
/// of type_data::flags, above the public type_flags (bits 0..13) and
/// type_init_flags (bits 19..23, stripped during translation).
enum class type_flags_internal : uint32_t {
    /// Cached copy of Py_TPFLAGS_HAVE_GC
    has_gc                   = (1 << 24),

    /// Does the type maintain a list of implicit conversions?
    has_implicit_conversions = (1 << 25),

    /// Is this a python type that extends a bound C++ type?
    is_python_type           = (1 << 26),

    /// Does the type implement a custom __new__ operator?
    has_new                  = (1 << 27),

    /// Does the type implement a custom __new__ operator that can take no
    /// args (except the type object)?
    has_nullary_new          = (1 << 28)
};

struct nb_alias_chain;
struct nb_inst;

/// LIFO Instance pool
struct nb_inst_pool {
    nb_inst **slots;
    uint32_t count;
    uint32_t capacity;
};

// Implicit conversions for C++ type bindings, used in type_data below
struct implicit_t {
    const std::type_info **cpp;
    bool (**py)(PyTypeObject *, PyObject *, cleanup_list *) noexcept;
};

// Forward and reverse mappings for enumerations, used in type_data below
struct enum_tbl_t {
    void *fwd;
    void *rev;
};

/// Internal version of a type binding record (see type_data_init in
/// nb_backend.h)
struct type_data {
    /// Size of a C++ instance in bytes
    uint32_t size;

    /// Full 32-bit flags word: public type_flags in the low bits (init-only
    /// flags stripped during translation) plus type_flags_internal (24+)
    uint32_t flags;

    /// Alignment of a C++ instance in bytes
    uint32_t align;

    /// Instance pool capacity
    uint32_t pool_capacity;

    /// Backend state of the domain that owns this type. A null pointer
    /// identifies the record of a Python subclass that is not yet initialized.
    nb_internals *internals;

    /// Type name
    const char *name;

    /// C++ RTTI record of the bound type
    const std::type_info *type;

    /// Associated Python type object
    PyTypeObject *type_py;

    /// Alternative C++ RTTI records that also map to this type
    nb_alias_chain *alias_chain;
#if defined(Py_LIMITED_API)
    /// Cached tp_vectorcall_offset target; the limited API provides no way
    /// to read it from PyTypeObject
    PyObject* (*vectorcall)(PyObject *, PyObject * const*, size_t, PyObject *);
#endif

    /// Constructor nb_func object
    void *init;

    /// Destruct an instance
    void (*destruct)(void *);

    /// Copy-construct an instance from another one
    void (*copy)(void *, const void *);

    /// Move-construct an instance from another one
    void (*move)(void *, void *) noexcept;

    union {
        implicit_t implicit;  // for C++ type bindings
        enum_tbl_t enum_tbl;  // for enumerations
    };

    /// Inform an intrusively reference-counted instance about its Python side
    void (*set_self_py)(void *, PyObject *) noexcept;

    /// Keep-alive callback of types deriving from enable_shared_from_this
    bool (*keep_shared_from_this_alive)(PyObject *) noexcept;

    /// Offset of the instance dictionary (or 0)
    uint32_t dictoffset;

    /// Offset of the weak reference list (or 0)
    uint32_t weaklistoffset;

    /// Out-of-line heap storage for an optional nb::supplement<T>
    void *supplement;

    /// Currently published trampoline table (see trampoline.cpp)
    void *trampoline_table_pub;

    /// Linked list of trampoline table allocations for later cleanup
    void *trampoline_allocs;
#if defined(NB_FREE_THREADED)
    /// Slot of this type's pool in the packed per-thread pool array
    uint32_t pool_index;
#else
    /// Per-type instance pool for non-FT builds
    nb_inst_pool pool;
#endif
};

/// Runtime record for enumeration bindings; extends the private type record
/// with the scope that enum_export() consults
struct enum_type_data : type_data {
    PyObject *scope;
};

/// Packed status of a nanobind type instance.
struct nb_inst_state {
    // Values for the 'state' field. Note that the numeric values of these are
    // relied upon for an optimization in `nb_type_get()`.
    static constexpr uint32_t state_uninitialized = 0; // not constructed
    static constexpr uint32_t state_relinquished = 1; // owned by C++, don't touch
    static constexpr uint32_t state_ready = 2; // constructed and usable

    /// State of the C++ object this instance points to: is it constructed?
    /// can we use it? (see the 'state_*' values below)
    uint8_t state : 2;

    /**
     * The variable 'offset' can either encode an offset relative to the
     * nb_inst address that leads to the instance data, or it can encode a
     * relative offset to a pointer that must be dereferenced to get to the
     * instance data. 'direct' is 'true' in the former case.
     */
    uint8_t direct : 1;

    /// Is the instance data co-located with the Python object?
    uint8_t internal : 1;

    /// Should the destructor be called when this instance is GCed?
    uint8_t destruct : 1;

    /// Should nanobind call 'operator delete' when this instance is GCed?
    uint8_t cpp_delete : 1;

    /// Does this instance use intrusive reference counting?
    uint8_t intrusive : 1;

    /// Currently not used (but needed to pad to 8 bit)
    uint8_t pad : 1;

    /// Does this instance hold references to others? (via internals.keep_alive)
    /// This may be accessed concurrently to the flag byte above, so it is kept
    /// in its own byte (never read-modify-written together with the flags).
    uint8_t clear_keep_alive;

    // That's a lot of unused space. I wonder if there is a good use for it..
    uint16_t unused;
};

static_assert(sizeof(nb_inst_state) == sizeof(uint32_t));

/// Python object representing an instance of a bound C++ type
struct nb_inst { // usually: 24 bytes
    PyObject_HEAD

    /// Offset to the actual instance data
    int32_t offset;

    /// Packed status flags (see nb_inst_state)
    nb_inst_state state;
};

static_assert(sizeof(nb_inst) == sizeof(PyObject) + sizeof(uint32_t) * 2);

/// Helper to ensure that nb_inst instance state updates produce one 4-byte store
inline void nb_inst_state_write(nb_inst *self, nb_inst_state state) noexcept {
    uint32_t w;
    std::memcpy(&w, &state, sizeof(w));
    std::memcpy(&self->state, &w, sizeof(w));
}

/// Dispatcher needed by an overload chain; chain merging takes the maximum
enum class call_complexity : uint8_t {
    /// No named/default/flagged arguments: nb_func_vectorcall_simple*
    simple = 0,

    /// Named/default/'none'-accepting args or arg-mutating annotations;
    /// keyword calls are forwarded to the complex dispatcher
    medium = 1,

    /// nb::args/nb::kwargs or more than NB_MAXARGS_SIMPLE arguments
    complex = 2
};

/// Python object representing a bound C++ function
struct nb_func {
    PyObject_VAR_HEAD
    PyObject* (*vectorcall)(PyObject *, PyObject * const*, size_t, PyObject *);
    uint32_t max_nargs; // maximum value of func_data::nargs for any overload
    call_complexity complexity;
    bool doc_uniform;
    nb_internals *internals; // backend state of the domain that owns this function
    PyObject *scope; // borrowed; the scope owns this function
    PyObject *module_name; // '__module__' captured at definition time
};

/// Python object representing a `nb_ndarray` (which wraps a DLPack ndarray)
struct nb_ndarray {
    PyObject_HEAD
    ndarray_handle *th;
    nb_internals *internals; // backend state that owns the nb_ndarray type
};

/// Python object representing an `nb_method` bound to an instance (analogous to non-public PyMethod_Type)
struct nb_bound_method {
    PyObject_HEAD
    PyObject* (*vectorcall)(PyObject *, PyObject * const*, size_t, PyObject *);
    nb_func *func;
    PyObject *self;
};

/// Pointers require a good hash function to randomize the mapping to buckets
struct ptr_hash {
    size_t operator()(const void *p) const {
        // fmix32/64 from MurmurHash by Austin Appleby (public domain)
        if constexpr (sizeof(void *) == 4)
            return (size_t) fmix32((uint32_t) (uintptr_t) p);
        else
            return (size_t) fmix64((uint64_t) (uintptr_t) p);
    }
};

// Minimal allocator definition, contains only the parts needed by tsl::*
template <typename T> class py_allocator {
public:
    using value_type = T;
    using pointer = T *;
    using size_type = std::size_t;

    py_allocator() = default;
    py_allocator(const py_allocator &) = default;

    template <typename U> py_allocator(const py_allocator<U> &) { }

    pointer allocate(size_type n, const void * /*hint*/ = nullptr) noexcept {
        if (NB_UNLIKELY(n > SIZE_MAX / sizeof(T)))
            fail("py_allocator::allocate(): integer overflow!");
        void *p = PyMem_Malloc(n * sizeof(T));
        if (NB_UNLIKELY(!p))
            fail("PyMem_Malloc(): out of memory!");
        return static_cast<pointer>(p);
    }

    void deallocate(T *p, size_type /*n*/) noexcept { PyMem_Free(p); }
};

// Linked list of instances with the same pointer address. Usually just 1.
struct nb_inst_seq {
    PyObject *inst;
    nb_inst_seq *next;
};

// Linked list of type aliases when there are multiple shared libraries with duplicate RTTI data
struct nb_alias_chain {
    const std::type_info *value;
    nb_alias_chain *next;
};

// Weak reference list. Usually, there is just one entry
struct nb_weakref_seq {
    void (*callback)(void *) noexcept;
    void *payload;
    nb_weakref_seq *next;
};

struct std_typeinfo_hash {
    size_t operator()(const std::type_info *a) const {
        const char *name = a->name();
        return std::hash<std::string_view>()({name, strlen(name)});
    }
};

struct std_typeinfo_eq {
    bool operator()(const std::type_info *a, const std::type_info *b) const {
        return a->name() == b->name() || strcmp(a->name(), b->name()) == 0;
    }
};

/// A simple pointer-to-pointer map that is reused a few times below (even if
/// not 100% ideal) to avoid template code generation bloat.
using nb_ptr_map  = tsl::robin_map<void *, void*, ptr_hash>;

using nb_type_map_fast = nb_ptr_map;
using nb_type_map_slow = tsl::robin_map<const std::type_info *, type_data *,
                                        std_typeinfo_hash, std_typeinfo_eq>;

#if defined(NB_FREE_THREADED)
// Per-thread state of one domain
struct nb_thread_state {
    // Backend state that this record belongs to
    nb_internals *internals;

    // C++ -> Python type cache
    nb_type_map_fast type_c2p_fast;

    /// Per-thread instance pools indexed by ``type_data::pool_index``
    /// Grown lazily by nb_pool_ensure() and freed when the thread exists
    nb_inst_pool *pools = nullptr;

    /// Number of entries currently allocated in ``pools``
    uint32_t pools_size = 0;
};

/// One-entry cache holding the most recently used domain's thread state
extern NB_THREAD_LOCAL nb_thread_state *nb_thread_state_tls;

/// Slow path: fetch or allocate this thread's state for the given domain
extern nb_thread_state *nb_thread_state_alloc(nb_internals *p) noexcept;

NB_INLINE nb_thread_state *nb_thread_state_get(nb_internals *p) noexcept {
    nb_thread_state *ts = nb_thread_state_tls;
    if (NB_UNLIKELY(!ts || ts->internals != p))
        ts = nb_thread_state_alloc(p);
    return ts;
}
#endif

/// Convenience functions to deal with the pointer encoding in 'internals.inst_c2p'

/// Does this entry store a linked list of instances?
NB_INLINE bool         nb_is_seq(void *p)   { return ((uintptr_t) p) & 1; }

/// Tag a nb_inst_seq* pointer as such
NB_INLINE void*        nb_mark_seq(void *p) { return (void *) (((uintptr_t) p) | 1); }

/// Retrieve the nb_inst_seq* pointer from an 'inst_c2p' value
NB_INLINE nb_inst_seq* nb_get_seq(void *p)  { return (nb_inst_seq *) (((uintptr_t) p) ^ 1); }

struct nb_translator_seq {
    exception_translator translator;
    void *payload;
    nb_translator_seq *next = nullptr;
};

#if defined(NB_FREE_THREADED)
#  define NB_SHARD_ALIGNMENT alignas(64)
#else
#  define NB_SHARD_ALIGNMENT
#endif

/**
 * The following data structure stores information associated with individual
 * instances. In free-threaded builds, it is split into multiple shards to avoid
 * lock contention.
 */
struct NB_SHARD_ALIGNMENT nb_shard {
    /**
     * C++ -> Python instance map
     *
     * This associative data structure maps a C++ instance pointer onto its
     * associated PyObject* (if bit 0 of the map value is zero) or a linked
     * list of type `nb_inst_seq*` (if bit 0 is set---it must be cleared before
     * interpreting the pointer in this case).
     *
     * The latter case occurs when several distinct Python objects reference
     * the same memory address (e.g. a struct and its first member).
     */
    nb_ptr_map inst_c2p;

    /// Dictionary storing keep_alive references
    nb_ptr_map keep_alive;

#if defined(NB_FREE_THREADED)
    PyMutex mutex { };
#endif
};


/**
 * Wraps a std::atomic if free-threading is enabled, otherwise a raw value.
 */
#if defined(NB_FREE_THREADED)
template<typename T>
struct nb_maybe_atomic {
  nb_maybe_atomic(T v = T()) : value(v) {}

  std::atomic<T> value;
  T load_acquire() { return value.load(std::memory_order_acquire); }
  T load_relaxed() { return value.load(std::memory_order_relaxed); }
  void store_release(T w) { value.store(w, std::memory_order_release); }
};
#else
template<typename T>
struct nb_maybe_atomic {
  nb_maybe_atomic(T v = T()) : value(v) {}

  T value;
  T load_acquire() { return value; }
  T load_relaxed() { return value; }
  void store_release(T w) { value = w; }
};
#endif

/// Cache slots for `nb_internals::ndarray_export`: cached callables that build a
/// framework's array from nanobind's DLPack/buffer wrapper.
enum ndarray_export_slot {
    nd_export_numpy_view, // numpy.asarray
    nd_export_numpy_copy, // numpy.copy
    nd_export_pytorch,    // torch.utils.dlpack.from_dlpack
    nd_export_tensorflow, // tensorflow.experimental.dlpack.from_dlpack
    nd_export_jax,        // jax.dlpack.from_dlpack
    nd_export_cupy,       // cupy.from_dlpack
    nd_export_mlx,        // mlx.core.array (constructor, not from_dlpack)
    nd_export_count
};

// Pre-interned strings in the per-domain state, alphabetically sorted.
// Use NB_INTERNED(p, name) below to access an entry.
#define NB_INTERNED_STRINGS(X)                                                 \
    X(__complex__)                                                             \
    X(__dict__)                                                                \
    X(__dlpack__)                                                              \
    X(__init__)                                                                \
    X(__length_hint__)                                                         \
    X(__module__)                                                              \
    X(__mro__)                                                                 \
    X(__name__)                                                                \
    X(__new__)                                                                 \
    X(__qualname__)                                                            \
    X(astype)                                                                  \
    X(cast)                                                                    \
    X(clone)                                                                   \
    X(contiguous)                                                              \
    X(copy)                                                                    \
    X(dl_device)                                                               \
    X(max_version)                                                             \
    X(stream)                                                                  \
    X(to)                                                                      \
    X(value)

// Names for the PyObject* entries in the per-domain state array.
// These names are scoped, but will implicitly convert to int.
struct pyobj_name {
    enum : int {
        #define NB_INTERNED_ENTRY(name) interned_##name,
        NB_INTERNED_STRINGS(NB_INTERNED_ENTRY)
        #undef NB_INTERNED_ENTRY
        string_count,

        // Cached constants using the same interning machinery
        interned_max_version_tpl = string_count, // tuple ("max_version")
        interned_dl_cpu_tpl,              // tuple (1, 0) == nb::device::cpu
        interned_dl_version_tpl,          // tuple (dlpack major, minor)
#if defined(Py_LIMITED_API) || defined(PYPY_VERSION)
        interned_u64_limit,               // 2**64, used by the integer casters
#endif
        total_count
    };
};

/// Access a cached PyObject (interned string or constant tuple) of the given
/// nb_internals by name, e.g. NB_INTERNED(p, __name__)
#define NB_INTERNED(p, name) ((p)->pyobjects[pyobj_name::interned_##name])

/**
 * `nb_internals` is the central data structure storing information related to
 * function/type bindings and instances. One instance exists per NB_DOMAIN;
 * separate nanobind extensions within the same NB_DOMAIN share `nb_internals`
 * to communicate with each other, hence any changes here generally require an
 * ABI version bump.
 *
 * The GIL protects the elements of this data structure from concurrent
 * modification. In free-threaded builds, a combination of locking schemes is
 * needed to achieve good performance.
 *
 * In particular, `inst_c2p` and `type_c2p_fast` are very hot and potentially
 * accessed several times while dispatching a single function call. The other
 * elements are accessed much less frequently and easier to deal with.
 *
 * The following list clarifies locking semantics for each member.
 *
 * - `nb_module`, `nb_type`, `nb_func`, `nb_method`, `nb_bound_method`,
 *   `*_Type_tp_*`, `shard_count`, `is_alive_ptr`: these are initialized when
 *   loading the first nanobind extension within a domain, which happens within
 *   a critical section. They do not require locking.
 *
 * - `nb_static_property` and `nb_static_propert_descr_set`: created only once
 *   on demand, protected by `mutex`.
 *
 * - `nb_ndarray`: created only once on demand, protected by `mutex`.
 *
 * - `inst_c2p`: stores the C++ instance to Python object mapping. This
 *   data struture is *hot* and uses a sharded locking scheme to reduce
 *   lock contention.
 *
 * - `keep_alive`: stores lifetime dependencies (e.g., from the
 *   reference_internal return value policy). This data structure is
 *   potentially hot and shares the sharding scheme of `inst_c2p`.
 *
 * - `type_c2p_slow`: This is the ground-truth source of the `std::type_info`
 *   to `type_info *` mapping. Unrelated to free-threading, lookups into this
 *   data struture are generally costly because they use a string comparison on
 *   some platforms. Because it is only used as a fallback for 'type_c2p_fast',
 *   protecting this member via the global `mutex` is sufficient.
 *
 * - `type_c2p_fast`: this data structure is *hot* and mostly read. It maps
 *   `std::type_info` to `type_info *` but uses pointer-based comparisons.
 *   The implementation depends on the Python build.
 *
 * - `translators`: This is an append-to-front-only singly linked list traversed
 *    while raising exceptions. The main concern is losing elements during
 *    concurrent append operations. We assume that this data structure is only
 *    written during module initialization and don't use locking.
 *
 * - `funcs`: data structure for function leak tracking. Not used in
 *   free-threaded mode .
 *
 * - `print_leak_warnings`, `print_implicit_cast_warnings`: simple boolean
 *   flags. No protection against concurrent conflicting updates.
 */
struct nb_internals {
    /// Internal nanobind module
    PyObject *nb_module;

    /// The metaclass shared by every bound type
    PyTypeObject *nb_type;

    /// Types of nanobind functions and methods
    PyTypeObject *nb_func, *nb_method, *nb_bound_method;

    /// Property variant for static attributes (created on demand)
    nb_maybe_atomic<PyTypeObject *> nb_static_property = nullptr;
    descrsetfunc nb_static_property_descr_set = nullptr;

    /// N-dimensional array wrapper (created on demand)
    nb_maybe_atomic<PyTypeObject *> nb_ndarray = nullptr;

    /// Cached callables used to export an ndarray to a framework, indexed by
    /// `ndarray_export_slot`.
    nb_maybe_atomic<PyObject *> ndarray_export[nd_export_count] {};

#if defined(NB_FREE_THREADED)
    nb_shard *shards = nullptr;
    size_t shard_mask = 0;

    // Heuristic shard selection (from pybind11 PR #5148 by @colesbury), uses
    // high pointer bits to group allocations by individual threads/cores.
    inline nb_shard &shard(void *p) {
        uintptr_t highbits = ((uintptr_t) p) >> 20;
        size_t index = ((size_t) fmix64((uint64_t) highbits)) & shard_mask;
        return shards[index];
    }
#else
    nb_shard shards[1];
    inline nb_shard &shard(void *) { return shards[0]; }
#endif

#if defined(NB_FREE_THREADED)
    // Per-domain key for reclaiming nb_thread_state at thread exit
#  if defined(_WIN32)
    unsigned long thread_state_key;
#  else
    pthread_key_t thread_state_key;
#  endif

    // Current index into the per-thread object pool. Grows proportional
    // to the number of pooled object types that are used across extensions
    std::atomic<uint32_t> pool_index_counter{0};
#endif

#if !defined(NB_FREE_THREADED)
    /// C++ -> Python type map -- fast version based on std::type_info pointer equality
    nb_type_map_fast type_c2p_fast;
#endif

    /// C++ -> Python type map -- slow fallback version based on hashed strings
    nb_type_map_slow type_c2p_slow;

#if !defined(NB_FREE_THREADED)
    /// nb_func/meth instance map for leak reporting (used as set, the value is unused)
    /// In free-threaded mode, functions are immortal and don't require this data structure.
    nb_ptr_map funcs;
#endif

    /// Counter bumped on every type modification (release increment).
    /// Trampoline override resolution reads it before resolving (acquire)
    /// and re-checks it before publishing an entry, discarding results
    /// that raced with a concurrent modification (see trampoline.cpp).
    nb_maybe_atomic<size_t> trampoline_epoch = 0;

    /// Registered C++ -> Python exception translators
    nb_maybe_atomic<nb_translator_seq *> translators = nullptr;

    /// Should nanobind print leak warnings on exit?
    bool print_leak_warnings = true;

    /// Should nanobind print warnings after implicit cast failures?
    bool print_implicit_cast_warnings = true;

    /// Pointer to a boolean that denotes if nanobind is fully initialized.
    bool *is_alive_ptr = nullptr;

#if defined(NB_FREE_THREADED)
    PyMutex mutex { };
#endif

    // Size of the 'shards' data structure. Only rarely accessed, hence at the end
    size_t shard_count = 1;

    /// Reference count tracking modules + types + functions using shared state
    nb_maybe_atomic<uint32_t> shared_ref_count = 0;

    /// PyList keeping managed PyObjects alive. Cleared when shared_ref_count
    /// reaches 0.
    PyObject *lifeline = nullptr;

    /// Caches filled by import_cached(); reset along with the lifeline
    std::vector<import_cache *> import_slots;

    /// Cached interned strings and constant tuples owned by the lifeline,
    /// indexed by 'pyobj_name' (see NB_INTERNED)
    PyObject *pyobjects[pyobj_name::total_count] = {};

    /// 2-way set-associative cache of interned strings exposed via
    /// see cached_string().
    struct name_cache_entry {
        uintptr_t key;
        const char *utf8;
        size_t len;
        nb_maybe_atomic<PyObject *> value { nullptr };
    };

    static constexpr uint32_t name_cache_bits = 12;
    alignas(64) name_cache_entry name_cache[(size_t) 1 << name_cache_bits];
};

extern void internals_inc_ref(nb_internals *p);
extern void internals_dec_ref(nb_internals *p);

/// Append 'o' to the lifeline and transfer ownership to it
inline void new_object(nb_internals *p, PyObject *o) {
    PyList_Append(p->lifeline, o);
    Py_DECREF(o);
}

/// Create a type via PyType_FromSpec and transfer ownership to the lifeline
inline PyTypeObject *new_type(nb_internals *p, PyType_Spec *spec) {
    PyTypeObject *tp = (PyTypeObject *) PyType_FromSpec(spec);
    if (tp)
        new_object(p, (PyObject *) tp);
    return tp;
}

#if defined(Py_LIMITED_API)
/* Cached functions from PyType_Type and PyProperty_Type along with the
   offset of the type_data record within a type object. These are constants
   of the running interpreter, filled in during initialization. */
extern freefunc PyType_Type_tp_free;
extern initproc PyType_Type_tp_init;
extern inquiry PyType_Type_tp_clear;
extern destructor PyType_Type_tp_dealloc;
extern setattrofunc PyType_Type_tp_setattro;
extern descrgetfunc PyProperty_Type_tp_descr_get;
extern descrsetfunc PyProperty_Type_tp_descr_set;
extern ptrdiff_t nb_type_data_offset;
#endif

/// Convenience macro to potentially access cached functions
#if defined(Py_LIMITED_API)
#  define NB_TYPE_SLOT(type, name) nanobind::detail::type##_##name
#else
#  define NB_TYPE_SLOT(type, name) type.name
#endif

#if defined(NB_HAVE_INTERP_VIEW)
// A view of the main interpreter, created during module initialization and
// deliberately never closed.
extern PyInterpreterView *nb_interp_view;

/// Cold path of attach_tstate() for callers that run before 'nb_interp_view'
extern void *attach_tstate_early() noexcept;
#endif

/// Token reported by attach_tstate() when there is nothing to undo
#define NB_TSTATE_ATTACHED ((void *) 1)

/* Implementation of the 'tstate_ensure' and 'tstate_release' backend slots.
   Hot paths like trampoline dispatch call these directly to skip the
   indirection through the exported functions. */

/// Does the calling thread have a Python thread state attached? Configurations
/// that cannot answer this report 'false' and take the slow path below. The
/// limited API lacks the query. PyPy and Python below 3.12 report a thread
/// state that the caller may not use (one released by this thread, or the one
/// of whichever thread holds the GIL), which would wrongly convince an
/// unattached thread that it can enter Python.
NB_INLINE bool tstate_attached() noexcept {
#if defined(Py_LIMITED_API) || defined(PYPY_VERSION) || PY_VERSION_HEX < 0x030C0000
    return false;
#elif PY_VERSION_HEX < 0x030D0000
    return _PyThreadState_UncheckedGet() != nullptr;
#else
    return PyThreadState_GetUnchecked() != nullptr;
#endif
}

NB_INLINE void *attach_tstate() noexcept {
    /* Threads that already have a thread state proceed with it and undo
       nothing later. Besides being much cheaper than the alternatives below,
       this skips an interpreter guard that would serve no purpose: such a
       thread cannot observe the interpreter disappearing underneath it. */
    if (tstate_attached())
        return NB_TSTATE_ATTACHED;

#if defined(NB_HAVE_INTERP_VIEW)
    if (NB_UNLIKELY(!nb_interp_view))
        return attach_tstate_early();

    return PyThreadState_EnsureFromView(nb_interp_view);
#else
    /* PyGILState_STATE is an enumeration starting at zero. The shift keeps a
       successful attachment distinguishable from both a null token and the
       sentinel above. */
    return (void *) (uintptr_t) ((int) PyGILState_Ensure() + 2);
#endif
}

NB_INLINE void detach_tstate(void *token) noexcept {
    if (token == NB_TSTATE_ATTACHED)
        return;
#if defined(NB_HAVE_INTERP_VIEW)
    PyThreadState_Release(token);
#else
    PyGILState_Release((PyGILState_STATE) ((uintptr_t) token - 2));
#endif
}

extern char *type_name(const std::type_info *t);

/// Construct 'nb_type' as an instance of the meta-metaclass 'nb_meta'
extern PyTypeObject *nb_type_create_metaclass(nb_internals *p,
                                              PyTypeObject *nb_meta) noexcept;

// Forward declarations
extern PyObject *inst_new_ext(PyTypeObject *tp, void *value);
extern PyObject *inst_new_int(PyTypeObject *tp, PyObject *args, PyObject *kwds);
extern PyTypeObject *nb_static_property_tp(nb_internals *p) noexcept;

/// Fetch the raw MRO entry 'key' of 't'. Wraps or emulates _PyType_LookupRef()
extern PyObject *type_lookup(nb_internals *p, PyObject *t,
                             PyObject *key) noexcept;

extern type_data *nb_type_c2p(nb_internals *internals,
                              const std::type_info *type);
extern void nb_type_unregister(type_data *t) noexcept;

/// Drop the published trampoline tables of 'tp' and its subclasses after a
/// type modification so that overrides are re-resolved (GIL held)
extern void nb_trampoline_invalidate(PyObject *tp) noexcept;

/// Free the trampoline allocations owned by a type record (GIL held)
extern void nb_trampoline_free(type_data *t) noexcept;

extern PyObject *call_one_arg(PyObject *fn, PyObject *arg) noexcept;

// String-keyed attribute access helpers for backend code. The header-side
// operators inject the extension's state pointer, which backend code does not
// have. These variants take it explicitly.

inline bool str_hasattr(nb_internals *p, handle h, const char *key) noexcept {
    return hasattr_str(p, h.ptr(), key, strlen(key) + 1);
}

inline object str_getattr(nb_internals *p, handle h, const char *key) {
    return steal(getattr_str(p, h.ptr(), key, strlen(key) + 1));
}

inline object str_getattr_def(nb_internals *p, handle h, const char *key,
                              handle def = handle()) noexcept {
    return steal(
        getattr_str_def(p, h.ptr(), key, strlen(key) + 1, def.ptr()));
}

inline void str_setattr(nb_internals *p, handle h, const char *key, handle v) {
    setattr_str(p, h.ptr(), key, strlen(key) + 1, v.ptr());
}

/// Call 'base' with borrowed positional arguments, raising on failure
template <typename... Args>
object obj_call(nb_internals *p, handle base, const Args &...args) {
    PyObject *argv[sizeof...(Args) + 1] = { nullptr, args.ptr()... };
    return steal(obj_vectorcall(p, base.ptr(), argv + 1,
                                sizeof...(Args) |
                                    PY_VECTORCALL_ARGUMENTS_OFFSET,
                                0, 0));
}

/// Fetch the nanobind function record from a 'nb_func' instance
NB_INLINE func_data *nb_func_data(void *o) {
    return (func_data *) (((char *) o) + sizeof(nb_func));
}

/// Fetch the backend state of the domain owning a 'nb_func' instance
NB_INLINE nb_internals *nb_func_internals(void *o) {
    return ((nb_func *) o)->internals;
}

/// Fetch the nanobind type record from a 'nb_type' instance
NB_INLINE type_data *nb_type_data(PyTypeObject *o) noexcept{
    #if !defined(Py_LIMITED_API)
        return (type_data *) (((char *) o) + sizeof(PyHeapTypeObject));
    #else
        #if 1
            // Fast path that can be inlines without spilling registers
            return (type_data *) ((char *) o + nb_type_data_offset);
        #else
            // Equivalent non-inlined reference version:
            return (type_data *) PyObject_GetTypeData((PyObject *) o, Py_TYPE((PyObject *) o));
        #endif
    #endif
}

inline void *inst_ptr(nb_inst *self) {
    void *ptr = (void *) ((intptr_t) self + self->offset);
    return self->state.direct ? ptr : *(void **) ptr;
}

// Return the instance pool associated with type `td`
NB_INLINE nb_inst_pool *nb_pool_lookup(type_data *td) noexcept {
#if !defined(NB_FREE_THREADED)
    // In GIL-protected Python, global pool data structure is reachable via `td`
    return &td->pool;
#else
    // In FT builds, the pool is per thread. Resolve the thread state through
    // the domain key so that a stale TLS cache does not bypass the pool.
    nb_thread_state *ts = nb_thread_state_get(td->internals);
    if (td->pool_index < ts->pools_size)
        return ts->pools + td->pool_index;
    return nullptr;
#endif
}

// Return the instance pool associated with type `td` or allocate it on demand
extern nb_inst_pool *nb_pool_ensure(type_data *td) noexcept;

/// Release all objects kept in the given instance pool
extern void nb_pool_drain(nb_inst_pool *pool, bool can_free) noexcept;

template <typename T> struct scoped_pymalloc {
    scoped_pymalloc(size_t size = 1, size_t extra_bytes = 0) {
        size_t total = size * sizeof(T);
        if (NB_UNLIKELY(size > SIZE_MAX / sizeof(T) ||
                        total > SIZE_MAX - extra_bytes))
            fail("scoped_pymalloc(): integer overflow!");
        total += extra_bytes;
        ptr = (T *) PyMem_Malloc(total);
        if (NB_UNLIKELY(!ptr))
            fail("scoped_pymalloc(): could not allocate %llu bytes of memory!",
                 (unsigned long long) total);
    }
    ~scoped_pymalloc() { PyMem_Free(ptr); }
    T *release() {
        T *temp = ptr;
        ptr = nullptr;
        return temp;
    }
    T *get() const { return ptr; }
    T &operator[](size_t i) { return ptr[i]; }
    T *operator->() { return ptr; }
private:
    T *ptr{ nullptr };
};


/// RAII lock/unlock guards for free-threaded builds
#if defined(NB_FREE_THREADED)
struct lock_shard {
    nb_shard &s;
    lock_shard(nb_shard &s) : s(s) { PyMutex_Lock(&s.mutex); }
    ~lock_shard() { PyMutex_Unlock(&s.mutex); }
};
struct lock_internals {
    nb_internals *i;
    lock_internals(nb_internals *i) : i(i) { PyMutex_Lock(&i->mutex); }
    ~lock_internals() { PyMutex_Unlock(&i->mutex); }
};
struct unlock_internals {
    nb_internals *i;
    unlock_internals(nb_internals *i) : i(i) { PyMutex_Unlock(&i->mutex); }
    ~unlock_internals() { PyMutex_Lock(&i->mutex); }
};
#else
struct lock_shard { lock_shard(nb_shard &) { } };
struct lock_internals { lock_internals(nb_internals *) { } };
struct unlock_internals { unlock_internals(nb_internals *) { } };
struct lock_obj { lock_obj(PyObject *) { } };
#endif

/// Report a warning that a warnings-as-errors filter turned into an exception
inline void warning_failed() noexcept {
#if !defined(Py_LIMITED_API) && !defined(PYPY_VERSION) && \
    PY_VERSION_HEX >= 0x030D0000
    PyErr_FormatUnraisable("Exception ignored while issuing a nanobind warning");
#else
    PyErr_WriteUnraisable(nullptr);
#endif
}

extern char *strdup_check(const char *);
extern void *malloc_check(size_t size);

extern char *extract_name(const char *cmd, const char *prefix, const char *s);


NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
