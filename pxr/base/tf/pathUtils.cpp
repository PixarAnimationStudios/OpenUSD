//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/errno.h"

#include <algorithm>
#include <cctype>
#include <errno.h>
#include <limits.h>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

#if defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <Shlwapi.h>
#else
#include <glob.h>
#endif

using std::pair;
using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

#if defined(ARCH_OS_WINDOWS)
// Expands symlinks in path.  Used on Windows as a partial replacement
// for realpath(), partial because is doesn't handle /./, /../ and
// duplicate slashes.
std::string
_ExpandSymlinks(const std::string& path)
{
    // Find the first directory in path that's a symbolic link, if any,
    // and the remaining part of the path.
    std::string::size_type i = path.find_first_of("/\\");
    while (i != std::string::npos) {
        std::string prefix = path.substr(0, i);
        // If the prefix is "X:", this will access the "current" directory on
        // drive X, when what we really want is the root of drive X, so append
        // a backslash. Also check that i>0. An i==0 value can happen if the
        // passed in path is a non-canonical Windows path such as "/tmp/foo".
        if (i > 0 && prefix.at(i-1) == ':') {
            prefix.push_back('\\');
        }
        if (TfIsLink(prefix)) {
            // Expand the link and repeat with the new path if the path changed.
            // The path may remain unchanged or be empty if the link type is
            // unsupported or the mount destination is not available.
            auto newPrefix = TfReadLink(prefix);
            if (!newPrefix.empty() && newPrefix != prefix)
                return _ExpandSymlinks(newPrefix + path.substr(i));
        }
        i = path.find_first_of("/\\", i + 1);
    }

    // No ancestral symlinks.
    if (TfIsLink(path)) {
        return _ExpandSymlinks(TfReadLink(path));
    }

    // No links at all.
    return path;
}
#endif

void
_ClearError()
{
#if defined(ARCH_OS_WINDOWS)
    SetLastError(ERROR_SUCCESS);
#else
    errno = 0;
#endif
}

void
_GetError(std::string* err)
{
    if (err->empty()) {
#if defined(ARCH_OS_WINDOWS)
        *err = ArchStrSysError(GetLastError());
#else
        *err = errno ? ArchStrerror() : std::string();
#endif
    }
}

} // anonymous namespace

string
TfRealPath(string const& path, bool allowInaccessibleSuffix, string* error)
{
    string localError;
    if (!error)
        error = &localError;
    else
        error->clear();

    if (path.empty())
        return string();

    string suffix, prefix = path;

    if (allowInaccessibleSuffix) {
        string::size_type split = TfFindLongestAccessiblePrefix(path, error);
        if (!error->empty())
            return string();

        prefix = string(path, 0, split);
        suffix = string(path, split);
    }

    if (prefix.empty()) {
        return TfAbsPath(suffix);
    }

#if defined(ARCH_OS_WINDOWS)
    // Expand all symbolic links.
    if (!TfPathExists(prefix)) {
        *error = "the named file does not exist";
        return string();
    }
    std::string resolved = _ExpandSymlinks(prefix);

    return TfAbsPath(resolved + suffix);
#else
    char resolved[ARCH_PATH_MAX];
    if (!realpath(prefix.c_str(), resolved)) {
        *error = ArchStrerror(errno);
        return string();
    }
    return TfAbsPath(resolved + suffix);
#endif
}

string::size_type
TfFindLongestAccessiblePrefix(string const &path, string* error)
{
    typedef string::size_type size_type;
    static const size_type npos = string::npos;

    struct _Local {
        // Sentinel is greater than existing paths, less than non-existing ones.
        static bool Compare(
            string const &str, size_type lhs, size_type rhs, string* err) {
            if (lhs == rhs)
                return false;
            if (lhs == npos)
                return !Accessible(str, rhs, err);
            if (rhs == npos)
                return Accessible(str, lhs, err);
            return lhs < rhs;
        }

        static bool Accessible(string const &str, size_type index, string *err) {
            string checkPath(str, 0, index);

            // False if non-existent or if a symlink and the target is
            // non-existent.  Also false on any error.
            _ClearError();
            if (!TfPathExists(checkPath)) {
                _GetError(err);
                return false;
            }
            if (TfIsLink(checkPath) &&
                !TfPathExists(checkPath, /* resolveSymlinks = */ true)) {
                _GetError(err);
                if (err->empty()) {
                    *err = "encountered dangling symbolic link";
                }
            }
            else {
                _GetError(err);
            }
            return err->empty();
        }
    };

    // Build a vector of split point indexes.
    vector<size_type> splitPoints;
#if defined(ARCH_OS_WINDOWS)
    for (size_type p = path.find_first_of("/\\", path.find_first_not_of("/\\"));
         p != npos; p = path.find_first_of("/\\", p+1))
#else
    for (size_type p = path.find('/', path.find_first_not_of('/'));
         p != npos; p = path.find('/', p+1))
#endif
        splitPoints.push_back(p);
    splitPoints.push_back(path.size());

    // Lower-bound to find first non-existent path.
    vector<size_type>::iterator result =
        std::lower_bound(splitPoints.begin(), splitPoints.end(), npos,
                         std::bind(_Local::Compare, path,
                                   std::placeholders::_1,
                                   std::placeholders::_2, error));

    // begin means nothing existed, end means everything did, else prior is last
    // existing path.
    if (result == splitPoints.begin())
        return 0;
    if (result == splitPoints.end())
        return path.length();
    return *(result - 1);
}

string
TfNormPath(string const &inPath, bool stripDriveSpecifier)
{
    return ArchNormPath(inPath, stripDriveSpecifier);
}

string
TfAbsPath(string const& path)
{
    return ArchAbsPath(path);
}

string
TfGetExtension(string const& path)
{
    static const string emptyPath;

    if (path.empty()) {
        return emptyPath;
    }

    const std::string fileName = TfGetBaseName(path);

    // If this is a dot file with no extension (e.g. /some/path/.folder), then
    // we return an empty string.
    if (TfStringGetBeforeSuffix(fileName).empty()) {
        return emptyPath;
    }

    return TfStringGetSuffix(fileName);
}

string
TfReadLink(string const& path)
{
    return ArchReadLink(path.c_str());
}

bool TfIsRelativePath(std::string const& path)
{
#if defined(ARCH_OS_WINDOWS)
    return path.empty() ||
        (PathIsRelativeW(ArchWindowsUtf8ToUtf16(path).c_str()) &&
         path[0] != '/' && path[0] != '\\');
#else
    return path.empty() || path[0] != '/';
#endif
}

#if !defined(ARCH_OS_WINDOWS)
vector<string>
TfGlob(vector<string> const& paths, unsigned int flags)
{
    if (paths.empty()) {
        return vector<string>();
    }

    // Ensure GLOB_APPEND is not set yet
    flags = flags & ~GLOB_APPEND;

    glob_t globbuf;
    glob(paths.at(0).c_str(), flags, NULL, &globbuf);

    for (size_t i = 1; i < paths.size(); i++) {
        glob(paths.at(i).c_str(), flags | GLOB_APPEND, NULL, &globbuf);
    }

    vector<string> results;
    for (size_t i = 0; i < globbuf.gl_pathc; i++) {
        if (globbuf.gl_pathv[i] != NULL) {
            results.push_back(globbuf.gl_pathv[i]);
        }
    }

    globfree(&globbuf);

    return results;
}

#else

namespace {

static
void
Tf_Glob(
    vector<string>* result,
    const std::string& prefix,
    const std::string& pattern,
    unsigned int flags)
{
    // Search for the first wildcard in pattern.
    const string::size_type i = pattern.find_first_of("*?");

    if (i == string::npos) {
        // No more patterns so we simply need to see if the file exists.
        // Conveniently GetFileAttributes() works on paths with a trailing
        // backslash.
        string path = prefix + pattern;
            const DWORD attributes =
                GetFileAttributesW(ArchWindowsUtf8ToUtf16(path).c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES) {
            // File exists.

            // Append directory mark if necessary.
            if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
                if ((flags & ARCH_GLOB_MARK) && path.back() != '\\') {
                    path.push_back('\\');
                }
            }

            result->push_back(path);
        }
    }
    else {
        // There are additional patterns to glob.  Find the next directory
        // after the wildcard.
        string::size_type j = pattern.find_first_of('\\', i);
        if (j == string::npos) {
            // We've bottomed out on the pattern.
            j = pattern.size();
        }

        // Construct the remaining pattern, if any.
        const string remainingPattern = pattern.substr(j);

        // Construct the leftmost pattern.
        const string leftmostPattern = prefix + pattern.substr(0, j);

        // Construct the leftmost pattern's directory. 
        const string leftmostDir = TfGetPathName(leftmostPattern);

        // Glob the leftmost pattern.
        WIN32_FIND_DATAW data;
            HANDLE find = FindFirstFileW(
                ArchWindowsUtf8ToUtf16(leftmostPattern).c_str(), &data);
        if (find != INVALID_HANDLE_VALUE) {
            do {
                // Recurse with next pattern.
                Tf_Glob(result,
                        leftmostDir + ArchWindowsUtf16ToUtf8(data.cFileName),
                        remainingPattern, flags);
            } while (FindNextFileW(find, &data));
            FindClose(find);
        }
    }
}

}

vector<string>
TfGlob(vector<string> const& paths, unsigned int flags)
{
    vector<string> result;

    for (auto path: paths) {
        const size_t n = result.size();

        // Convert slashes to backslashes for Windows.
        path = TfStringReplace(path, "/", "\\");

        // Do the real work.
        Tf_Glob(&result, "", path, flags);

        // If no match and NOCHECK then append the input.
        if ((flags & ARCH_GLOB_NOCHECK) && n == result.size()) {
            result.push_back(path);
        }
    }

    if ((flags & ARCH_GLOB_NOSORT) == 0) {
        std::sort(result.begin(), result.end());
    }

    // Convert to forward slashes.
    for (auto& path: result) {
        path = TfStringReplace(path, "\\", "/");
    }

    return result;
}

#endif

vector<string>
TfGlob(string const& path, unsigned int flags)
{
    return path.empty()
        ? vector<string>()
        : TfGlob(vector<string>(1, path), flags);
}

PXR_NAMESPACE_CLOSE_SCOPE
