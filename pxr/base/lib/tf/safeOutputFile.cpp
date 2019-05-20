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
///
/// \file Tf/safeOutputFile.cpp
#include "pxr/base/tf/safeOutputFile.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/fileSystem.h"

#include "pxr/base/tf/atomicRenameUtil.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"

#if defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <io.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

TfSafeOutputFile::~TfSafeOutputFile()
{
    Close();
}

bool
TfSafeOutputFile::IsOpenForUpdate() const
{
    return _file && _tempFileName.empty();
}

FILE *
TfSafeOutputFile::ReleaseUpdatedFile()
{
    if (!IsOpenForUpdate()) {
        TF_CODING_ERROR("Invalid output file (failed to open, or opened for "
                        "replace)");
        return nullptr;
    }
    FILE *ret = _file;
    _file = nullptr;
    _tempFileName.clear();
    _targetFileName.clear();
    return ret;
}

void
TfSafeOutputFile::Close()
{
    if (!_file)
        return;

    // Close the file.
    fclose(_file);
    _file = nullptr;

    // If this was for update, we have nothing else to do.
    if (_tempFileName.empty())
        return;

    std::string error;
    if (!Tf_AtomicRenameFileOver(_tempFileName, _targetFileName, &error)) {
        TF_RUNTIME_ERROR(error);
    }

    _tempFileName.clear();
    _targetFileName.clear();
}

void 
TfSafeOutputFile::Discard()
{
    if (IsOpenForUpdate()) {
        TF_CODING_ERROR("Invalid output file (failed to open, or opened for "
                        "update)");
        return;
    }

    // Move _tempFileName aside so that Close() will not rename
    // the temporary file to the final destination.
    std::string tempFileToRemove;
    tempFileToRemove.swap(_tempFileName);
    Close();

    if (!tempFileToRemove.empty()) {
        TfDeleteFile(tempFileToRemove);
    }
}

TfSafeOutputFile
TfSafeOutputFile::Update(std::string const &fileName)
{
    TfSafeOutputFile result;
    result._targetFileName = fileName;
    FILE *file = ArchOpenFile(fileName.c_str(), "rb+");
    if (!file) {
        TF_RUNTIME_ERROR("Unable to open file '%s' for writing",
                         fileName.c_str());
        return result;
    }
    result._file = file;
    return result;
}

TfSafeOutputFile
TfSafeOutputFile::Replace(std::string const &fileName)
{
    TfSafeOutputFile result;
    std::string error;
    int tmpFd = Tf_CreateSiblingTempFile(fileName,
                                         &result._targetFileName,
                                         &result._tempFileName,
                                         &error);
    if (tmpFd == -1) {
        TF_RUNTIME_ERROR(error);
        return result;
    }

    // Obtain a FILE *.
    result._file = ArchFdOpen(tmpFd, "wb");
    if (!result._file) {
        TF_RUNTIME_ERROR("Unable to obtain writable FILE pointer: %s",
                         ArchStrerror(errno).c_str());
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
