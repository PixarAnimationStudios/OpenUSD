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
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/errno.h"

#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>

#include <algorithm>
#include <errno.h>
#include <limits.h>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>
#include <ciso646>

#if !defined(ARCH_OS_WINDOWS)
#include <glob.h>
#else
#include <Windows.h>
#include <Shlwapi.h>
#endif

using std::pair;
using std::string;
using std::vector;

string
TfRealPath(string const& path, bool allowInaccessibleSuffix, string* error)
{
    if (path.empty())
        return string();

#if defined(ARCH_OS_WINDOWS)
	char fullPath[ARCH_PATH_MAX];

	if (!GetFullPathName(path.c_str(), ARCH_PATH_MAX, fullPath, NULL))
	{
		if (error)
		{
			*error = "Call to GetFullPathName failed";
		}
		return string();
	}

	return std::string(fullPath);
#else
    string localError;
    if (not error)
        error = &localError;
    else
        error->clear();

    string suffix, prefix = path;

    if (allowInaccessibleSuffix) {
        string::size_type split = TfFindLongestAccessiblePrefix(path, error);
        if (not error->empty())
            return string();

        prefix = string(path, 0, split);
        suffix = string(path, split);
    }

    char resolved[ARCH_PATH_MAX];
    return TfAbsPath(TfSafeString(realpath(prefix.c_str(), resolved)) + suffix);
#endif
}

string::size_type
TfFindLongestAccessiblePrefix(string const &path, string* error)
{
#if defined(ARCH_OS_WINDOWS)
    printf("TfFindLongestAccessiblePrefix not yet implemented on Windows.\n");
    return path.size();
#else
    typedef string::size_type size_type;
    static const size_type npos = string::npos;

    struct _Local {
        // Sentinel is greater than existing paths, less than non-existing ones.
        static bool Compare(
            string const &str, size_type lhs, size_type rhs, string* err) {
            if (lhs == rhs)
                return false;
            if (lhs == npos)
                return not Accessible(str, rhs, err);
            if (rhs == npos)
                return Accessible(str, lhs, err);
            return lhs < rhs;
        }

        static bool Accessible(string const &str, size_type index, string *err) {
            string checkPath(str, 0, index);
            struct stat st;
            if (lstat(checkPath.c_str(), &st) == -1) {
                if (errno != ENOENT and err->empty())
                    *err = ArchStrerror(errno).c_str();
                return false;
            }

            // If the lstat succeeds and the target is a symbolic link, do
            // an extra stat to see if the link is dangling.
            if (S_ISLNK(st.st_mode)) {
                if (stat(checkPath.c_str(), &st) == -1) {
                    if (err->empty()) {
                        if (errno == ENOENT)
                            *err = "encountered dangling symbolic link";
                        else
                            *err = ArchStrerror(errno).c_str();
                    }
                    return false;
                }
            }

            // Path exists, no errors occurred.
            return true;
        }
    };

    // Build a vector of split point indexes.
    vector<size_type> splitPoints;
    for (size_type p = path.find('/', path.find_first_not_of('/'));
         p != npos; p = path.find('/', p+1))
        splitPoints.push_back(p);
    splitPoints.push_back(path.size());

    // Lower-bound to find first non-existent path.
    vector<size_type>::iterator result =
        std::lower_bound(splitPoints.begin(), splitPoints.end(), npos,
                         boost::bind(_Local::Compare, path, _1, _2, error));

    // begin means nothing existed, end means everything did, else prior is last
    // existing path.
    if (result == splitPoints.begin())
        return 0;
    if (result == splitPoints.end())
        return path.length();
    return *(result - 1);
#endif
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
         t.first != end and *t.first == '/'; ++t.first) {}
    for (t.second = t.first;
         t.second != end and *t.second != '/'; ++t.second) {}
    return t;
}

template <class Iter>
inline TokenType
_GetTokenType(pair<Iter, Iter> t) {
    size_t len = distance(t.first, t.second);
    if (len == 1 and t.first[0] == '.')
        return Dot;
    if (len == 2 and t.first[0] == '.' and t.first[1] == '.')
        return DotDot;
    return Elem;
}

} // anon

string
TfNormPath(string const &inPath)
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
            if ((rstart == path.rend() and backToken.first == rstart) or
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
    if (writeIdx > firstWriteIdx and cbegin(path)[writeIdx-1] == '/')
        --writeIdx;

    // Trim the string to length if necessary.
    if (writeIdx != path.size())
        path.erase(writeIdx);
    
    // If the resulting path is empty, return "."
    if (path.empty())
        path.assign(".");
    
    return path;
}

string
TfAbsPath(string const& path)
{
#if defined(ARCH_OS_WINDOWS)
	char buffer[ARCH_PATH_MAX];
	if (GetFullPathName(path.c_str(), ARCH_PATH_MAX, buffer, nullptr)) {
		return string(buffer);
	} else {
		printf("TfAbsPath failed on %s with error code %d\n", 
			path.c_str(), GetLastError());
		return path;
	}
#else
    if (path.empty()) {
        return path;
    }
    else if (TfStringStartsWith(path, "/")) {
        return TfNormPath(path);
    }

    boost::scoped_array<char> cwd(new char[ARCH_PATH_MAX]);

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
#if defined(ARCH_OS_WINDOWS)
	return path;
#else
    if (path.empty()) {
        return path;
    }

    boost::scoped_array<char> buf(new char[ARCH_PATH_MAX]);
    ssize_t len;

    if ((len = readlink(path.c_str(), buf.get(), ARCH_PATH_MAX)) == -1) {
        return string();
    }
    buf.get()[len] = '\0';

    return TfSafeString(buf.get());
#endif
}

bool TfIsRelativePath(std::string const& path)
{
#if defined(ARCH_OS_WINDOWS)
    return PathIsRelative(path.c_str()) ? true : false;
#else
    return path[0] != '/';
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

vector<string>
TfGlob(vector<string> const& paths, unsigned int flags)
{
    if (paths.empty())
    {
        return vector<string>();
    }

    vector<string> results;

    for (auto path : paths)
    {
        size_t wildcard = path.find("/*/");
        if(wildcard != std::string::npos)
        {
            string rootDir(path, 0, wildcard);

            vector<string> dirNames;
            TfReadDir(rootDir, &dirNames, nullptr, nullptr, nullptr);

            for (auto dirName : dirNames)
            {
                string dir = path;
                dir.replace(wildcard + 1, 1, dirName);

                results.push_back(dir);
            }
        }
    }

    return results;
}

#endif

vector<string>
TfGlob(string const& path, unsigned int flags)
{
    return path.empty()
        ? vector<string>()
        : TfGlob(vector<string>(1, path), flags);
}
