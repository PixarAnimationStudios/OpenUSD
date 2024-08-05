//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_BINARY_H
#define PXR_BASE_TS_BINARY_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/vt/dictionary.h"

#include <vector>
#include <unordered_map>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE


// For writing splines to, and reading them from, binary files.
//
struct Ts_BinaryDataAccess
{
public:
    // Get current version that will be written.
    // Version history:
    // 1: initial version.
    static constexpr uint8_t GetBinaryFormatVersion() { return 1; }

    // Write a spline to binary data.  There are two outputs: a blob, and a
    // customData map-of-dictionaries that consists of standard types.
    TS_API
    static void GetBinaryData(
        const TsSpline &spline,
        std::vector<uint8_t> *buf,
        const std::unordered_map<TsTime, VtDictionary> **customDataOut);

    // Read a spline out of binary data.
    TS_API
    static TsSpline CreateSplineFromBinaryData(
        const std::vector<uint8_t> &buf,
        std::unordered_map<TsTime, VtDictionary> &&customData);

private:
    static TsSpline _ParseV1(
        const std::vector<uint8_t> &buf,
        std::unordered_map<TsTime, VtDictionary> &&customData);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
