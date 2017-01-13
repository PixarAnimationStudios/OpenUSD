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
#ifndef HD_INSTANCE_REGISTRY_H
#define HD_INSTANCE_REGISTRY_H

#include <mutex>
#include <boost/shared_ptr.hpp>
#include <tbb/concurrent_unordered_map.h>

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/imaging/hf/perfLog.h"

/// \class HdInstance
///
/// This class is used as a pointer to the shared instance in
/// HdInstanceRegistry.
///
/// KEY has to be hashable index type and VALUE is shared_ptr. In most use
/// cases, the client computes a hash key which represents large bulky data
/// (like topology, primVars) and registers it into HdInstanceRegistry. If the
/// key has already been registered, the registry returns HdInstance and the
/// client can use GetValue() without setting/computing actual bulky data. If
/// it doesn't exist, IsFirstInstance() returns true for the first instance
/// and the client needs to populate an appropriate data into through the
/// instance by SetValue().
///
template <typename KEY, typename VALUE>
class HdInstance {
public:
    typedef KEY KeyType;
    typedef VALUE ValueType;

    typedef tbb::concurrent_unordered_map<KeyType, ValueType> Dictionary;

    /// Constructor.
    HdInstance() {}

    /// Initalize the members of HdInstance
    void Create(KeyType const &key,
                ValueType const &value,
                 Dictionary *parent,
                 bool isFirstInstance)
    {
        _key             = key;
        _value           = value;
        _parent          = parent;
        _isFirstInstance = isFirstInstance;
    }
  
    /// Returns the key
    KeyType const &GetKey() const { return _key; }

    /// Returns the value
    ValueType const &GetValue() const { return _value; }

    /// Update the value in dictionary indexed by the key.
    void SetValue(ValueType const &value) {
        if (_parent) (*_parent)[_key] = value;
        _value = value;
    }

    /// Returns true if the value has not been initialized.
    bool IsFirstInstance() const {
        return _isFirstInstance;
    }

private:
    KeyType     _key;
    ValueType   _value;
    Dictionary *_parent;
    bool        _isFirstInstance;
};

/// \class HdInstanceRegistry
///
/// HdInstanceRegistry is a dictionary container of HdInstance.
/// This class is almost just a dictionary from key to value.
/// For cleaning unused entries, it provides GarbageCollect() API.
/// It sweeps all entries in the dictionary and erase unreferenced entries.
/// When HdInstance::ValueType is shared_ptr, it is regarded as unreferenced
/// if the shared_ptr is unique (use_count==1). Note that Key is not
/// involved to determine the lifetime of entries.
///
template <typename INSTANCE>
class HdInstanceRegistry {
public:
    HdInstanceRegistry() = default;
    
    /// Copy constructor.  Need as HdInstanceRegistry is placed in a map
    /// and mutex is not copy constructable, so can't use default
    HdInstanceRegistry(const HdInstanceRegistry &other)
        : _dictionary(other._dictionary),
        _regLock()  // Lock is not copied
    {
    }

    /// Returns a shared instance for given key as a pair of (key, value).
    std::unique_lock<std::mutex> GetInstance(typename INSTANCE::KeyType const &key,
                                             INSTANCE *instance);

    /// Returns a shared instance for a given key as a pair of (key, value)
    /// only if the key exists in the dictionary.
    std::unique_lock<std::mutex> FindInstance(typename INSTANCE::KeyType const &key, 
                                             INSTANCE *instance, bool *found);

    /// Remove entries which has unreferenced key and returns the count of
    /// remaining entries.
    size_t GarbageCollect();

    /// Returns a const iterator being/end of dictionary. Mainly used for
    /// resource auditing.
    typedef typename INSTANCE::Dictionary::const_iterator const_iterator;
    const_iterator begin() const { return _dictionary.begin(); }
    const_iterator end() const { return _dictionary.end(); }

    void Invalidate();

private:
    template <typename T>
    static bool _IsUnique(boost::shared_ptr<T> const &value) {
        return value.unique();
    }

    typedef typename INSTANCE::Dictionary _Dictionary;
    _Dictionary _dictionary;
    std::mutex _regLock;

    HdInstanceRegistry &operator =(HdInstanceRegistry &) = delete;
};

// ---------------------------------------------------------------------------
// instance registry impl

template <typename INSTANCE>
std::unique_lock<std::mutex>
HdInstanceRegistry<INSTANCE>::GetInstance(typename INSTANCE::KeyType const &key, 
                                          INSTANCE *instance)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Grab Registry lock
    // (and don't release it in this function, return it instead)
    std::unique_lock<std::mutex> lock(_regLock);

    typename _Dictionary::iterator it = _dictionary.find(key);
    bool firstInstance = false;
    if (it == _dictionary.end()) {
        // not found. create new one
        it = _dictionary.insert(
            std::make_pair(key, typename INSTANCE::ValueType())).first;

        firstInstance = true;
    }

    instance->Create(key, it->second, &_dictionary, firstInstance);

    return lock;
}

template <typename INSTANCE>
std::unique_lock<std::mutex>
HdInstanceRegistry<INSTANCE>::FindInstance(typename INSTANCE::KeyType const &key, 
                                          INSTANCE *instance, bool *found)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Grab Registry lock
    // (and don't release it in this function, return it instead)
    std::unique_lock<std::mutex> lock(_regLock);

    typename _Dictionary::iterator it = _dictionary.find(key);
    if (it == _dictionary.end()) {
        *found = false;
    } else {
        *found = true;
        instance->Create(key, it->second, &_dictionary, false /*firstInstance*/);
    }

    return lock;
}

template <typename INSTANCE>
size_t
HdInstanceRegistry<INSTANCE>::GarbageCollect()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    size_t count = 0;
    for (typename _Dictionary::iterator it = _dictionary.begin();
         it != _dictionary.end();) {

        // erase instance which isn't referred from anyone
        if (_IsUnique(it->second)) {
            it = _dictionary.unsafe_erase(it);
        } else {
            ++it;
            ++count;
        }
    }
    return count;
}

template <typename INSTANCE>
void
HdInstanceRegistry<INSTANCE>::Invalidate()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _dictionary.clear();
}

#endif  // HD_INSTANCE_REGISTRY_H
