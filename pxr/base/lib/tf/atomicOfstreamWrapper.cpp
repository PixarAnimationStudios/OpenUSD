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

#include "pxr/pxr.h"
#include "pxr/base/tf/atomicOfstreamWrapper.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/fileSystem.h"

#include "pxr/base/tf/atomicRenameUtil.h"
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
#endif

PXR_NAMESPACE_OPEN_SCOPE

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

    std::string localError, *err = reason ? reason : &localError;
    int tmpFd = Tf_CreateSiblingTempFile(
        _filePath, &_filePath, &_tmpFilePath, err);
    if (!tmpFd) {
        return false;
    }
    
    // Close the temp file descriptor returned by Arch, and open this buffer
    // with the same file name.
    ArchCloseFile(tmpFd);

    _stream.open(_tmpFilePath.c_str(),
                 std::fstream::out|std::fstream::binary|std::fstream::trunc);
    if (!_stream) {
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
    if (!_stream.is_open()) {
        if (reason) {
            *reason = "Stream is not open";
        }
        return false;
    }

    // Flush any pending writes to disk and close the temporary file stream
    // before calling rename.
    _stream.close();

    std::string localError, *err = reason ? reason : &localError;
    return Tf_AtomicRenameFileOver(_tmpFilePath, _filePath, err);
}

bool
TfAtomicOfstreamWrapper::Cancel(
    string* reason)
{
    if (!_stream.is_open()) {
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

PXR_NAMESPACE_CLOSE_SCOPE
