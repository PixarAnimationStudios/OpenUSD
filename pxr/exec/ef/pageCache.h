//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_PAGE_CACHE_H
#define PXR_EXEC_EF_PAGE_CACHE_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/ef/api.h"
#include "pxr/exec/ef/outputValueCache.h"
#include "pxr/exec/ef/vectorKey.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class Ef_PageCache
///
/// \brief Organizes output-to-value caches into logical groups, called pages.
///        Pages are keyed off of VdfVector values, such as for example,
///        time values.
///
class Ef_PageCache
{
public:
    /// Destructor.
    ///
    EF_API
    ~Ef_PageCache();

    /// Construct a new Ef_PageCache, which uses VdfVectors holding type T
    /// values as keys.
    ///
    /// The client is responsible for deleting the heap allocated return
    /// value.
    ///
    template < typename T >
    static Ef_PageCache *New() {
        return new Ef_PageCache(&_KeyFactory<T>);
    }

    /// Get the output-to-value cache associated with the given key,
    /// if one exists.
    ///
    EF_API
    Ef_OutputValueCache *Get(const VdfVector &key) const;

    /// Get the output-to-value cache associated with the given key,
    /// or create a new, empty output-to-value cache if one does not
    /// already exist.
    ///
    EF_API
    Ef_OutputValueCache *GetOrCreate(const VdfVector &key);

    /// Clear the entire page cache.
    ///
    EF_API
    size_t Clear();

    /// Type of the cache map.
    ///
    typedef
        Ef_VectorKey::Map<Ef_OutputValueCache *>::Type
        CacheMap;

    /// Type of the cache map iterator.
    ///
    typedef
        typename CacheMap::iterator 
        iterator;

    /// Returns an iterator to the output-to-value cache in the first page.
    ///
    iterator begin() {
        return _cache.begin();
    }

    /// Returns an iterator past the output-to-value cache in the last page.
    ///
    iterator end() {
        return _cache.end();
    }

private:
    // The type of the factory function, which is responsible for creating a
    // Ef_VectorKey from a given VdfVector.
    using _KeyFactoryFunction =
        Ef_VectorKey::StoredType (*) (const VdfVector &);

    // Constructor.
    EF_API
    Ef_PageCache(_KeyFactoryFunction keyFactory);

    // A factory functor, which creates an Ef_VectorKey (for use as a key in
    // a hash map) from a VdfVector holding data of type T. 
    template < typename T >
    static Ef_VectorKey::StoredType _KeyFactory(const VdfVector &value) {
        return Ef_VectorKey::StoredType(new Ef_TypedVectorKey<T>(value));
    }

private:
    // The map of pages to output-to-value caches.
    CacheMap _cache;

    // The key factory function for building new Ef_VectorKeys.
    _KeyFactoryFunction _keyFactory;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
