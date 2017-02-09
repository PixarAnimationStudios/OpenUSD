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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/systemInfo.h"

#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/functional/hash.hpp>
#include <boost/noncopyable.hpp>
#include "pxr/base/tf/hashset.h"

#include <set>
#include <string>
#include <cstdio>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include <dirent.h>
#include <errno.h>
#include <utime.h>

using std::set;
using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

static bool
Tf_Stat(string const& path, bool resolveSymlinks, struct stat* st = 0)
{
    if (path.empty()) {
        return false;
    }

    struct stat unused;
    if (!st) {
        st = &unused;
    }

    int result = resolveSymlinks ?
        stat(path.c_str(), st) : lstat(path.c_str(), st);
    return result == 0;
}

bool
TfPathExists(string const& path, bool resolveSymlinks)
{
    return Tf_Stat(path, resolveSymlinks);
}

bool
TfIsDir(string const& path, bool resolveSymlinks)
{
    struct stat st;
    if (Tf_Stat(path, resolveSymlinks, &st)) {
        return S_ISDIR(st.st_mode);
    }
    return false;
}

bool
TfIsFile(string const& path, bool resolveSymlinks)
{
    struct stat st;
    if (Tf_Stat(path, resolveSymlinks, &st)) {
        return S_ISREG(st.st_mode);
    }
    return false;
}

bool
TfIsLink(string const& path)
{
    struct stat st;
    if (Tf_Stat(path, /* resolveSymlinks */ false, &st)) {
        return S_ISLNK(st.st_mode);
    }
    return false;
}

bool
TfIsWritable(string const& path)
{
    struct stat st;
    if (Tf_Stat(path, /* resolveSymlinks */ true, &st)) {
        return ArchStatIsWritable(&st);
    }
    return false;
}

bool
TfIsDirEmpty(string const& path)
{
    if (!TfIsDir(path))
        return false;

    if (DIR *dirp = opendir(path.c_str()))
    {
        struct dirent *dent;
        while ((dent = readdir(dirp)) != NULL)
        {
            if ((dent->d_ino > 0) &&
                (strcmp(dent->d_name, ".") != 0) &&
                (strcmp(dent->d_name, "..") != 0)) {
                (void) closedir(dirp);
                return false;
            }
        }
        (void) closedir(dirp);
        return true;
    }
    return false;
}

bool
TfSymlink(string const& src, string const& dst)
{
    return (symlink(src.c_str(), dst.c_str()) != -1);
}

bool
TfDeleteFile(std::string const& path)
{
    if (unlink(path.c_str()) != 0) {
        TF_RUNTIME_ERROR("Failed to delete '%s'", path.c_str());
        return false;
    }
    return true;
}

bool
TfMakeDir(string const& path, int mode)
{
    // Default mode is 0777
    if (mode == -1)
        mode = S_IRWXU|S_IRWXG|S_IRWXO;

    return (mkdir(path.c_str(), mode) != -1);
}

static bool
Tf_MakeDirsRec(string const& path, int mode)
{
    string head = TfStringTrimRight(TfGetPathName(path), "/");

    if (head.empty()) {
        return TfIsDir(path) ? true : TfMakeDir(path, mode);
    }

    string tail = TfGetBaseName(path);

    if (!head.empty() && !tail.empty()) {

        struct stat st;
        if (stat(head.c_str(), &st) != -1) {
            // Path exists
            if (!S_ISDIR(st.st_mode)) {
                TF_RUNTIME_ERROR("Path %s exists, and is not a directory",
                        head.c_str());
                return false;
            }
        }
        else {
            // Path does not exist
            if (!Tf_MakeDirsRec(head, mode)) {
                return false;
            }
        }
    }

    return TfIsDir(path) ? true : TfMakeDir(path, mode);
}

bool
TfMakeDirs(string const& path, int mode)
{
    if (path.empty() || TfIsDir(path)) {
        return false;
    }
    return Tf_MakeDirsRec(TfNormPath(path), mode);
}

bool
TfReadDir(
    const string& dirPath,
    vector<string>* dirnames,
    vector<string>* filenames,
    vector<string>* symlinknames,
    string *errMsg)
{
    DIR *dir;
    struct dirent entry;
    struct dirent *result;
    int rc;

    if ((dir = opendir(dirPath.c_str())) == NULL) {
        if (errMsg) {
            *errMsg = TfStringPrintf("opendir failed: %s", strerror(errno));
        }
        return false;
    }

#if defined(HAVE_FSTATAT)
    const int dirFD = dirfd(dir);
#else
    std::string entryPath(dirPath);
    if (entryPath.back() != '/')
      entryPath += '/';
    const size_t entryStart = entryPath.size();
#endif

    for (rc = readdir_r(dir, &entry, &result);
         result && rc == 0;
         rc = readdir_r(dir, &entry, &result)) {

        if (strcmp(entry.d_name, ".") == 0 || 
            strcmp(entry.d_name, "..") == 0)
            continue;

        bool entryIsDir = false;
        bool entryIsLnk = false;
#if defined(_DIRENT_HAVE_D_TYPE)
        // If we are on a BSD-like system, and the underlying filesystem has
        // support for it, we can use dirent.d_type to avoid lstat.
        if (entry.d_type == DT_DIR) {
            entryIsDir = true;
        } else if (entry.d_type == DT_LNK) {
            entryIsLnk = true;
        } else if (entry.d_type == DT_UNKNOWN) {
#endif
            // If d_type is not available, or the filesystem has no support
            // for d_type, fall back to lstat.
            struct stat st;
#if defined(HAVE_FSTATAT)
            if (fstatat(dirFD, entry.d_name, &st, AT_SYMLINK_NOFOLLOW) != 0)
#else
            entryPath.replace(entryStart, string::npos, entry.d_name);
            if (lstat(entryPath.c_str(), &st) != 0)
#endif
                continue;

            if (S_ISDIR(st.st_mode)) {
                entryIsDir = true;
            } else if (S_ISLNK(st.st_mode)) {
                entryIsLnk = true;
            }
#if defined(_DIRENT_HAVE_D_TYPE)
        }
#endif

        if (entryIsDir) {
            if (dirnames)
                dirnames->push_back(entry.d_name);
        } else if (entryIsLnk) {
            if (symlinknames)
                symlinknames->push_back(entry.d_name);
        } else if (filenames) {
            filenames->push_back(entry.d_name);
        }
    }

    closedir(dir);
    
    return true;
}

static void
Tf_ReadDir(
    const string& dirPath,
    const TfWalkErrorHandler& onError,
    vector<string>* dirnames,
    vector<string>* filenames,
    vector<string>* symlinknames)
{
    if (!(TF_VERIFY(dirnames) &&
          TF_VERIFY(filenames) &&
          TF_VERIFY(symlinknames)))
        return;

    string errMsg;
    if (!TfReadDir(dirPath, dirnames, filenames, symlinknames, &errMsg))
        if (onError) {
            onError(dirPath, errMsg);
        }
}    

struct Tf_FileId {
    Tf_FileId(const struct stat& st)
        : dev(st.st_dev), ino(st.st_ino)
    { }

    bool operator==(const Tf_FileId& other) const {
        return dev == other.dev && ino == other.ino;
    }

    dev_t dev;
    ino_t ino;
};

static size_t hash_value(const Tf_FileId& fileId) {
    size_t seed = 0;
    boost::hash_combine(seed, fileId.dev);
    boost::hash_combine(seed, fileId.ino);
    return seed;
}

typedef TfHashSet<Tf_FileId, boost::hash<Tf_FileId> > Tf_FileIdSet;

static bool
Tf_WalkDirsRec(
    const string& dirpath,
    const TfWalkFunction& func,
    bool topDown,
    const TfWalkErrorHandler& onError,
    bool followLinks,
    Tf_FileIdSet* linkTargets)
{
    if (!TF_VERIFY(linkTargets))
        return false;

    vector<string> dirnames, filenames, symlinknames;
    Tf_ReadDir(dirpath, onError, &dirnames, &filenames, &symlinknames);

    // If we're following symbolic links, stat each symlink name returned by
    // readdir. If the symlink is to a directory, record information about the
    // directory to which the symlink points. If we visit this directory via a
    // symbolic link again, omit it from the directory list to prevent the
    // directory from being followed until stat() eventually fails with ELOOP.
    if (followLinks) {
        for (const auto& name : symlinknames) {
            struct stat st;
            if (stat(string(dirpath + "/" + name).c_str(), &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    Tf_FileId fileId(st);
                    if (linkTargets->find(fileId) != linkTargets->end())
                        continue;
                    linkTargets->insert(fileId);

                    dirnames.push_back(name);
                } else
                    filenames.push_back(name);
            } else
                filenames.push_back(name);
        }
    } else
        filenames.insert(filenames.end(),
            symlinknames.begin(), symlinknames.end());

    if (topDown && !func(dirpath, &dirnames, filenames))
       return false;

    for (const auto& name : dirnames) {
        if (!Tf_WalkDirsRec(dirpath + "/" + name,
                func, topDown, onError, followLinks, linkTargets))
            return false;
    }

    if (!topDown && !func(dirpath, &dirnames, filenames))
       return false;

    return true;
}

void
TfWalkDirs(
    const string& top,
    TfWalkFunction func,
    bool topDown,
    TfWalkErrorHandler onError,
    bool followLinks)
{
    if (!TfIsDir(top, /* followSymlinks */ true)) {
        if (onError)
            onError(top, TfStringPrintf("%s is not a directory", top.c_str()));
        return;
    }

    Tf_FileIdSet linkTargets;
    Tf_WalkDirsRec(TfNormPath(top),
        func, topDown, onError, followLinks, &linkTargets);
}

void
TfWalkIgnoreErrorHandler(string const& path, string const& msg)
{
}

static bool
Tf_RmTree(string const& dirpath,
          vector<string> *dirnames,
          vector<string> const& filenames,
          TfWalkErrorHandler onError)
{
    vector<string>::const_iterator it;
    for (it = filenames.begin(); it != filenames.end(); ++it) {
        string path = dirpath + "/" + *it;
        if (unlink(path.c_str()) != 0) {
            // CODE_COVERAGE_OFF this could happen if the file is removed by
            // another process before we get there, or a file exists but is
            // not writable by us, or the parent directory is not writable by
            // us.
            if (onError)
                onError(dirpath, TfStringPrintf("unlink failed for '%s': %s",
                                                path.c_str(), strerror(errno)));
            // CODE_COVERAGE_ON
        }
    }

    if (rmdir(dirpath.c_str()) != 0) {
        // CODE_COVERAGE_OFF this could happen for all the same reasons the
        // unlink above could fail.
        if (onError)
            onError(dirpath, TfStringPrintf("rmdir failed for '%s': %s",
                                            dirpath.c_str(), strerror(errno)));
        // CODE_COVERAGE_ON
    }

    return true;
}

static void
Tf_RmTreeRaiseErrors(string const& path, string const& msg)
{
    TF_RUNTIME_ERROR("failed to remove '%s': %s", path.c_str(), msg.c_str());
}

void
TfRmTree(string const& path, TfWalkErrorHandler onError)
{
    TfWalkDirs(path,
               boost::bind(Tf_RmTree, _1, _2, _3, onError),
               /* topDown */ false,
               onError ? onError : Tf_RmTreeRaiseErrors);
}

static bool
Tf_ListDir(string const& dirpath,
           vector<string> *dirnames,
           vector<string> const& filenames,
           vector<string> *paths,
           bool recursive)
{
    for (vector<string>::const_iterator it = dirnames->begin();
         it != dirnames->end(); ++it) {
        paths->push_back(dirpath + "/" + *it + "/");
    }

    for (vector<string>::const_iterator it = filenames.begin();
         it != filenames.end(); ++it)
        paths->push_back(dirpath + "/" + *it);

    return recursive;
}

vector<string>
TfListDir(string const& path, bool recursive)
{
    vector<string> result;
    TfWalkDirs(path, boost::bind(Tf_ListDir, _1, _2, _3, &result, recursive));
    return result;
}

bool
TfTouchFile(string const &fileName, bool create)
{
    if (create) {
        // Attempt to create the file so it is readable and writable by user,
        // group and other.
        int fd = open(fileName.c_str(),
            O_WRONLY | O_CREAT | O_NONBLOCK | O_NOCTTY,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (fd == -1)
            return false;
        close(fd);
    }

    // Passing NULL to the 'times' argument sets both the atime and mtime to
    // the current time, with millisecond precision.
    return utimes(fileName.c_str(), /* times */ NULL) == 0;
}

PXR_NAMESPACE_CLOSE_SCOPE
