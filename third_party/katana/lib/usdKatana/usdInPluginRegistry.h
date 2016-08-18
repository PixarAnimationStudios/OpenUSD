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
#ifndef PXRUSDKATANA_USDIN_PLUGINREGISTRY_H
#define PXRUSDKATANA_USDIN_PLUGINREGISTRY_H

#include "usdKatana/api.h"
#include "usdKatana/usdInPrivateData.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/base/tf/type.h"

#include <FnGeolib/op/FnGeolibOp.h>

/// \brief Maintains the registry for usd types and kind.
class PxrUsdKatanaUsdInPluginRegistry
{
public:
    /// \brief Register \p opName to handle the usd type \T.
    template <typename T>
    static void RegisterUsdType(
            const std::string& opName)
    {
        if (TfType tfType = TfType::Find<T>()) {
            _RegisterUsdType(tfType.GetTypeName(), opName);
        }
        else {
            TF_CODING_ERROR("Could not find type.");
        }
    }

    /// \brief Register \p site-specific opName to handle the usd type \T.
    template <typename T>
    static void RegisterUsdTypeForSite(
            const std::string& opName)
    {
        if (TfType tfType = TfType::Find<T>()) {
            _RegisterUsdTypeForSite(tfType.GetTypeName(), opName);
        }
        else {
            TF_CODING_ERROR("Could not find type.");
        }
    }

    /// \brief Register \p opName to handle the prims with an unknown usd type \T.
    static void RegisterUnknownUsdType(const std::string& opName)
    {
        _RegisterUsdType(TfType::GetUnknownType().GetTypeName(), opName);
    }

    /// \brief Registers \p opName to handle \p kind (and possibly other kinds
    /// that are descendents of \p kind in the kind hierarchy).
    USDKATANA_API
    static void RegisterKind(
            const TfToken& kind,
            const std::string& opName);

    /// \brief Registers \p opName to extend or override \p the core op for 
    /// kind (and possibly other kinds that are descendents of \p 
    /// kind in the kind hierarchy).
    USDKATANA_API
    static void RegisterKindForSite(
            const TfToken& kind,
            const std::string& opName);

    /// \brief Returns true if there are any site-specific ops registered
    /// for at least one \p kind.
    USDKATANA_API
    static bool HasKindsForSite();

    /// \brief Finds a reader if one exists for \p usdTypeName.
    ///
    /// \p usdTypeName should be a usd typeName, for example, 
    /// \code
    /// usdPrim.GetTypeName()
    /// \endcode
    USDKATANA_API
    static bool FindUsdType(
            const TfToken& usdTypeName,
            std::string* opName);

    /// \brief Finds a site-specific reader if one exists for \p usdTypeName.
    ///
    /// \p usdTypeName should be a usd typeName, for example, 
    /// \code
    /// usdPrim.GetTypeName()
    /// \endcode
    USDKATANA_API
    static bool FindUsdTypeForSite(
            const TfToken& usdTypeName,
            std::string* opName);

    /// \brief Finds a reader if one exists for \p kind.  This will walk up the
    /// kind hierarchy and find the nearest applicable one.
    USDKATANA_API
    static bool FindKind(
            const TfToken& kind,
            std::string* opName);

    /// \brief Finds a reader that extends or overrides the core op, if one 
    /// exists, for \p kind.  This will walk up the kind hierarchy and find the 
    /// nearest applicable one.
    USDKATANA_API
    static bool FindKindForSite(
            const TfToken& kind,
            std::string* opName);

private:
    USDKATANA_API
    static void _RegisterUsdType(
            const std::string& tfTypeName, 
            const std::string& opName);

    USDKATANA_API
    static void _RegisterUsdTypeForSite(
            const std::string& tfTypeName, 
            const std::string& opName);

    static bool _DoFindKind(
        const TfToken& kind,
        std::string* opName,
        const std::map<TfToken, std::string>& reg);

};

/// \def PXRUSDKATANA_USDIN_PLUGIN_DECLARE(T)
/// \brief Declares a plugin of opType T.
#define PXRUSDKATANA_USDIN_PLUGIN_DECLARE(T) \
class T : public FnKat::GeolibOp\
{\
public:\
    static void setup(FnKat::GeolibSetupInterface& interface);\
    static void cook(FnKat::GeolibCookInterface& interface);\
};\

/// \def PXRUSDKATANA_USDIN_PLUGIN_DEFINE(T, argsName, interfaceName) 
/// \brief Defines a plugin of opType T.  
#define PXRUSDKATANA_USDIN_PLUGIN_DEFINE(T, argsName, interfaceName) \
void T::setup(FnKat::GeolibSetupInterface& interface) {\
    interface.setThreading(FnKat::GeolibSetupInterface::ThreadModeConcurrent);\
}\
void _PxrUsdKatana_PrimReaderFn_##T(\
        const PxrUsdKatanaUsdInPrivateData&,\
        Foundry::Katana::GeolibCookInterface &);\
void T::cook(FnKat::GeolibCookInterface& interface) \
{\
    if (PxrUsdKatanaUsdInPrivateData* args \
            = static_cast<PxrUsdKatanaUsdInPrivateData*>(interface.getPrivateData())) {\
        _PxrUsdKatana_PrimReaderFn_##T(*args, interface);\
    }\
}\
void _PxrUsdKatana_PrimReaderFn_##T(\
        const PxrUsdKatanaUsdInPrivateData& argsName,\
        Foundry::Katana::GeolibCookInterface &interfaceName )\

#endif // PXRUSDKATANA_USDIN_PLUGINREGISTRY_H
