//
// Copyright 2017 Pixar
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

#include "pxr/usd/usdGeom/boundableComputeExtent.h"
#include "pxr/usd/usdGeom/boundable.h"

#include "pxr/base/js/value.h"
#include "pxr/base/plug/notice.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/weakBase.h"

#include <tbb/queuing_rw_mutex.h>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

class _FunctionRegistry
    : public TfWeakBase
{
public:
    static _FunctionRegistry& GetInstance() 
    {
        return TfSingleton<_FunctionRegistry>::GetInstance();
    }

    _FunctionRegistry()
    {
        // Calling SubscribeTo may cause functions to be registered
        // while we're still in the c'tor, so make sure to call
        // SetInstanceConstructed to allow reentrancy.
        TfSingleton<_FunctionRegistry>::SetInstanceConstructed(*this);
        TfRegistryManager::GetInstance().SubscribeTo<UsdGeomBoundable>();

        // Register for new plugins being registered so we can invalidate
        // this registry.
        TfNotice::Register(
            TfCreateWeakPtr(this), 
            &_FunctionRegistry::_DidRegisterPlugins);
    }

    void 
    RegisterComputeFunction(
        const TfType& schemaType, 
        const UsdGeomComputeExtentFunction& fn)
    {
        bool didInsert = false;
        {
            _RWMutex::scoped_lock lock(_mutex, /* write = */ true);
            didInsert = _registry.emplace(schemaType, fn).second;
        }
        
        if (!didInsert) {
            TF_CODING_ERROR(
                "UsdGeomComputeExtentFunction already registered for "
                "prim type '%s'", schemaType.GetTypeName().c_str());
        }
    }

    UsdGeomComputeExtentFunction
    GetComputeFunction(const UsdPrim& prim)
    {
        static const TfType schemaBaseType = TfType::Find<UsdSchemaBase>();
        const TfType primSchemaType = schemaBaseType.FindDerivedByName(
            prim.GetTypeName().GetString());
        if (!primSchemaType) {
            TF_CODING_ERROR(
                "Could not find prim type '%s' for prim %s",
                prim.GetTypeName().GetText(), UsdDescribe(prim).c_str());
            return nullptr;
        }

        UsdGeomComputeExtentFunction fn = nullptr;
        if (_FindFunctionForType(primSchemaType, &fn)) {
            return fn;
        }

        const std::vector<TfType> primSchemaTypeAndBases = 
            _GetTypesThatMayHaveRegisteredFunctions(primSchemaType);

        auto i = primSchemaTypeAndBases.cbegin();
        for (auto e = primSchemaTypeAndBases.cend(); i != e; ++i) {
            const TfType& type = *i;
            if (_FindFunctionForType(type, &fn)) {
                break;
            }

            if (_LoadPluginForType(type)) {
                // If we loaded the plugin for this type, a new function may
                // have been registered so look again.
                if (_FindFunctionForType(type, &fn)) {
                    break;
                }
            }
        }

        // fn should point to the function to use for all types in the
        // range [primSchemaTypeAndBases.begin(), i). Note it may
        // also be nullptr if no function was found; we cache this
        // as well to avoid looking it up again.
        _RWMutex::scoped_lock lock(_mutex, /* write = */ true);
        for (auto it = primSchemaTypeAndBases.cbegin(); it != i; ++it) {
            _registry.emplace(*it, fn);
        }

        return fn;
    }

private:
    // Return a list of TfTypes that should be examined to find a compute
    // function for the given type.
    std::vector<TfType> 
    _GetTypesThatMayHaveRegisteredFunctions(const TfType& type) const
    {
        std::vector<TfType> result;
        type.GetAllAncestorTypes(&result);

        // Functions can only be registered on UsdGeomBoundable-derived
        // classes, so remove all other types, taking care not to alter
        // the relative order of the remaining results.
        static const TfType boundableType = TfType::Find<UsdGeomBoundable>();
        result.erase(
            std::remove_if(
                result.begin(), result.end(),
                [](const TfType& t) { return !t.IsA(boundableType); }),
            result.end());
        return result;
    }

    // Load the plugin for the given type if it supplies a compute function.
    bool _LoadPluginForType(const TfType& type) const
    {
        PlugRegistry& plugReg = PlugRegistry::GetInstance();

        const JsValue implementsComputeExtent = 
            plugReg.GetDataFromPluginMetaData(type, "implementsComputeExtent");
        if (!implementsComputeExtent.Is<bool>() ||
            !implementsComputeExtent.Get<bool>()) {
            return false;
        }

        const PlugPluginPtr pluginForType = plugReg.GetPluginForType(type);
        if (!pluginForType) {
            TF_CODING_ERROR(
                "Could not find plugin for '%s'", type.GetTypeName().c_str());
            return false;
        }

        return pluginForType->Load();
    }

    void _DidRegisterPlugins(const PlugNotice::DidRegisterPlugins& n)
    {
        // Invalidate the registry, since newly-registered plugins may
        // provide functions that we did not see previously. This is
        // a heavy hammer but we expect this situation to be uncommon.
        _RWMutex::scoped_lock lock(_mutex, /* write = */ true);
        _registry.clear();
    }

    bool _FindFunctionForType(
        const TfType& type, UsdGeomComputeExtentFunction* fn) const
    {
        _RWMutex::scoped_lock lock(_mutex, /* write = */ false);
        return TfMapLookup(_registry, type, fn);
    }
    
private:
    using _RWMutex = tbb::queuing_rw_mutex;
    mutable _RWMutex _mutex;

    using _Registry = 
        std::unordered_map<TfType, UsdGeomComputeExtentFunction, TfHash>;
    _Registry _registry;
};

}

TF_INSTANTIATE_SINGLETON(_FunctionRegistry);

bool
UsdGeomBoundable::ComputeExtentFromPlugins(
    const UsdGeomBoundable& boundable,
    const UsdTimeCode& time,
    VtVec3fArray* extent)
{
    if (!boundable) {
        TF_CODING_ERROR(
            "Invalid UsdGeomBoundable %s", 
            UsdDescribe(boundable.GetPrim()).c_str());
        return false;
    }

    const UsdGeomComputeExtentFunction fn = _FunctionRegistry::GetInstance()
        .GetComputeFunction(boundable.GetPrim());
    return fn && (*fn)(boundable, time, extent);
}

void
UsdGeomRegisterComputeExtentFunction(
    const TfType& primType,
    const UsdGeomComputeExtentFunction& fn)
{
    if (!primType.IsA<UsdGeomBoundable>()) {
        TF_CODING_ERROR(
            "Prim type '%s' must derive from UsdGeomBoundable",
            primType.GetTypeName().c_str());
        return;
    }

    if (!fn) {
        TF_CODING_ERROR(
            "Invalid function registered for prim type '%s'",
            primType.GetTypeName().c_str());
        return;
    }

    _FunctionRegistry::GetInstance().RegisterComputeFunction(primType, fn);
}

PXR_NAMESPACE_CLOSE_SCOPE
