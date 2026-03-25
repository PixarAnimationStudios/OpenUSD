//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#pragma once

#include <unordered_map>
#include <string>
#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// A struct to represent each individual entry within a USDZ package.
struct HttpPackageEntry {
    uint32_t size;    ///< Total size of the entry (local header + data)
    uint32_t offset;  ///< Byte offset from the start of the zip file
};

/// A struct to represent metadata about a USDZ package for lookup later.
struct HttpPackageInfo {
    bool streamable;   ///< Whether the package was successfully parsed for streaming
    std::string entrypoint; ///< The first entry in the zip (USDZ entrypoint by spec)
    std::unordered_map<std::string, HttpPackageEntry> entries;
};

PXR_NAMESPACE_CLOSE_SCOPE
