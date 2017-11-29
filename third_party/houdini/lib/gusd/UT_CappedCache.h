//
// Copyright 2017 Pixar
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
/**
   \file
   \brief
*/
#ifndef _GUSD_UT_CAPPEDCACHE_H_
#define _GUSD_UT_CAPPEDCACHE_H_

#include <pxr/pxr.h>

#include <SYS/SYS_AtomicInt.h>
#include <UT/UT_Assert.h>
#include <UT/UT_CappedCache.h>
#include <UT/UT_ConcurrentHashMap.h>
#include <UT/UT_IntrusivePtr.h>

PXR_NAMESPACE_OPEN_SCOPE

/** Convenience wrapper around UT_CappedKey.
    This allows keys to be constructed in TBB's style.
    These can only be used in a UT_CappedCache if all keys
    in the cache have the same type.*/
template <typename KeyT,
          typename HashCompare=tbb::tbb_hash_compare<KeyT> >
class GusdUT_CappedKey : public UT_CappedKey
{
public:
    GusdUT_CappedKey() : UT_CappedKey() {}
    GusdUT_CappedKey(const KeyT& key) : UT_CappedKey(), _key(key) {}

    virtual ~GusdUT_CappedKey() {}

    virtual UT_CappedKey*   duplicate() const   
                            { return new GusdUT_CappedKey(_key); }

    virtual unsigned        getHash() const
                            { return HashCompare().hash(_key); }

    virtual bool            isEqual(const UT_CappedKey& key) const
                            {
                                return HashCompare().equal(_key,
                                    UTverify_cast<const GusdUT_CappedKey*>(
                                        &key)->_key);
                            }

    KeyT*                   operator->()        { return &_key; }
    const KeyT*             operator->() const  { return &_key; }
    KeyT&                   operator*()         { return _key; }
    const KeyT&             operator*() const   { return _key; }
    
private:
    KeyT    _key;
};


/** Variant of UT_CappedCache that improves on item construction.
    This adds in a mechanism for locking items during construction,
    to prevent multiple threads from performing the same work to
    initialize cache items.*/
class GusdUT_CappedCache : public UT_CappedCache
{
public:
    GusdUT_CappedCache(const char* name, int64 size_in_mb=32)
        : UT_CappedCache(name, size_in_mb) {}
    ~GusdUT_CappedCache() {}

    template <typename Item>
    UT_IntrusivePtr<const Item> Find(const UT_CappedKey& key);
    
    template <typename Item,typename Creator,typename... Args> 
    UT_IntrusivePtr<const Item> FindOrCreate(const UT_CappedKey& key,
                                             const Creator& creator,
                                             Args&... args);

    template <typename T>
    bool                        FindVal(const UT_CappedKey& key);

    template <typename T,typename Creator,typename... Args>
    bool                        FindOrCreateVal(const UT_CappedKey& key,
                                                const Creator& creator,
                                                Args&... args);
    
    template <typename MatchFn>
    int64                       ClearEntries(const MatchFn& matchFn);
    
private:

    struct _HashCompare
    {
        static size_t   hash(const UT_CappedKeyHandle& k)
                        { return (size_t)k->getHash(); }
        static bool     equal(const UT_CappedKeyHandle& a,
                              const UT_CappedKeyHandle& b)
                        { return a->isEqual(*b); }
    };
    
    
    typedef UT_ConcurrentHashMap<UT_CappedKeyHandle,
                                 UT_CappedItemHandle,
                                 _HashCompare>  _ConstructMap;
    _ConstructMap   _constructMap;
};


template <typename Item>
UT_IntrusivePtr<const Item>
GusdUT_CappedCache::Find(const UT_CappedKey& key)
{
    if(UT_CappedItemHandle hnd = findItem(key))
        return UT_IntrusivePtr<const Item>(
            UTverify_cast<const Item*>(hnd.get()));
    return UT_IntrusivePtr<const Item>();
}


template <typename Item, typename Creator, typename... Args>
UT_IntrusivePtr<const Item>
GusdUT_CappedCache::FindOrCreate(const UT_CappedKey& key,
                                 const Creator& creator,
                                 Args&... args)
{
    if(auto item = Find<Item>(key))
        return item;

    UT_CappedKeyHandle keyHnd(key.duplicate());

    _ConstructMap::accessor a;
    if(_constructMap.insert(a, keyHnd)) {
        // Make sure another thread didn't beat us to it.
        a->second = findItem(key);
        if(!a->second) {
            if((a->second = creator(args...))) {
                addItem(key, a->second);
            } else {
                _constructMap.erase(a);
                return UT_IntrusivePtr<const Item>();
            }
        }
    }
    UT_IntrusivePtr<const Item> item(
        UTverify_cast<const Item*>(a->second.get()));
    _constructMap.erase(a);
    return item;
}

template <typename MatchFn>
int64
GusdUT_CappedCache::ClearEntries(const MatchFn& matchFn)
{
    int64 freed = 0;

    auto traverseFn([&](const UT_CappedKeyHandle& key,
                        const UT_CappedItemHandle& item)
        {
            if(matchFn(key, item)) {
                freed += item->getMemoryUsage();
                // XXX: deleteItem() is safe in threadSafeTraversal!
                this->deleteItem(*key);
            }
            return true;
        });
    threadSafeTraversal(traverseFn);
   return freed;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_UT_CAPPEDCACHE_H_*/
