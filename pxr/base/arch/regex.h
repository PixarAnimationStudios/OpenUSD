//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_REGEX_H
#define PXR_BASE_ARCH_REGEX_H

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class ArchRegex {
public:
    enum : unsigned int {
        CASE_INSENSITIVE = 1u,
        GLOB             = 2u
    };

    /// Create an empty regex.
    ArchRegex() = default;

    ArchRegex(ArchRegex &&) noexcept = default;
    ArchRegex(ArchRegex const &) = default;
    ArchRegex &operator=(ArchRegex &&) noexcept = default;
    ArchRegex &operator=(ArchRegex const &) = default;

    /// Create with pattern and optional flags.
    ARCH_API ArchRegex(const std::string& pattern, unsigned int flags = 0);

    /// Destructor.
    ARCH_API ~ArchRegex();

    /// Returns \c true if the regex is valid.
    ARCH_API explicit operator bool() const;

    /// Returns the reason the regex is invalid or the empty string if
    /// it's valid.
    ARCH_API std::string GetError() const;

    /// Returns the flags used to construct the regex.
    ARCH_API unsigned int GetFlags() const;

    /// Returns \c true if the regex matches \p query anywhere, otherwise
    /// returns \c false.
    ARCH_API bool Match(const std::string& query) const;

private:
    class _Impl;
    unsigned int _flags = 0;
    std::string _error;
    std::shared_ptr<const _Impl> _impl;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_REGEX_H
