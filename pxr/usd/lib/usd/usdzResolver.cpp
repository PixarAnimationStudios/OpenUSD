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
//
#include "pxr/pxr.h"

#include "pxr/usd/usd/usdzResolver.h"
#include "pxr/usd/usd/zipFile.h"

#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/definePackageResolver.h"
#include "pxr/usd/ar/resolver.h"

#include <tbb/concurrent_hash_map.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_PACKAGE_RESOLVER(Usd_UsdzResolver, ArPackageResolver);

Usd_UsdzResolver::Usd_UsdzResolver()
{
}

struct Usd_UsdzResolver::_Cache
{
    using _Map = tbb::concurrent_hash_map<std::string, _AssetAndZipFile>;
    _Map _pathToEntryMap;
};

void 
Usd_UsdzResolver::BeginCacheScope(
    VtValue* cacheScopeData)
{
    _caches.BeginCacheScope(cacheScopeData);
}

void
Usd_UsdzResolver::EndCacheScope(
    VtValue* cacheScopeData)
{
    _caches.EndCacheScope(cacheScopeData);
}

Usd_UsdzResolver::_CachePtr 
Usd_UsdzResolver::_GetCurrentCache()
{
    return _caches.GetCurrentCache();
}

Usd_UsdzResolver::_AssetAndZipFile
Usd_UsdzResolver::_OpenZipFile(const std::string& path)
{
    _AssetAndZipFile result;
    result.first = ArGetResolver().OpenAsset(path);
    if (result.first) {
        result.second = UsdZipFile::Open(result.first);
    }
    return result;
}

Usd_UsdzResolver::_AssetAndZipFile 
Usd_UsdzResolver::_FindOrOpenZipFile(const std::string& packagePath)
{
    _CachePtr currentCache = _GetCurrentCache();
    if (currentCache) {
        _Cache::_Map::accessor accessor;
        if (currentCache->_pathToEntryMap.insert(
                accessor, std::make_pair(packagePath, _AssetAndZipFile()))) {
            accessor->second = _OpenZipFile(packagePath);
        }
        return accessor->second;
    }

    return  _OpenZipFile(packagePath);
}

std::string 
Usd_UsdzResolver::Resolve(
    const std::string& packagePath,
    const std::string& packagedPath)
{
    std::shared_ptr<ArAsset> asset;
    UsdZipFile zipFile;
    std::tie(asset, zipFile) = _FindOrOpenZipFile(packagePath);

    if (!zipFile) {
        return std::string();
    }
    return zipFile.Find(packagedPath) != zipFile.end() ? 
        packagedPath : std::string();
}

namespace
{

class _Asset
    : public ArAsset
{
private:
    std::shared_ptr<ArAsset> _sourceAsset;
    UsdZipFile _zipFile;
    const char* _dataInZipFile;
    size_t _offsetInZipFile;
    size_t _sizeInZipFile;

public:
    explicit _Asset(std::shared_ptr<ArAsset>&& sourceAsset,
                    UsdZipFile&& zipFile,
                    const char* dataInZipFile,
                    size_t offsetInZipFile,
                    size_t sizeInZipFile)
        : _sourceAsset(std::move(sourceAsset))
        , _zipFile(std::move(zipFile))
        , _dataInZipFile(dataInZipFile)
        , _offsetInZipFile(offsetInZipFile)
        , _sizeInZipFile(sizeInZipFile)
    {
    }

    virtual size_t GetSize() override
    {
        return _sizeInZipFile;
    }

    virtual std::shared_ptr<const char> GetBuffer() override
    {
        struct _Deleter
        {
            void operator()(const char* b)
            {
                zipFile = UsdZipFile();
            }
            UsdZipFile zipFile;
        };

        _Deleter d;
        d.zipFile = _zipFile;

        return std::shared_ptr<const char>(_dataInZipFile, d);
    }

    virtual size_t Read(void* buffer, size_t count, size_t offset)
    {
        memcpy(buffer, _dataInZipFile + offset, count);
        return count;
    }
    
    virtual std::pair<FILE*, size_t> GetFileUnsafe() override
    {
        std::pair<FILE*, size_t> result = _sourceAsset->GetFileUnsafe();
        if (result.first) {
            result.second += _offsetInZipFile;
        }
        return result;
    }
};

} // end anonymous namespace

std::shared_ptr<ArAsset> 
Usd_UsdzResolver::OpenAsset(
    const std::string& packagePath,
    const std::string& packagedPath)
{
    std::shared_ptr<ArAsset> asset;
    UsdZipFile zipFile;
    std::tie(asset, zipFile) = _FindOrOpenZipFile(packagePath);

    if (!zipFile) {
        return nullptr;
    }

    auto iter = zipFile.Find(packagedPath);
    if (iter == zipFile.end()) {
        return nullptr;
    }

    const UsdZipFile::FileInfo info = iter.GetFileInfo();
    return std::shared_ptr<ArAsset>(
        new _Asset(
            std::move(asset), std::move(zipFile),
            iter.GetFile(), info.dataOffset, info.size));
}

PXR_NAMESPACE_CLOSE_SCOPE
