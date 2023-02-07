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

// This header file enables testing of Unicode collation independently
// of the TF test suite via unicodeCollationConformance.cpp
#ifndef PXR_BASE_TF_UNICODE_DUCET_COMMON_H
#define PXR_BASE_TF_UNICODE_DUCET_COMMON_H

#include <string>
#include <vector>

///
/// Trims whitespace from the front and back of \c input. 
/// Returns a new string with the whitepsace trimmed.
/// 
std::string Trim(const std::string& input);

///
/// Splits a string into multiple substrings at the given \c delimeter.
/// 
/// Returns a vector of strings representing the split output.
/// 
std::vector<std::string> Split(const std::string& input, char delimeter);

///
/// Extracts each collation element of the form [.x.x.x] or [*x.x.x] into individual 64-bit values.
/// 
/// Returns a vector of extracted collation element values.
/// 
std::vector<uint64_t> ExtractCollationElements(const std::string& input);

#endif