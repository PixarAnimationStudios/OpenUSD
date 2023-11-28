//
// Copyright 2023 Pixar
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
/// \file usdUtils/localizeAsset.cpp

#include "pxr/pxr.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/writableAsset.h"

#include "pxr/usd/sdf/assetPath.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"

#include "pxr/usd/usdUtils/assetLocalization.h"
#include "pxr/usd/usdUtils/assetLocalizationDelegate.h"
#include "pxr/usd/usdUtils/assetLocalizationPackage.h"
#include "pxr/usd/usdUtils/localizeAsset.h"
#include "pxr/usd/usdUtils/debugCodes.h"

#include "pxr/base/trace/trace.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

class UsdUtils_LocalizedAssetBuilder : public UsdUtils_AssetLocalizationPackage
{
public:
    virtual bool 
    Write(
        const std::string &localizationRoot) override
    {
        _localizationRoot = localizationRoot;

        return UsdUtils_AssetLocalizationPackage::Write(_localizationRoot);
    }

protected:
    virtual bool 
    _WriteToPackage(
        const std::string &src, 
        const std::string &dest) override
    {
        auto& resolver = ArGetResolver();

        const std::string destPath = TfStringCatPaths(_localizationRoot, dest);
        ArResolvedPath srcResolvedPath = resolver.Resolve(src);
        ArResolvedPath destResolvedPath = resolver.ResolveForNewAsset(destPath);

        if (srcResolvedPath.empty()) {
            TF_WARN("Failed to resolve source path: %s", src.c_str());
            
            return false;
        }

        if (destResolvedPath.empty()) {
            TF_WARN("Failed to resolve source path: %s", dest.c_str());
            
            return false;
        }

        auto sourceAsset = resolver.OpenAsset(srcResolvedPath);
        auto destAsset = resolver.OpenAssetForWrite(
                destResolvedPath, ArResolver::WriteMode::Replace);

        if (!sourceAsset) {
            TF_WARN("Failed to open source asset: %s", src.c_str());
            return false;
        }

        if (!destAsset) {
            TF_WARN("Failed to open destination asset: %s", dest.c_str());
            return false;
        }

        constexpr size_t COPY_BUFFER_SIZE = 4096U;
        char buffer[COPY_BUFFER_SIZE];
        size_t dataRemaining = sourceAsset->GetSize();

        while (dataRemaining > 0) {
            size_t chunkSize = std::min(dataRemaining, COPY_BUFFER_SIZE);

            sourceAsset->Read(buffer, chunkSize, 0);
            destAsset->Write(buffer, chunkSize, 0);

            dataRemaining -= chunkSize;
        }

        return true;
    }

    std::string _localizationRoot;
};

bool
UsdUtilsLocalizeAsset(
    const SdfAssetPath& assetPath,
    const std::string& localizationDir,
    bool editLayersInPlace,
    std::function<UsdUtilsProcessingFunc> processingFunc)
{
    TRACE_FUNCTION();

    if (TfPathExists(localizationDir) && !TfIsDir(localizationDir)) {
        TF_CODING_ERROR("Unable to localize to non directory path: %s", 
            localizationDir.c_str());
        return false;
    }

    UsdUtils_LocalizedAssetBuilder builder;

    builder.SetEditLayersInPlace(editLayersInPlace);
    builder.SetUserProcessingFunc(processingFunc);

    if (!builder.Build(assetPath, std::string())) {
        return false;
    }

    return builder.Write(localizationDir);
}

PXR_NAMESPACE_CLOSE_SCOPE
