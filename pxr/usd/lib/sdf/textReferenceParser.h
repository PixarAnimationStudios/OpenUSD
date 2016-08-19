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
#ifndef SDF_TEXT_REFERENCE_PARSER_H
#define SDF_TEXT_REFERENCE_PARSER_H

/// \file sdf/textReferenceParser.h

#include <string>
#include <vector>

/// Parses the file at \p layerPath, identifying external references, and
/// sorting them into separate type-based buckets. Sublayers are returned in
/// the \p sublayers vector, references, whether prim references or values
/// from asset path attributes, are returned in the \p references vector.
/// Payload paths are returned in \p payloads.
void SdfExtractExternalReferences(
    const std::string& layerPath,
    std::vector<std::string>* subLayers,
    std::vector<std::string>* references,
    std::vector<std::string>* payloads);

/// Parses the data \p layerData, identifying external references, and sorting
/// them into separate type-based buckets. This is identical to
/// SdfExtractExternalReferences, except that the input string is a string
/// containing scene description in sdf text file format.
void SdfExtractExternalReferencesFromString(
    const std::string& layerData,
    std::vector<std::string>* subLayers,
    std::vector<std::string>* references,
    std::vector<std::string>* payloads);

#endif // SDF_TEXT_REFERENCE_PARSER_H
