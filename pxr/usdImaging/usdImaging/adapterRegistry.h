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
#ifndef PXR_USD_IMAGING_USD_IMAGING_ADAPTER_REGISTRY_H
#define PXR_USD_IMAGING_USD_IMAGING_ADAPTER_REGISTRY_H

/// \file usdImaging/adapterRegistry.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE


class UsdImagingPrimAdapter;
using UsdImagingPrimAdapterSharedPtr = std::shared_ptr<UsdImagingPrimAdapter>;

class UsdImagingAPISchemaAdapter;
using UsdImagingAPISchemaAdapterSharedPtr =
    std::shared_ptr<UsdImagingAPISchemaAdapter>;

#define USD_IMAGING_ADAPTER_KEY_TOKENS          \
    ((instanceAdapterKey, "__instanceAdapter")) \
    ((drawModeAdapterKey, "__drawModeAdapter"))       \

TF_DECLARE_PUBLIC_TOKENS(UsdImagingAdapterKeyTokens,
                         USDIMAGING_API,
                         USD_IMAGING_ADAPTER_KEY_TOKENS);

/// \class UsdImagingAdapterRegistry
///
/// Registry of PrimAdapter plug-ins. Note: this is a registry of adapter
/// factories, and not adapter instances; we expect to store adapter instances
/// (created via ConstructAdapter) with per-stage data.
///
class UsdImagingAdapterRegistry : public TfSingleton<UsdImagingAdapterRegistry> 
{
    friend class TfSingleton<UsdImagingAdapterRegistry>;
    UsdImagingAdapterRegistry();

    typedef std::unordered_map<TfToken,TfType,TfToken::HashFunctor> _TypeMap;
    _TypeMap _typeMap;
    TfTokenVector _adapterKeys;
    _TypeMap _apiSchemaTypeMap;
    TfTokenVector _apiSchemaAdapterKeys;

    template <typename T, typename factoryT>
    std::shared_ptr<T> _ConstructAdapter(
        TfToken const& adapterKey, const _TypeMap &tm);

public:

    /// Returns true if external plugins are enabled.
    /// Internal plugins have isInternal=1 set in their metadata. This flag is
    /// only intended to be set for critical imaging plugins  (mesh, cube,
    /// sphere, curve, etc). This allows users to disable plugins that are
    /// crashing or executing slowly.
    ///
    /// Driven by by the USDIMAGING_ENABLE_PLUGINS environment variable.
    USDIMAGING_API
    static bool AreExternalPluginsEnabled();

    USDIMAGING_API
    static UsdImagingAdapterRegistry& GetInstance() {
        return TfSingleton<UsdImagingAdapterRegistry>::GetInstance();
    }

    /// Returns true if an adapter has been registered to handle the given
    /// \p adapterKey.
    USDIMAGING_API
    bool HasAdapter(TfToken const& adapterKey);

    /// Returns a new instance of the UsdImagingPrimAdapter that has been
    /// registered to handle the given \p adapterKey. This key is either
    /// a prim typename or a key specified in UsdImagingAdapterKeyTokens.
    /// Returns NULL if no adapter was registered for this key.
    USDIMAGING_API
    UsdImagingPrimAdapterSharedPtr ConstructAdapter(TfToken const& adapterKey);

    /// Returns the set of adapter keys this class responds to; i.e. the set of
    /// usd prim types for which we've registered a prim adapter.
    USDIMAGING_API
    const TfTokenVector& GetAdapterKeys();

    /// Returns true if an api schema adapter has been registered to handle
    /// the given \p adapterKey.
    USDIMAGING_API
    bool HasAPISchemaAdapter(TfToken const& adapterKey);

    /// Returns a new instance of the UsdImagingAPISchemaAdapter that has been
    /// registered to handle the given \p adapterKey.
    /// Returns NULL if no adapter was registered for this key.
    USDIMAGING_API
    UsdImagingAPISchemaAdapterSharedPtr ConstructAPISchemaAdapter(
        TfToken const& adapterKey);

    /// Returns the set of api schema adapter keys this class responds to;
    /// i.e. the set of usd api schema types for which we've registered an
    /// adapter.
    USDIMAGING_API
    const TfTokenVector& GetAPISchemaAdapterKeys();


};

USDIMAGING_API_TEMPLATE_CLASS(TfSingleton<UsdImagingAdapterRegistry>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_IMAGING_USD_IMAGING_ADAPTER_REGISTRY_H
