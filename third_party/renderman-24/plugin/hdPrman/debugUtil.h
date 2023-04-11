//
// Copyright 2023 Pixar
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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DEBUG_UTIL_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DEBUG_UTIL_H

#include "pxr/pxr.h"
#include "pxr/base/arch/stackTrace.h"
#include "hdPrman/renderParam.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

namespace HdPrmanDebugUtil {

std::string
RtParamListToString(const RtParamList& params, const std::string& name = "");

std::string
GetCallerAsString(const TfCallContext& ctx);

template <typename T>
std::string
RileyIdVecToString(const std::vector<T>& vec)
{
    std::string out;
    for (const T& val : vec) {
        if (!out.empty()) {
            out += ", ";
        }
        out += std::to_string(val.AsUInt32());
    }
    return out;
}

std::string
SdfPathVecToString(const std::vector<SdfPath>& vec);

}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DEBUG_UTIL_H
