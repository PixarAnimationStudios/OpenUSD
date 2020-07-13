//
// Copyright 2020 benmalartre
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
#ifndef PXR_USD_PLUGIN_USD_ANIMX_KEYFRAME_H
#define PXR_USD_PLUGIN_USD_ANIMX_KEYFRAME_H

#include "pxr/pxr.h"
#include "pxr/usd/usdAnimX/api.h"
#include "pxr/usd/usdAnimX/desc.h"
#include "pxr/usd/usdAnimX/animx.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/array.h"
#include <vector>
#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

struct UsdAnimXKeyframe : public adsk::Keyframe {
    ANIMX_API
    UsdAnimXKeyframe();
    ANIMX_API
    UsdAnimXKeyframe(const UsdAnimXKeyframeDesc &desc, size_t index);
    ANIMX_API
    UsdAnimXKeyframe(double time, VtValue &value, size_t index);

    ANIMX_API
    UsdAnimXKeyframeDesc GetDesc();
    ANIMX_API
    VtValue GetAsSample();

    /// Hash.
    friend inline size_t hash_value(UsdAnimXKeyframe const &key) {
        size_t h = 0;
        h ^= (std::hash<double>{}(key.time) << 1);
        h ^= (std::hash<double>{}(key.value) << 1);
        h ^= (std::hash<short>{}((short)key.tanIn.type) << 1);
        h ^= (std::hash<double>{}(key.tanIn.x) << 1);
        h ^= (std::hash<double>{}(key.tanIn.y) << 1);
        h ^= (std::hash<short>{}((short)key.tanOut.type) << 1);
        h ^= (std::hash<double>{}(key.tanOut.x) << 1);
        h ^= (std::hash<double>{}(key.tanOut.y) << 1);
        h ^= (std::hash<double>{}(key.quaternionW) << 1);
        return h;
    }

    /// Equality comparison.
    bool operator==(UsdAnimXKeyframe const &other) const {
        return time == other.time &&
               value == other.value &&
               tanIn.type == other.tanIn.type &&
               tanIn.x == other.tanIn.x &&
               tanIn.y == other.tanIn.y &&
               tanOut.type == other.tanOut.type &&
               tanOut.x == other.tanOut.x &&
               tanOut.y == other.tanOut.y &&
               quaternionW == other.quaternionW;
    }
    bool operator!=(UsdAnimXKeyframe const &other) const {
        return !(*this == other);
    }
};

ANIMX_API std::ostream& operator<<(std::ostream&, const UsdAnimXKeyframe&);
ANIMX_API std::ostream& operator<<(std::ostream&, const UsdAnimXKeyframeDesc&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PLUGIN_USD_ANIMX_KEYFRAME_H
