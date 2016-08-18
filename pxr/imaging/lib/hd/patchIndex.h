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
#ifndef HD_PATCH_INDEX_H
#define HD_PATCH_INDEX_H

#include <cstddef>
#include <iostream>

/// \class Hd_PatchIndex
///
/// N integers array for storing patch indices.
///
template <int N>
class Hd_PatchIndex {
public:
    typedef int ScalarType;
    static const size_t dimension = N;

    /// Equality comparison.
    bool operator==(Hd_PatchIndex const &other) const {
        for (int i = 0 ; i < dimension; ++i)
            if (_indices[i] != other._indices[i]) return false;
        return true;
    }
    bool operator!=(Hd_PatchIndex const &other) const {
        return !(*this == other);
    }

    ScalarType &operator [](size_t i) { return _indices[i]; }
    ScalarType const &operator [](size_t i) const { return _indices[i]; }

private:
    int _indices[N];
};
typedef Hd_PatchIndex<16> Hd_BSplinePatchIndex;

std::ostream& operator<<(std::ostream&, const Hd_BSplinePatchIndex&);

#endif  // HD_PATCH_INDEX_H
