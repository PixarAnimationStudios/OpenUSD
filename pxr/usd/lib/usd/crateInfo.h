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
#ifndef USD_CRATEINFO_H
#define USD_CRATEINFO_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/base/tf/token.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdCrateInfo
///
/// A class for introspecting the underlying qualities of .usdc 'crate' files,
/// for diagnostic purposes.
///
class UsdCrateInfo
{
public:
    struct Section {
        Section() = default;
        Section(std::string const &name, int64_t start, int64_t size)
            : name(name), start(start), size(size) {}
        std::string name;
        int64_t start = -1, size = -1;
    };

    struct SummaryStats {
        size_t numSpecs = 0;
        size_t numUniquePaths = 0;
        size_t numUniqueTokens = 0;
        size_t numUniqueStrings = 0;
        size_t numUniqueFields = 0;
        size_t numUniqueFieldSets = 0;
    };

    /// Attempt to open and read \p fileName.
    USD_API
    static UsdCrateInfo Open(std::string const &fileName);

    /// Return summary statistics structure for this file.
    USD_API
    SummaryStats GetSummaryStats() const;

    /// Return the named file sections, their location and sizes in the file.
    USD_API
    std::vector<Section> GetSections() const;

    /// Return the file version.
    USD_API
    TfToken GetFileVersion() const;
    
    /// Return the software version.
    USD_API
    TfToken GetSoftwareVersion() const;

    /// Return true if this object refers to a valid file.
    explicit operator bool() const { return (bool)_impl; }

private:

    struct _Impl;
    std::shared_ptr<_Impl> _impl;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_CRATEINFO_H
