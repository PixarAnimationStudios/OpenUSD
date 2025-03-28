//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TRACE_DYNAMIC_KEY_H
#define PXR_BASE_TRACE_DYNAMIC_KEY_H

#include "pxr/pxr.h"
#include "pxr/base/trace/staticKeyData.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
/// \class TraceDynamicKey
///
/// This class stores data used to create dynamic keys which can be referenced
/// in TraceEvent instances.
///
/// Is a key is known at compile time, it is preferable to use
/// a static constexpr TraceStaticKeyData instance instead.
class TraceDynamicKey {
public:
    /// Constructor for TfToken.
    TraceDynamicKey(TfToken name) : _key(std::move(name)) {
        _data._name = _key.GetText();
    }

    /// Constructor for string.
    TraceDynamicKey(const std::string& name) : _key(name) {
        _data._name = _key.GetText();
    }

    /// Constructor for C string.
    TraceDynamicKey(const char* name) : _key(name) {
        _data._name = _key.GetText();
    }
    
    /// Equality operator.
    bool operator == (const TraceDynamicKey& other) const {
        return _key == other._key;
    }

    /// Return a cached hash code for this key.
    size_t Hash() const {
        return _key.Hash();
    }

    /// A Hash functor which uses the cached hash which may be used to store
    /// keys in a TfHashMap.
    struct HashFunctor {
        size_t operator()(const TraceDynamicKey& key) const {
            return key.Hash();
        }
    };

    /// Returns a reference to TraceStaticKeyData.
    const TraceStaticKeyData& GetData() const { return _data; }

private:
    TraceStaticKeyData _data;
    TfToken _key;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_DYNAMIC_KEY_H