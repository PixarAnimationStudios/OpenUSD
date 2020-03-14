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
#ifndef PXR_USD_SDF_ASSET_PATH_H
#define PXR_USD_SDF_ASSET_PATH_H

/// \file sdf/assetPath.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include <boost/functional/hash.hpp>
#include <boost/operators.hpp>
#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfAssetPath
///
/// Contains an asset path and an optional resolved path.
///
class SdfAssetPath:
    public boost::totally_ordered<SdfAssetPath>
{
public:
    /// \name Constructors
    /// @{
    ///

    /// Construct an empty asset path.
    SDF_API SdfAssetPath();

    /// Construct asset path with no associated resolved path.
    SDF_API explicit SdfAssetPath(const std::string &path);

    /// Construct an asset path with an associated resolved path.
    SDF_API SdfAssetPath(const std::string &path, const std::string &resolvedPath);

    /// @}

    ///\name Operators
    /// @{

    /// Equality, including the resolved path.
    bool operator==(const SdfAssetPath &rhs) const {
        return _assetPath == rhs._assetPath &&
               _resolvedPath == rhs._resolvedPath;
    }

    /// Ordering first by asset path, then by resolved path.
    SDF_API bool operator<(const SdfAssetPath &rhs) const;

    /// Hash function
    size_t GetHash() const {
        size_t hash = 0;
        boost::hash_combine(hash, _assetPath);
        boost::hash_combine(hash, _resolvedPath);
        return hash;
    }

    /// \class Hash
    struct Hash
    {
        size_t operator()(const SdfAssetPath &ap) const {
            return ap.GetHash();
        }
    };

    friend size_t hash_value(const SdfAssetPath &ap) { return ap.GetHash(); }

    /// @}

    /// \name Accessors
    /// @{

    /// Return the asset path.
    const std::string &GetAssetPath() const {
        return _assetPath;
    }

    /// Return the resolved asset path, if any.
    ///
    /// Note that SdfAssetPath only carries a resolved path if the creator of
    /// an instance supplied one to the constructor.  SdfAssetPath will never
    /// perform any resolution itself.
    const std::string &GetResolvedPath() const {
        return _resolvedPath;
    }

    /// @}

private:
    friend inline void swap(SdfAssetPath &lhs, SdfAssetPath &rhs) {
        lhs._assetPath.swap(rhs._assetPath);
        lhs._resolvedPath.swap(rhs._resolvedPath);
    }

    std::string _assetPath;
    std::string _resolvedPath;
};

/// \name Related
/// @{

/// Stream insertion operator for the string representation of this assetPath.
///
/// \note This always encodes only the result of GetAssetPath().  The resolved
///       path is ignored for the purpose of this operator.  This means that
///       two SdfAssetPath that do not compare equal may produce
///       indistinguishable ostream output.
SDF_API std::ostream& operator<<(std::ostream& out, const SdfAssetPath& ap);

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_ASSET_PATH_H
