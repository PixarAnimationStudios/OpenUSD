//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DEBUG_UTIL_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DEBUG_UTIL_H

#include "pxr/pxr.h"

#include "pxr/base/gf/matrix4d.h"

#include "Riley.h"
#include "RiTypesHelper.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;
class TfCallContext;

namespace HdPrmanDebugUtil {

std::string
MatrixToString(
    const GfMatrix4d& mat,
    const int indent = 0,
    const int precision = 0);

std::string
MatrixToString(
    const RtMatrix4x4& mat,
    const int indent = 0,
    const int precision = 0);

std::string
RtParamListToString(const RtParamList& params, const int indent = 0);

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

std::string
RileyOutputTypeToString(const riley::RenderOutputType type);

}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DEBUG_UTIL_H
