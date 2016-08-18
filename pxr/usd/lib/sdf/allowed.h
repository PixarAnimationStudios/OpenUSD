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
#ifndef SD_ALLOWED_H
#define SD_ALLOWED_H

/// \file sdf/allowed.h

#include "pxr/base/tf/diagnostic.h"
#include "pxr/usd/sdf/api.h"
#include <string>
#include <utility>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

/// \class SdfAllowed
///
/// Indicates if an operation is allowed and, if not, why not.
///
/// A \c SdfAllowed either evaluates to \c true in a boolean context
/// or evaluates to \c false and has a string annotation.
///
class SdfAllowed : private boost::equality_comparable<SdfAllowed> {
private:
    typedef boost::optional<std::string> _State;

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
        _state(not condition, std::string(whyNot)) { }
    /// Construct in \p condition with annotation \p whyNot if \c false.
    SdfAllowed(bool condition, const std::string& whyNot) :
        _state(not condition, whyNot) { }
    /// Construct from bool,string pair \p x.
    SdfAllowed(const Pair& x) : _state(not x.first, x.second) { }
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
        if (whyNot and _state) {
            *whyNot = *_state;
        }
        return not _state;
    }

    /// Compare to \p other.  Returns \c true if both are \c true or
    /// both are \c false and reasons why not are identical.
    bool operator==(const SdfAllowed& other) const
    {
        return _state == other._state;
    }

private:
    _State _state;
};

#endif
