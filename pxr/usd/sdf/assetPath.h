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

class SdfAssetPath;

/// \struct SdfAssetPathParams
/// Helper class for explicitly setting values when creating a SdfAssetPath
///
/// Example usage:
/// \code
/// SdfAssetPath myAssetPath(
///    SdfAssetPathParams()
///        .Authored("blah_{VAR}.usda")
///        .Evaluated("blah_foo.usda")
///        .Resolved("/foo/bar/blah_foo.usda")
/// );
/// \endcode
///
class SdfAssetPathParams {
public:
    SdfAssetPathParams& Authored(const std::string &authoredPath_) {
        authoredPath = authoredPath_;
        return *this;
    }

    SdfAssetPathParams& Evaluated(const std::string &evaluatedPath_) {
        evaluatedPath = evaluatedPath_;
        return *this;
    }

    SdfAssetPathParams& Resolved(const std::string &resolvedPath_) {
        resolvedPath = resolvedPath_;
        return *this;
    }

private:
    friend class SdfAssetPath;

    std::string authoredPath;
    std::string evaluatedPath;
    std::string resolvedPath;
};

/// \class SdfAssetPath
///
/// Contains an asset path and optional evaluated and resolved paths.
/// When this class is used to author scene description, the value returned
/// by GetAssetPath() is serialized out, all other fields are ignored.
/// Asset paths may contain non-control UTF-8 encoded characters.
/// Specifically, U+0000..U+001F (C0 controls), U+007F (delete), 
/// and U+0080..U+009F (C1 controls) are disallowed.  Attempts to construct
/// asset paths with such characters will issue a TfError and produce the 
/// default-constructed empty asset path.
///
class SdfAssetPath
{
public:
    /// \name Constructors
    /// @{
    ///

    /// Construct an empty asset path.
    SDF_API SdfAssetPath();

    /// Construct an asset path with \p authoredPath and no associated
    /// evaluated or resolved path.
    ///
    /// If the passed \p authoredPath is not valid UTF-8 or contains C0 or C1
    /// control characters, raise a TfError and return the default-constructed
    /// empty asset path.
    SDF_API explicit SdfAssetPath(const std::string &authoredPath);

    /// Construct an asset path with \p authoredPath and an associated
    /// \p resolvedPath.
    ///
    /// If either the passed \p authoredPath or \p resolvedPath are not valid 
    /// UTF-8 or either contain C0 or C1 control characters, raise a TfError and
    /// return the default-constructed empty asset path.
    SDF_API
    SdfAssetPath(const std::string &authoredPath, 
                 const std::string &resolvedPath);

    /// Construct an asset path using a SdfAssetPathParams object
    ///
    /// If any fields of the passed in structure are not valid UTF-8 or either 
    /// contain C0 or C1 control, characters, raise a TfError and return
    /// the default-constructed empty asset path.
    SDF_API
    SdfAssetPath(const SdfAssetPathParams &params);

    /// @}

    ///\name Operators
    /// @{

    /// Equality, including the evaluated and resolved paths.
    bool operator==(const SdfAssetPath &rhs) const {
        return _authoredPath == rhs._authoredPath &&
               _evaluatedPath == rhs._evaluatedPath &&
               _resolvedPath == rhs._resolvedPath;
    }

    /// Inequality operator
    /// \sa SdfAssetPath::operator==(const SdfAssetPath&)
    bool operator!=(const SdfAssetPath& rhs) const {
        return !(*this == rhs);
    }

    /// Ordering first by asset path, resolved path, then by evaluated path.
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
        return TfHash::Combine(_authoredPath, _evaluatedPath, _resolvedPath);
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

    /// Returns the asset path as it was authored in the original layer.  When
    /// authoring scene description, this value is used for serialization.
    const std::string &GetAuthoredPath() const & {
        return _authoredPath;
    }

    /// Overload for rvalues, move out the asset path.
    std::string GetAuthoredPath() const && {
        return std::move(_authoredPath);
    }

    /// Return the evaluated asset path, if any.  The evaluated path's value
    /// consists of the authored path with any expression variables
    /// evaluated.  If the authored path does not contain any expression
    /// variables, this field will be empty.
    ///
    /// Note that SdfAssetPath carries an evaluated path only if its creator
    /// passed one to the constructor.  SdfAssetPath never performs variable
    /// expression evaluation itself.
    const std::string &GetEvaluatedPath() const & {
        return _evaluatedPath;
    }

    /// Overload for rvalues, move out the evaluated path.
    std::string GetEvaluatedPath() const && {
        return std::move(_evaluatedPath);
    }

    /// Return the asset path. If the the evaluated path is not empty, it will
    /// be returned, otherwise the raw, authored path is returned.
    /// The value this function returns is the exact input that is passed to
    /// asset resolution.
    const std::string &GetAssetPath() const & {
        return _evaluatedPath.empty() ? _authoredPath : _evaluatedPath;
    }

    /// Overload for rvalues, move out the asset path.
    std::string GetAssetPath() const && {
        return std::move(
                _evaluatedPath.empty() ? _authoredPath : _evaluatedPath);
    }

    /// Return the resolved asset path, if any.  This is the resolved value of
    /// GetAssetPath()
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

    /// \name Setters
    /// @{

    /// Sets the authored path.  This value is the path exactly as it is
    /// authored in the layer.
    void SetAuthoredPath(const std::string &authoredPath) {
        _authoredPath = authoredPath;
    }

    /// Sets the evaluated path.  This value is the result of performing
    /// variable expression resolution on the authored path.
    void SetEvaluatedPath(const std::string &evaluatedPath) {
        _evaluatedPath = evaluatedPath;
    }

    /// Sets the resolved path.  This value is the result of asset resolution.
    void SetResolvedPath(const std::string &resolvedPath) {
        _resolvedPath = resolvedPath;
    }

    /// @}

private:
    friend inline void swap(SdfAssetPath &lhs, SdfAssetPath &rhs) {
        lhs._authoredPath.swap(rhs._authoredPath);
        lhs._evaluatedPath.swap(rhs._evaluatedPath);
        lhs._resolvedPath.swap(rhs._resolvedPath);
    }

    /// Raw path, as authored in the layer.
    std::string _authoredPath;
    /// Contains the evaluated authored path, if variable expressions
    /// were present, otherwise empty.
    std::string _evaluatedPath;
    /// Fully evaluated and resolved path
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
