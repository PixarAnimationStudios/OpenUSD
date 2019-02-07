//
// Copyright 2019 Google LLC
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

#ifndef USDDRACO_FLAG_H
#define USDDRACO_FLAG_H


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdDracoFlag
///
/// Stores command-line flag value and whether it has been explicitly specified
/// by the user.
///
template <class T>
class UsdDracoFlag {
public:
    UsdDracoFlag() : _specified(false), _value() {}
    UsdDracoFlag(const T &value) : _specified(true), _value(value) {}
    bool HasValue() const { return _specified; }
    const T &GetValue() const { return _value; }
    static UsdDracoFlag<bool> MakeBooleanFlag(int value) {
        if (value == 0 || value == 1)
            return UsdDracoFlag<bool>(value);
        return UsdDracoFlag<bool>();
    }

private:
    const bool _specified;
    const T _value;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // USDDRACO_FLAG_H
