//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/definitionRegistry.h"

#include "pxr/exec/exec/builtinAttributeComputations.h"
#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/builtinStageComputations.h"
#include "pxr/exec/exec/registrationBarrier.h"
#include "pxr/exec/exec/typeRegistry.h"
#include "pxr/exec/exec/types.h"

#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/journal.h"

#include "pxr/base/arch/hints.h"
#include "pxr/base/js/types.h"
#include "pxr/base/js/utils.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/trace/trace.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

static
std::vector<TfType> _GetFullyExpandedSchemaTypeVector(
    const EsfStage &stage,
    const TfType typedSchema,
    const TfTokenVector &appliedSchemas);

namespace {

// A structure used to statically initialize a map from schema type names to
// plugin data for the named schema.
//
struct _ExecPluginData {
    _ExecPluginData();

    // For each schema, we record the plugin that (may) define computations for
    // that schema and a bool that indicates whether or not the schema is
    // allowed to have plugin computations.
    //
    struct SchemaData {
        PlugPluginPtr plugin;
        bool allowsPluginComputations;
    };

    std::unordered_map<TfType, SchemaData, TfHash> execSchemaPlugins;

private:
    void _GetPluginMetadata(const PlugPluginPtr &plugin);
};

} // anonymous namespace

static TfStaticData<_ExecPluginData> execPluginData;

//
// Exec_DefinitionRegistry
//

TF_INSTANTIATE_SINGLETON(Exec_DefinitionRegistry);

Exec_DefinitionRegistry::Exec_DefinitionRegistry()
    : _registrationBarrier(std::make_unique<Exec_RegistrationBarrier>())
{
    TRACE_FUNCTION();

    // Ensure the type registry is initialized before the definition registry so
    // that computation registrations will be able to look up value types.
    ExecTypeRegistry::GetInstance();

    // Populate the registry with builtin computation definitions.
    _RegisterBuiltinComputations();

    TfNotice::Register(
        TfCreateWeakPtr(this),
        &Exec_DefinitionRegistry::_DidRegisterPlugins);

    // Calling SetInstanceConstructed() makes it possible to call
    // TfSingleton<>::GetInstance() before this constructor has finished.
    //
    // This is neccessary because the following call to SubscribeTo() will
    // _immediately_ invoke all registry functions which will, in turn, most
    // likely call TfSingleton<>::GetInstance().
    TfSingleton<Exec_DefinitionRegistry>::SetInstanceConstructed(*this);

    // Now initialize the registry.
    //
    // We use ExecDefinitionRegistryTag to identify registry functions, rather
    // than the definition registry type, so Exec_DefinitionRegistry can remain
    // private.
    TfRegistryManager::GetInstance().SubscribeTo<ExecDefinitionRegistryTag>();

    // Callers of Exec_DefinitionRegistry::GetInstance() can now safely return
    // a fully-constructed registry.
    _registrationBarrier->SetFullyConstructed();
}

// This must be defined in the cpp file, or we get undefined symbols when
// linking.
// 
const Exec_DefinitionRegistry&
Exec_DefinitionRegistry::GetInstance()
{
    Exec_DefinitionRegistry &instance =
        TfSingleton<Exec_DefinitionRegistry>::GetInstance();
    instance._registrationBarrier->WaitUntilFullyConstructed();
    return instance;
}

Exec_DefinitionRegistry&
Exec_DefinitionRegistry::_GetInstanceForRegistration()
{
    return TfSingleton<Exec_DefinitionRegistry>::GetInstance();
}

const Exec_ComputationDefinition *
Exec_DefinitionRegistry::GetComputationDefinition(
    const EsfPrimInterface &providerPrim,
    const TfToken &computationName,
    const EsfSchemaConfigKey dispatchingConfigKey,
    EsfJournal *const journal) const
{
    TRACE_FUNCTION();

    const bool hasBuiltinPrefix =
        TfStringStartsWith(
            computationName.GetString(),
            Exec_BuiltinComputations::builtinComputationNamePrefix);

    // If the provider is the stage, we only support builtin computations.
    if (providerPrim.IsPseudoRoot()) {
        if (!hasBuiltinPrefix) {
            return nullptr;
        }

        const auto builtinIt =
            _builtinStageComputationDefinitions.find(computationName);
        if (builtinIt != _builtinStageComputationDefinitions.end()) {
            return builtinIt->second.get();
        }

        return nullptr;
    }

    if (hasBuiltinPrefix) {
        // Look for a builtin computation.
        const auto builtinIt =
            _builtinPrimComputationDefinitions.find(computationName);
        if (builtinIt != _builtinPrimComputationDefinitions.end()) {
            return builtinIt->second.get();
        }

        return nullptr;
    }

    // Otherwise, look for a local plugin computation.
    if (const Exec_ComputationDefinition *const compDef =
        _LookUpLocalPrimComputation(providerPrim, computationName, journal)) {
        return compDef;
    }
        
    // If we didn't find a computation on the provider prim, look for a matching
    // dispatched computation, if dispatched computations are requested.
    if (dispatchingConfigKey != EsfSchemaConfigKey()) {
        return _LookUpDispatchedPrimComputation(
            providerPrim, computationName, dispatchingConfigKey, journal);
    }

    return nullptr;
}

const Exec_ComputationDefinition *
Exec_DefinitionRegistry::_LookUpLocalPrimComputation(
    const EsfPrimInterface &providerPrim,
    const TfToken &computationName,
    EsfJournal *const journal) const
{
    const EsfSchemaConfigKey providerSchemaConfigKey =
        providerPrim.GetSchemaConfigKey(journal);

    // Get the composed prim definition, creating it if necesseary, and use
    // it to look up the computation, or to determine that the requested
    // computation isn't provided by this prim.
    auto composedDefIt =
        _composedPrimDefinitions.find(providerSchemaConfigKey);
    if (composedDefIt == _composedPrimDefinitions.end()) {

        // We don't journal the calls below to GetType and GetAppliedSchemas
        // because the journaling already done by the call to
        // GetPrimSchemaConfigKey is sufficient, since the above call
        // combines the same information accessed by the calls below. If we
        // did rely on journaling these calls, we would have to move them
        // out of the check for the cache hit.
        EsfJournal *const nullJournal = nullptr;

        // Note that we allow concurrent callers to race to compose prim
        // definitions, since it is safe to do so and we don't expect it to
        // happen in the common case.
        _ComposedPrimDefinition primDef =
            _ComposePrimDefinition(
                providerPrim.GetStage(),
                providerPrim.GetType(nullJournal),
                providerPrim.GetAppliedSchemas(nullJournal));

        composedDefIt = _composedPrimDefinitions.emplace(
            providerSchemaConfigKey, std::move(primDef)).first;
    }

    const auto &compDefs = composedDefIt->second.primComputationDefinitions;
    const auto it = compDefs.find(computationName);
    if (it != compDefs.end()) {
        return it->second;
    }

    return nullptr;
}

const Exec_ComputationDefinition *
Exec_DefinitionRegistry::_LookUpDispatchedPrimComputation(
    const EsfPrimInterface &providerPrim,
    const TfToken &computationName,
    const EsfSchemaConfigKey dispatchingConfigKey,
    EsfJournal *const journal) const
{
    if (dispatchingConfigKey == EsfSchemaConfigKey()) {
        return nullptr;
    }

    TRACE_FUNCTION();

    // The only way we can end up here is if a non-dispatched computation was
    // found on the dispatching prim (that's the computation that had the input
    // that requests dispatched computations that got us here), which means that
    // we will always find a composed prim definition here.
    const auto composedDefIt =
        _composedPrimDefinitions.find(dispatchingConfigKey);
    if (!TF_VERIFY(composedDefIt != _composedPrimDefinitions.end())) {
        return nullptr;
    }

    const auto &compDefs =
        composedDefIt->second.dispatchedPrimComputationDefinitions;
    const auto it = compDefs.find(computationName);
    if (it == compDefs.end()) {
        return nullptr;
    }
    const Exec_PluginComputationDefinition *const compDef = it->second;
    if (!TF_VERIFY(compDef)) {
        return nullptr;
    }

    // If the computation has no schema restrictions, then we have a match.
    const ExecDispatchesOntoSchemas &dispatchesOntoSchemas =
        compDef->GetDispatchesOntoSchemas();
    if (dispatchesOntoSchemas.empty()) {
        return compDef;
    }

    // Otherwise, we iterate over the schema types for the prim (strongest
    // to weakest) and see if any of them match the schema restrictions for
    // the dispatched computation.
    const std::vector<TfType> primSchemaTypes =
        _GetFullyExpandedSchemaTypeVector(
            providerPrim.GetStage(),
            providerPrim.GetType(journal),
            providerPrim.GetAppliedSchemas(journal));
    for (const TfType type : primSchemaTypes) {
        if (std::find(
                dispatchesOntoSchemas.begin(),
                dispatchesOntoSchemas.end(), type) !=
            dispatchesOntoSchemas.end()) {
            return compDef;
        }
    }

    return nullptr;
}

const Exec_ComputationDefinition *
Exec_DefinitionRegistry::GetComputationDefinition(
    const EsfAttributeInterface &providerAttribute,
    const TfToken &computationName,
    EsfJournal *const journal) const
{
    // First look for a matching builtin computation.
    const auto builtinIt =
        _builtinAttributeComputationDefinitions.find(computationName);
    if (builtinIt != _builtinAttributeComputationDefinitions.end()) {
        return builtinIt->second.get();
    }

    // TODO: Look up plugin attribute computations.
    const EsfPrim owningPrim = providerAttribute.GetPrim(journal);
    const TfType primSchemaType = owningPrim->GetType(journal);
    (void)owningPrim;
    (void)primSchemaType;
    return nullptr;
}

const Exec_ComputationDefinition *
Exec_DefinitionRegistry::GetComputationDefinition(
    const EsfObjectInterface &providerObject,
    const TfToken &computationName,
    const EsfSchemaConfigKey dispatchingConfigKey,
    EsfJournal *journal) const
{
    if (providerObject.IsPrim()) {
        return GetComputationDefinition(
            *providerObject.AsPrim(),
            computationName,
            dispatchingConfigKey,
            journal);
    }
    else if (providerObject.IsAttribute()) {
        return GetComputationDefinition(
            *providerObject.AsAttribute(),
            computationName,
            journal);
    }
    else {
        const SdfPath providerPath =
            providerObject.GetPath(/* journal */ nullptr);
        TF_CODING_ERROR(
            "Provider '%s' is not a prim or attribute",
            providerPath.GetText());
        // Add a resync dependency on the provider.  If the object at this
        // path is removed and replaced with an object of a supported type, a
        // computation definition could be found for the new provider.
        if (journal) {
            journal->Add(providerPath, EsfEditReason::ResyncedObject);
        }
        return nullptr;
    }
}

Exec_DefinitionRegistry::_ComposedPrimDefinition
Exec_DefinitionRegistry::_ComposePrimDefinition(
    const EsfStage &stage,
    const TfType typedSchema,
    const TfTokenVector &appliedSchemas) const
{
    TRACE_FUNCTION();

    // Iterate over all ancestor types of the provider's schema type, from
    // derived to base, starting with the schema type itself, followed by the
    // fully expanded list of applied API schemas. Ensure that plugin
    // computations have been loaded for each schema type for which they are
    // registered. Add all plugin computations registered for each type to the
    // composed prim definition.
    //
    // Note that we allow concurrent callers to race to load plugins. Plugin
    // loading is serialized by PlugPlugin. Also, importantly, invocation of
    // registry functions, and therefore the registration of plugin
    // computations, is serialized by TfRegistryManager. However, computation
    // registration *can* happen concurrently with computation lookup and prim
    // definition composition.

    // Build up the composed prim definition.
    _ComposedPrimDefinition primDef;

    // Here, we are iterating from the strongest schema to the weakest, so the
    // first one to emplace a given computation wins.
    for (const TfType type : _GetFullyExpandedSchemaTypeVector(
             stage, typedSchema, appliedSchemas)) {
        if (!_EnsurePluginComputationsLoaded(type)) {
            continue;
        }

        // TODO: For all but the first type, it makes sense to look in
        // _composedPrimDefinitions to see if we have already composed the base
        // type, and then to merge, rather than keep searching up the type
        // hierarchy.

        // Compose prim computation definitions.
        if (const auto pluginIt = _pluginPrimComputationDefinitions.find(type);
            pluginIt != _pluginPrimComputationDefinitions.end()) {
            for (const Exec_PluginComputationDefinition &computationDef :
                     pluginIt->second) {
                primDef.primComputationDefinitions.emplace(
                    computationDef.GetComputationName(),
                    &computationDef);
            }
        }

        // Compose dispatched prim computation definitions.
        if (const auto pluginIt =
            _pluginDispatchedPrimComputationDefinitions.find(type);
            pluginIt != _pluginDispatchedPrimComputationDefinitions.end()) {
            for (const Exec_PluginComputationDefinition &computationDef :
                     pluginIt->second) {
                primDef.dispatchedPrimComputationDefinitions.emplace(
                    computationDef.GetComputationName(),
                    &computationDef);
            }
        }
    }

    return primDef;
}

void
Exec_DefinitionRegistry::_RegisterPrimComputation(
    TfType schemaType,
    const TfToken &computationName,
    TfType resultType,
    ExecCallbackFn &&callback,
    Exec_InputKeyVectorRefPtr &&inputKeys,
    std::unique_ptr<ExecDispatchesOntoSchemas> &&dispatchesOntoSchemas)
{
    if (schemaType.IsUnknown()) {
        TF_CODING_ERROR(
            "Attempt to register computation '%s' using an unknown type.",
            computationName.GetText());
        return;
    }

    if (_IsComputationRegistrationComplete(schemaType)) {
        TF_CODING_ERROR(
            "Attempt to register computation '%s' for schema %s, for which "
            "computation registration has already been completed.",
            computationName.GetText(),
            schemaType.GetTypeName().c_str());
        return;
    }

    if (const auto it = execPluginData->execSchemaPlugins.find(schemaType);
        it != execPluginData->execSchemaPlugins.end() &&
        !it->second.allowsPluginComputations) {
        TF_CODING_ERROR(
            "Attempt to register computation '%s' for schema %s, which was "
            "declared as not allowing plugin computations by plugin '%s'.",
            computationName.GetText(),
            schemaType.GetTypeName().c_str(),
            it->second.plugin->GetName().c_str());
        return;
    }

    if (TfStringStartsWith(
            computationName.GetString(),
            Exec_BuiltinComputations::builtinComputationNamePrefix)) {
        TF_CODING_ERROR(
            "Attempt to register computation '%s' with a name that uses the "
            "prefix '%s', which is reserved for builtin computations.",
            computationName.GetText(),
            Exec_BuiltinComputations::builtinComputationNamePrefix);
        return;
    }

    // If dispatchesOntoSchemas is non-null, the computation being registered is
    // a dispatched computation.
    const bool dispatched = static_cast<bool>(dispatchesOntoSchemas);

    const bool emplaced =
        (dispatched
         ? _pluginDispatchedPrimComputationDefinitions
         : _pluginPrimComputationDefinitions)
        [schemaType].emplace(
            resultType,
            computationName,
            std::move(callback),
            std::move(inputKeys),
            std::move(dispatchesOntoSchemas)).second;

    // TODO: We need to allow more than one dispatched computation with a given
    // name to be registered. E.g., it makes sense to dispatch one computation
    // for schema A and a different computation for schema B. First, we'll have
    // to figure out the policies that determine how we handle multiple
    // definitions with overlapping sets of schemas to which they apply, such as
    // how to resolve strength order and when to emit errors.
    if (!emplaced) {
        TF_CODING_ERROR(
            "Duplicate %sprim computation registration for computation named "
            "'%s' on schema %s",
            (dispatched ? "dispatched " : " "),
            computationName.GetText(),
            schemaType.GetTypeName().c_str());
    }
}

void
Exec_DefinitionRegistry::_RegisterBuiltinStageComputation(
    const TfToken &computationName,
    std::unique_ptr<Exec_ComputationDefinition> &&definition)
{
    if (!TF_VERIFY(
            TfStringStartsWith(
                computationName.GetString(),
                Exec_BuiltinComputations::builtinComputationNamePrefix))) {
        return;
    }

    const bool emplaced = 
        _builtinStageComputationDefinitions.emplace(
            computationName,
            std::move(definition)).second;

    if (!emplaced) {
        TF_CODING_ERROR(
            "Duplicate builtin computation registration for stage computation "
            "named '%s'",
            computationName.GetText());
    }
}

void
Exec_DefinitionRegistry::_RegisterBuiltinPrimComputation(
    const TfToken &computationName,
    std::unique_ptr<Exec_ComputationDefinition> &&definition)
{
    if (!TF_VERIFY(
            TfStringStartsWith(
                computationName.GetString(),
                Exec_BuiltinComputations::builtinComputationNamePrefix))) {
        return;
    }

    const bool emplaced =
        _builtinPrimComputationDefinitions.emplace(
            computationName,
            std::move(definition)).second;

    if (!emplaced) {
        TF_CODING_ERROR(
            "Duplicate builtin computation registration for prim computation "
            "named '%s'",
            computationName.GetText());
    }
}

void
Exec_DefinitionRegistry::_RegisterBuiltinAttributeComputation(
    const TfToken &computationName,
    std::unique_ptr<Exec_ComputationDefinition> &&definition)
{
    if (!TF_VERIFY(
            TfStringStartsWith(
                computationName.GetString(),
                Exec_BuiltinComputations::builtinComputationNamePrefix))) {
        return;
    }

    const bool emplaced =
        _builtinAttributeComputationDefinitions.emplace(
            computationName,
            std::move(definition)).second;

    if (!emplaced) {
        TF_CODING_ERROR(
            "Duplicate builtin attribute computation registration for "
            "computation named '%s'",
            computationName.GetText());
    }
}

void
Exec_DefinitionRegistry::_RegisterBuiltinComputations()
{
    _RegisterBuiltinStageComputation(
        ExecBuiltinComputations->computeTime,
        std::make_unique<Exec_TimeComputationDefinition>());

    _RegisterBuiltinAttributeComputation(
        ExecBuiltinComputations->computeValue,
        std::make_unique<Exec_ComputeValueComputationDefinition>());

    // Make sure we registered all builtins.
    TF_VERIFY(_builtinStageComputationDefinitions.size() +
              _builtinPrimComputationDefinitions.size() +
              _builtinAttributeComputationDefinitions.size() ==
              ExecBuiltinComputations->GetComputationTokens().size());
}

void
Exec_DefinitionRegistry::_DidRegisterPlugins(
    const PlugNotice::DidRegisterPlugins &notice)
{
    const bool foundExecRegistration = [&] {
        for (const PlugPluginPtr &plugin : notice.GetNewPlugins()) {
            if (JsFindValue(plugin->GetMetadata(), "Exec")) {
                return true;
            }
        }
        return false;
    }();

    if (foundExecRegistration) {
        TF_CODING_ERROR(
            "Illegal attempt to register plugins that contain exec "
            "registrations after the exec definition registry has been "
            "initialized.");
    }
}

bool
Exec_DefinitionRegistry::_EnsurePluginComputationsLoaded(
    const TfType schemaType) const
{
    // If plugin computations were already registered for this schema type or we
    // already determined that no computations are registered for this schema,
    // we can return early.
    if (const auto it = _computationsRegisteredForSchema.find(schemaType);
        it != _computationsRegisteredForSchema.end()) {
        return it->second;
    }

    TRACE_FUNCTION();

    // If a plugin defines computations for this schema, load it.
    if (const auto it = execPluginData->execSchemaPlugins.find(schemaType);
        it != execPluginData->execSchemaPlugins.end()) {

        if (const PlugPluginPtr &plugin = it->second.plugin;
            TF_VERIFY(plugin)) {
            plugin->Load();
            return true;
        }
    }

    // Record the fact that no plugin compuations are registered for this schema
    // type.
    _computationsRegisteredForSchema.emplace(schemaType, false);

    return false;
}

bool
Exec_DefinitionRegistry::_IsComputationRegistrationComplete(
    const TfType schemaType)
{
    return _computationsRegisteredForSchema.find(schemaType)
        != _computationsRegisteredForSchema.end();
}

void
Exec_DefinitionRegistry::_SetComputationRegistrationComplete(
    const TfType schemaType)
{
    _computationsRegisteredForSchema.emplace(schemaType, true).second;
}

//
// _ExecPluginData
//

_ExecPluginData::_ExecPluginData() {

    // For each plugin found by plugin discovery, look for the metadata that
    // tells us which schemas that plugin defines computations for.
    for (const PlugPluginPtr &plugin :
             PlugRegistry::GetInstance().GetAllPlugins()) {
        _GetPluginMetadata(plugin);
    }
}

static bool
_AllowsPluginComputations(const JsValue &schemaValue) {
    const JsOptionalValue allowsPluginComputationsValue =
        JsFindValue(schemaValue.GetJsObject(), "allowsPluginComputations");
    if (!allowsPluginComputationsValue) {
        // In the absense of 'allowsPluginComputations' metadata, the schema is
        // allowsPluginComputations by default.
        return true;
    }

    if (!allowsPluginComputationsValue->IsBool()) {
        TF_CODING_ERROR(
            "Exec 'allowsPluginComputations' metadatum holding type %s; "
            "expected type bool.",
            allowsPluginComputationsValue->GetTypeName().c_str());
        // On error, we consider the schema to *not* allow plugin computations.
        return false;
    }

    return allowsPluginComputationsValue->GetBool();
}

// The plugInfo that we look for here is of the form:
//
//     "Info": {
//         "Exec": {
//             "Schemas": {
//                 "MyComputationalSchema1": {
//                     "allowsPluginComputations": true
//                 },
//                 "MyComputationalSchema2": {
//                 },
//                 "MyNonComputationalSchema": {
//                     "allowsPluginComputations": false
//                 }
//             }
//         }
//     }
//
// The boolean `allowsPluginComputations` is used to declare schemas for which
// computations _cannot_ be registered. If `allowsPluginComputations` isn't
// present in the plugInfo, its value defaults to true. I.e., schemas that
// appear in the Exec/Schemas plugInfo allow plugin computations by default.
//
void
_ExecPluginData::_GetPluginMetadata(const PlugPluginPtr &plugin) {
    const JsOptionalValue metadataValue =
        JsFindValue(plugin->GetMetadata(), "Exec");
    if (!metadataValue) {
        return;
    }

    const JsOptionalValue schemasValue =
        JsFindValue(metadataValue->GetJsObject(), "Schemas");
    if (!schemasValue) {
        return;
    }

    for (const auto& [schemaName, schemaValue] : schemasValue->GetJsObject()) {
        const TfType schemaType = TfType::FindByName(schemaName);
        if (schemaType.IsUnknown()) {
            TF_CODING_ERROR(
                "Unknown schema type name '%s' encountered when reading Exec "
                "plugInfo.",
                schemaName.c_str());
            continue;
        }

        // Attempt to emplace an entry mapping the schema type to the plugin,
        // noting whether or not the schema allows computations to be registered
        // for it.
        const bool allowsPluginComputations =
            _AllowsPluginComputations(schemaValue);
        const auto [it, emplaced] =
            execSchemaPlugins.emplace(
                schemaType,
                _ExecPluginData::SchemaData{plugin, allowsPluginComputations});
        if (emplaced) {
            continue;
        }

        // Emit a suitable error, since we already had an entry for this schema.
        const PlugPluginPtr &oldPlugin = it->second.plugin;
        const bool oldAllowsPluginComputations =
            it->second.allowsPluginComputations;
        if (allowsPluginComputations == oldAllowsPluginComputations) {
            TF_CODING_ERROR(
                "Plugin '%s' declares schema %s as %sallowing plugin "
                "computations, but plugin '%s' already declared this schema.",
                (allowsPluginComputations ? " " : "not "),
                plugin->GetName().c_str(),
                schemaType.GetTypeName().c_str(),
                oldPlugin->GetName().c_str());
        } else {
            // In the case of disagreement, ensure the schema is marked as
            // not allowing plugin computations.
            it->second.allowsPluginComputations = false;

            TF_CODING_ERROR(
                "Plugin '%s' declares schema %s as %sallowing plugin "
                "computations, but plugin '%s' already declared it as "
                "%sallowing plugin computations.",
                (allowsPluginComputations ? " " : "not "),
                plugin->GetName().c_str(),
                schemaType.GetTypeName().c_str(),
                oldPlugin->GetName().c_str(),
                (oldAllowsPluginComputations ? " " : "not "));
        }
    }
}

// Returns all ancestor types of the provider's schema type, from derived to
// base, starting with the schema type itself, followed by the fully expanded
// list of applied API schemas.
//
// The returned list of schemas is ordered from strongest to weakest.
//
static
std::vector<TfType> _GetFullyExpandedSchemaTypeVector(
    const EsfStage &stage,
    const TfType typedSchema,
    const TfTokenVector &appliedSchemas)
{
    std::vector<TfType> schemaTypes;
    typedSchema.GetAllAncestorTypes(&schemaTypes);

    schemaTypes.reserve(schemaTypes.size() + appliedSchemas.size());
    for (const TfToken &schema : appliedSchemas) {
        const auto [schemaTypeName, appliedInstance] =
            stage->GetTypeNameAndInstance(schema);

        // TODO: Add support for computations on multi-apply schemas; for now,
        // we silently skip them.
        if (!appliedInstance.IsEmpty()) {
            continue;
        }

        const TfType schemaType =
            stage->GetAPITypeFromSchemaTypeName(schemaTypeName);
        if (!schemaType.IsUnknown()) {
            schemaTypes.push_back(schemaType);
        }
    }

    return schemaTypes;
}

PXR_NAMESPACE_CLOSE_SCOPE
