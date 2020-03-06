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
#ifndef PXR_BASE_TF_FILE_UTILS_H
#define PXR_BASE_TF_FILE_UTILS_H

/// \file tf/fileUtils.h
/// \ingroup group_tf_File
/// Definitions of basic file utilities in tf.

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

#include <string>
#include <vector>
#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

/// Returns true if the path exists.
///
/// If \p resolveSymlinks is false (default), the path is checked using
/// lstat(). if \p resolveSymlinks is true, the path is checked using stat(),
/// which resolves all symbolic links in the path.
TF_API
bool TfPathExists(std::string const& path, bool resolveSymlinks = false);

/// Returns true if the path exists and is a directory.
///
/// If \p resolveSymlinks is false (default), the path is checked using
/// lstat(). if \p resolveSymlinks is true, the path is checked using stat(),
/// which resolves all symbolic links in the path.
TF_API
bool TfIsDir(std::string const& path, bool resolveSymlinks = false);

/// Returns true if the path exists and is a file.
///
/// If \p resolveSymlinks is false (default), the path is checked using
/// lstat(). if \p resolveSymlinks is true, the path is checked using stat(),
/// which resolves all symbolic links in the path.
TF_API
bool TfIsFile(std::string const& path, bool resolveSymlinks = false);

/// Returns true if the path exists and is a symbolic link.
TF_API
bool TfIsLink(std::string const& path);

/// Returns true if the file or directory at \p path is writable.
///
/// For this function to return true, the file must exist and be writable by
/// the effective user, effective group, or all users. This function
/// dereferences symbolic links, returning whether or not the resolved file or
/// directory path is writable. If the file or directory does not exist, this
/// function returns false.
TF_API
bool TfIsWritable(std::string const& path);

/// Returns true if the path is an empty directory.
TF_API
bool TfIsDirEmpty(std::string const& path);

/// Creates a symbolic link from \p src to \p dst.
TF_API
bool TfSymlink(std::string const& src, std::string const& dst);

/// Deletes a file at path.
TF_API
bool TfDeleteFile(std::string const& path);

/// Creates a directory.
///
/// If the directory cannot be created, this function returns false.  If no
/// mode is specified, the default mode is 0777. If the specified path already
/// exists, or an error occurs while creating the directory, this method
/// returns false.
TF_API
bool TfMakeDir(std::string const& path, int mode=-1);

/// Creates a directory hierarchy.
///
/// If any element of the path cannot be created, this function will return
/// false. The specified mode will be used to create all new directories.  If
/// no mode is specified, the default mode of \c TfMakeDir is used. If the
/// target directory exists, this function returns false if \p existOk is
/// false.
TF_API
bool TfMakeDirs(std::string const& path, int mode=-1, bool existOk=false);

/// Function type for TfWalkDirs.
///
/// This function is called once for each directory visited by TfWalkDirs.
/// The first parameter is the directory path; if the topmost directory passed
/// to TfWalkDirs is relative, this path will also be relative. The second
/// parameter is a vector of subdirectory names, relative to the directory
/// path. This parameter is a pointer, allowing the subdirectory list to be
/// modified, thus controlling which directories are visited.  Note that
/// modifying the subdirectory vector has no effect when TfWalkDirs is called
/// with \c topDown set to \c false. The final parameter is a vector of file
/// names found in the directory path. The returned value determines whether
/// the walk should be terminated (\c false), or continue (\c true).
typedef std::function<bool (std::string const&,
                            std::vector<std::string> *,
                            std::vector<std::string> const&)> TfWalkFunction;

/// TfRmTree error handler function.
///
/// The first parameter is the path which caused the error (file or directory),
/// and the second parameter is an error message indicating why the error
/// occurred.
typedef std::function<void (std::string const&,
                            std::string const&)> TfWalkErrorHandler;

/// error handler to use when you want to ignore errors
///
/// When calling TfWalkDirs/ChmodTree/RmTree and you want to ignore errors,
/// you can pass in this public error handler which will ignore all the
/// errors.
TF_API
void TfWalkIgnoreErrorHandler(std::string const& path, std::string const& msg);

/// Directory tree walker.
///
/// This function attempts to be as compatible as possible with Python's
/// os.walk() function.
///
/// For each directory in the tree rooted at \p top (including \p top itself,
/// but excluding '.' and '..'), the std::function \p fn is called with
/// three arguments: \c dirpath, \c dirnames, and \c filenames.
///
/// \c dirpath is a string, the path to the directory.  \c dirnames is a list
/// of the names of the subdirectories in \c dirpath (excluding '.' and '..').
/// \c filenames is a list of the names of the non-directory files in
/// \c dirpath.  Note that the names in the sets are just names, with no path
/// components.  To get a full path (which begins with \p top) to a file or
/// directory in \c dirpath, use \c TfStringCatPaths(dirpath, name).
///
/// If optional argument \p topDown is true, or not specified, \p fn is called
/// for a directory before any subdirectories.  If topdown is false, \p fn is
/// called for a directory after all subdirectories.  Additionally, when
/// \p topDown is true, the walk function can modify the \c dirnames set in
/// place.  This can be used to prune the search, or to impose a specific
/// visitation order.  Modifying \c dirnames when \p topDown is false has no
/// effect, since the directories in \c dirnames have already been visited
/// by the time they are passed to \p fn.
///
/// The value returned by the error handler function \p onError determines what
/// further action will be taken if an error is encountered. If \c true is
/// returned, the walk will continue; if \c false, the walk will not continue.
///
/// If \p followLinks is false, symbolic links to directories encountered
/// during the walk are passed to the walk function in the \c filenames vector.
/// If \p followLinks is true, symbolic links to directories are passed to the
/// walk function in the \p dirnames vector, and the walk will recurse into
/// these directories.
///
/// If \p top is a symbolic link to a directory, it is followed regardless of
/// the value of \p followLinks. Calling TfWalkDirs with a file argument
/// returns immediately without calling \p fn.
TF_API
void TfWalkDirs(std::string const& top,
                TfWalkFunction fn,
                bool topDown=true,
                TfWalkErrorHandler onError = 0,
                bool followLinks = false);

/// Recursively delete a directory tree rooted at \p path.
///
/// Tf runtime errors are raised if any errors are encountered while deleting
/// the specified \p path.  Pass in TfWalkIgnoreErrorHandler() to ignore errors.
/// Alternately, sending in a custom TfWalkErrorHandler will
/// call this handler when errors occur.  This handler receives the path which
/// caused the error, and a message indicating why the error occurred.
TF_API
void TfRmTree(std::string const& path,
              TfWalkErrorHandler onError = 0);

/// Return a list containing files and directories in \p path.
///
/// A trailing path separator character is appended to directories returned
/// in the listing.  If \p recursive is true, the directory listing will
/// include all subdirectory structure of \p path.
TF_API
std::vector<std::string> TfListDir(std::string const& path,
                                   bool recursive = false);

/// Read the contents of \p dirPath and append the names of the contained
/// directories, files, and symlinks to \p dirnames, \p filenames, and
/// \p symlinknames, respectively.
///
/// Return true if \p dirPath 's contents were read successfully. Otherwise
/// return false and set \p errMsg with a description of the error if \p errMsg
/// is not NULL.
///
/// It is safe to pass NULL for any of \p dirnames, \p filenames, and
/// \p symlinknames. In that case those elements are not reported
TF_API
bool
TfReadDir(std::string const &dirPath,
          std::vector<std::string> *dirnames,
          std::vector<std::string> *filenames,
          std::vector<std::string> *symlinknames,
          std::string *errMsg = NULL);

/// Touch \p fileName, updating access and modification time to 'now'.
///
/// A simple touch-like functionality. Simple in a sense that it does not
/// offer as many options as the same-name unix touch command, but otherwise
/// is identical to the default touch behavior. If \p create is true, an empty
/// file gets created, otherwise the touch call fails if the file does not
/// already exist. 
TF_API
bool TfTouchFile(std::string const &fileName, bool create=true);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_FILE_UTILS_H
