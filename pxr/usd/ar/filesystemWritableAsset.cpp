//
// Copyright 2020 Pixar
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

#include "pxr/pxr.h"

#include "pxr/usd/ar/filesystemWritableAsset.h"

#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/safeOutputFile.h"

PXR_NAMESPACE_OPEN_SCOPE

std::shared_ptr<ArFilesystemWritableAsset>
ArFilesystemWritableAsset::Create(
    const ArResolvedPath& resolvedPath,
    ArResolver::WriteMode writeMode)
{
    TfErrorMark m;

    TfSafeOutputFile f;
    switch (writeMode) {
    case ArResolver::WriteMode::Update:
        f = TfSafeOutputFile::Update(resolvedPath);
        break;
    case ArResolver::WriteMode::Replace:
        f = TfSafeOutputFile::Replace(resolvedPath);
        break;
    }

    if (!m.IsClean()) {
        return nullptr;
    }

    return std::make_shared<ArFilesystemWritableAsset>(std::move(f));
}

ArFilesystemWritableAsset::ArFilesystemWritableAsset(TfSafeOutputFile&& file)
    : _file(std::move(file))
{
    if (!_file.Get()) {
        TF_CODING_ERROR("Invalid output file");
    }
}

ArFilesystemWritableAsset::~ArFilesystemWritableAsset() = default;

bool
ArFilesystemWritableAsset::Close()
{
    TfErrorMark m;
    _file.Close();
    return m.IsClean();
}

size_t
ArFilesystemWritableAsset::Write(
    const void* buffer, size_t count, size_t offset)
{
    int64_t numWritten = ArchPWrite(_file.Get(), buffer, count, offset);
    if (numWritten == -1) {
        TF_RUNTIME_ERROR(
            "Error occurred writing file: %s", ArchStrerror().c_str());
        return 0;
    }
    return numWritten;
}

PXR_NAMESPACE_CLOSE_SCOPE
