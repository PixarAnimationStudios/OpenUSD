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

#include "pxr/pxr.h"
#include "pxr/usd/ar/packageUtils.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{
// Returns iterator in \p path pointing to outermost ']' delimiter.
std::string::const_iterator
_FindOutermostClosingDelimiter(const std::string& path)
{
    if (path.empty() || path.back() != ']') {
        return path.end();
    }
    return path.end() - 1;
}

// Delete rvalue overload to avoid returning an iterator to a temporary.
std::string::const_iterator
_FindOutermostClosingDelimiter(std::string&& path) = delete;

// Returns iterator in \p path pointing to the innermost ']' delimiter.
std::string::const_iterator
_FindInnermostClosingDelimiter(const std::string& path)
{
    if (path.empty() || path.back() != ']') {
        return path.end();
    }

    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        if (*it == '\\') {
            // The previous ']' character was escaped, so the innermost
            // delimiter is really the one before that character.
            return (it - 1).base();
        }
        else if (*it != ']') {
            return it.base();
        }
    }

    return path.end();
}

// Delete rvalue overload to avoid returning an iterator to a temporary.
std::string::const_iterator
_FindInnermostClosingDelimiter(std::string&& path) = delete;

// Given iterator \p closingDelimIt in \p path pointing to a closing
// ']' character, returns iterator to the corresponding opening '['
// character, or path.end() if one can't be found.
std::string::const_iterator
_FindMatchingOpeningDelimiter(
    const std::string& path,
    std::string::const_iterator closingDelimIt)
{
    size_t numOpenNeeded = 1;
    std::string::const_reverse_iterator revIt(closingDelimIt);
    for (; revIt != path.rend() && numOpenNeeded != 0; ++revIt) {
        if (*revIt == '[' || *revIt == ']') {
            // Ignore this delimiter if it's been escaped.
            auto prevCharIt = revIt + 1;
            if (prevCharIt != path.rend() && *prevCharIt == '\\') {
                continue;
            }
            numOpenNeeded += (*revIt == '[') ? -1 : 1;
        }
    }

    return (numOpenNeeded == 0) ? revIt.base() : path.end();
}

// Escape delimiters in the given path to preserve them when placing
// placing the path into the packaged part of a package-relative path.
//
// If path is a package-relative path, we assume the packaged portion of
// that path has already been escaped and only process the package portion.
std::string
_EscapeDelimiters(const std::string& path)
{
    if (path.empty()) {
        return path;
    }

    auto escapeRangeBegin = path.begin(), escapeRangeEnd = path.end();
    if (path.back() == ']') {
        auto outermostOpenIt = 
            _FindMatchingOpeningDelimiter(path, path.end() - 1);
        if (outermostOpenIt != path.end()) {
            escapeRangeEnd = outermostOpenIt;
        }
    }

    std::string escapedString(escapeRangeBegin, escapeRangeEnd);
    escapedString = TfStringReplace(escapedString, "[", "\\[");
    escapedString = TfStringReplace(escapedString, "]", "\\]");
    return escapedString + std::string(escapeRangeEnd, path.end());
}

// Unescape delimiters in the given path to give clients the 'real' path
// when extracting paths from the packaged part of a package-relative path.
//
// If path is a package-relative path, we assume the packaged portion of
// that path has already been escaped and only process the package portion.
std::string
_UnescapeDelimiters(const std::string& path)
{
    if (path.empty()) {
        return path;
    }

    auto escapeRangeBegin = path.begin(), escapeRangeEnd = path.end();
    if (path.back() == ']') {
        auto outermostOpenIt = 
            _FindMatchingOpeningDelimiter(path, path.end() - 1);
        if (outermostOpenIt != path.end()) {
            escapeRangeEnd = outermostOpenIt;
        }
    }

    std::string escapedString(escapeRangeBegin, escapeRangeEnd);
    escapedString = TfStringReplace(escapedString, "\\[", "[");
    escapedString = TfStringReplace(escapedString, "\\]", "]");
    return escapedString + std::string(escapeRangeEnd, path.end());
}

} // end anonymous namespace

bool
ArIsPackageRelativePath(
    const std::string& path)
{
    return !path.empty() && path.back() == ']' && 
        _FindMatchingOpeningDelimiter(path, path.end() - 1) != path.end();
}

namespace
{

template <class Iter>
const std::string&
_Get(Iter it) 
{
    return *it;
}

const std::string&
_Get(const std::string* const* it)
{
    return **it;
}

template <class Iter>
std::string 
_JoinPackageRelativePath(Iter begin, Iter end)
{
    Iter pathIt = begin;
    for (; pathIt != end; ++pathIt) {
        if (!_Get(pathIt).empty()) {
            break;
        }
    }

    if (pathIt == end) {
        return std::string();
    }

    // Start the result packageRelativePath with the first non-empty path
    // in the range and determine where the next path should be inserted.
    // If this path is itself a package-relative path, this insert location
    // should come just before the innermost ']' delimiter.
    std::string packageRelativePath = _Get(pathIt++);
    size_t insertIdx = packageRelativePath.length();
    if (packageRelativePath.back() == ']') {
        auto innermostCloseRevIt = std::find_if(
            packageRelativePath.rbegin(), packageRelativePath.rend(),
            [](char c) { return c != ']'; });
        insertIdx = std::distance(
            packageRelativePath.begin(), innermostCloseRevIt.base());
    }

    // Loop through and insert the rest of the paths.
    for (; pathIt != end; ++pathIt) {
        if (_Get(pathIt).empty()) {
            continue;
        }

        // Since we're enclosing this path in delimiters, we need to escape
        // any existing delimiters.
        const std::string pathToInsert = 
            '[' + _EscapeDelimiters(_Get(pathIt)) + ']';
        packageRelativePath.insert(insertIdx, pathToInsert);
        insertIdx += pathToInsert.length() - 1;
    }

    return packageRelativePath;
}

} // end anonymous namespace

std::string
ArJoinPackageRelativePath(
    const std::vector<std::string>& paths)
{
    return _JoinPackageRelativePath(paths.begin(), paths.end());
}

std::string
ArJoinPackageRelativePath(
    const std::pair<std::string, std::string>& paths)
{
    const std::string* const arr[2] = { &paths.first, &paths.second };
    return _JoinPackageRelativePath(arr, arr + 2);
}

std::string
ArJoinPackageRelativePath(
    const std::string& packagePath, const std::string& packagedPath)
{
    const std::string* const arr[2] = { &packagePath, &packagedPath };
    return _JoinPackageRelativePath(arr, arr + 2);
}

std::pair<std::string, std::string>
ArSplitPackageRelativePathOuter(
    const std::string& path)
{
    // For example, given a path like "/dir/foo.package[bar.package[baz.file]]",
    // find the range [outermostOpenIt, outermostCloseIt] containing 
    // "[bar.package[baz.file]]"
    auto outermostCloseIt = _FindOutermostClosingDelimiter(path);
    if (outermostCloseIt == path.end()) {
        return std::make_pair(path, std::string());
    }
    auto outermostOpenIt = _FindMatchingOpeningDelimiter(path, outermostCloseIt);
    if (outermostOpenIt == path.end()) {
        return std::make_pair(path, std::string());
    }

    // The package path is everything before the outermost opening delimiter.
    std::string packagePath(path.begin(), outermostOpenIt);

    // Drop the opening and closing delimiters to create the packaged path,
    // making sure to unescape delimiters now that this path has been split.
    std::string packagedPath(outermostOpenIt + 1, outermostCloseIt);
    packagedPath = _UnescapeDelimiters(packagedPath);

    return std::make_pair(std::move(packagePath), std::move(packagedPath));
}

std::pair<std::string, std::string>
ArSplitPackageRelativePathInner(
    const std::string& path)
{
    // For example, given a path like "/dir/foo.package[bar.package[baz.file]]",
    // find the range [innermostOpenIt, innermostCloseIt] containing 
    // "[baz.file]"
    auto innermostCloseIt = _FindInnermostClosingDelimiter(path);
    if (innermostCloseIt == path.end()) {
        return std::make_pair(path, std::string());
    }
    auto innermostOpenIt = _FindMatchingOpeningDelimiter(path, innermostCloseIt);
    if (innermostOpenIt == path.end()) {
        return std::make_pair(path, std::string());
    }        

    // Erase "[baz.file]" from the given path to generate the package path.
    std::string packagePath = path;
    packagePath.erase(
        std::distance(path.begin(), innermostOpenIt),
        std::distance(innermostOpenIt, innermostCloseIt) + 1);

    // Drop the opening and closing delimiters to create the packaged path,
    // making sure to unescape delimiters now that this path has been split.
    std::string packagedPath(innermostOpenIt + 1, innermostCloseIt);
    packagedPath = _UnescapeDelimiters(packagedPath);

    return std::make_pair(std::move(packagePath), std::move(packagedPath));
}

PXR_NAMESPACE_CLOSE_SCOPE
