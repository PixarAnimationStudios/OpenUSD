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
#ifndef TF_FLYWEIGHT_H
#define TF_FLYWEIGHT_H

/// \file tf/flyweight.h
/// An implementation of the "flyweight pattern".

#include "pxr/pxr.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/mallocTag.h"

#include <boost/functional/hash.hpp>
#include <tbb/concurrent_hash_map.h>

#include <atomic>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

// Set this to 1 to enable stats.
#define TF_FLYWEIGHT_STATS 0

#if TF_FLYWEIGHT_STATS
#include <cstdio>
#define TF_FLYWEIGHT_INC_STAT(name) ++_GetData()-> name
#else
#define TF_FLYWEIGHT_INC_STAT(name)
#endif

// Non-template base class.  Note that this class does not have a virtual
// destructor.  It needs one if these objects are ever expected to be destroyed
// through a base class pointer.  Implement a protected destructor to help guard
// against this.
struct Tf_FlyweightDataBase
{
protected:
    ~Tf_FlyweightDataBase() {}
};

// Flyweight data.
template <class HashMap>
struct Tf_FlyweightData : public Tf_FlyweightDataBase {
    Tf_FlyweightData()
        : defaultPtr(nullptr)
#if TF_FLYWEIGHT_STATS
        , findOrCreateCalls(0)
        , numFound(0)
        , numCreated(0)
        , numCulled(0)
        , numGetDefault(0)
        , numDefaultCtors(0)
        , numValueCtors(0)
        , numAssignValues(0)
        , numCopyCtors(0)
        , numAssignFlyweights(0)
        , numEqualityChecks(0)
        , numDtors(0)
#endif
    {}

    // Noncopyable (because of atomic defaultPtr)
    Tf_FlyweightData(const Tf_FlyweightData&) = delete;
    Tf_FlyweightData& operator=(const Tf_FlyweightData&) = delete;

    // The object pool itself.  Note that currently, TfFlyweight requires that
    // the objects in \a pool be stable in memory since they are referred to by
    // pointer.  This means that objects should not be moved in memory once
    // they're inserted into the pool.
    HashMap pool;

    // A pointer to the default object.
    std::atomic<typename HashMap::value_type const *> defaultPtr;

#if TF_FLYWEIGHT_STATS
    size_t findOrCreateCalls;
    size_t numFound;
    size_t numCreated;
    size_t numCulled;
    size_t numGetDefault;
    size_t numDefaultCtors;
    size_t numValueCtors;
    size_t numAssignValues;
    size_t numCopyCtors;
    size_t numAssignFlyweights;
    size_t numEqualityChecks;
    size_t numDtors;
#endif

};

// Pools must be globally unique.  This sets the data associated with \a
// poolName.  If successful, \a data is installed and the returned value is the
// same as \a data.  If unsuccessful, the returned value is a pointer to the
// existing data.
TF_API
Tf_FlyweightDataBase *
Tf_TrySetFlyweightData(std::string const &poolName, Tf_FlyweightDataBase *data);

/// \class TfFlyweight
///
/// An implementation of the "flyweight pattern":
/// http://en.wikipedia.org/wiki/Flyweight_pattern
///
/// This class maintains object instances in a shared pool so that two or more
/// objects that compare equal share the same instance in the pool.  This can
/// help reduce memory usage if there are many equivalent object instances in
/// a program.  There is overhead associated with these savings.  The primary
/// overhead is in constructing a flyweight object.  This requires searching
/// the pool to determine if there exists an equivalent object, and possibly
/// inserting one if there is not.  Minor overhead exists in accessing a
/// flyweight object.  This incurs an extra indirection.
///
/// TfFlyweight objects are thread-safe assuming the held value type provides
/// the basic thread safety guarantee (see below).  Thread safety at the
/// TfFlyweight level is accomplished by guarding the global object pool with
/// a mutex lock.  The lock is only taken when constructing and assigning
/// TfFlyweights from value types.  The lock is not taken for constructing and
/// assigning TfFlyweights with other TfFlyweights.  Note that the common case
/// of default construction is special-cased not to require locking (except on
/// first construction).
///
/// To use TfFlyweight with some value type concurrently in different threads,
/// that value type must support basic thread safety.  Specifically, it must
/// safely allow multiple concurrent const accesses, but can assume that
/// non-const accesses are serialized by another party.  The basic rule to
/// keep in mind is that if there are any 'mutable' member variables in \e
/// type, then those members are potentially mutable shared state in const
/// contexts. Modifications of those member variables inside const methods
/// must be made thread-safe by mutual exclusion or other means.  Modifying
/// those variables in non-const methods need not be guarded.
template <class T, class HashFn>
class TfFlyweight
{
private:

    // A rep just stores an object's reference count.
    struct _Rep {
        _Rep() : refCount(0) {}
        _Rep(_Rep const &other) : refCount(other.refCount.load()) {}
        mutable std::atomic_size_t refCount;
    };

    struct _TbbHashEq {
        inline bool equal(T const &l, T const &r) const {
            return l == r;
        }
        inline size_t hash(T const &val) const {
            return HashFn()(val);
        }
    };

    typedef tbb::concurrent_hash_map<
        T, _Rep, _TbbHashEq, std::allocator<std::pair<T, _Rep> > > _PoolHash;
    typedef typename _PoolHash::value_type const *_ElementPairPtr;

    typedef Tf_FlyweightData<_PoolHash> _Data;

    // Return a _Rep with a pre-incremented reference count for \a value.
    // Adds \a value to the pool if it isn't already there and never returns
    // NULL.
    static _ElementPairPtr
    _FindOrCreate(T const &value) {
        TfAutoMallocTag2 tag2("Tf", "TfFlyweight::_FindOrCreate");
        TfAutoMallocTag tag(__ARCH_PRETTY_FUNCTION__);

        _Data *data = _GetData();

        TF_FLYWEIGHT_INC_STAT(findOrCreateCalls);

        _PoolHash &pool = data->pool;
        
        typename _PoolHash::const_accessor acc;
        if (pool.insert(acc, std::make_pair(value, _Rep()))) {
            TF_FLYWEIGHT_INC_STAT(numCreated);
        }
        ++acc->second.refCount;
        return &(*acc);
    }

    // Return an iterator with a pre-incremented reference count for the
    // default-constructed value.
    static inline _ElementPairPtr _GetDefault() {
        _Data *data = _GetData();
        // XXX this is (technically speaking) broken double-checked locking.
        _ElementPairPtr defaultPtr = data->defaultPtr;
        if (ARCH_UNLIKELY(!defaultPtr)) {
            data->defaultPtr = defaultPtr = _FindOrCreate(T());
        }
        TF_FLYWEIGHT_INC_STAT(numGetDefault);
        ++defaultPtr->second.refCount;
        return defaultPtr;
    }

    static inline void _TryToErase(_ElementPairPtr ptr) {
        _Data *data = _GetData();
        // Try to remove \p elem from the data table.  It's possible that
        // its refcount is greater than one, in which case we do nothing.

        typename _PoolHash::accessor acc;
        if (TF_VERIFY(data->pool.find(acc, ptr->first))) {
            // We hold a write lock here so we can't be racing a _FindOrCreate
            // caller that's not yet incremented its refcount.
            if (ARCH_LIKELY(--ptr->second.refCount == 0))
                data->pool.erase(acc);
        }
    }

    static inline void _DumpStats() {
#if TF_FLYWEIGHT_STATS
        _Data *data = _GetData();
        printf("================================\n");
        printf("== Stats for %s\n",
               ArchGetDemangled(typeid(TfFlyweight)).c_str());
        printf("   %8zu FindOrCreate calls\n"
               "   %8zu Found\n"
               "   %8zu Created\n"
               "   %8zu Culled\n"
               "   %8zu GetDefault calls\n"
               "   %8zu Default Ctor calls\n"
               "   %8zu Value Ctor calls\n"
               "   %8zu Assign Value calls\n"
               "   %8zu Copy Ctor calls\n"
               "   %8zu Assign Flyweight calls\n"
               "   %8zu Equality checks\n"
               "   %8zu Dtor calls\n"
               "   %8d  Expired values (approximate)\n"
               , data->findOrCreateCalls
               , data->numFound
               , data->numCreated
               , data->numCulled
               , data->numGetDefault
               , data->numDefaultCtors
               , data->numValueCtors
               , data->numAssignValues
               , data->numCopyCtors
               , data->numAssignFlyweights
               , data->numEqualityChecks
               , data->numDtors
               , data->numExpired.Get()
               );
#endif
        }

    static inline void _ClearStats() {
#if TF_FLYWEIGHT_STATS
        _Data *data = _GetData();
        data->findOrCreateCalls
            = data->numFound
            = data->numCreated
            = data->numCulled
            = data->numGetDefault
            = data->numDefaultCtors
            = data->numValueCtors
            = data->numAssignValues
            = data->numCopyCtors
            = data->numAssignFlyweights
            = data->numEqualityChecks
            = data->numDtors
            = data->numExpired
            = 0;
#endif
    }


public:

    /// Default constructed flyweight references default constructed value.
    TfFlyweight() : _ptr(_GetDefault()) {
        TF_FLYWEIGHT_INC_STAT(numDefaultCtors);
    }

    /// Construct a flyweight with \a val.
    explicit TfFlyweight(T const &val) : _ptr(_FindOrCreate(val)) {
        TF_FLYWEIGHT_INC_STAT(numValueCtors);
    }

    TfFlyweight(TfFlyweight const &other) : _ptr(other._ptr) {
        _AddRef();
        TF_FLYWEIGHT_INC_STAT(numCopyCtors);
    }

    /// Destructor
    ~TfFlyweight() {
        _RemoveRef();
        TF_FLYWEIGHT_INC_STAT(numDtors);
    }

    /// Assign this flyweight to refer to \a val.
    TfFlyweight &operator=(T const &val) {
        // Saving the old rep and calling _RemoveRef after obtaining the new one
        // will avoid destruction in the case that the new and old reps are the
        // same.
        _ElementPairPtr oldPtr = _ptr;
        _ptr = _FindOrCreate(val);
        _RemoveRef(oldPtr);
        TF_FLYWEIGHT_INC_STAT(numAssignValues);
        return *this;
    }

    /// Assign this flyweight to refer to what \a other refers to.
    TfFlyweight &operator=(TfFlyweight const &other) {
        if (*this == other)
            return *this;
        // Bump other ref count, decrement ours, then reassign.
        _AddRef(other._ptr);
        _RemoveRef();
        _ptr = other._ptr;
        TF_FLYWEIGHT_INC_STAT(numAssignFlyweights);
        return *this;
    }

    /// Return true if this flyweight refers to the exact same object as \a
    /// other.  Note that this does not invoke the equality operator on the
    /// underlying objects.  It returns true if the referred-to objects are
    /// identical.
    bool operator==(TfFlyweight const &other) const {
        TF_FLYWEIGHT_INC_STAT(numEqualityChecks);
        return _ptr == other._ptr;
    }

    bool operator!=(TfFlyweight const &other) const {
        return !(*this == other);
    }

    /// Return a const reference to the object this flyweight refers to.
    T const &Get() const {
        return _ptr->first;
    }

    /// Implicitly convert to a const reference of the underlying value.
    operator T const &() const {
        return Get();
    }

    /// Return a hash value for this flyweight object.
    size_t Hash() const {
        return boost::hash<_ElementPairPtr>()(_ptr);
    }

    /// Hash functor.
    struct HashFunctor {
        size_t operator()(const TfFlyweight &flyweight) const {
            return flyweight.Hash();
        }
    };

    /// Swap this flyweight with another.
    void Swap(TfFlyweight &other) {
        std::swap(_ptr, other._ptr);
    }

    /// If stats are enabled, dump their current values to stdout.
    static void DumpStats() {
        _DumpStats();
    }

    /// If stats are enabled, clear out the current stat values.
    static void ClearStats() {
        _ClearStats();
    }

private:
    friend inline void swap(TfFlyweight &a, TfFlyweight &b) {
        a.Swap(b);
    }

    friend struct TfFlyweightTotalOrderLessThan;

    static _Data *_GetData() {
        static _Data* _data = 
            static_cast<_Data *>(Tf_TrySetFlyweightData(
                typeid(TfFlyweight).name(), new _Data));
        return _data;
    }

    inline void _AddRef() { _AddRef(_ptr); }
    inline void _RemoveRef() { _RemoveRef(_ptr); }

    static inline void _AddRef(_ElementPairPtr const &ptr) {
        ++ptr->second.refCount;
    }

    static inline void _RemoveRef(_ElementPairPtr const &ptr) {
        // This refcount check is fleeting and unreliable -- see comment in the
        // else clause below.
        if (ARCH_UNLIKELY(ptr->second.refCount == 1)) {
            _TryToErase(ptr);
        } else {
            // This could potentially take the refcount to zero, in which case
            // we have an unused element left in the table.  This is a tradeoff
            // for not taking the lock on every refcount decrement.  In this
            // case we (nonatomically) increment a counter that estimates the
            // number of such entries, so we can possibly take a culling pass if
            // they build up.
            --ptr->second.refCount;
        }
    }

    _ElementPairPtr _ptr;
};

/// A functor that gives a total order for flyweight objects.  Note that the
/// specific order that the functor produces is arbitrary, and may differ from
/// run to run of the program.  It does not depend on the underlying values at
/// all.  Further, the result of this functor applied to two given flyweights is
/// only guaranteed to be consistent while both of those flyweights are alive.
/// Thus, it is recommended that this functor be used for things like storing
/// flyweight objects in ordered associative containers, such as set<Flyweight>,
/// or map<Flyweight, T>, or for binary searching sorted containers of flyweight
/// objects.
struct TfFlyweightTotalOrderLessThan {
    template <class T, class H>
    bool operator()(TfFlyweight<T, H> const &lhs,
                    TfFlyweight<T, H> const &rhs) const {
        return lhs._ptr < rhs._ptr;
    }
};

// Overload hash_value for TfFlyweight.
template <class Type, class HashFn>
inline
size_t hash_value(const TfFlyweight<Type, HashFn>& x)
{
    return x.Hash();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_FLYWEIGHT_H
