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

#include "pxr/pxr.h"
#include "usdKatana/usdInPrivateData.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/base/tf/type.h"

#include <FnGeolib/op/FnGeolibOp.h>

PXR_NAMESPACE_OPEN_SCOPE

class PxrUsdKatanaUtilsLightListAccess;

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
    static void RegisterKind(
            const TfToken& kind,
            const std::string& opName);

    /// \brief Registers \p opName to extend or override \p the core op for 
    /// kind (and possibly other kinds that are descendents of \p 
    /// kind in the kind hierarchy).
    static void RegisterKindForSite(
            const TfToken& kind,
            const std::string& opName);

    /// \brief Returns true if there are any site-specific ops registered
    /// for at least one \p kind.
    static bool HasKindsForSite();

    /// \brief Finds a reader if one exists for \p usdTypeName.
    ///
    /// \p usdTypeName should be a usd typeName, for example, 
    /// \code
    /// usdPrim.GetTypeName()
    /// \endcode
    static bool FindUsdType(
            const TfToken& usdTypeName,
            std::string* opName);

    /// \brief Finds a site-specific reader if one exists for \p usdTypeName.
    ///
    /// \p usdTypeName should be a usd typeName, for example, 
    /// \code
    /// usdPrim.GetTypeName()
    /// \endcode
    static bool FindUsdTypeForSite(
            const TfToken& usdTypeName,
            std::string* opName);

    /// \brief Finds a reader if one exists for \p kind.  This will walk up the
    /// kind hierarchy and find the nearest applicable one.
    static bool FindKind(
            const TfToken& kind,
            std::string* opName);

    /// \brief Finds a reader that extends or overrides the core op, if one 
    /// exists, for \p kind.  This will walk up the kind hierarchy and find the 
    /// nearest applicable one.
    static bool FindKindForSite(
            const TfToken& kind,
            std::string* opName);


    /// \brief The signature for a plug-in "light list" function.
    /// These functions are called for each light path.  The
    /// argument allows for building the Katana light list.
    typedef void (*LightListFnc)(PxrUsdKatanaUtilsLightListAccess&);

    /// \brief Register a plug-in function to be called at a light path.
    /// This allows for modifying the Katana light list.  It should set
    /// the entry, links, and initial enabled status.  (The linking
    /// resolver does not necessarily run at the location where this
    /// function is run so the function needs to establish the initial
    /// enabled status correctly.)
    static void RegisterLightListFnc(LightListFnc);

    /// \brief Run the registered plug-in light list functions at a light
    /// path. This allows for modifying the Katana light list.
    static void ExecuteLightListFncs(
                    PxrUsdKatanaUtilsLightListAccess& access);
    
    
    
    
    
    typedef void (*OpDirectExecFnc)(
            const PxrUsdKatanaUsdInPrivateData& privateData,
            FnKat::GroupAttribute opArgs,
            FnKat::GeolibCookInterface& interface);
    
    /// \brief Makes an PxrUsdIn kind./type op's cook function available for
    ///        to invoke directly without execOp. This is to allow for
    ///        privateData to be locally overriden in a way that's not directly
    ///        possible via execOp in katana 2.x. While possible in katana 3.x,
    ///        this technique has slightly less overhead and remains compatible
    ///        between versions
    ///        NOTE: This is normally not necessary to call directly as it's
    ///              handled as part of the USD_OP_REGISTER_PLUGIN used to
    ///              define the op.
    static void RegisterOpDirectExecFnc(
           const std::string& opName,
           OpDirectExecFnc fnc);
    
    /// \brief Directly invoke the cook method of a PxrUsdIn extension op
    ///        Ops called in this manner should retrieve op arguments and
    ///        and private data not from the interface but from their function
    ///        parameters. This is to allow either to be locally overriden
    ///        without the overhead or limitations (in 2.x) of execOp
    static void ExecuteOpDirectExecFnc(
            const std::string& opName,
            const PxrUsdKatanaUsdInPrivateData& privateData,
            FnKat::GroupAttribute opArgs,
            FnKat::GeolibCookInterface& interface);
    
    
    
    
    /// \brief Register an op name which will be called for every
    /// katana location created from a UsdPrim. This allows for specialization
    /// beyond specific types and kinds. The specific op must have been
    /// previously registered with RegisterOpDirectExecFnc -- which will
    /// happen automatically for any op defined with one of the PXRUSDKATANA_*
    /// macros and registered via USD_OP_REGISTER_PLUGIN.
    static void RegisterLocationDecoratorOp(const std::string& opName);
    
    
    // \brief Run the registered plug-in ops at a katana location
    /// and UsdPrim. It returns opArgs -- which may be altered by the executed
    /// ops.
    static FnKat::GroupAttribute ExecuteLocationDecoratorOps(
            const PxrUsdKatanaUsdInPrivateData& privateData,
            FnKat::GroupAttribute opArgs,
            FnKat::GeolibCookInterface& interface);


private:
    static void _RegisterUsdType(
            const std::string& tfTypeName, 
            const std::string& opName);

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
PXR_NAMESPACE_USING_DIRECTIVE \
class T : public FnKat::GeolibOp\
{\
public:\
    static void setup(FnKat::GeolibSetupInterface& interface);\
    static void cook(FnKat::GeolibCookInterface& interface);\
    static void directExec(const PxrUsdKatanaUsdInPrivateData& privateData, \
            FnKat::GroupAttribute opArgs, \
            Foundry::Katana::GeolibCookInterface &interface);\
};\

/// \def PXRUSDKATANA_USDIN_PLUGIN_DEFINE(T, argsName, interfaceName) 
/// \brief Defines a plugin of opType T.  
#define PXRUSDKATANA_USDIN_PLUGIN_DEFINE(T, argsName, opArgsName, interfaceName) \
void T::setup(FnKat::GeolibSetupInterface& interface) {\
    interface.setThreading(FnKat::GeolibSetupInterface::ThreadModeConcurrent);\
}\
void _PxrUsdKatana_PrimReaderFn_##T(\
        const PxrUsdKatanaUsdInPrivateData&,\
        FnKat::GroupAttribute, \
        Foundry::Katana::GeolibCookInterface &);\
void T::cook(FnKat::GeolibCookInterface& interface) \
{\
    if (PxrUsdKatanaUsdInPrivateData* args \
            = PxrUsdKatanaUsdInPrivateData::GetPrivateData(interface)) {\
        _PxrUsdKatana_PrimReaderFn_##T(*args, interface.getOpArg(), interface);\
    }\
}\
void T::directExec(const PxrUsdKatanaUsdInPrivateData& privateData, \
            FnKat::GroupAttribute opArgs, \
            Foundry::Katana::GeolibCookInterface &interface)\
{\
    _PxrUsdKatana_PrimReaderFn_##T(privateData, opArgs, interface);\
}\
void _PxrUsdKatana_PrimReaderFn_##T(\
        const PxrUsdKatanaUsdInPrivateData& argsName,\
        FnKat::GroupAttribute opArgsName,\
        Foundry::Katana::GeolibCookInterface &interfaceName )\


/// \def PXRUSDKATANA_USDIN_PLUGIN_DECLARE_WITH_FLUSH(T)
/// \brief Declares a plugin of opType which also includes a flush function T.
#define PXRUSDKATANA_USDIN_PLUGIN_DECLARE_WITH_FLUSH(T) \
PXR_NAMESPACE_USING_DIRECTIVE \
class T : public FnKat::GeolibOp\
{\
public:\
    static void setup(FnKat::GeolibSetupInterface& interface);\
    static void cook(FnKat::GeolibCookInterface& interface);\
    static void flush();\
    static void directExec(const PxrUsdKatanaUsdInPrivateData& privateData, \
            FnKat::GroupAttribute opArgs, \
            Foundry::Katana::GeolibCookInterface &interface);\
};\

/// \def PXRUSDKATANA_USDIN_PLUGIN_DEFINE_WITH_FLUSH(T, argsName, interfaceName) 
/// \brief Defines a plugin of opType T with inclusion of a flush function.  
#define PXRUSDKATANA_USDIN_PLUGIN_DEFINE_WITH_FLUSH(T, argsName, opArgsName, interfaceName, flushFnc) \
void T::setup(FnKat::GeolibSetupInterface& interface) {\
    interface.setThreading(FnKat::GeolibSetupInterface::ThreadModeConcurrent);\
}\
void T::flush() {\
    flushFnc();\
}\
void _PxrUsdKatana_PrimReaderFn_##T(\
        const PxrUsdKatanaUsdInPrivateData&,\
        FnKat::GroupAttribute, \
        Foundry::Katana::GeolibCookInterface &);\
void T::cook(FnKat::GeolibCookInterface& interface) \
{\
    if (PxrUsdKatanaUsdInPrivateData* args \
            = PxrUsdKatanaUsdInPrivateData::GetPrivateData(interface)) {\
        _PxrUsdKatana_PrimReaderFn_##T(*args, interface.getOpArg(), interface);\
    }\
}\
void T::directExec(const PxrUsdKatanaUsdInPrivateData& privateData, \
            FnKat::GroupAttribute opArgs,\
            Foundry::Katana::GeolibCookInterface &interface)\
{\
    _PxrUsdKatana_PrimReaderFn_##T(privateData, opArgs, interface);\
}\
void _PxrUsdKatana_PrimReaderFn_##T(\
        const PxrUsdKatanaUsdInPrivateData& argsName,\
        FnKat::GroupAttribute opArgsName, \
        Foundry::Katana::GeolibCookInterface &interfaceName )\

/// \brief Equivalent of the standard REGISTER_PLUGIN with additional
///        registration in service direct execution.
#define USD_OP_REGISTER_PLUGIN(PLUGIN_CLASS, PLUGIN_NAME, PLUGIN_MAJOR_VERSION, PLUGIN_MINOR_VERSION) \
    REGISTER_PLUGIN(PLUGIN_CLASS, PLUGIN_NAME, PLUGIN_MAJOR_VERSION, PLUGIN_MINOR_VERSION) \
    PxrUsdKatanaUsdInPluginRegistry::RegisterOpDirectExecFnc(PLUGIN_NAME, PLUGIN_CLASS::directExec);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDKATANA_USDIN_PLUGINREGISTRY_H
