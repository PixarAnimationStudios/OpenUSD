//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_FILE_VERSION_H
#define PXR_USD_SDF_FILE_VERSION_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include <cstdint>
#include <cstdlib>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

// Hold parse, and compare file format versions. Used by both crate and
// text file formats.
class SdfFileVersion {
public:
    // Not named 'major' since that's a macro name conflict on POSIXes.
    uint8_t majver, minver, patchver;
        
    constexpr SdfFileVersion()
    : SdfFileVersion(0,0,0) {}
    
    constexpr SdfFileVersion(uint8_t majver, uint8_t minver, uint8_t patchver)
    : majver(majver), minver(minver), patchver(patchver) {}

    // For constructing from crate files header data. version is required to
    // contain at least 3 values for the major, minor, and patch.
    explicit SdfFileVersion(const uint8_t version[])
    : SdfFileVersion(version[0], version[1], version[2]) {}

    /// Create a version from a dot-separated c-style string, e.g. "1.2.3".
    SDF_API
    static SdfFileVersion FromString(char const *str);

    /// Create a version from a dot-separated std::string.
    static SdfFileVersion FromString(std::string str) {
        return FromString(str.c_str());
    }

    /// Return a version number as a single 32-bit integer.  From most to
    /// least significant, the returned integer's bytes are 0,
    /// major-version, minor-version, patch-version.
    constexpr uint32_t AsInt() const {
        return static_cast<uint32_t>(majver) << 16 |
            static_cast<uint32_t>(minver) << 8 |
            static_cast<uint32_t>(patchver);
    }

    /// Return a dotted decimal integer string for this version, the
    /// patch version is excluded if it is 0, e.g. "1.0" or "1.2.3".
    SDF_API    
    std::string AsString() const;

    /// Return a dotted decimal integer string for this version, the
    /// patch version is excluded if it is 0, e.g. "1.0.0" or "1.2.3".
    SDF_API
    std::string AsFullString() const;

    /// Return true if this version is not zero in all components.
    bool IsValid() const { return AsInt() != 0; }

    explicit operator bool() const {
        return IsValid();
    }

    // Return true if fileVer has the same major version as this, and has a
    // lesser or same minor version.  Patch version irrelevant, since the
    // versioning scheme specifies that patch level changes are
    // forward-compatible.
    bool CanRead(SdfFileVersion const &fileVer) const {
        return fileVer.majver == majver && fileVer.minver <= minver;
    }

    // Return true if fileVer has the same major version as this, and has a
    // lesser minor version, or has the same minor version and a lesser or
    // equal patch version.
    bool CanWrite(SdfFileVersion const &fileVer) const {
        return fileVer.majver == majver &&
            (fileVer.minver < minver ||
             (fileVer.minver == minver && fileVer.patchver <= patchver));
    }
    
#define LOGIC_OP(op)                                                    \
    constexpr bool operator op(SdfFileVersion const &other) const {     \
        return AsInt() op other.AsInt();                                \
    }
    LOGIC_OP(==); LOGIC_OP(!=);
    LOGIC_OP(<);  LOGIC_OP(>);
    LOGIC_OP(<=); LOGIC_OP(>=);
#undef LOGIC_OP
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_FILE_VERSION_H
