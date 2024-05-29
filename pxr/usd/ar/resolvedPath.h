//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_AR_RESOLVED_PATH_H
#define PXR_USD_AR_RESOLVED_PATH_H

/// \file ar/resolvedPath.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/base/tf/hash.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class ArResolvedPath
/// Represents a resolved asset path.
class ArResolvedPath
{
public:
    /// Construct an ArResolvedPath holding the given \p resolvedPath.
    explicit ArResolvedPath(const std::string& resolvedPath)
        : _resolvedPath(resolvedPath)
    {
    }
    
    /// \overload
    explicit ArResolvedPath(std::string&& resolvedPath)
        : _resolvedPath(std::move(resolvedPath))
    {
    }

    ArResolvedPath() = default;

    ArResolvedPath(const ArResolvedPath& rhs) = default;
    ArResolvedPath(ArResolvedPath&& rhs) = default;
    
    ArResolvedPath& operator=(const ArResolvedPath& rhs) = default;
    ArResolvedPath& operator=(ArResolvedPath&& rhs) = default;

    bool operator==(const ArResolvedPath& rhs) const
    { return _resolvedPath == rhs._resolvedPath; }

    bool operator!=(const ArResolvedPath& rhs) const
    { return _resolvedPath != rhs._resolvedPath; }

    bool operator<(const ArResolvedPath& rhs) const
    { return _resolvedPath < rhs._resolvedPath; }

    bool operator>(const ArResolvedPath& rhs) const
    { return _resolvedPath > rhs._resolvedPath; }

    bool operator<=(const ArResolvedPath& rhs) const
    { return _resolvedPath <= rhs._resolvedPath; }

    bool operator>=(const ArResolvedPath& rhs) const
    { return _resolvedPath >= rhs._resolvedPath; }

    bool operator==(const std::string& rhs) const
    { return _resolvedPath == rhs; }

    bool operator!=(const std::string& rhs) const
    { return _resolvedPath != rhs; }

    bool operator<(const std::string& rhs) const
    { return _resolvedPath < rhs; }

    bool operator>(const std::string& rhs) const
    { return _resolvedPath > rhs; }

    bool operator<=(const std::string& rhs) const
    { return _resolvedPath <= rhs; }

    bool operator>=(const std::string& rhs) const
    { return _resolvedPath >= rhs; }

    /// Return hash value for this object.
    size_t GetHash() const { return TfHash()(*this); }

    /// Return true if this object is holding a non-empty resolved path,
    /// false otherwise.
    explicit operator bool() const { return !IsEmpty(); }

    /// Return true if this object is holding an empty resolved path,
    /// false otherwise.
    bool IsEmpty() const { return _resolvedPath.empty(); }

    /// Equivalent to IsEmpty. This exists primarily for backwards
    /// compatibility.
    bool empty() const { return IsEmpty(); }

    /// Return the resolved path held by this object as a string.
    operator const std::string&() const { return GetPathString(); }

    /// Return the resolved path held by this object as a string.
    const std::string& GetPathString() const { return _resolvedPath; }

private:
    std::string _resolvedPath;
};

template <class HashState>
void
TfHashAppend(HashState& h, const ArResolvedPath& p)
{
    h.Append(p.GetPathString());
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif 
