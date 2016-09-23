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
#ifndef VT_KEY_VALUE_H
#define VT_KEY_VALUE_H

#include "pxr/base/vt/value.h"

#include <string>

/// \class VtKeyValue
///
/// Provides a container for a key-value pair where the key is a std::sting
/// and the value is a \a VtValue.
/// 
/// Used for creating a key-value pair to be stored in a \a VtDictionary.
///
class VtKeyValue {

  public:

    /// Constructor taking a key and a value.
    template <typename T>
    VtKeyValue(std::string const &key, T const &value) :
        _key(key), _value(value) {}

    /// Key accessor.
    std::string const &GetKey() const {
        return _key;
    }
    /// Value accessor.
    VtValue const &GetValue() const {
        return _value;
    }

  private:
    std::string _key;
    VtValue _value;
};

#endif // VT_KEY_VALUE_H
