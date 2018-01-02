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
#ifndef HD_TYPES_H
#define HD_TYPES_H

#include "pxr/pxr.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

///
/// Type representing a set of dirty bits.
///
typedef uint32_t HdDirtyBits;

///
/// HdVec4f_2_10_10_10_REV is a compact representation of a GfVec4f.
/// It uses 10 bits for x, y, and z, and 2 bits for w.
///
/// XXX We expect this type to move again as we continue work on
/// refactoring the GL dependencies.
/// 
struct HdVec4f_2_10_10_10_REV {
    // we treat packed type as single-component values
    static const size_t dimension = 1;

    HdVec4f_2_10_10_10_REV() { }

    template <typename Vec3Type>
    HdVec4f_2_10_10_10_REV(Vec3Type const &value) {
        x = to10bits(value[0]);
        y = to10bits(value[1]);
        z = to10bits(value[2]);
        w = 0;
    }

    // ref. GL spec 2.3.5.2
    //   Conversion from floating point to normalized fixed point
    template <typename R>
    int to10bits(R v) {
        return int(
            std::round(
                std::min(std::max(v, static_cast<R>(-1)), static_cast<R>(1))
                *static_cast<R>(511)));
    }

    bool operator==(const HdVec4f_2_10_10_10_REV &other) const {
        return (other.w == w && 
                other.z == z && 
                other.y == y && 
                other.x == x);
    }

    int x : 10;
    int y : 10;
    int z : 10;
    int w : 2;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_TYPES_H
