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
#ifndef USD_ADAPTER_REGISTRY_H
#define USD_ADAPTER_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class TfType;
class UsdImagingPrimAdapter;
typedef boost::shared_ptr<UsdImagingPrimAdapter> UsdImagingPrimAdapterSharedPtr;

#define USD_IMAGING_ADAPTER_KEY_TOKENS          \
    ((instanceAdapterKey, "__instanceAdapter")) \
    ((drawModeAdapterKey, "__drawModeAdapter"))       \

TF_DECLARE_PUBLIC_TOKENS(UsdImagingAdapterKeyTokens,
                         USDIMAGING_API,
                         USD_IMAGING_ADAPTER_KEY_TOKENS);

/// \class UsdImagingAdapterRegistry
///
/// Registry of PrimAdapter plug-ins.
///
class UsdImagingAdapterRegistry : public TfSingleton<UsdImagingAdapterRegistry> 
{
    friend class TfSingleton<UsdImagingAdapterRegistry>;
    UsdImagingAdapterRegistry();

    typedef boost::unordered_map<TfToken,TfType,TfToken::HashFunctor> _TypeMap;
    _TypeMap _typeMap;

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

    /// Returns a new instance of the UsdImagingPrimAdapter that has been
    /// registered to handle the given \p adapterKey. This key is either
    /// a prim typename or a key specified in UsdImagingAdapterKeyTokens.
    /// Returns NULL if no adapter was registered for this key.
    USDIMAGING_API
    UsdImagingPrimAdapterSharedPtr ConstructAdapter(TfToken const& adapterKey);
};

USDIMAGING_API_TEMPLATE_CLASS(TfSingleton<UsdImagingAdapterRegistry>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //USD_ADAPTER_REGISTRY_H
