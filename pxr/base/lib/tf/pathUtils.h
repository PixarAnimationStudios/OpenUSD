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
#ifndef TF_PATHUTILS_H
#define TF_PATHUTILS_H


#include <string>
#include <vector>
#include <glob.h>

/// \file tf/pathUtils.h
/// \ingroup group_tf_Path
/// Definitions of basic path utilities in tf.
///
/// These are utilities that operate on paths (represented by strings as
/// something like: "/chars/Buzz/Torso".

/// Returns the canonical path of the specified filename, eliminating any
/// symbolic links encountered in the path.
///
/// This is a wrapper to realpath(3), which caters for situations where the
/// real realpath() would return a NULL string, such as the case where the
/// path is really just a program name.  The memory allocated by realpath is
/// managed internally.
///
/// If \a allowInaccessibleSuffix is true, then this function will only invoke
/// realpath on the longest accessible prefix of \a path, and then append the
/// inaccessible suffix.
///
/// If \a error is provided, it is set to the error reason should an error
/// occur while computing the real path. If no error occurs, the string is
/// cleared.
std::string TfRealPath(std::string const& path,
                       bool allowInaccessibleSuffix = false,
                       std::string* error = 0);

/// Normalizes the specified path, eliminating double slashes, etc.
///
/// This canonicalizes paths, removing any double slashes, and eliminiating
/// '.', and '..' components of the path.  This emulates the behavior of
/// os.path.normpath in Python.
std::string TfNormPath(std::string const& path);

/// Return the index delimiting the longest accessible prefix of \a path.
///
/// The returned value is safe to use to split the string via it's generalized
/// copy constructor. If the entire path is accessible, return the length of
/// the input string. If none of the path is accessible, return 0.  Otherwise
/// the index points to the path separator that delimits the existing prefix
/// from the non-existing suffix. 
/// 
/// Examples: suppose the paths /, /usr, and /usr/anim exist, but no other
/// paths exist.
///
/// TfFindLongestAccessiblePrefix('/usr/anim')     -> 9
/// TfFindLongestAccessiblePrefix('/usr/anim/foo') -> 9
/// TfFindLongestAccessiblePrefix('/foo/bar')      -> 0
///
/// If an error occurs, and the \a error string is not null, it is set to the
/// reason for the error. If the error string is set, the returned index is
/// the path separator before the element at which the error occurred.
std::string::size_type
TfFindLongestAccessiblePrefix(std::string const &path, std::string* error = 0);

/// Returns the canonical absolute path of the specified filename.
///
/// This makes the specified path absolute, by prepending the current working
/// directory.  If the path is already absolute, it is returned unmodified.
/// This function differs from TfRealPath in that the path may point to a
/// symlink, or not exist at all, and still result in an absolute path, rather
/// than an empty string.
std::string TfAbsPath(std::string const& path);

/// Returns the source path for a symbolic link.
///
/// This is a wrapper to readlink(2).
std::string TfReadLink(std::string const& path);

/// Expands one or more shell glob patterns.
///
/// This is a wrapper to glob(3), which manages the C structures necessary to
/// glob a pattern, returning a std::vector of results. If no flags are
/// specified, the GLOB_MARK and GLOB_NOCHECK flags are set by default.
/// GLOB_MARK marks directories which match the glob pattern with a trailing
/// slash. GLOB_NOCHECK returns any unexpanded patterns in the result.
std::vector<std::string> TfGlob(std::vector<std::string> const& paths,
                                unsigned int flags=GLOB_NOCHECK|GLOB_MARK);

/// Expands a shell glob pattern.
///
/// This form of Glob calls TfGlob.  For efficiency reasons, if expanding more
/// than one pattern, use the vector form.  As with the vector form of TfGlob,
/// if flags is not set, the default glob flags are GLOB_MARK and
/// GLOB_NOCHECK.
std::vector<std::string> TfGlob(std::string const& path,
                                unsigned int flags=GLOB_NOCHECK|GLOB_MARK);

#endif /* TF_PATHUTILS_H */
