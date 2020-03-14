//
// Copyright 2017 Pixar
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

#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/fileSystem.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"

#if defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <io.h>
#endif

#include <string>
#include <cerrno>

PXR_NAMESPACE_OPEN_SCOPE

bool
Tf_AtomicRenameFileOver(std::string const &srcFileName,
                        std::string const &dstFileName,
                        std::string *error)
{
    bool result = true;
#if defined(ARCH_OS_WINDOWS)
    bool moved = MoveFileEx(srcFileName.c_str(),
                            dstFileName.c_str(),
                            MOVEFILE_REPLACE_EXISTING |
                            MOVEFILE_COPY_ALLOWED) != FALSE;
    if (!moved) {
        *error = TfStringPrintf(
            "Failed to rename temporary file '%s' to '%s': %s",
            srcFileName.c_str(),
            dstFileName.c_str(),
            ArchStrSysError(::GetLastError()).c_str());
        result = false;
    }
#else
    // The mode of the temporary file is set by ArchMakeTmpFile, which tries to
    // be slightly less restrictive by setting the mode to 0660, whereas the
    // underlying temporary file API used by arch creates files with mode
    // 0600. When renaming our temporary file into place, we either want the
    // permissions to match that of an existing target file, or to be created
    // with default permissions modulo umask.
    mode_t fileMode = 0;
    struct stat st;
    if (stat(dstFileName.c_str(), &st) != -1) {
        fileMode = st.st_mode & DEFFILEMODE;
    } else {
        const mode_t mask = umask(0);
        umask(mask);
        fileMode = DEFFILEMODE - mask;
    }

    if (chmod(srcFileName.c_str(), fileMode) != 0) {
        TF_WARN("Unable to set permissions for temporary file '%s': %s",
                srcFileName.c_str(), ArchStrerror(errno).c_str());
    }

    if (rename(srcFileName.c_str(), dstFileName.c_str()) != 0) {
        *error = TfStringPrintf(
            "Failed to rename temporary file '%s' to '%s': %s",
            srcFileName.c_str(), dstFileName.c_str(),
            ArchStrerror(errno).c_str());
        result = false;
    }
#endif
    return result;
}

int
Tf_CreateSiblingTempFile(std::string fileName,
                         std::string *realFileName,
                         std::string *tempFileName,
                         std::string *error)
{
    int result = -1;
    if (fileName.empty()) {
        *error = "Empty fileName";
        return result;
    }

    // The file path could be a symbolic link. If that's the case, we need to
    // write the temporary file into the real path. This is both so we can
    // experience the appropriate failures while writing the temp file on the
    // same volume as the destination file, and so we can efficiently rename, as
    // that requires both source and destination to be on the same mount.
    std::string realPathError;
    std::string realFilePath = TfRealPath(fileName,
                                          /* allowInaccessibleSuffix */ true,
                                          &realPathError);
    if (realFilePath.empty()) {
        *error = TfStringPrintf(
            "Unable to determine the real path for '%s': %s",
            fileName.c_str(), realPathError.c_str());
        return result;
    }

#if defined(ARCH_OS_WINDOWS)
    // XXX: This is not fully ported, also notice the non-platform agnostic "/"
    // in the code below.
    std::string dirPath = TfStringGetBeforeSuffix(realFilePath, '\\');
#else
    // Check destination directory permissions. The destination directory must
    // exist and be writable so we can write the temporary file and rename the
    // temporary to the destination name.
    std::string dirPath = TfStringGetBeforeSuffix(realFilePath, '/');
#endif

    if (ArchFileAccess(dirPath.c_str(), W_OK) != 0) {
        *error = TfStringPrintf(
            "Insufficient permissions to write to destination "
            "directory '%s'", dirPath.c_str());
        return result;
    }

    // Directory exists and has write permission. Check whether the destination
    // file exists and has write permission. We can rename into this path
    // successfully even if we can't write to the file, but we retain the policy
    // that if the user couldn't open the file for writing, they can't write to
    // the file via this object.
    if (ArchFileAccess(realFilePath.c_str(), W_OK) != 0 && errno != ENOENT) {
        *error = TfStringPrintf(
            "Insufficient permissions to write to destination "
            "file '%s'", realFilePath.c_str());
        return result;
    }

    std::string tmpFilePrefix =
        TfStringGetBeforeSuffix(TfGetBaseName(realFilePath));
    std::string tmpFN;
    result = ArchMakeTmpFile(dirPath, tmpFilePrefix, &tmpFN);
    if (result == -1) {
        *error = TfStringPrintf("Unable to create temporary file '%s': %s",
                                tmpFN.c_str(),
                                ArchStrerror(errno).c_str());
        return result;
    }

    *tempFileName = tmpFN;
    *realFileName = realFilePath;
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
