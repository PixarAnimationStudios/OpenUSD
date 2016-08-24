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
///
/// \file Tf/AtomicOfstreamWrapper.cpp

#include "pxr/base/tf/atomicOfstreamWrapper.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/fileSystem.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <iostream>
#include <cerrno>
#include <cstdio>

#if defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <io.h>
#include <ciso646>
#endif

using std::string;

TfAtomicOfstreamWrapper::TfAtomicOfstreamWrapper(
    const string& filePath)
    : _filePath(filePath)
{
    // Do Nothing.
}

TfAtomicOfstreamWrapper::~TfAtomicOfstreamWrapper()
{
    Cancel();
}

bool
TfAtomicOfstreamWrapper::Open(
    string* reason)
{
    if (_stream.is_open()) {
        if (reason) {
            *reason = "Stream is already open";
        }
        return false;
    }

    if (_filePath.empty()) {
        if (reason) {
            *reason = "File path is empty";
        }
        return false;
    }

    // The file path could be a symbolic link. If that's the case, we need to
    // write the temporary file into the real path. This is both so we can
    // experience the appropriate failures while writing the temp file on the
    // same volume as the destination file, and so we can efficiently rename,
    // as that requires both source and destination to be on the same mount.
    string realPathError;
    string realFilePath = TfRealPath(_filePath,
        /* allowInaccessibleSuffix */ true, &realPathError);

    if (realFilePath.empty()) {
        if (reason) {
            *reason = TfStringPrintf(
                "Unable to determine the real path for '%s': %s",
                _filePath.c_str(), realPathError.c_str());
        }
        return false;
    }

    _filePath = realFilePath;
#if defined(ARCH_OS_WINDOWS)
    // XXX: This is not fully ported, also notice the non-platform agnostic "/"
    // in the code below.
    string dirPath = TfStringGetBeforeSuffix(realFilePath, '\\');
#else
    // Check destination directory permissions. The destination directory must
    // exist and be writable so we can write the temporary file and rename the
    // temporary to the destination name.
    string dirPath = TfStringGetBeforeSuffix(realFilePath, '/');
#endif
    if (ArchFileAccess(dirPath.c_str(), W_OK) != 0) {
        if (reason) {
            *reason = TfStringPrintf(
                "Insufficient permissions to write to destination "
                "directory '%s'",
                dirPath.c_str());
        }
        return false;
    } else {
        // Directory exists and has write permission. Check whether the
        // destination file exists and has write permission. We can rename
        // into this path successfully even if we can't write to the file, but
        // we retain the policy that if the user couldn't open the file for
        // writing, they can't write to the file via this buffer object.
        if (ArchFileAccess(realFilePath.c_str(), W_OK) != 0) {
            if (errno != ENOENT) {
                if (reason) {
                    *reason = TfStringPrintf(
                        "Insufficient permissions to write to destination "
                        "file '%s'",
                        realFilePath.c_str());
                }
                return false;
            }
        }
    }

    string tmpFilePrefix = TfStringGetBeforeSuffix(TfGetBaseName(realFilePath));
    int tmpFd = ArchMakeTmpFile(dirPath, tmpFilePrefix, &_tmpFilePath);
    if (tmpFd == -1) {
        if (reason) {
            *reason = TfStringPrintf(
                "Unable to open temporary file '%s' for writing: %s",
                _tmpFilePath.c_str(),
				ArchStrerror(errno).c_str());
        }
        return false;
    }

    // Close the temp file descriptor returned by Arch, and open this buffer
    // with the same file name.
    ArchCloseFile(tmpFd);

    _stream.open(_tmpFilePath.c_str(),
        std::fstream::out|std::fstream::binary|std::fstream::trunc);
    if (not _stream) {
        if (reason) {
            *reason = TfStringPrintf(
                "Unable to open '%s' for writing: %s",
				_tmpFilePath.c_str(), ArchStrerror().c_str());
        }
        return false;
    }

    return true;
}

bool
TfAtomicOfstreamWrapper::Commit(
    string* reason)
{
    if (not _stream.is_open()) {
        if (reason) {
            *reason = "Stream is not open";
        }
        return false;
    }

    // Flush any pending writes to disk and close the temporary file stream
    // before calling rename.
    _stream.close();

#if defined(ARCH_OS_WINDOWS)

	bool success = MoveFileEx(_tmpFilePath.c_str(),
		_filePath.c_str(),
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != FALSE;

	if (!success) {

		TF_RUNTIME_ERROR("Failed to move temporary file %s to file %s: %s.",
			_tmpFilePath.c_str(),
			_filePath.c_str(),
			ArchStrSysError(::GetLastError()).c_str());
		return false;
	}

	return true;

#else
    // The default file mode for new files is user r/w and group r/w, subject
    // to the process umask. If the file already exists, renaming the
    // temporary file results in temporary file permissions, which doesn't
    // allow the group to write.
    mode_t fileMode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP;
    struct stat st;
    if (stat(_filePath.c_str(), &st) != -1) {
        fileMode = st.st_mode & 0777;
    }

    if (chmod(_tmpFilePath.c_str(), fileMode) != 0) {
        // CODE_COVERAGE_OFF
        TF_WARN("Unable to set permissions for temporary file '%s': %s",
            _tmpFilePath.c_str(), ArchStrerror(errno).c_str());
        // CODE_COVERAGE_ON
    }

    bool success = true;
    if (rename(_tmpFilePath.c_str(), _filePath.c_str()) != 0) {
        if (reason) {
            *reason = TfStringPrintf(
                "Unable to rename '%s' to '%s': %s",
                _tmpFilePath.c_str(),
                _filePath.c_str(),
                ArchStrerror(errno).c_str());
        }
        success = false;
    }

    return success;
#endif
}

bool
TfAtomicOfstreamWrapper::Cancel(
    string* reason)
{
    if (not _stream.is_open()) {
        if (reason) {
            *reason = "Buffer is not open";
        }
        return false;
    }

    // Flush any pending writes to disk and close the temporary file stream
    // before unlinking the temporary file.
    _stream.close();

    bool success = true;

    if (ArchUnlinkFile(_tmpFilePath.c_str()) != 0) {
        if (errno != ENOENT) {
            if (reason) {
				*reason = TfStringPrintf(
					"Unable to remove temporary file '%s': %s",
					_tmpFilePath.c_str(),
					ArchStrerror(errno).c_str());
            }
            success = false;
        }
    }

    return success;
}

