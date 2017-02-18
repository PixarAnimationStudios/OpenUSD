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
#ifndef TF_TIME_STAMP_H
#define TF_TIME_STAMP_H

/// \file tf/timeStamp.h
/// \ingroup group_tf_Multithreading

#include "pxr/pxr.h"

#include "pxr/base/arch/inttypes.h"
#include "pxr/base/tf/api.h"
#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfTimeStamp
/// \ingroup group_tf_Multithreading
///
/// Class that contains a time stamp.
///
/// This class contains a 64-bit timestamp. 
///
class TfTimeStamp
{
public:
    /// Default constructor. Leaves timestamp uninitialized.
    inline TfTimeStamp() {}

    /// Initializes timestamp to given value.
    inline explicit TfTimeStamp(const uint64_t &value) {
        _value = value;
    }

    /// Copy constructor.
    inline TfTimeStamp(const TfTimeStamp &timeStamp) {
        _value = timeStamp._value;
    }

    /// Assignment operator
    inline const TfTimeStamp & operator=(const TfTimeStamp &timeStamp) {
        _value = timeStamp._value;
        return *this;
    }

    inline const TfTimeStamp & operator=(uint64_t value) {
        _value = value;
        return *this;
    }

    /// Cast operator to an uint64_t
    operator uint64_t() const {
        return _value;
    }

    /// Set timestamp.
    void Set(uint64_t value) {
        _value = value;
    }

    /// Get timestamp.
    uint64_t Get() const {
        return _value;
    }

    /// Increment the timestamp by one.
    TfTimeStamp Increment() {
        ++_value;
        return *this;
    }

    /// Decrement the timestamp by one.
    TfTimeStamp Decrement() {
        --_value;
        return *this;
    }

    /// Equality operator.
    bool operator==(const TfTimeStamp &timeStamp) {
        return _value == timeStamp._value;
    }

    /// Inequality operator.
    bool operator!=(const TfTimeStamp &timeStamp) {
        return _value != timeStamp._value;
    }

    /// Greater than operator.
    bool operator>(const TfTimeStamp &timeStamp) {
        return _value > timeStamp._value;
    }

    /// Less than operator.
    bool operator<(const TfTimeStamp &timeStamp) {
        return _value < timeStamp._value;
    }

    /// Greater than or equal operator.
    bool operator>=(const TfTimeStamp &timeStamp) {
        return _value >= timeStamp._value;
    }

    /// Less than or equal operator.
    bool operator<=(const TfTimeStamp &timeStamp) {
        return _value <= timeStamp._value;
    }

private:
    uint64_t _value;
};

/// \name Related
/// @{

/// Stream insertion operator for the string representation of this timestamp
TF_API std::ostream& operator<<(std::ostream& out, const TfTimeStamp& t);

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_TIME_STAMP_H
