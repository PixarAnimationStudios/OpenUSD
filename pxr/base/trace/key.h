//
// Copyright 2018 Pixar
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

#ifndef PXR_BASE_TRACE_KEY_H
#define PXR_BASE_TRACE_KEY_H

#include "pxr/pxr.h"
#include "pxr/base/trace/staticKeyData.h"


PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
/// \class TraceKey
///
/// A wrapper around a TraceStaticKeyData pointer that is stored in TraceEvent 
/// instances.
///
class TraceKey {
public:
    /// Constructor.
    constexpr TraceKey(const TraceStaticKeyData& data) : _ptr(&data) {}

    /// Equality comparison.
    bool operator == (const TraceKey& other) const {
        if (_ptr == other._ptr) {
            return true;
        } else {
            return *_ptr == *other._ptr;
        }
    }

    /// Hash function.
    size_t Hash() const {
        return reinterpret_cast<size_t>(_ptr)/sizeof(TraceStaticKeyData);
    }

    /// A Hash functor which may be used to store keys in a TfHashMap.
    struct HashFunctor {
        size_t operator()(const TraceKey& key) const {
            return key.Hash();
        }
    };

private:
    const TraceStaticKeyData* _ptr;

    // TraceCollection converts TraceKeys to TfTokens for visitors.
    friend class TraceCollection;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_KEY_H
