//
// Copyright 2018 Pixar
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
#include "pxr/usd/ar/filesystemAsset.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/fileSystem.h"

PXR_NAMESPACE_OPEN_SCOPE

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
ArFilesystemAsset::GetSize()
{
    return ArchGetFileLength(_file);
}

std::shared_ptr<const char> 
ArFilesystemAsset::GetBuffer()
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
ArFilesystemAsset::Read(void* buffer, size_t count, size_t offset)
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
ArFilesystemAsset::GetFileUnsafe()
{
    return std::make_pair(_file, 0);
}

PXR_NAMESPACE_CLOSE_SCOPE
