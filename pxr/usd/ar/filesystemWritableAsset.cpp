//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/pxr.h"

#include "pxr/usd/ar/filesystemWritableAsset.h"

#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/safeOutputFile.h"

PXR_NAMESPACE_OPEN_SCOPE

std::shared_ptr<ArFilesystemWritableAsset>
ArFilesystemWritableAsset::Create(
    const ArResolvedPath& resolvedPath,
    ArResolver::WriteMode writeMode)
{
    const std::string dir = TfGetPathName(resolvedPath);
    // Call TfMakedirs with existOk = true so we don't fail if the directory is
    // created by another thread or process at the same time.
    if (!dir.empty() && !TfIsDir(dir) && !TfMakeDirs(dir, -1, true)) {
        TF_RUNTIME_ERROR(
            "Could not create directory '%s' for asset '%s'", 
            dir.c_str(), resolvedPath.GetPathString().c_str());
        return nullptr;
    }

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
