//
// Copyright 2018 Pixar
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

#ifndef PXR_USD_NDR_DECLARE_H
#define PXR_USD_NDR_DECLARE_H

/// \file ndr/declare.h

#include "pxr/pxr.h"
#include "pxr/usd/ndr/api.h"
#include "pxr/base/tf/token.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class NdrNode;
class NdrProperty;
class SdfValueTypeName;

/// \file declare.h
///
/// Common typedefs that are used throughout the NDR library.

typedef TfToken NdrIdentifier;
typedef TfToken::HashFunctor NdrIdentifierHashFunctor;
inline const std::string&
NdrGetIdentifierString(const NdrIdentifier& id) { return id.GetString(); }
typedef std::vector<NdrIdentifier> NdrIdentifierVec;
typedef std::unordered_set<NdrIdentifier,
                           NdrIdentifierHashFunctor> NdrIdentifierSet;

// Token
typedef std::vector<TfToken> NdrTokenVec;
typedef std::unordered_map<TfToken, std::string,
                           TfToken::HashFunctor> NdrTokenMap;

// Property
typedef NdrProperty* NdrPropertyPtr;
typedef NdrProperty const* NdrPropertyConstPtr;
typedef std::unique_ptr<NdrProperty> NdrPropertyUniquePtr;
typedef std::vector<NdrPropertyUniquePtr> NdrPropertyUniquePtrVec;
typedef std::unordered_map<TfToken, NdrPropertyConstPtr,
                           TfToken::HashFunctor> NdrPropertyPtrMap;

// Node
typedef NdrNode* NdrNodePtr;
typedef NdrNode const* NdrNodeConstPtr;
typedef std::unique_ptr<NdrNode> NdrNodeUniquePtr;
typedef std::vector<NdrNodeConstPtr> NdrNodeConstPtrVec;
typedef std::vector<NdrNodeUniquePtr> NdrNodeUniquePtrVec;

// Misc
typedef std::vector<std::string> NdrStringVec;
typedef std::pair<TfToken, TfToken> NdrOption;
typedef std::vector<NdrOption> NdrOptionVec;
typedef std::unordered_set<std::string> NdrStringSet;
typedef std::pair<SdfValueTypeName, TfToken> SdfTypeIndicator;

// Version
class NdrVersion {
public:
    /// Create an invalid version.
    NDR_API
    NdrVersion() = default;
    /// Create a version with the given major and minor numbers.
    /// Numbers must be non-negative, and at least one must be non-zero.  
    /// On failure generates an error and yields an invalid version.
    NDR_API
    NdrVersion(int major, int minor = 0);
    /// Create a version from a string.  On failure generates an error and
    /// yields an invalid version.
    NDR_API
    NdrVersion(const std::string& x);

    /// Return an equal version marked as default.  It's permitted to mark
    /// an invalid version as the default.
    NDR_API
    NdrVersion GetAsDefault() const
    {
        return NdrVersion(*this, true);
    }

    /// Return the major version number or zero for an invalid version.
    NDR_API
    int GetMajor() const { return _major; }
    /// Return the minor version number or zero for an invalid version.
    NDR_API
    int GetMinor() const { return _minor; }
    /// Return true iff this version is marked as default.
    NDR_API
    bool IsDefault() const { return _isDefault; }

    /// Return the version as a string.
    NDR_API
    std::string GetString() const;

    /// Return the version as a identifier suffix.
    NDR_API
    std::string GetStringSuffix() const;

    /// Return a hash for the version.
    NDR_API
    std::size_t GetHash() const
    {
        return (static_cast<std::size_t>(_major) << 32) +
                static_cast<std::size_t>(_minor);
    }

    /// Return true iff the version is valid.
    NDR_API
    explicit operator bool() const
    {
        return !!*this;
    }

    /// Return true iff the version is invalid.
    NDR_API
    bool operator!() const
    {
        return _major == 0 && _minor == 0;
    }

    /// Return true iff versions are equal.
    NDR_API
    friend bool operator==(const NdrVersion& lhs, const NdrVersion& rhs)
    {
        return lhs._major == rhs._major && lhs._minor == rhs._minor;
    }

    /// Return true iff versions are not equal.
    NDR_API
    friend bool operator!=(const NdrVersion& lhs, const NdrVersion& rhs)
    {
        return !(lhs == rhs);
    }

    /// Return true iff the left side is less than the right side.
    NDR_API
    friend bool operator<(const NdrVersion& lhs, const NdrVersion& rhs)
    {
        return lhs._major < rhs._major ||
               (lhs._major == rhs._major && lhs._minor < rhs._minor);
    }

    /// Return true iff the left side is less than or equal to the right side.
    NDR_API
    friend bool operator<=(const NdrVersion& lhs, const NdrVersion& rhs)
    {
        return lhs._major < rhs._major ||
               (lhs._major == rhs._major && lhs._minor <= rhs._minor);
    }

    /// Return true iff the left side is greater than the right side.
    NDR_API
    friend bool operator>(const NdrVersion& lhs, const NdrVersion& rhs)
    {
        return !(lhs <= rhs);
    }

    /// Return true iff the left side is greater than or equal to the right side.
    NDR_API
    friend bool operator>=(const NdrVersion& lhs, const NdrVersion& rhs)
    {
        return !(lhs < rhs);
    }

private:
    NdrVersion(const NdrVersion& x, bool)
        : _major(x._major), _minor(x._minor), _isDefault(true) { }

private:
    int _major = 0, _minor = 0;
    bool _isDefault = false;
};

/// Enumeration used to select nodes by version.
enum NdrVersionFilter {
    NdrVersionFilterDefaultOnly,
    NdrVersionFilterAllVersions,
    NdrNumVersionFilters
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_NDR_DECLARE_H
