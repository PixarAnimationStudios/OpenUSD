//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_ALLOWED_H
#define PXR_USD_SDF_ALLOWED_H

/// \file sdf/allowed.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/tf/diagnostic.h"

#include <optional>
#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfAllowed
///
/// Indicates if an operation is allowed and, if not, why not.
///
/// A \c SdfAllowed either evaluates to \c true in a boolean context
/// or evaluates to \c false and has a string annotation.
///
class SdfAllowed {
private:
    typedef std::optional<std::string> _State;

public:
    typedef std::pair<bool, std::string> Pair;

    /// Construct \c true.
    SdfAllowed() { }
    /// Construct \c true.
    SdfAllowed(bool x) { TF_AXIOM(x); }
    /// Construct \c false with annotation \p whyNot.
    SdfAllowed(const char* whyNot) : _state(std::string(whyNot)) { }
    /// Construct \c false with annotation \p whyNot.
    SdfAllowed(const std::string& whyNot) : _state(whyNot) { }
    /// Construct in \p condition with annotation \p whyNot if \c false.
    SdfAllowed(bool condition, const char* whyNot) :
        SdfAllowed(condition, std::string(whyNot)) { }
    /// Construct in \p condition with annotation \p whyNot if \c false.
    SdfAllowed(bool condition, const std::string& whyNot) :
        _state(condition ? std::nullopt :
               std::make_optional(whyNot)) { }
    /// Construct from bool,string pair \p x.
    SdfAllowed(const Pair& x) : SdfAllowed(x.first, x.second) { }
    ~SdfAllowed() { }

#if !defined(doxygen)
    typedef _State SdfAllowed::*UnspecifiedBoolType;
#endif

    /// Returns \c true in a boolean context if allowed, \c false otherwise.
    operator UnspecifiedBoolType() const
    {
        return _state ? NULL : &SdfAllowed::_state;
    }

    /// Returns \c false in a boolean context if allowed, \c true otherwise.
    bool operator!() const
    {
        return static_cast<bool>(_state);
    }

    /// Returns the reason why the operation is not allowed.  If the
    /// operation is allowed this returns the empty string.
    operator const std::string&() const
    {
        return GetWhyNot();
    }

    /// Returns the reason why the operation is not allowed.  If the
    /// operation is allowed this returns the empty string.
    SDF_API const std::string& GetWhyNot() const;

    /// Returns \c true if allowed, otherwise fills \p whyNot if not \c NULL
    /// and returns \c false.
    bool IsAllowed(std::string* whyNot) const
    {
        if (whyNot && _state) {
            *whyNot = *_state;
        }
        return !_state;
    }

    /// Compare to \p other.  Returns \c true if both are \c true or
    /// both are \c false and reasons why not are identical.
    bool operator==(const SdfAllowed& other) const
    {
        return _state == other._state;
    }

    bool operator!=(const SdfAllowed& other) const
    {
        return !(*this == other);
    }

private:
    _State _state;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_ALLOWED_H
