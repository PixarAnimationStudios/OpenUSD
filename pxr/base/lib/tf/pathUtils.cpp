//
// Copyright 2016 Pixar
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
        const std::string prefix = path.substr(0, i);
        if (TfIsLink(prefix)) {
            // Expand the link and repeat with the new path.
            return _ExpandSymlinks(TfReadLink(prefix) + path.substr(i));
        }
        else {
            i = path.find_first_of("/\\", i + 1);
        }
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

    // Make sure drive letters are always lower-case out of TfRealPath on
    // Windows -- this is so that we can be sure we can reliably use the
    // paths as keys in tables, etc.
    if (resolved[0] && resolved[1] == ':') {
        resolved[0] = tolower(resolved[0]);
    }
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

namespace { // Helpers for TfNormPath.

enum TokenType { Dot, DotDot, Elem };

// Helper to get a const_iterator begin() -- to do this in code where you have a
// non-const lvalue string you have to do something like: const_iterator i =
// const_cast<const string &>(myStr).begin(); which is kind of ugly.  C++11 adds
// cbegin()/cend() members, but we don thave that yet.
inline std::string::const_iterator
cbegin(std::string const &str) { return str.begin(); }

typedef pair<string::const_iterator, string::const_iterator> Token;
typedef pair<string::reverse_iterator, string::reverse_iterator> RToken;

template <class Iter>
inline pair<Iter, Iter>
_NextToken(Iter i, Iter end)
{
    pair<Iter, Iter> t;
    for (t.first = i;
         t.first != end && *t.first == '/'; ++t.first) {}
    for (t.second = t.first;
         t.second != end && *t.second != '/'; ++t.second) {}
    return t;
}

template <class Iter>
inline TokenType
_GetTokenType(pair<Iter, Iter> t) {
    size_t len = distance(t.first, t.second);
    if (len == 1 && t.first[0] == '.')
        return Dot;
    if (len == 2 && t.first[0] == '.' && t.first[1] == '.')
        return DotDot;
    return Elem;
}

string
_NormPath(string const &inPath)
{
    // We take one pass through the string, transforming it into a normalized
    // path in-place.  This works since the normalized path never grows, except
    // in the trivial case of '' -> '.'.  In all other cases, every
    // transformation we make either shrinks the string or maintains its size.
    //
    // We track a current 'write' iterator, indicating the end of the normalized
    // path we've built so far and a current token 't', the next slash-delimited
    // path element we will process.  For example, let's walk through the steps
    // we take to normalize the input '/foo/../bar' to produce '/bar'.  To
    // start, the state looks like the following, with the write iterator past
    // any leading slashes, and 't' at the first path token.
    //
    // /foo/../bar
    //  w            <------ 'write' iterator
    //  [  ]         <------ next token 't'
    //
    // We look at the token 't' to determine its type: one of DotDot, Dot, or
    // Elem.  In this case, it's a regular path Elem 'foo' so we simply copy it
    // to the 'write' iterator and advance 't' to the next token.  Then the
    // state looks like:
    //
    // /foo/../bar
    //      w
    //      [ ]
    //
    // Now 't' is a DotDot token '..', so we remove the last path element in the
    // normalized result by scanning backwards from 'w' resetting 'w' to that
    // location to effectively remove the element, then advance 't' to the next
    // token.  Now the state looks like:
    //
    // /foo/../bar
    //  w      [  ]
    // 
    // The final token is the regular path Elem 'bar' so we copy it and trim the
    // string to produce the final result '/bar'.
    //

    // This code is fairly optimized for libstdc++'s copy-on-write string.  It
    // takes a copy of 'inPath' to start (refcount bump) but it avoids doing any
    // mutating operation on 'path' until it actually has to.  Doing a mutating
    // operation (even grabbing a non-const iterator) will pay for the malloc
    // and deep copy so we want to avoid that in the common case where the input
    // path is already normalized.

    string path(inPath);

    // Find the first path token.
    Token t = _NextToken(inPath.begin(), inPath.end());

    // Allow zero, one, or two leading slashes, per POSIX.  Three or more get
    // collapsed to one.
    const size_t numLeadingSlashes = distance(inPath.begin(), t.first);
    size_t writeIdx = numLeadingSlashes >= 3 ? 1 : numLeadingSlashes;

    // Save a reverse iterator at where we start the output, we'll use this when
    // scanning backward to handle DotDot tokens.
    size_t firstWriteIdx = writeIdx;
    
    // Now walk through the string, copying tokens, looking for slashes and dots
    // to handle.
    for (; t.first != inPath.end(); t = _NextToken(t.second, inPath.end())) {
        switch (_GetTokenType(t)) {
        case Elem:
            // Copy the elem.  We avoid mutating 'path' if we've made no changes
            // to the output yet, which is true if the write head is in the same
            // place in the output as it is in the input.
            if (inPath.begin() + writeIdx == t.first) {
                writeIdx += distance(t.first, t.second);
                t.first = t.second;
                if (writeIdx != path.size())
                    ++writeIdx;
            } else {
                while (t.first != t.second)
                    path[writeIdx++] = *t.first++;
                if (writeIdx != path.size())
                    path[writeIdx++] = '/';
            }
            break;
        case Dot:
            // Do nothing, Dots are simply ignored.
            break;
        case DotDot: {
            // Here we are very likely to be modifying the string, so we use
            // non-const iterators and mutate.
            string::reverse_iterator
                rstart(path.begin() + firstWriteIdx),
                rwrite(path.begin() + writeIdx);
            // Find the last token of the output by finding the next token in
            // reverse.
            RToken backToken = _NextToken(rwrite, rstart);
            // If there are no more Elems to consume with DotDots and this is a
            // relative path, or this token is already a DotDot, then copy it to
            // the output.
            if ((rstart == path.rend() && backToken.first == rstart) ||
                _GetTokenType(backToken) == DotDot) {
                path[writeIdx++] = '.';
                path[writeIdx++] = '.';
                if (writeIdx != path.size())
                    path[writeIdx++] = '/';
            } else if (backToken.first != rstart) {
                // Otherwise, consume the last elem by moving writeIdx back to
                // before the elem.
                writeIdx = distance(path.begin(), backToken.second.base());
            }
        }
            break;
        };
    }
    
    // Remove a trailing slash if we wrote one.  We're careful to use const
    // iterators here to avoid incurring a string copy if it's not necessary (in
    // the case of libstdc++'s copy-on-write basic_string)
    if (writeIdx > firstWriteIdx && cbegin(path)[writeIdx-1] == '/')
        --writeIdx;

    // Trim the string to length if necessary.
    if (writeIdx != path.size())
        path.erase(writeIdx);
    
    // If the resulting path is empty, return "."
    if (path.empty())
        path.assign(".");
    
    return path;
}

} // anon

string
TfNormPath(string const &inPath)
{
#if defined(ARCH_OS_WINDOWS)
    // Convert backslashes to forward slashes.
    string path = TfStringReplace(inPath, "\\", "/");

    // Extract the drive specifier.  Note that we don't correctly handle
    // UNC paths or paths that start with \\? (which allow longer paths).
    //
    // Also make sure drive letters are always lower-case out of TfNormPath
    // on Windows -- this is so that we can be sure we can reliably use the
    // paths as keys in tables, etc.
    string prefix;
    if (path.size() >= 2 && path[1] == ':') {
        prefix.assign(2, ':');
        prefix[0] = std::tolower(path[0]);
        path.erase(0, 2);
    }

    // Normalize and prepend drive specifier, if any.
    return prefix + _NormPath(path);
#else
    return _NormPath(inPath);
#endif // defined(ARCH_OS_WINDOWS)
}

string
TfAbsPath(string const& path)
{
    if (path.empty()) {
        return path;
    }

#if defined(ARCH_OS_WINDOWS)
    char buffer[ARCH_PATH_MAX];
    if (GetFullPathName(path.c_str(), ARCH_PATH_MAX, buffer, nullptr)) {
        return buffer;
    }
    else {
        return path;
    }
#else
    if (TfStringStartsWith(path, "/")) {
        return TfNormPath(path);
    }

    std::unique_ptr<char[]> cwd(new char[ARCH_PATH_MAX]);

    if (getcwd(cwd.get(), ARCH_PATH_MAX) == NULL) {
        // CODE_COVERAGE_OFF hitting this would require creating a directory,
        // chdir'ing into it, deleting that directory, *then* calling this
        // function.
        return path;
        // CODE_COVERAGE_ON
    }

    return TfNormPath(TfSafeString(cwd.get()) + "/" + path);
#endif
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
        (PathIsRelative(path.c_str()) && path[0] != '/' && path[0] != '\\');
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
        const DWORD attributes = GetFileAttributes(path.c_str());
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
        WIN32_FIND_DATA data;
        HANDLE find = FindFirstFile(leftmostPattern.c_str(), &data);
        if (find != INVALID_HANDLE_VALUE) {
            do {
                // Recurse with next pattern.
                Tf_Glob(result, leftmostDir + data.cFileName,
                        remainingPattern, flags);
            } while (FindNextFile(find, &data));
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
