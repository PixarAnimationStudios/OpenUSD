//
// Copyright 2021 Pixar
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

#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/writableAsset.h"
#include "pxr/base/vt/value.h"

#include <memory>
#include <string>

/// \class UsdResolverExampleResolver
///
/// Example URI resolver supporting asset paths of the form:
///     asset:<asset_name>/<path_to_file>
///
class UsdResolverExampleResolver final
    : public PXR_NS::ArResolver
{
public:
    UsdResolverExampleResolver();
    virtual ~UsdResolverExampleResolver();

protected:
    std::string _CreateIdentifier(
        const std::string& assetPath,
        const PXR_NS::ArResolvedPath& anchorAssetPath) const final;

    std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const PXR_NS::ArResolvedPath& anchorAssetPath) const final;

    PXR_NS::ArResolvedPath _Resolve(
        const std::string& assetPath) const final;

    PXR_NS::ArResolvedPath _ResolveForNewAsset(
        const std::string& assetPath) const final;

    PXR_NS::ArResolverContext _CreateDefaultContext() const final;

    PXR_NS::ArResolverContext _CreateDefaultContextForAsset(
        const std::string& assetPath) const final;

    PXR_NS::ArResolverContext _CreateContextFromString(
        const std::string& contextStr) const final;

    bool _IsContextDependentPath(
        const std::string& assetPath) const final;

    void _RefreshContext(
        const PXR_NS::ArResolverContext& context) final;

    PXR_NS::ArTimestamp _GetModificationTimestamp(
        const std::string& assetPath,
        const PXR_NS::ArResolvedPath& resolvedPath) const final;

    PXR_NS::ArAssetInfo _GetAssetInfo(
        const std::string& assetPath,
        const PXR_NS::ArResolvedPath& resolvedPath) const final;

    std::shared_ptr<PXR_NS::ArAsset> _OpenAsset(
        const PXR_NS::ArResolvedPath& resolvedPath) const final;

    std::shared_ptr<PXR_NS::ArWritableAsset>
    _OpenAssetForWrite(
        const PXR_NS::ArResolvedPath& resolvedPath,
        WriteMode writeMode) const final;

private:
    PXR_NS::ArResolvedPath _ResolveHelper(
        const std::string& assetPath,
        bool forNewAsset) const;
    
};
