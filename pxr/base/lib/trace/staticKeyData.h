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

#ifndef TRACE_STATIC_KEY_DATA_H
#define TRACE_STATIC_KEY_DATA_H

#include "pxr/pxr.h"

#include <boost/operators.hpp>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// \class TraceStaticKeyData
///
/// This class holds data necessary to create keys for TraceEvent instances.
/// This class is meant to be used as constexpr static data.
///
class TraceStaticKeyData : boost::totally_ordered<TraceStaticKeyData> {
public:
    /// \class StringLiteral
    ///
    /// This is a helper class for the constructors of TraceStaticKeyData.
    ///
    class StringLiteral {
    public:
        /// Constructor from string literals.
        template <size_t N>
        constexpr StringLiteral(const char(&s)[N]) : str(s) {}

        /// Default Constructor.
        constexpr StringLiteral() : str(nullptr) {}

    private:
        const char* str;

        friend class TraceStaticKeyData;
    };

    /// Constructor for a \p name.
    constexpr TraceStaticKeyData(const StringLiteral name) 
        : _name(name.str) {}
    
    /// Constructor for a function (\p func, \p prettyFunc) and optional 
    /// scope \p name.
    constexpr TraceStaticKeyData(
        const StringLiteral func,
        const StringLiteral prettyFunc,
        const StringLiteral name = StringLiteral())
        : _funcName(func.str)
        , _prettyFuncName(prettyFunc.str)
        , _name(name.str) {}

    /// Equality comparison.  Inequality is also defined.
    bool operator == (const TraceStaticKeyData& other) const;

    /// Returns the string representation of the key data.
    std::string GetString() const;

private:
    TraceStaticKeyData() {}

    const char* _funcName = nullptr;
    const char* _prettyFuncName = nullptr;
    const char* _name = nullptr;

    friend class TraceDynamicKey;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TRACE_STATIC_KEY_DATA_H