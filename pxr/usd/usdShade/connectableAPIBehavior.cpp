//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
#include "pxr/base/tf/hash.h"
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

using SharedConnectableAPIBehaviorPtr = 
    std::shared_ptr<UsdShadeConnectableAPIBehavior>;

UsdShadeConnectableAPIBehavior::~UsdShadeConnectableAPIBehavior()
{
}

bool
UsdShadeConnectableAPIBehavior::CanConnectInputToSource(
    const UsdShadeInput &input,
    const UsdAttribute &source,
    std::string *reason) const
{
    return _CanConnectInputToSource(input, source, reason);
}

bool
UsdShadeConnectableAPIBehavior::_CanConnectInputToSource(
    const UsdShadeInput &input,
    const UsdAttribute &source,
    std::string *reason,
    ConnectableNodeTypes nodeType) const
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

    const bool requiresEncapsulation = RequiresEncapsulation();
    if (inputConnectability == UsdShadeTokens->full) {
        if (UsdShadeInput::IsInput(source)) {
            if (!requiresEncapsulation ||
                    encapsulationCheckForInputSources(reason)) {
                return true;
            }
            return false;
        }
        /* source is an output - allow connection */
        if (!requiresEncapsulation ||
                encapsulationCheckForOutputSources(reason)) {
            return true;
        }
        return false;
    } else if (inputConnectability == UsdShadeTokens->interfaceOnly) {
        if (UsdShadeInput::IsInput(source)) {
            TfToken sourceConnectability = 
                UsdShadeInput(source).GetConnectability();
            if (sourceConnectability == UsdShadeTokens->interfaceOnly) {
                if (!requiresEncapsulation ||
                        encapsulationCheckForInputSources(reason)) {
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
    ConnectableNodeTypes nodeType) const
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


    const bool requiresEncapsulation = RequiresEncapsulation();
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
        // it, unless explicitly marked to ignore encapsulation rule.

        if (requiresEncapsulation &&
                sourcePrimPath.GetParentPath() != outputPrimPath) {
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
    const UsdShadeOutput &output,
    const UsdAttribute &source,
    std::string *reason) const
{
    return _CanConnectOutputToSource(output, source, reason);
}

bool
UsdShadeConnectableAPIBehavior::IsContainer() const
{
    return _isContainer;
}

bool
UsdShadeConnectableAPIBehavior::RequiresEncapsulation() const
{
    return _requiresEncapsulation;
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
private:
    // A struct to hold the "type identity" of a prim, which is a collection of
    // its Type and all the ApiSchemas applied to it.
    // Inspired by UsdPrimTypeInfo::_TypeId
    struct _PrimTypeId {
        TfToken primTypeName;

        TfTokenVector appliedAPISchemas;

        size_t hash;

        _PrimTypeId() = default;

        explicit _PrimTypeId(const UsdPrimTypeInfo &primTypeInfo)
            : primTypeName(primTypeInfo.GetTypeName()),
              appliedAPISchemas(primTypeInfo.GetAppliedAPISchemas()) {
                  hash = TfHash()(*this);
          }

        explicit _PrimTypeId(const TfToken& typeName)
            : primTypeName(typeName) {
                hash = TfHash()(*this);
        }

        explicit _PrimTypeId(const TfType &type)
            : primTypeName(UsdSchemaRegistry::GetSchemaTypeName(type)) {
                hash = TfHash()(*this);
        }

        _PrimTypeId(const _PrimTypeId &primTypeId) = default;
        _PrimTypeId(_PrimTypeId &&primTypeId) = default;

        template <class HashState>
        friend void TfHashAppend(HashState &h, _PrimTypeId const &primTypeId)
        {
            h.Append(primTypeId.primTypeName, primTypeId.appliedAPISchemas);
        }

        size_t Hash() const
        {
            return hash;
        }

        bool IsEmpty() const 
        {
            return primTypeName.IsEmpty() && appliedAPISchemas.empty();
        }

        bool operator==(const _PrimTypeId &other) const 
        {
            return primTypeName == other.primTypeName &&
                appliedAPISchemas == other.appliedAPISchemas;
        }

        bool operator!=(const _PrimTypeId &other) const 
        {
            return !(*this == other);
        }

        // returns a string representation of the PrimTypeId by ";" delimiting
        // the primTypeName and all the appliedAPISchemas. Useful in debugging
        // and error handling.
        std::string
        GetString() const 
        {
            static const std::string &DELIM = ";";
            std::string result = primTypeName.GetString();
            for(const auto &apiSchema : appliedAPISchemas) {
                result += DELIM;
                result += apiSchema.GetString();
            }
            return result;
        }

        struct Hasher
        {
            size_t operator()(const _PrimTypeId &id) const
            {
                return id.Hash();
            }
        };
    };

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
        //
        TfNotice::Register(
            TfCreateWeakPtr(this), 
            &_BehaviorRegistry::_DidRegisterPlugins);
    }

    // Cache behavior for _PrimTypeId
    void
    RegisterBehaviorForPrimTypeId(
            const _PrimTypeId &primTypeId,
            const SharedConnectableAPIBehaviorPtr &behavior)
    {
        bool didInsert = false;
        {
            _RWMutex::scoped_lock lock(_primTypeCacheMutex, /* write = */ true);
            didInsert = _primTypeIdCache.emplace(primTypeId, behavior).second;
        }

        if (!didInsert) {
            
            TF_CODING_ERROR(
                "UsdShade Connectable behavior already registered for "
                "primTypeId comprised of '%s' type and apischemas.",
                primTypeId.GetString().c_str());
        }
    }

    // Cache behavior for TfType
    // - Used to register behaviors via TF_REGISTRY_FUNCTION for types.
    void 
    RegisterBehaviorForType(
        const TfType& connectablePrimType, 
        const SharedConnectableAPIBehaviorPtr &behavior)
    {
        const _PrimTypeId &primTypeId = _PrimTypeId(connectablePrimType);
        // Try to insert the behavior in PrimTypeId cache created from the 
        // given type. 
        RegisterBehaviorForPrimTypeId(primTypeId, behavior);
    }

    void RegisterPlugConfiguredBehaviorForType(
        const TfType &type, SharedConnectableAPIBehaviorPtr &behavior)
    {
        // Lambda which takes a key and returns the metadata value if one 
        // exists corresponding to the key, defaultValue otherwise.
        auto GetBoolPlugMetadataValue = [&type](const std::string &key,
                const bool defaultValue) {
            PlugRegistry &plugReg = PlugRegistry::GetInstance();
            const JsValue value = plugReg.GetDataFromPluginMetaData(type, key);
            if (value.Is<bool>()) {
                return value.Get<bool>();
            }
            return defaultValue;
        };
        bool isContainer = 
            GetBoolPlugMetadataValue("isUsdShadeContainer", false);
        bool requiresEncapsulation =
            GetBoolPlugMetadataValue("requiresUsdShadeEncapsulation", true);
        behavior = SharedConnectableAPIBehaviorPtr(
                new UsdShadeConnectableAPIBehavior(
                    isContainer, requiresEncapsulation));
        RegisterBehaviorForType(type, behavior);
    }

    const UsdShadeConnectableAPIBehavior*
    GetBehaviorForPrimTypeId(const _PrimTypeId &primTypeId)
    {
        _WaitUntilInitialized();
        return _GetBehaviorForPrimTypeId(primTypeId, TfType(), UsdPrim());
    }

    const UsdShadeConnectableAPIBehavior*
    GetBehaviorForType(const TfType &type)
    {
        _WaitUntilInitialized();
        return _GetBehaviorForPrimTypeId(_PrimTypeId(type), type, UsdPrim());
    }

    bool
    HasBehaviorForType(const TfType& type)
    {
        return bool(GetBehaviorForType(type));
    }

    const UsdShadeConnectableAPIBehavior*
    GetBehavior(const UsdPrim& prim)
    {
        _WaitUntilInitialized();

        // Get the actual schema type from the prim definition.
        const TfType &primSchemaType = prim.GetPrimTypeInfo().GetSchemaType();
        const _PrimTypeId &primTypeId = _PrimTypeId(prim.GetPrimTypeInfo());
        return _GetBehaviorForPrimTypeId(primTypeId, primSchemaType, prim);
    }

private:
    // Note that below functionality is such that the order of precedence for
    // which a behavior is chosen is:
    // 1. Behavior defined on an authored API schemas, wins over 
    // 2. Behavior defined for a prim type, wins over
    // 3. Behavior defined for the prim's ancestor types, wins over
    // 4. Behavior defined for any built-in API schemas.
    // 5. If no Behavior is found but an api schema adds
    //    providesUsdShadeConnectableAPIBehavior plug metadata then a default
    //    behavior is registered for the primTypeId.
    //
    const UsdShadeConnectableAPIBehavior*
    _GetBehaviorForPrimTypeId(const _PrimTypeId &primTypeId,
                              TfType primSchemaType,
                              const UsdPrim& prim)
    {
        SharedConnectableAPIBehaviorPtr behavior;

        // Has a behavior cached for this primTypeId, if so fetch it and return!
        if (_FindBehaviorForPrimTypeId(primTypeId, &behavior)) {
            return behavior.get();
        }

        // Look up the the schema type if we don't have it already.
        // This is delayed until now in order to make the above cache
        // check as fast as possible.
        if (primSchemaType.IsUnknown()) {
            primSchemaType =
                TfType::FindByName(primTypeId.primTypeName.GetString());
            // Early return if we do not have a valid primSchemaType (type not
            // registered with UsdSchemaRegistry and has no appliedAPISchemas 
            // which can impart ConnectableAPIBehavior).
            if (primSchemaType.IsUnknown() && 
                    primTypeId.appliedAPISchemas.empty()) {
                return nullptr;
            }
        }


        // If a behavior is not found for primTypeId, we try to look for a
        // registered behavior in prim's ancestor types.
        // And if primSchemaType is not defined, we skip this to look for a
        // behavior in appliedAPISchemas
        bool foundBehaviorInAncestorType = false;
        if (!primSchemaType.IsUnknown()) {
            std::vector<TfType> primSchemaTypeAndBases;
            primSchemaType.GetAllAncestorTypes(&primSchemaTypeAndBases);
            auto i = primSchemaTypeAndBases.cbegin();
            for (auto e = primSchemaTypeAndBases.cend(); i != e; ++i) {
                const TfType& type = *i;
                if (_FindBehaviorForType(type, &behavior)) {
                    foundBehaviorInAncestorType = true;
                    break;
                }

                if (_LoadPluginDefiningBehaviorForType(type)) {
                    // If we loaded the plugin for this type, a new function may
                    // have been registered so look again.
                    if (!_FindBehaviorForType(type, &behavior)) {
                        // If no registered behavior is found, then register a
                        // behavior via plugInfo configuration, since this types
                        // plug has a providesUsdShadeConnectableAPIBehavior
                        RegisterPlugConfiguredBehaviorForType(type, behavior);
                    }
                    foundBehaviorInAncestorType = true;
                    break;
                }
            }
            // If a behavior is found on primType's ancestor, we can safely 
            // cache this behavior for all types between this prim's type and 
            // the ancestor type for which the behavior is found.
            if (foundBehaviorInAncestorType) {
                // Note that we need to atomically add insert behavior for all
                // ancestor types, hence acquiring a write lock here.
                _RWMutex::scoped_lock lock(_primTypeCacheMutex, /* write = */ true);

                // behavior should point to the functions to use for all types 
                // in the range [primSchemaTypeAndBases.begin(), i).
                for (auto it = primSchemaTypeAndBases.cbegin(); it != i; ++it) {
                    const _PrimTypeId &ancestorPrimTypeId = _PrimTypeId(*it);
                    _primTypeIdCache.emplace(ancestorPrimTypeId, behavior);
                }
            }

            // A behavior is found for the type in its lineage -- look for 
            // overriding behavior on all explicitly authored apiSchemas on the 
            // prim. If found cache this overriding behavior against the primTypeId.
            if (behavior) {
                for (auto& appliedSchema : primTypeId.appliedAPISchemas) {
                    // Override the prim type registered behavior if any of the 
                    // authored apiSchemas (in strength order) provides a 
                    // UsdShadeConnectableAPIBehavior
                    SharedConnectableAPIBehaviorPtr apiBehavior;
                    if (_FindBehaviorForApiSchema(appliedSchema, apiBehavior)) {
                        behavior = apiBehavior;
                        RegisterBehaviorForPrimTypeId(primTypeId, behavior);
                        break;
                    }
                }
                // If no behavior was found for any of the apischemas on the prim, 
                // we can return the behavior found on the ancestor. Note that we
                // have already inserted the behavior for all types between this 
                // prim's type and the ancestor for which behavior was found to the
                // cache.
                return behavior.get();
            }
        }

        // No behavior was found to be registered on prim type or primTypeId or
        // we have a typeless prim being queried, lookup all apiSchemas and if 
        // found, register it against primTypeId in the _primTypeIdCache. Note 
        // that codeless api schemas could contain 
        // providesUsdShadeConnectableAPIBehavior plug metadata without 
        // providing a c++ Behavior implementation, for such applied schemas, 
        // a default UsdShadeConnectableAPIBehavior is created and 
        // registered/cached with the appliedSchemaType and the primTypeId.
        auto appliedSchemas = [&prim, &primSchemaType] {
            // We do not register any primDefinition for Abstract types,
            // Hence we can not query builtin api schemas on any of the abstract
            // types. Return an empty vector here.
            // XXX: Ideally this should be handled such that builtin applied
            // schemas on an abstract types are consulted for
            // UsdShadeConnectableAPIBehavior registry , but that involves
            // much larger refactor, hence just ignoring the abstract types for
            // now, and will be addressed in detail later.
            if (UsdSchemaRegistry::IsAbstract(primSchemaType)) {
                return TfTokenVector{};
            }

            if (prim) {
                return prim.GetAppliedSchemas();
            } else {
                // Get built-in schemas for primSchemaType
                const UsdSchemaRegistry &usdSchemaReg =
                    UsdSchemaRegistry::GetInstance();
                const TfToken &typeName =
                    usdSchemaReg.GetSchemaTypeName(primSchemaType);
                
                const UsdPrimDefinition *primDefinition =
                    UsdSchemaRegistry::IsConcrete(primSchemaType) ?
                    usdSchemaReg.FindConcretePrimDefinition(typeName) :
                    usdSchemaReg.FindAppliedAPIPrimDefinition(typeName);
                return primDefinition->GetAppliedAPISchemas();
            }
        }();

        for (auto& appliedSchema : appliedSchemas) {
            if (_FindBehaviorForApiSchema(appliedSchema, behavior)) {
                RegisterBehaviorForPrimTypeId(primTypeId, behavior);
                return behavior.get();
            }
        }

        // If behavior is still not found and hence at this point we are 
        // certain that behavior is still null, the primTypeId is lacking one, 
        // cache a null behavior for this primTypeId. 
        // Note that for a primTypeId which has an invalid primTypeName set 
        // we have done an early return already.
        // Note that if a null behavior is found in one of the AncestorTypes,
        // the cache is updated already and hence we do not need to update the
        // cache again here.
        if (TF_VERIFY(!behavior) && !foundBehaviorInAncestorType) {
            RegisterBehaviorForPrimTypeId(primTypeId, behavior);
        }

        return behavior.get();
    }

    // Wait until initialization of the singleton is completed. 
    void _WaitUntilInitialized()
    {
        while (ARCH_UNLIKELY(!_initialized.load(std::memory_order_acquire))) {
            std::this_thread::yield(); 
        }
    }

    // Load the plugin for the given type if it supplies connectable behavior.
    bool _LoadPluginDefiningBehaviorForType(const TfType& type) const
    {
        // type being queried is not Usd compliant.
        if (!type.IsA<UsdTyped>() && !type.IsA<UsdAPISchemaBase>()) {
            return false;
        }

        PlugRegistry& plugReg = PlugRegistry::GetInstance();

        const JsValue providesUsdShadeConnectableAPIBehavior = 
            plugReg.GetDataFromPluginMetaData(type, 
                    "providesUsdShadeConnectableAPIBehavior");
        if (!providesUsdShadeConnectableAPIBehavior.Is<bool>() ||
            !providesUsdShadeConnectableAPIBehavior.Get<bool>()) {
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
        // Erase the entries in _primTypeIdCache which have a null behavior
        // registered, since newly-registered plugins may provide valid
        // behavior for these primTypeId entries.
        // Note that we retain entries which have valid connectableAPIBehavior
        // defined.
        //
        {
            _RWMutex::scoped_lock lock(_primTypeCacheMutex, /* write = */ true);
            _PrimTypeIdCache::iterator itr = _primTypeIdCache.begin(),
                end = _primTypeIdCache.end();
            while (itr != end) {
                if (!itr->second) {
                    itr = _primTypeIdCache.erase(itr);
                    end = _primTypeIdCache.end();
                    continue;
                }
                itr++;
            }
        }
    }

    bool _FindBehaviorForPrimTypeId(
        const _PrimTypeId &primTypeId,
        SharedConnectableAPIBehaviorPtr *behavior) const
    {
        _RWMutex::scoped_lock lock(_primTypeCacheMutex, /* write = */ false);
        return TfMapLookup(_primTypeIdCache, primTypeId, behavior);
    }

    bool _FindBehaviorForType(
        const TfType& type,
        SharedConnectableAPIBehaviorPtr *behavior) const
    {
        return _FindBehaviorForPrimTypeId(_PrimTypeId(type), behavior);
    }

    bool _FindBehaviorForApiSchema(const TfToken &appliedSchema,
            SharedConnectableAPIBehaviorPtr &apiBehavior) 
    {
        const TfType &appliedSchemaType =
            UsdSchemaRegistry::GetAPITypeFromSchemaTypeName(appliedSchema);
        UsdSchemaKind sk = UsdSchemaRegistry::GetSchemaKind(appliedSchemaType);

        // Of all the schema types enumerated in UsdSchemaKind, the *only*
        // kind we can (and/or expect to) process is singleApply
        if (sk != UsdSchemaKind::SingleApplyAPI) {
            return false;
        }
            
        if (_LoadPluginDefiningBehaviorForType(appliedSchemaType)) {
            if (!_FindBehaviorForType(appliedSchemaType, &apiBehavior)) {
                // If a behavior is not found/registered (but an
                // appliedSchema specified its implementation, create a
                // default behavior here and register it against this
                // primTypeId.
                RegisterPlugConfiguredBehaviorForType(
                        appliedSchemaType, apiBehavior);
            }
            return true;
        }
        return false;
    }
    
private:
    using _RWMutex = tbb::queuing_rw_mutex;
    mutable _RWMutex _primTypeCacheMutex;


    using _PrimTypeIdCache = 
        std::unordered_map<_PrimTypeId, SharedConnectableAPIBehaviorPtr, 
            _PrimTypeId::Hasher>;
    _PrimTypeIdCache _primTypeIdCache;

    std::atomic<bool> _initialized;
};

}

TF_INSTANTIATE_SINGLETON(_BehaviorRegistry);

void
UsdShadeRegisterConnectableAPIBehavior(
    const TfType& connectablePrimType,
    const SharedConnectableAPIBehaviorPtr &behavior)
{
    if (!behavior || connectablePrimType.IsUnknown()) {
        TF_CODING_ERROR(
            "Invalid behavior registration for prim type '%s'",
            connectablePrimType.GetTypeName().c_str());
        return;
    }

    _BehaviorRegistry::GetInstance().RegisterBehaviorForType(
            connectablePrimType, behavior);
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
    if (const UsdShadeConnectableAPIBehavior *behavior =
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
    if (const UsdShadeConnectableAPIBehavior *behavior =
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
    if (const UsdShadeConnectableAPIBehavior *behavior =
        _BehaviorRegistry::GetInstance().GetBehavior(GetPrim())) {
        return behavior->IsContainer();
    }
    return false;
}

bool
UsdShadeConnectableAPI::RequiresEncapsulation() const
{
    if (const UsdShadeConnectableAPIBehavior *behavior =
        _BehaviorRegistry::GetInstance().GetBehavior(GetPrim())) {
        return behavior->RequiresEncapsulation();
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
