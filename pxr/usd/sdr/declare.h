//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDR_DECLARE_H
#define PXR_USD_SDR_DECLARE_H

/// \file sdr/declare.h
///
/// \note
/// All Ndr objects are deprecated in favor of the corresponding Sdr objects
/// in this file. All existing pxr/usd/ndr implementations will be moved to
/// pxr/usd/sdr.

#include "pxr/pxr.h"
#include "pxr/usd/sdr/api.h"
#include "pxr/usd/ndr/declare.h"
#include "pxr/base/tf/token.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class SdrShaderNode;
class SdrShaderProperty;
class SdfValueTypeName;

/// Common typedefs that are used throughout the SDR library.

typedef TfToken SdrIdentifier;
typedef TfToken::HashFunctor SdrIdentifierHashFunctor;
inline const std::string&
SdrGetIdentifierString(const SdrIdentifier& id) { return id.GetString(); }
typedef std::vector<SdrIdentifier> SdrIdentifierVec;
typedef std::unordered_set<SdrIdentifier,
                           SdrIdentifierHashFunctor> SdrIdentifierSet;

// Token
typedef std::vector<TfToken> SdrTokenVec;
typedef std::unordered_map<TfToken, std::string,
                           TfToken::HashFunctor> SdrTokenMap;

// ShaderNode
typedef SdrShaderNode* SdrShaderNodePtr;
typedef SdrShaderNode const* SdrShaderNodeConstPtr;
typedef std::unique_ptr<SdrShaderNode> SdrShaderNodeUniquePtr;
typedef std::vector<SdrShaderNodeConstPtr> SdrShaderNodeConstPtrVec;
typedef SdrShaderNodeConstPtrVec SdrShaderNodePtrVec;
typedef std::vector<SdrShaderNodeUniquePtr> SdrShaderNodeUniquePtrVec;

// ShaderProperty
typedef SdrShaderProperty* SdrShaderPropertyPtr;
typedef SdrShaderProperty const* SdrShaderPropertyConstPtr;
typedef std::unique_ptr<SdrShaderProperty> SdrShaderPropertyUniquePtr;
typedef std::vector<SdrShaderPropertyUniquePtr> SdrShaderPropertyUniquePtrVec;
typedef std::unordered_map<TfToken, SdrShaderPropertyConstPtr,
                           TfToken::HashFunctor> SdrShaderPropertyMap;
typedef SdrShaderPropertyMap SdrPropertyMap;

// Misc
typedef std::vector<std::string> SdrStringVec;
typedef std::pair<TfToken, TfToken> SdrOption;
typedef std::vector<SdrOption> SdrOptionVec;
typedef std::unordered_set<std::string> SdrStringSet;

/// SdrVersion
class SdrVersion {
public:
    /// Create an invalid version.
    SDR_API
    SdrVersion() = default;
    /// Create a version with the given major and minor numbers.
    /// Numbers must be non-negative, and at least one must be non-zero.  
    /// On failure generates an error and yields an invalid version.
    SDR_API
    SdrVersion(int major, int minor = 0);
    /// Create a version from a string.  On failure generates an error and
    /// yields an invalid version.
    SDR_API
    SdrVersion(const std::string& x);

    /// Return an equal version marked as default.  It's permitted to mark
    /// an invalid version as the default.
    SDR_API
    SdrVersion GetAsDefault() const
    {
        return SdrVersion(*this, true);
    }

    /// Return the major version number or zero for an invalid version.
    SDR_API
    int GetMajor() const { return _major; }
    /// Return the minor version number or zero for an invalid version.
    SDR_API
    int GetMinor() const { return _minor; }
    /// Return true iff this version is marked as default.
    SDR_API
    bool IsDefault() const { return _isDefault; }

    /// Return the version as a string.
    SDR_API
    std::string GetString() const;

    /// Return the version as a identifier suffix.
    SDR_API
    std::string GetStringSuffix() const;

    /// Return a hash for the version.
    SDR_API
    std::size_t GetHash() const
    {
        return (static_cast<std::size_t>(_major) << 32) +
                static_cast<std::size_t>(_minor);
    }

    /// Return true iff the version is valid.
    SDR_API
    explicit operator bool() const
    {
        return !!*this;
    }

    /// Return true iff the version is invalid.
    SDR_API
    bool operator!() const
    {
        return _major == 0 && _minor == 0;
    }

    /// Return true iff versions are equal.
    SDR_API
    friend bool operator==(const SdrVersion& lhs, const SdrVersion& rhs)
    {
        return lhs._major == rhs._major && lhs._minor == rhs._minor;
    }

    /// Return true iff versions are not equal.
    SDR_API
    friend bool operator!=(const SdrVersion& lhs, const SdrVersion& rhs)
    {
        return !(lhs == rhs);
    }

    /// Return true iff the left side is less than the right side.
    SDR_API
    friend bool operator<(const SdrVersion& lhs, const SdrVersion& rhs)
    {
        return lhs._major < rhs._major ||
               (lhs._major == rhs._major && lhs._minor < rhs._minor);
    }

    /// Return true iff the left side is less than or equal to the right side.
    SDR_API
    friend bool operator<=(const SdrVersion& lhs, const SdrVersion& rhs)
    {
        return lhs._major < rhs._major ||
               (lhs._major == rhs._major && lhs._minor <= rhs._minor);
    }

    /// Return true iff the left side is greater than the right side.
    SDR_API
    friend bool operator>(const SdrVersion& lhs, const SdrVersion& rhs)
    {
        return !(lhs <= rhs);
    }

    /// Return true iff the left side is greater than or equal to the right side.
    SDR_API
    friend bool operator>=(const SdrVersion& lhs, const SdrVersion& rhs)
    {
        return !(lhs < rhs);
    }

private:
    SdrVersion(const SdrVersion& x, bool)
        : _major(x._major), _minor(x._minor), _isDefault(true) { }

private:
    int _major = 0, _minor = 0;
    bool _isDefault = false;
};

/// Enumeration used to select nodes by version.
enum SdrVersionFilter {
    SdrVersionFilterDefaultOnly,
    SdrVersionFilterAllVersions,
    SdrNumVersionFilters
};

/// Helper function to translate SdrVersionFilter values to NdrVersionFilter
/// values.
///
/// \deprecated
/// This function is deprecated and will be removed with the removal of
/// the Ndr library.
SDR_API
NdrVersionFilter SdrVersionFilterToNdr(SdrVersionFilter filter);

/// Helper function to translate SdrVersion to NdrVersion
///
/// \deprecated
/// This function is deprecated and will be removed with the removal of
/// the Ndr library.
SDR_API
NdrVersion SdrToNdrVersion(SdrVersion version);

/// Helper function to translate NdrVersion to SdrVersion
///
/// \deprecated
/// This function is deprecated and will be removed with the removal of
/// the Ndr library.
SDR_API
SdrVersion NdrToSdrVersion(NdrVersion version);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_DECLARE_H
