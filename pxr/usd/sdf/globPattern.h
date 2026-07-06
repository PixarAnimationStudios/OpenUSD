//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_GLOB_PATTERN_H
#define PXR_USD_SDF_GLOB_PATTERN_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

PXR_NAMESPACE_OPEN_SCOPE

// A compiled glob pattern matcher optimized for matching short UTF-8 strings
// (e.g. path component names).  Supports *, ?, [abc], [a-z], [!a-z], and
// backslash escapes.  The pattern is compiled into a flat bytecoded
// representation for fast matching.
//
// UTF-8 contract: both the pattern passed to Compile() and the string passed to
// Match() must be valid UTF-8.  This is NOT checked -- malformed input produces
// undefined behavior (out-of-bounds reads, false positives/negatives, etc.).
// Callers are responsible for validating input upstream.
//
// Size limits: the compiled pattern is held in a buffer of at most 64KB, so
// pattern strings producing more than ~64KB of bytecode will not compile
// correctly.  In practice patterns are far shorter than that.
class Sdf_GlobPattern {
public:
    Sdf_GlobPattern() = default;

    SDF_API
    Sdf_GlobPattern(Sdf_GlobPattern const &);
    SDF_API
    Sdf_GlobPattern &operator=(Sdf_GlobPattern const &);

    Sdf_GlobPattern(Sdf_GlobPattern &&) noexcept = default;
    Sdf_GlobPattern &operator=(Sdf_GlobPattern &&) noexcept = default;

    // Compile a glob pattern.  The pattern must be valid UTF-8 (not checked).
    // Returns an invalid pattern on parse error.
    SDF_API
    static Sdf_GlobPattern Compile(const char *pattern, size_t patLen);
    static Sdf_GlobPattern Compile(std::string_view pattern) {
        return Compile(pattern.data(), pattern.size());
    }

    // Match a UTF-8 string against the compiled pattern.  The string must be
    // valid UTF-8 (not checked).
    SDF_API
    bool Match(const char *str, size_t len) const;
    bool Match(std::string_view str) const {
        return Match(str.data(), str.size());
    }

    // True if the pattern compiled successfully.
    explicit operator bool() const {
        return _flags & _Compiled;
    }

private:
    // Sentinel flag bit set by Compile() on success.  Default-constructed
    // (flags == 0) patterns are therefore invalid until populated.
    static constexpr uint16_t _Compiled = 1u << 15;

    std::unique_ptr<uint8_t[]> _buf;
    uint16_t _bufSize = 0;
    uint16_t _minBytes = 0;
    uint16_t _flags = 0;
    uint16_t _suffixCodePointCount = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_GLOB_PATTERN_H
