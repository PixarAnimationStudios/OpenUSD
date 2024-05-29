//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_ASSET_PATH_H
#define PXR_USD_SDF_ASSET_PATH_H

/// \file sdf/assetPath.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/tf/hash.h"

#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfAssetPath
///
/// Contains an asset path and an optional resolved path.  Asset paths may
/// contain non-control UTF-8 encoded characters.  Specifically, U+0000..U+001F
/// (C0 controls), U+007F (delete), and U+0080..U+009F (C1 controls) are
/// disallowed.  Attempts to construct asset paths with such characters will
/// issue a TfError and produce the default-constructed empty asset path.
///
class SdfAssetPath
{
public:
    /// \name Constructors
    /// @{
    ///

    /// Construct an empty asset path.
    SDF_API SdfAssetPath();

    /// Construct an asset path with \p path and no associated resolved path.
    ///
    /// If the passed \p path is not valid UTF-8 or contains C0 or C1 control
    /// characters, raise a TfError and return the default-constructed empty
    /// asset path.
    SDF_API explicit SdfAssetPath(const std::string &path);

    /// Construct an asset path with \p path and an associated \p resolvedPath.
    ///
    /// If either the passed \path or \p resolvedPath are not valid UTF-8 or
    /// either contain C0 or C1 control characters, raise a TfError and return
    /// the default-constructed empty asset path.
    SDF_API
    SdfAssetPath(const std::string &path, const std::string &resolvedPath);

    /// @}

    ///\name Operators
    /// @{

    /// Equality, including the resolved path.
    bool operator==(const SdfAssetPath &rhs) const {
        return _assetPath == rhs._assetPath &&
               _resolvedPath == rhs._resolvedPath;
    }

    /// Inequality operator
    /// \sa SdfAssetPath::operator==(const SdfAssetPath&)
    bool operator!=(const SdfAssetPath& rhs) const {
        return !(*this == rhs);
    }

    /// Ordering first by asset path, then by resolved path.
    SDF_API bool operator<(const SdfAssetPath &rhs) const;

    /// Less than or equal operator
    /// \sa SdfAssetPath::operator<(const SdfAssetPath&)
    bool operator<=(const SdfAssetPath& rhs) const {
        return !(rhs < *this);
    }

    /// Greater than operator
    /// \sa SdfAssetPath::operator<(const SdfAssetPath&)
    bool operator>(const SdfAssetPath& rhs) const {
        return rhs < *this;
    }

    /// Greater than or equal operator
    /// \sa SdfAssetPath::operator<(const SdfAssetPath&)
    bool operator>=(const SdfAssetPath& rhs) const {
        return !(*this < rhs);
    }

    /// Hash function
    size_t GetHash() const {
        return TfHash::Combine(_assetPath, _resolvedPath);
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
    const std::string &GetAssetPath() const & {
        return _assetPath;
    }

    /// Overload for rvalues, move out the asset path.
    std::string GetAssetPath() const && {
        return std::move(_assetPath);
    }

    /// Return the resolved asset path, if any.
    ///
    /// Note that SdfAssetPath carries a resolved path only if its creator
    /// passed one to the constructor.  SdfAssetPath never performs resolution
    /// itself.
    const std::string &GetResolvedPath() const & {
        return _resolvedPath;
    }

    /// Overload for rvalues, move out the asset path.
    std::string GetResolvedPath() const && {
        return std::move(_resolvedPath);
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
///       two SdfAssetPath s that do not compare equal may produce
///       indistinguishable ostream output.
SDF_API std::ostream& operator<<(std::ostream& out, const SdfAssetPath& ap);

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_ASSET_PATH_H
