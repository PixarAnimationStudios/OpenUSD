//
// Copyright 2020 Pixar
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
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/pxr.h"

#include "pxr/usd/usdShade/connectableAPIBehavior.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/tokens.h"

#include "pxr/base/js/value.h"
#include "pxr/base/plug/notice.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/weakBase.h"

#include <tbb/queuing_rw_mutex.h>
#include <memory>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////
//
// UsdShadeConnectableAPIBehavior base implementation
//

UsdShadeConnectableAPIBehavior::~UsdShadeConnectableAPIBehavior()
{
}

bool
UsdShadeConnectableAPIBehavior::CanConnectInputToSource(
    const UsdShadeInput &input,
    const UsdAttribute &source,
    std::string *reason)
{
    return _CanConnectInputToSource(input, source, reason);
}

bool
UsdShadeConnectableAPIBehavior::_CanConnectInputToSource(
    const UsdShadeInput &input,
    const UsdAttribute &source,
    std::string *reason,
    ConnectableNodeTypes nodeType)
{
    if (!input.IsDefined()) {
        if (reason) {
            *reason = TfStringPrintf("Invalid input: %s",  
                input.GetAttr().GetPath().GetText());
        }
        return false;
    }

    if (!source) {
        if (reason) {
            *reason = TfStringPrintf("Invalid source: %s", 
                source.GetPath().GetText());
        }
        return false;
    }


    // Ensure that the source prim is the closest ancestor container of the 
    // NodeGraph owning the input.
    auto encapsulationCheckForInputSources = [&input, &source](
            std::string *reason) {
        const SdfPath inputPrimPath = input.GetPrim().GetPath();
        const SdfPath sourcePrimPath = source.GetPrim().GetPath();

        if (!UsdShadeConnectableAPI(source.GetPrim()).IsContainer()) {
            if (reason) {
                *reason = TfStringPrintf("Encapsulation check failed - "
                        "prim '%s' owning the input source '%s' is not a "
                        "container.", sourcePrimPath.GetText(),
                        source.GetName().GetText());
            }
            return false;
        }
        if (inputPrimPath.GetParentPath() != sourcePrimPath) {
            if (reason) {
                *reason = TfStringPrintf("Encapsulation check failed - "
                        "input source prim '%s' is not the closest ancestor "
                        "container of the NodeGraph '%s' owning the input "
                        "attribute '%s'.", sourcePrimPath.GetText(), 
                        inputPrimPath.GetText(), input.GetFullName().GetText());
            }
            return false;
        }

        return true;
    };

    // Ensure that the source prim and input prim are contained by the same
    // inner most container for all nodes, other than DerivedContainerNodes,
    // for these make sure source prim is an immediate descendent of the input
    // prim.
    auto encapsulationCheckForOutputSources = [&input, &source, 
         &nodeType](std::string *reason) {
        const SdfPath inputPrimPath = input.GetPrim().GetPath();
        const SdfPath sourcePrimPath = source.GetPrim().GetPath();

        switch (nodeType) {
            case ConnectableNodeTypes::DerivedContainerNodes:
                if (!UsdShadeConnectableAPI(input.GetPrim()).IsContainer()) {
                    if (reason) {
                        *reason = TfStringPrintf("Encapsulation check failed - "
                                "For input's prim type '%s', prim owning the "
                                "input '%s' is not a container.",
                                input.GetPrim().GetTypeName().GetText(),
                                input.GetAttr().GetPath().GetText());
                    }
                    return false;
                }
                if (sourcePrimPath.GetParentPath() != inputPrimPath) {
                    if (reason) {
                        *reason = TfStringPrintf("Encapsulation check failed - "
                                "For input's prim type '%s', Output source's "
                                "prim '%s' is not an immediate descendent of "
                                "the input's prim '%s'.",
                                input.GetPrim().GetTypeName().GetText(),
                                sourcePrimPath.GetText(), 
                                inputPrimPath.GetText());
                    }
                    return false;
                }
                return true;
                break;

            case ConnectableNodeTypes::BasicNodes:
            default:
                if (!UsdShadeConnectableAPI(input.GetPrim().GetParent()).
                        IsContainer()) {
                    if (reason) {
                        *reason = TfStringPrintf("Encapsulation check failed - "
                                "For input's prim type '%s', Immediate ancestor"
                                " '%s' for the prim owning the output source "
                                "'%s' is not a container.",
                                input.GetPrim().GetTypeName().GetText(),
                                sourcePrimPath.GetParentPath().GetText(),
                                source.GetPath().GetText());
                    }
                    return false;
                }
                if (inputPrimPath.GetParentPath() != 
                        sourcePrimPath.GetParentPath()) {
                    if (reason) {
                        *reason = TfStringPrintf("Encapsulation check failed - "
                                "For input's prim type '%s', Input's prim '%s' "
                                "and source's prim '%s' are not contained by "
                                "the same container prim.",
                                input.GetPrim().GetTypeName().GetText(),
                                inputPrimPath.GetText(), 
                                sourcePrimPath.GetText());
                    }
                    return false;
                }
                return true;
                break;
        }
    };

    TfToken inputConnectability = input.GetConnectability();
    if (inputConnectability == UsdShadeTokens->full) {
        if (UsdShadeInput::IsInput(source)) {
            if (encapsulationCheckForInputSources(reason)) {
                return true;
            }
            return false;
        }
        /* source is an output - allow connection */
        if (encapsulationCheckForOutputSources(reason)) {
            return true;
        }
        return false;
    } else if (inputConnectability == UsdShadeTokens->interfaceOnly) {
        if (UsdShadeInput::IsInput(source)) {
            TfToken sourceConnectability = 
                UsdShadeInput(source).GetConnectability();
            if (sourceConnectability == UsdShadeTokens->interfaceOnly) {
                if (encapsulationCheckForInputSources(reason)) {
                    return true;
                }
                return false;
            } else {
                if (reason) {
                    *reason = "Input connectability is 'interfaceOnly' and " \
                        "source does not have 'interfaceOnly' connectability.";
                }
                return false;
            }
        } else {
            if (reason) {
                *reason = "Input connectability is 'interfaceOnly' but " \
                    "source is not an input";
                return false;
            }
        }
    } else {
        if (reason) {
            *reason = "Input connectability is unspecified";
        }
        return false;
    }
    return false;
}

bool
UsdShadeConnectableAPIBehavior::_CanConnectOutputToSource(
    const UsdShadeOutput &output,
    const UsdAttribute &source,
    std::string *reason,
    ConnectableNodeTypes nodeType)
{
    // Nodegraphs allow connections to their outputs, but only from
    // internal nodes.
    if (!output.IsDefined()) {
        if (reason) {
            *reason = TfStringPrintf("Invalid output");
        }
        return false;
    }
    if (!source) {
        if (reason) {
            *reason = TfStringPrintf("Invalid source");
        }
        return false;
    }

    const SdfPath sourcePrimPath = source.GetPrim().GetPath();
    const SdfPath outputPrimPath = output.GetPrim().GetPath();

    if (UsdShadeInput::IsInput(source)) {
        // passthrough usage is not allowed for DerivedContainerNodes
        if (nodeType == ConnectableNodeTypes::DerivedContainerNodes) {
            if (reason) {
                *reason = TfStringPrintf("Encapsulation check failed - "
                        "passthrough usage is not allowed for output prim '%s' "
                        "of type '%s'.", outputPrimPath.GetText(),
                        output.GetPrim().GetTypeName().GetText());
            }
            return false;
        }
        // output can connect to an input of the same container as a
        // passthrough.
        if (sourcePrimPath != outputPrimPath) {
            if (reason) {
                *reason = TfStringPrintf("Encapsulation check failed - output "
                        "'%s' and input source '%s' must be encapsulated by "
                        "the same container prim", 
                        output.GetAttr().GetPath().GetText(),
                        source.GetPath().GetText());
            }
            return false;
        }
        return true;
    } else { // Source is an output
        // output can connect to other node's output directly encapsulated by
        // it.
        if (sourcePrimPath.GetParentPath() != outputPrimPath) {
            if (reason) {
                *reason = TfStringPrintf("Encapsulation check failed - prim "
                        "owning the output '%s' is not an immediate descendent "
                        " of the prim owning the output source '%s'.",
                        output.GetAttr().GetPath().GetText(),
                        source.GetPath().GetText());
            }
            return false;
        }
        return true;
    }
}

bool
UsdShadeConnectableAPIBehavior::CanConnectOutputToSource(
    const UsdShadeOutput &,
    const UsdAttribute &,
    std::string *reason)
{
    // Most outputs have their value defined by the node definition, and do
    // not allow connections to override that.
    if (reason) {
        *reason = "Outputs for this prim type are not connectable";
    }
    return false;
}

bool
UsdShadeConnectableAPIBehavior::IsContainer() const
{
    return false;
}

////////////////////////////////////////////////////////////////////////
//
// UsdShadeConnectableAPIBehavior registry
//

namespace
{

// This registry is closely modeled after the one in UsdGeomBoundableComputeExtent.
class _BehaviorRegistry
    : public TfWeakBase
{
public:
    static _BehaviorRegistry& GetInstance() 
    {
        return TfSingleton<_BehaviorRegistry>::GetInstance();
    }

    _BehaviorRegistry()
        : _initialized(false)
    {
        // Calling SubscribeTo may cause functions to be registered
        // while we're still in the c'tor, so make sure to call
        // SetInstanceConstructed to allow reentrancy.
        TfSingleton<_BehaviorRegistry>::SetInstanceConstructed(*this);
        TfRegistryManager::GetInstance().SubscribeTo<UsdShadeConnectableAPI>();

        // Mark initialization as completed for waiting consumers.
        _initialized.store(true, std::memory_order_release);

        // Register for new plugins being registered so we can invalidate
        // this registry.
        TfNotice::Register(
            TfCreateWeakPtr(this), 
            &_BehaviorRegistry::_DidRegisterPlugins);
    }

    void 
    RegisterBehavior(
        const TfType& connectablePrimType, 
        const std::shared_ptr<UsdShadeConnectableAPIBehavior> &behavior)
    {
        bool didInsert = false;
        {
            _RWMutex::scoped_lock lock(_mutex, /* write = */ true);
            didInsert = _registry.emplace(connectablePrimType, behavior).second;
        }
        
        if (!didInsert) {
            TF_CODING_ERROR(
                "UsdShade Connectable behavior already registered for "
                "prim type '%s'", connectablePrimType.GetTypeName().c_str());
        }
    }

    bool
    HasBehaviorForType(const TfType& type) {
        _WaitUntilInitialized();
        _RWMutex::scoped_lock lock(_mutex, /* write = */ false);
        return TfMapLookupPtr(_registry, type);
    }

    UsdShadeConnectableAPIBehavior*
    GetBehavior(const UsdPrim& prim)
    {
        _WaitUntilInitialized();

        // Get the actual schema type from the prim definition.
        const TfType &primSchemaType = prim.GetPrimTypeInfo().GetSchemaType();
        if (!primSchemaType) {
            TF_CODING_ERROR(
                "Could not find prim type '%s' for prim %s",
                prim.GetTypeName().GetText(), UsdDescribe(prim).c_str());
            return nullptr;
        }

        std::shared_ptr<UsdShadeConnectableAPIBehavior> behavior;
        if (_FindBehaviorForType(primSchemaType, &behavior)) {
            return behavior.get();
        }

        std::vector<TfType> primSchemaTypeAndBases;
        primSchemaType.GetAllAncestorTypes(&primSchemaTypeAndBases);

        auto i = primSchemaTypeAndBases.cbegin();
        for (auto e = primSchemaTypeAndBases.cend(); i != e; ++i) {
            const TfType& type = *i;
            if (_FindBehaviorForType(type, &behavior)) {
                break;
            }

            if (_LoadPluginForType(type)) {
                // If we loaded the plugin for this type, a new function may
                // have been registered so look again.
                if (_FindBehaviorForType(type, &behavior)) {
                    break;
                }
            }
        }

        // behavior should point to the functions to use for all types in the
        // range [primSchemaTypeAndBases.begin(), i). Note it may
        // also be nullptr if no function was found; we cache this
        // as well to avoid looking it up again.
        _RWMutex::scoped_lock lock(_mutex, /* write = */ true);
        for (auto it = primSchemaTypeAndBases.cbegin(); it != i; ++it) {
            _registry.emplace(*it, behavior);
        }

        return behavior.get();
    }

private:
    // Wait until initialization of the singleton is completed. 
    void _WaitUntilInitialized()
    {
        while (ARCH_UNLIKELY(!_initialized.load(std::memory_order_acquire))) {
            std::this_thread::yield(); 
        }
    }

    // Load the plugin for the given type if it supplies connectable behavior.
    bool _LoadPluginForType(const TfType& type) const
    {
        PlugRegistry& plugReg = PlugRegistry::GetInstance();

        const JsValue implementsUsdShadeConnectableAPIBehavior = 
            plugReg.GetDataFromPluginMetaData(type, "implementsUsdShadeConnectableAPIBehavior");
        if (!implementsUsdShadeConnectableAPIBehavior.Is<bool>() ||
            !implementsUsdShadeConnectableAPIBehavior.Get<bool>()) {
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

    bool _FindBehaviorForType(
        const TfType& type,
        std::shared_ptr<UsdShadeConnectableAPIBehavior> *behavior) const
    {
        _RWMutex::scoped_lock lock(_mutex, /* write = */ false);
        return TfMapLookup(_registry, type, behavior);
    }
    
private:
    using _RWMutex = tbb::queuing_rw_mutex;
    mutable _RWMutex _mutex;

    using _Registry = 
        std::unordered_map<TfType, std::shared_ptr<UsdShadeConnectableAPIBehavior>, TfHash>;
    _Registry _registry;

    std::atomic<bool> _initialized;
};

}

TF_INSTANTIATE_SINGLETON(_BehaviorRegistry);

void
UsdShadeRegisterConnectableAPIBehavior(
    const TfType& connectablePrimType,
    const std::shared_ptr<UsdShadeConnectableAPIBehavior> &behavior)
{
    if (!behavior || connectablePrimType.IsUnknown()) {
        TF_CODING_ERROR(
            "Invalid behavior registration for prim type '%s'",
            connectablePrimType.GetTypeName().c_str());
        return;
    }

    _BehaviorRegistry::GetInstance().RegisterBehavior(connectablePrimType, behavior);
}

////////////////////////////////////////////////////////////////////////
//
// UsdShadeConnectableAPI implementations using registered behavior
//

/* virtual */
bool 
UsdShadeConnectableAPI::_IsCompatible() const
{
    if (!UsdAPISchemaBase::_IsCompatible() )
        return false;

    // The API is compatible as long as its behavior has been defined.
    return bool(_BehaviorRegistry::GetInstance().GetBehavior(GetPrim()));
}

bool
UsdShadeConnectableAPI::CanConnect(
    const UsdShadeInput &input, 
    const UsdAttribute &source)
{
    // The reason why a connection can't be made isn't exposed currently.
    // We may want to expose it in the future, especially when we have 
    // validation in USD.
    std::string reason;
    if (UsdShadeConnectableAPIBehavior *behavior =
        _BehaviorRegistry::GetInstance().GetBehavior(input.GetPrim())) {
        return behavior->CanConnectInputToSource(input, source, &reason);
    }
    return false;
}

bool
UsdShadeConnectableAPI::CanConnect(
    const UsdShadeOutput &output, 
    const UsdAttribute &source)
{
    // The reason why a connection can't be made isn't exposed currently.
    // We may want to expose it in the future, especially when we have 
    // validation in USD.
    std::string reason;
    if (UsdShadeConnectableAPIBehavior *behavior =
        _BehaviorRegistry::GetInstance().GetBehavior(output.GetPrim())) {
        return behavior->CanConnectOutputToSource(output, source, &reason);
    }
    return false;
}

/* static */
bool
UsdShadeConnectableAPI::HasConnectableAPI(const TfType& schemaType)
{
    return _BehaviorRegistry::GetInstance().HasBehaviorForType(schemaType);
}

bool
UsdShadeConnectableAPI::IsContainer() const
{
    if (UsdShadeConnectableAPIBehavior *behavior =
        _BehaviorRegistry::GetInstance().GetBehavior(GetPrim())) {
        return behavior->IsContainer();
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
