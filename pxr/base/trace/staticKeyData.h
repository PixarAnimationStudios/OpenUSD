//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TRACE_STATIC_KEY_DATA_H
#define PXR_BASE_TRACE_STATIC_KEY_DATA_H

#include "pxr/pxr.h"
#include "pxr/base/trace/api.h"

#include <cstddef>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// \class TraceStaticKeyData
///
/// This class holds data necessary to create keys for TraceEvent instances.
/// This class is meant to be used as constexpr static data.
///
class TraceStaticKeyData {
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
    TRACE_API
    bool operator == (const TraceStaticKeyData& other) const;

    bool operator != (const TraceStaticKeyData& other) const {
        return !(*this == other);
    }

    /// Returns the string representation of the key data.
    TRACE_API
    std::string GetString() const;

private:
    TraceStaticKeyData() {}

    const char* _funcName = nullptr;
    const char* _prettyFuncName = nullptr;
    const char* _name = nullptr;

    friend class TraceDynamicKey;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_STATIC_KEY_DATA_H
