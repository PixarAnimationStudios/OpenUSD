//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/assetPath.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

// Register this class with the TfType registry
// Array registration included to facilitate Sdf/Types and Sdf/ParserHelpers
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdfAssetPath>();
    TfType::Define<VtArray<SdfAssetPath>>();
}

TF_REGISTRY_FUNCTION(VtValue)
{
    VtValue::RegisterSimpleCast<std::string, SdfAssetPath>();
}

static const char Delimiter = '@';

// Read a UTF-8 char starting at 'cp' and return its value as an int.  Also
// advance 'cp' to the start of the next UTF-8 character.  If 'cp' does not
// point to a valid UTF-8 char, leave 'cp' unmodified and return -1.
static int
_ReadUTF8(unsigned char const *&cp, std::string *errMsg)
{
    // Return a byte with the high `n` bits set, rest clear.
    auto highBits = [](int n) {
        return static_cast<unsigned char>(((1 << n) - 1) << (8 - n));
    }; 
    
    // Return true if `ch` is a continuation byte.
    auto isContinuation = [&highBits](unsigned char ch) {
        return (ch & highBits(2)) == highBits(1);
    };
    
    // Check for single-character code.
    if ((*cp & highBits(1)) == 0) {
        return *cp++;
    }

    // Check for 2, 3, or 4-byte code.
    for (int i = 2; i <= 4; ++i) {
        // This is an N-byte code if the high-order N+1 bits are N 1s
        // followed by a single 0.
        if ((*cp & highBits(i + 1)) == highBits(i)) {
            int ret = *cp & ~highBits(i + 1);
            // If that's the case then the following N-1 bytes must be
            // "continuation bytes".
            for (int j = 1; j != i; ++j) {
                if (!isContinuation(cp[j])) {
                    char const *ordinalWords[] = {
                        "first", "second", "third", "fourth"
                    };
                    *errMsg = TfStringPrintf("%d-byte UTF-8 code point lacks "
                                             "%s continuation byte",
                                             i, ordinalWords[j-1]);
                    return -1;
                }
                ret = (ret << 6) | (cp[j] & ~highBits(2));
            }
            cp += i;
            return ret;
        }
    }
    *errMsg = TfStringPrintf("invalid UTF-8 code point byte 0x%hhx", *cp);
    return -1;
}

// Check that the given \p path is valid UTF-8 sans C0 & C1 control characters.
// Return true if so.  Return false and issue an error indicating the problem if
// not.
static bool
_ValidateAssetPathString(char const *path)
{
    // Return true if 'code' is a C0 or C1 control code, false otherwise.
    auto isControlCode = [](int code) {
        return ((0x0 <= code && code <= 0x1f) ||
                code == 0x7f ||
                (0x80 <= code && code <= 0x9f));
    };

    unsigned char const *cp = reinterpret_cast<unsigned char const*>(path);
    std::string err;
    int utf8Char = _ReadUTF8(cp, &err);
    int charNum = 1;
    for (; utf8Char > 0; utf8Char = _ReadUTF8(cp, &err)) {
        if (isControlCode(utf8Char)) {
            TF_CODING_ERROR("Invalid asset path string -- character %d is "
                            "control character 0x%x",
                            charNum, utf8Char);
            return false;
        }
        ++charNum;
    }
    if (utf8Char == -1) {
        TF_CODING_ERROR("Invalid asset path string -- character %d: %s\n",
                        charNum, err.c_str());
        return false;
    }
    return true;
}

SdfAssetPath::SdfAssetPath()
{
}

SdfAssetPath::SdfAssetPath(const std::string &path)
    : _assetPath(path)
{
    if (!_ValidateAssetPathString(path.c_str())) {
        *this = SdfAssetPath();
    }
}

SdfAssetPath::SdfAssetPath(const std::string &path,
                           const std::string &resolvedPath)
    : _assetPath(path)
    , _resolvedPath(resolvedPath)
{
    if (!_ValidateAssetPathString(path.c_str()) ||
        !_ValidateAssetPathString(resolvedPath.c_str())) {
        *this = SdfAssetPath();
    }
}

bool
SdfAssetPath::operator<(const SdfAssetPath &rhs) const
{
    if (_assetPath < rhs._assetPath)
        return true;
    if (rhs._assetPath < _assetPath)
        return false;

    return _resolvedPath < rhs._resolvedPath;
}

std::ostream& 
operator<<(std::ostream& out, const SdfAssetPath& ap)
{
    return out << Delimiter << ap.GetAssetPath() << Delimiter;
}

PXR_NAMESPACE_CLOSE_SCOPE
