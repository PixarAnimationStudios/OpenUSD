//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/pxr.h"
#include "pxr/usd/ar/filesystemAsset.h"
#include "pxr/usd/ar/resolvedPath.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/fileSystem.h"

PXR_NAMESPACE_OPEN_SCOPE

std::shared_ptr<ArFilesystemAsset>
ArFilesystemAsset::Open(const ArResolvedPath& resolvedPath)
{
    FILE* f = ArchOpenFile(resolvedPath.GetPathString().c_str(), "rb");
    if (!f) {
        return nullptr;
    }

    return std::make_shared<ArFilesystemAsset>(f);
}

ArTimestamp
ArFilesystemAsset::GetModificationTimestamp(const ArResolvedPath& resolvedPath)
{
    double time;
    if (ArchGetModificationTime(resolvedPath.GetPathString().c_str(), &time)) {
        return ArTimestamp(time);
    }
    return ArTimestamp();
}

ArFilesystemAsset::ArFilesystemAsset(FILE* file) 
    : _file(file) 
{ 
    if (!_file) {
        TF_CODING_ERROR("Invalid file handle");
    }
}

ArFilesystemAsset::~ArFilesystemAsset() 
{ 
    fclose(_file); 
}

size_t
ArFilesystemAsset::GetSize() const
{
    return ArchGetFileLength(_file);
}

std::shared_ptr<const char> 
ArFilesystemAsset::GetBuffer() const
{
    ArchConstFileMapping mapping = ArchMapFileReadOnly(_file);
    if (!mapping) {
        return nullptr;
    }

    struct _Deleter {
        explicit _Deleter(ArchConstFileMapping&& mapping) 
            : _mapping(new ArchConstFileMapping(std::move(mapping)))
        { }

        void operator()(const char* b)
        {
            _mapping.reset();
        }
        
        std::shared_ptr<ArchConstFileMapping> _mapping;
    };

    const char* buffer = mapping.get();
    return std::shared_ptr<const char>(buffer, _Deleter(std::move(mapping)));
}

size_t
ArFilesystemAsset::Read(void* buffer, size_t count, size_t offset) const
{
    int64_t numRead = ArchPRead(_file, buffer, count, offset);
    if (numRead == -1) {
        TF_RUNTIME_ERROR(
            "Error occurred reading file: %s", ArchStrerror().c_str());
        return 0;
    }
    return numRead;
}
        
std::pair<FILE*, size_t>
ArFilesystemAsset::GetFileUnsafe() const
{
    return std::make_pair(_file, 0);
}

PXR_NAMESPACE_CLOSE_SCOPE
