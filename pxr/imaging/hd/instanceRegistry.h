//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_INSTANCE_REGISTRY_H
#define PXR_IMAGING_HD_INSTANCE_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hf/perfLog.h"

#include <tbb/concurrent_unordered_map.h>

#include <memory>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

// Forward declarations
template <typename VALUE>
class HdInstanceRegistry;

using HdInstanceKey = uint64_t;
using HdInstanceRegistryMutex = std::mutex;
using HdInstanceRegistryLock = std::unique_lock<HdInstanceRegistryMutex>;

/// \class HdInstance
///
/// This class is used as an interface to a shared instance stored in
/// a REGISTRY object.
///
/// HdInstanceKeyType is a hashable index type and VALUE is shared_ptr. In most
/// use cases, the client computes a hash key which represents large bulky data
/// (like topology, primvars) and registers it into REGISTRY. If the
/// key has already been registered, the registry returns an HdInstance and the
/// client can use GetValue() without setting/computing actual bulky data. If
/// it doesn't exist, IsFirstInstance() returns true for the first instance
/// and the client needs to populate an appropriate data VALUE into the
/// instance by SetValue().
///
/// In order to support concurrent access to REGISTRY, this
/// class holds a lock to a mutex in REGISTRY. This lock will
/// be held until the instance of this interface class is destroyed.
///
template <typename VALUE, class REGISTRY = HdInstanceRegistry<VALUE>>
class HdInstance {
public:
    typedef VALUE ValueType;
    typedef HdInstanceKey ID;

    struct ValueHolder {
        ValueHolder(ValueType const & value = ValueType())
            : value(value)
            , recycleCounter(0)
        { }
        void ResetRecycleCounter() {
            recycleCounter = 0;
        }

        ValueType value;
        int recycleCounter;
    };

    HdInstance() = delete;

    /// Construct an instance holding a registry lock, representing a value
    /// held in a registry container.
    explicit HdInstance(HdInstanceKey           key,
                        ValueType const         &value,
                        HdInstanceRegistryLock  &&registryLock,
                        REGISTRY                *registry)
        : _key(key)
        , _value(value)
        , _registryLock(std::move(registryLock))
        , _registry(registry)
        , _isFirstInstance(!bool(_value))
    { }

    /// Construct an instance with no lock or registry container. This
    /// is used to present a consistent interface to clients in cases
    /// where shared resource registration is disabled.
    explicit HdInstance(HdInstanceKey key)
        : _key(key)
        , _value(ValueType())
        , _registryLock()
        , _registry(nullptr)
        , _isFirstInstance(!bool(_value))
    { }

    /// Returns the key
    HdInstanceKey GetKey() const { return _key; }

    /// Returns the value
    ValueType const &GetValue() const { return _value; }

    /// Update the value in the registry
    void SetValue(ValueType const &value) {
        if (_registry) {
            _registry->_SetValue(_key, value);
        }
        _value = value;
    }

    /// Returns true if the value has not been initialized.
    bool IsFirstInstance() const {
        return _isFirstInstance;
    }

private:
    HdInstanceKey           _key;
    ValueType               _value;
    HdInstanceRegistryLock  _registryLock;
    REGISTRY                *_registry = nullptr;
    bool                    _isFirstInstance;
};

/// \class HdInstanceRegistryBase
///
/// HdInstanceRegistryBase is a dictionary container of HdInstance.
/// This class is mostly just a dictionary from key to value, ensuring
/// thread-safe access to the values.
/// The DERIVED template parameter can be used in a Curiously Recurring
/// Template Pattern to extend this class with a persistent cache backing the
/// in-memory dictionary.
/// For cleaning up unused entries, the GarbageCollect() API is provided.
/// It sweeps all entries in the dictionary and erases unreferenced entries.
/// When HdInstance::ValueType is shared_ptr, it is regarded as unreferenced
/// if the shared_ptr is unique (use_count==1). Note that Key is not
/// involved to determine the lifetime of entries.
///
template <typename VALUE, class DERIVED>
class HdInstanceRegistryBase {
public:
    friend class HdInstance<VALUE, DERIVED>;
    using InstanceType = HdInstance<VALUE, DERIVED>;
    
    using Dictionary = tbb::concurrent_unordered_map<
        HdInstanceKey,
        typename InstanceType::ValueHolder
    >;

    HdInstanceRegistryBase() = default;

    /// Copy constructor.  Need as HdInstanceRegistryBase is placed in a map
    /// and mutex is not copy-constructible, so can't use default
    HdInstanceRegistryBase(const HdInstanceRegistryBase &other)
        : _dictionary(other._dictionary)
        , _mutex()  // mutex is not copied
    { }

    /// Returns a shared instance for a given key.
    InstanceType GetInstance(
        HdInstanceKey key);

    /// Returns a shared instance for a given key
    /// only if the key exists in the dictionary.
    InstanceType FindInstance(
        HdInstanceKey key, bool *found);

    /// Removes unreferenced entries and returns the count
    /// of remaining entries. When recycleCount is greater than zero,
    /// unreferenced entries will not be removed until GarbageCollect() is
    /// called that many more times, i.e. allowing unreferenced entries to
    /// be recycled if they are needed again.
    size_t GarbageCollect(int recycleCount = 0);

    /// Removes unreferenced entries and returns the count
    /// of remaining entries. If an entry is to be removed, callback function
    /// "callback" will be called on the entry before removal. When 
    /// recycleCount is greater than zero, unreferenced entries will not be 
    /// removed until GarbageCollect() is called that many more times, i.e. 
    /// allowing unreferenced entries to be recycled if they are needed again.
    template <typename Callback>
    size_t GarbageCollect(Callback &&callback, int recycleCount = 0);

    /// Returns a const iterator being/end of dictionary. Mainly used for
    /// resource auditing.
    typedef typename Dictionary::const_iterator const_iterator;
    const_iterator begin() const { return _dictionary.begin(); }
    const_iterator end() const { return _dictionary.end(); }

    size_t size() const { return _dictionary.size(); }

    void Invalidate();

private:
    template <typename T>
    static bool _IsUnique(std::shared_ptr<T> const &value) {
        return value.use_count() == 1;
    }

    /// Store the value in the in-memory dictionary and write it to the 
    /// persistent cache if implemented by the derived class.
    void _SetValue(
        HdInstanceKey key,
        VALUE const& value);

    /// Look a value up by the key in the in-memory dictionary. If the key is
    /// not found there and the derived class implements a persistent cache,
    /// try to load the value from there.
    typename Dictionary::iterator _LookUp(
        HdInstanceKey key);

    Dictionary              _dictionary;
    HdInstanceRegistryMutex _mutex;

    HdInstanceRegistryBase &operator =(HdInstanceRegistryBase &) = delete;
};

/// \class HdInstanceRegistry
///
/// The default implementation of HdInstanceRegistryBase without persistent
/// cache support.
///
template <typename VALUE>
class HdInstanceRegistry
    : public HdInstanceRegistryBase<VALUE, HdInstanceRegistry<VALUE>>
{
public:
    friend class HdInstanceRegistryBase<VALUE, HdInstanceRegistry<VALUE>>;

    void SaveToDisk(
        HdInstanceKey key,
        VALUE const& value)
    {
    }

    VALUE LoadFromDisk(HdInstanceKey key)
    {
        return nullptr;
    }
};

// ---------------------------------------------------------------------------
// Implementations

template <typename VALUE, class DERIVED>
typename HdInstanceRegistryBase<VALUE, DERIVED>::Dictionary::iterator
HdInstanceRegistryBase<VALUE, DERIVED>::_LookUp(
    HdInstanceKey key)
{
    typename Dictionary::iterator it = _dictionary.find(key);
    if (it != _dictionary.end()) {
        return it;
    }

    VALUE value = static_cast<DERIVED*>(this)->LoadFromDisk(key);
    if (value) {
        it = _dictionary.insert(
            std::make_pair(key,
                typename InstanceType::ValueHolder(value))).first;
    }
    return it;
}

template <typename VALUE, class DERIVED>
void
HdInstanceRegistryBase<VALUE, DERIVED>::_SetValue(
    HdInstanceKey key,
    VALUE const& value)
{
    _dictionary[key] = typename InstanceType::ValueHolder(value);
    static_cast<DERIVED*>(this)->SaveToDisk(key, value);
}

template <typename VALUE, class DERIVED>
HdInstance<VALUE, DERIVED>
HdInstanceRegistryBase<VALUE, DERIVED>::GetInstance(
    HdInstanceKey key)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Grab Registry lock
    // (and don't release it in this function, return it instead)
    HdInstanceRegistryLock lock(_mutex);

    typename Dictionary::iterator it = _LookUp(key);
    if (it == _dictionary.end()) {
        // not found. create new one
        it = _dictionary.insert(
            std::make_pair(key, typename InstanceType::ValueHolder())).first;
    }

    it->second.ResetRecycleCounter();
    return InstanceType(
        key, it->second.value, std::move(lock), static_cast<DERIVED*>(this));
}

template <typename VALUE, class DERIVED>
HdInstance<VALUE, DERIVED>
HdInstanceRegistryBase<VALUE, DERIVED>::FindInstance(
    HdInstanceKey key, bool *found)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Grab Registry lock
    // (and don't release it in this function, return it instead)
    HdInstanceRegistryLock lock(_mutex);

    typename Dictionary::iterator it = _LookUp(key);
    if (it == _dictionary.end()) {
        *found = false;
        return InstanceType(key, VALUE(), std::move(lock), nullptr);
    } else {
        *found = true;
        it->second.ResetRecycleCounter();
        return InstanceType(
            key, it->second.value, std::move(lock),
            static_cast<DERIVED*>(this));
    }
}

template <typename VALUE, class DERIVED>
size_t
HdInstanceRegistryBase<VALUE, DERIVED>::GarbageCollect(int recycleCount)
{
    // Call GarbageCollect with empty callback function
    return GarbageCollect([](void*){}, recycleCount);
}

template <typename VALUE, class DERIVED>
template <typename Callback>
size_t
HdInstanceRegistryBase<VALUE, DERIVED>::GarbageCollect(
    Callback &&callback, int recycleCount)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Skip garbage collection entirely when then the recycleCount is < 0
    if (recycleCount < 0) {
        return _dictionary.size();
    }

    size_t inUseCount = 0;
    for (typename Dictionary::iterator it = _dictionary.begin();
         it != _dictionary.end();) {

        // erase instance which isn't referred from anyone
        bool isUnique = _IsUnique(it->second.value);
        if (isUnique && (++it->second.recycleCounter > recycleCount)) {
            std::forward<Callback>(callback)(it->second.value.get());
            it = _dictionary.unsafe_erase(it);
        } else {
            ++it;
            ++inUseCount;
        }
    }
    return inUseCount;
}

template <typename VALUE, class DERIVED>
void
HdInstanceRegistryBase<VALUE, DERIVED>::Invalidate()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _dictionary.clear();
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_INSTANCE_REGISTRY_H
