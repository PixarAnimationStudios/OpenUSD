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
#ifndef USDUTILS_DEPENDENCIES_H
#define USDUTILS_DEPENDENCIES_H

/// \file usdUtils/dependencies.h
///
/// Utilities for extracting asset dependencies from a USD file.

#include "pxr/usd/usdUtils/api.h"

#include <string>
#include <vector>

/// Parses the file at \p filePath, identifying external references, and
/// sorting them into separate type-based buckets. Sublayers are returned in
/// the \p sublayers vector, references, whether prim references or values
/// from asset path attributes, are returned in the \p references vector.
/// Payload paths are returned in \p payloads.
///
/// \note No recursive chasing of dependencies is performed; that is the
/// client's responsibility, if desired.
USDUTILS_API
void UsdUtilsExtractExternalReferences(
    const std::string& filePath,
    std::vector<std::string>* subLayers,
    std::vector<std::string>* references,
    std::vector<std::string>* payloads);

#endif // USDUTILS_DEPENDENCIES_H
