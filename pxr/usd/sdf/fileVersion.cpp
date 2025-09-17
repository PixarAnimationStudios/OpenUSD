//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/usd/sdf/fileVersion.h"

#include "pxr/base/tf/stringUtils.h"

#include <limits>
#include <cctype>
#include <cinttypes>
#include <charconv>

PXR_NAMESPACE_OPEN_SCOPE

// Read a version from a dotted decimal integer string, e.g. "1.2.3".
// or "1.0". The string must have at least major and minor version
// numbers, all numbers must be separated by '.' characters, and the
// after the last number there must be white space or end-of-string.

//static
SdfFileVersion SdfFileVersion::FromString(char const *str)
{
    uint8_t maj{0}, min{0}, pat{0};
    bool hasMin = false, hasPat = false;
    bool reqPat = false;

    const char* beg = str;
    const char* end = str + strlen(str);

    // std::from_chars has some nice advantages over strtoul or sscanf. In
    // particular it does not allow leading spaces and is fully type-safe,
    // converting text to a uint8_t just works.
    std::from_chars_result result;
    
    result = std::from_chars(beg, end, maj, 10);
    if (result.ec == std::errc() && *result.ptr == '.') {
        beg = result.ptr + 1;                // advance past the '.'

        result = std::from_chars(beg, end, min, 10);
        hasMin = result.ec == std::errc();
        if (hasMin && *result.ptr == '.')
        {
            beg = result.ptr + 1;

            // A patch number is required because of the trailing '.'
            reqPat = true;
            result = std::from_chars(beg, end, pat, 10);
            hasPat = result.ec == std::errc();
        }
    }

    // If there is a character after the last number, it must be white space.
    // Note: strchr returns a pointer to the matched character, including the
    // trailing null character if (*result.ptr == '\0'). If the character is not
    // found, it returns a nullptr. So it's perfect for looking for white space
    // or a NULL character.
    if (!strchr(" \t\n", *result.ptr)) {
        // we did not consume the whole string. Fail
        return SdfFileVersion();
    } else if (!hasMin || (reqPat && !hasPat)) {
        // Either missing a minor version number or missing a patch number
        // when there was a '.' after the minor version number.
        return SdfFileVersion();
    }

    return SdfFileVersion(maj, min, pat);
}

/// Return a dotted decimal integer string for this version, the patch
/// version is elided if it is 0, e.g. "1.0" or "1.2.3".
std::string SdfFileVersion::AsString() const
{
    if (patchver == 0) {
        return TfStringPrintf(
            "%" PRId8 ".%" PRId8, majver, minver);
    } else {
        return TfStringPrintf(
            "%" PRId8 ".%" PRId8 ".%" PRId8, majver, minver, patchver);
    }
}

/// Return a dotted decimal integer string for this version, the patch
/// version is excluded if it is 0, e.g. "1.0.0" or "1.2.3".
std::string SdfFileVersion::AsFullString() const
{
    return TfStringPrintf(
        "%" PRId8 ".%" PRId8 ".%" PRId8, majver, minver, patchver);
}


PXR_NAMESPACE_CLOSE_SCOPE
