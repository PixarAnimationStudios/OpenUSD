//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_TYPE_INFO_MAP_H
#define PXR_BASE_TF_TYPE_INFO_MAP_H

/// \file tf/typeInfoMap.h
/// \ingroup group_tf_RuntimeTyping
/// \ingroup group_tf_Containers

#include "pxr/pxr.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/iterator.h"

#include "pxr/base/tf/hashmap.h"

#include <typeinfo>
#include <string>
#include <list>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfTypeInfoMap
/// \ingroup group_tf_RuntimeTyping
/// \ingroup group_tf_Containers
///
/// A map whose key is a const std::type_info&, or a string alias.
///
/// A \c TfTypeInfoMap stores values of arbitrary type (template parameter
/// VALUE) under a key that is either a \c const \c std::type_info&, or an \c
/// std::string.  Note that the \c std::type_info structure is many-to-one
/// with respect to its name, i.e. two distinct instances of a \c
/// std::type_info can represent the same type.  Thus, a naive implementation
/// that does pointer comparison on the address of a \c std::type_info can
/// fail.  The \c TfTypeInfoMap takes care of this aliasing.
///
/// Additionally, the table lets one create additional string aliases for a
/// given entry.
///
template <class VALUE>
class TfTypeInfoMap {
    TfTypeInfoMap(const TfTypeInfoMap&) = delete;
    TfTypeInfoMap& operator=(const TfTypeInfoMap&) = delete;
public:

    // Default constructor passes 0 to TfHashMap constructors to keep size
    // small. This is good since each defined TfType has one of these maps in it.
    TfTypeInfoMap() : _nameMap(0), _stringCache(0) {}

    /// Return true if the given key is present in the map.
    bool Exists(const std::type_info& key) const {
        return Find(key) != NULL;
    }

    /// Return true if the given key is present in the map.
    ///
    /// Note that lookup by \c std::type_info is preferable for speed reasons.
    bool Exists(const std::string& key) const {
        return Find(key) != NULL;
    }
    
    /// Return a pointer to the value stored under \p key, and NULL if \p key
    /// is not a key in the map.
    VALUE* Find(const std::type_info& key) const {
        typename _TypeInfoCache::const_iterator i = _typeInfoCache.find(&key);
        if (i != _typeInfoCache.end())
            return &i->second->value;
        else if (VALUE* v = Find(key.name())) {
            return v;
        }
        return NULL;
    }

    /// Return a pointer to the value stored under \p key, and NULL if \p key
    /// is not a key in the map.  For efficiency of future lookups this will
    /// cache the result if it falls back to a string based lookup.  In that
    /// case before updating the cache it will call the functor \p upgrader
    /// to allow the client to upgrade any lock to exclusive access.
    template <class Upgrader>
    VALUE* Find(const std::type_info& key, Upgrader& upgrader) {
        typename _TypeInfoCache::const_iterator i = _typeInfoCache.find(&key);
        if (i != _typeInfoCache.end())
            return &i->second->value;
        else if (VALUE* v = Find(key.name())) {
            upgrader();
            _CreateAlias(key, key.name());
            return v;
        }
        return NULL;
    }

    /// Return a pointer to the value stored under \p key, and NULL if \p key
    /// is not a key in the map.
    ///
    /// Note that lookup by \c std::type_info is preferable for speed reasons.
    VALUE* Find(const std::string& key) const {
        typename _StringCache::const_iterator i = _stringCache.find(key);
        return (i == _stringCache.end()) ? NULL : &i->second->value;
    }

    /// Set the value for a given key.
    ///
    /// Note that if \p key is not already in the table, this creates a new
    /// entry.  Also, \p key.name() is automatically made linked with this
    /// entry, so that future queries can be made via \p key.name(), though
    /// lookup by \c std::type_info is greatly preferred.
    void Set(const std::type_info& key, const VALUE& value) {
        if (VALUE* v = Find(key))
            *v = value;
        else {
            Set(key.name(), value);
            _CreateAlias(key, key.name());
        }
    }

    /// Set the value for a given key.
    ///
    /// Note that if \p key is not already in the table, this creates a new
    /// entry.  Also, lookup by \c std::type_info is preferable for speed
    /// reasons.
    void Set(const std::string& key, const VALUE& value) {
        typename _StringCache::iterator i = _stringCache.find(key);

        if (i != _stringCache.end())
            i->second->value = value;
        else {
            _Entry* e = &_nameMap[key];
            e->primaryKey = key;
            e->value = value;

            _stringCache[key] = e;
            e->stringAliases.push_back(key);
        }
    }

    /// Create an alias for a key.
    ///
    /// Queries with a key of \p alias will return the same data associated
    /// with queries for \p key.
    ///
    /// If \p key is not presently a member of the map, this function does
    /// nothing and returns \c false.
    bool CreateAlias(const std::string& alias, const std::string& key) {
        typename _StringCache::iterator i = _stringCache.find(key);
        if (i != _stringCache.end())
            return (_CreateAlias(alias, i->second), true);
        else
            return false;
    }

    /// \overload
    bool CreateAlias(const std::string& alias, const std::type_info& key) {
        typename _TypeInfoCache::iterator i = _typeInfoCache.find(&key);
        if (i != _typeInfoCache.end())
            return (_CreateAlias(alias, i->second), true);
        else
            return false;
    }
    
    /// Remove this key (and any aliases associated with it).
    void Remove(const std::type_info& key) {
        Remove(key.name());
    }

    /// Remove this key (and any aliases associated with it).
    void Remove(const std::string& key) {
        typename _StringCache::iterator i = _stringCache.find(key);
        if (i == _stringCache.end())
            return;
        
        _Entry* e = i->second;

        for (TfIterator<_TypeInfoList> j = e->typeInfoAliases; j; ++j) {
            _typeInfoCache.erase(*j);
        }
        
        for (TfIterator<std::list<std::string> > j = e->stringAliases; j; ++j) {
            _stringCache.erase(*j);
        }

        // `e` points into the node owned by _nameMap.  Passing
        // e->primaryKey to erase() would cause the node to be
        // deleted.  However, the implementation of erase may access
        // the key again after the node has been deleted (at least,
        // when TfHashMap is __gnu_cxx::hash_map.)
        const std::string primaryKey = std::move(e->primaryKey);
        _nameMap.erase(primaryKey);
    }

private:
     typedef std::list<const std::type_info*> _TypeInfoList;
     
     struct _Entry {
         mutable _TypeInfoList typeInfoAliases;
         mutable std::list<std::string> stringAliases;
         std::string primaryKey;
         VALUE value;
     };

    void _CreateAlias(const std::type_info& alias, const std::string& key) {
        typename _StringCache::iterator i = _stringCache.find(key);
        if (i != _stringCache.end())
            _CreateAlias(alias, i->second);
    }

    void _CreateAlias(const std::type_info& alias, _Entry* e) {
        if (_typeInfoCache.find(&alias) == _typeInfoCache.end()) {
            _typeInfoCache[&alias] = e;
            e->typeInfoAliases.push_back(&alias);
        }
    }

    void _CreateAlias(const std::string& alias, _Entry* e) {
        if (_stringCache.find(alias) == _stringCache.end()) {
            _stringCache[alias] = e;
            e->stringAliases.push_back(alias);
        }
    }

    typedef TfHashMap<std::string, _Entry, TfHash> _NameMap;
    typedef TfHashMap<const std::type_info*, _Entry*, TfHash>
        _TypeInfoCache;
    typedef TfHashMap<std::string, _Entry*, TfHash> _StringCache;

    _NameMap _nameMap;

    _TypeInfoCache _typeInfoCache;
    _StringCache _stringCache;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_TYPE_INFO_MAP_H
