//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_DEFINITION_REGISTRY_H
#define PXR_EXEC_EXEC_DEFINITION_REGISTRY_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"
#include "pxr/exec/exec/computationDefinition.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/pluginComputationDefinition.h"
#include "pxr/exec/exec/types.h"

#include "pxr/exec/esf/prim.h"
#include "pxr/exec/esf/schemaConfigKey.h"

#include "pxr/base/plug/notice.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/pxrTslRobinMap/robin_map.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/vt/value.h"

#include <tbb/concurrent_unordered_map.h>

#ifdef TBB_PREVIEW_CONCURRENT_ORDERED_CONTAINERS
#include <tbb/concurrent_map.h>
#else
#define TBB_PREVIEW_CONCURRENT_ORDERED_CONTAINERS 1
#include <tbb/concurrent_map.h>
#undef TBB_PREVIEW_CONCURRENT_ORDERED_CONTAINERS
#endif

#include <functional>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class Exec_RegistrationBarrier;
class EsfAttributeInterface;
class EsfJournal;
class EsfObjectInterface;
class EsfStage;

/// Singleton that stores computation definitions registered for schemas that
/// define computations.
///
class Exec_DefinitionRegistry : public TfWeakBase
{
public:
    Exec_DefinitionRegistry(
        const Exec_DefinitionRegistry &) = delete;
    Exec_DefinitionRegistry &operator=(
        const Exec_DefinitionRegistry &) = delete;

    /// Provides access to the singleton instance, first ensuring it is
    /// constructed, and ensuring that all currently-loaded plugins have
    /// registered their computations.
    ///
    EXEC_API
    static const Exec_DefinitionRegistry& GetInstance();

    /// Returns the definition for the prim computation named
    /// \p computationName registered for \p providerPrim.
    ///
    /// If dispatched computations are requested, \p dispatchingConfigKey is
    /// used for dispatched computation lookup.
    ///
    /// Any scene access needed to determine the input keys is recorded in
    /// \p journal.
    ///
    EXEC_API
    const Exec_ComputationDefinition *GetComputationDefinition(
        const EsfPrimInterface &providerPrim,
        const TfToken &computationName,
        EsfSchemaConfigKey dispatchingConfigKey,
        EsfJournal *journal) const;

    /// Returns the definition for the attribute computation named
    /// \p computationName registered for \p providerAttribute.
    ///
    /// Any scene access needed to determine the input keys is recorded in
    /// \p journal.
    ///
    EXEC_API
    const Exec_ComputationDefinition *GetComputationDefinition(
        const EsfAttributeInterface &providerAttribute,
        const TfToken &computationName,
        EsfSchemaConfigKey dispatchingConfigKey,
        EsfJournal *journal) const;

    /// Returns the definition for the computation named \p computationName
    /// registered for \p providerObject.
    ///
    /// If dispatched computations are requested, \p dispatchingConfigKey is
    /// used for dispatched computation lookup.
    ///
    /// Any scene access needed to determine the input keys is recorded in
    /// \p journal.
    ///
    EXEC_API
    const Exec_ComputationDefinition *GetComputationDefinition(
        const EsfObjectInterface &providerObject,
        const TfToken &computationName,
        EsfSchemaConfigKey dispatchingConfigKey,
        EsfJournal *journal) const;

    /// Selectively allow non-const access to the definition registry for
    /// performing registration.
    ///
    class RegistrationAccess
    {
        friend class Exec_ComputationBuilder;
        friend class Exec_PrimComputationBuilder;
        friend class Exec_AttributeComputationBuilder;
        friend struct Exec_ComputationBuilderConstantValueSpecifier;

        static Exec_DefinitionRegistry& _GetInstanceForRegistration() {
            return Exec_DefinitionRegistry::_GetInstanceForRegistration();
        }
    };

    /// Registers a prim computation on \p schemaType.
    ///
    /// If \p dispatchesOntoSchemas is null, the computation is local
    /// (non-dispatched). Otherwise, it is a dispatched computation that
    /// dispatches onto prims with the given list of schemas, or onto all prims,
    /// if the list is empty.
    /// 
    void RegisterPrimComputation(
        TfType schemaType,
        const TfToken &computationName,
        TfType resultType,
        ExecCallbackFn &&callback,
        Exec_InputKeyVectorRefPtr &&inputKeys,
        std::unique_ptr<ExecDispatchesOntoSchemas> &&dispatchesOntoSchemas);

    /// Registers an attribute computation on \p schemaType for attributes named
    /// \p attributeName.
    ///
    /// If \p dispatchesOntoSchemas is null, the computation is local
    /// (non-dispatched). Otherwise, it is a dispatched computation that
    /// dispatches onto attributes owned by prims with the given list of
    /// schemas, or onto all attributes, if the list is empty.
    /// 
    void RegisterAttributeComputation(
        const TfToken &attributeName,
        TfType schemaType,
        const TfToken &computationName,
        TfType resultType,
        ExecCallbackFn &&callback,
        Exec_InputKeyVectorRefPtr &&inputKeys,
        std::unique_ptr<ExecDispatchesOntoSchemas> &&dispatchesOntoSchemas);

    /// Should be called when plugin computation registration for \p schemaType
    /// is complete.
    ///
    void SetComputationRegistrationComplete(const TfType schemaType);

    /// Registers \p value as a value that can be used for constant value inputs.
    ///
    /// Returns a token to be used to identify the constant value and be used as
    /// a disambiguating ID in input and output keys.
    ///
    TfToken RegisterConstantValue(VtValue &&value);

    /// Given the \p uniqueKey that identifies a constant value (returned by
    /// _RegisterConstantValue), returns the corresponding constant value.
    ///
    VtValue GetConstantValue(const TfToken &uniqueKey) const;

private:

    // Only TfSingleton can create instances.
    friend class TfSingleton<Exec_DefinitionRegistry>;

    Exec_DefinitionRegistry();

    // Looks for a local (non-dispatched) plugin prim computation on the given
    // \p providerPrim, composing the prim definition if it's not already
    // composed.
    //
    const Exec_ComputationDefinition *_LookUpLocalPrimComputation(
        const EsfPrimInterface &providerPrim,
        const TfToken &computationName,
        EsfJournal *journal) const;

    // Looks for a dispatched prim or attribute computation using the given \p
    // dispatchingConfigKey.
    //
    const Exec_ComputationDefinition *_LookUpDispatchedComputation(
        const EsfPrimInterface &providerPrim,
        const TfToken &computationName,
        bool isPrimComputation,
        EsfSchemaConfigKey dispatchingConfigKey,
        EsfJournal *journal) const;

    // Looks for a local (non-dispatched) plugin attribute computation on the
    // given \p providerPrim, composing the prim definition if it's not already
    // composed.
    //
    const Exec_ComputationDefinition *_LookUpLocalAttributeComputation(
        const EsfAttributeInterface &providerAttribute,
        const TfToken &computationName,
        EsfJournal *journal) const;

    // Returns a reference to the singleton that is suitable for registering
    // new computations.
    //
    // The returned instance cannot be used to look up computations.
    //
    EXEC_API
    static Exec_DefinitionRegistry& _GetInstanceForRegistration();

    // A structure that contains the definitions for all computations that can
    // be found on a prim of a given type.
    //
    struct _ComposedPrimDefinition {
        // Map from computation name to plugin computation definition.
        using _ComputationDefinitionMap =
            std::unordered_map<
                TfToken,
                const Exec_PluginComputationDefinition *,
                TfHash>;

        _ComputationDefinitionMap primComputationDefinitions;
        _ComputationDefinitionMap dispatchedPrimComputationDefinitions;

        // Map from (attribute name, computation name) to plugin attribute
        // computation definition.
        using _AttributeComputationDefinitionMap =
            std::unordered_map<
                std::pair<TfToken, TfToken>,
                const Exec_PluginComputationDefinition *,
                TfHash>;

        _AttributeComputationDefinitionMap attributeComputationDefinitions;
        _ComputationDefinitionMap dispatchedAttributeComputationDefinitions;
    };

    // Returns the composed prim definition for \p providerPrim, composing it
    // if necessary.
    //
    const Exec_DefinitionRegistry::_ComposedPrimDefinition &
    _GetOrCreateComposedPrimDefinition(
        const EsfPrimInterface &providerPrim,
        EsfJournal *journal) const;

    // Creates and returns the composed prim definition for a prim on \p stage
    // with typed schema \p schemaType and API schemas \p appliedSchemas.
    //
    _ComposedPrimDefinition _ComposePrimDefinition(
        const EsfStage &stage,
        TfType schemaType,
        const TfTokenVector &appliedSchemas) const;

    // Returns true if it's valid to register a computation named \p
    // computationName for \p schemaType.
    //
    bool _ValidateComputationRegistration(
        TfType schemaType,
        const TfToken &computationName) const;

    void _RegisterBuiltinStageComputation(
        const TfToken &computationName,
        std::unique_ptr<Exec_ComputationDefinition> &&definition);

    void _RegisterBuiltinPrimComputation(
        const TfToken &computationName,
        std::unique_ptr<Exec_ComputationDefinition> &&definition);

    void _RegisterBuiltinAttributeComputation(
        const TfToken &computationName,
        std::unique_ptr<Exec_ComputationDefinition> &&definition);

    void _RegisterBuiltinComputations();

    // Returns true if plugin computation registration for \p schemaType is
    // complete.
    //
    bool _IsComputationRegistrationComplete(const TfType schemaType) const;

    // Load plugin computations for the given schemaType, if we haven't loaded
    // them yet.
    //
    // Returns false if no plugin computations are registered for the given
    // schemaType.
    //
    bool _EnsurePluginComputationsLoaded(const TfType schemaType) const;

    // Notifies if there is an attempt to register plugin computations after the
    // registry is already initialized, which is not suported.
    //
    void _DidRegisterPlugins(const PlugNotice::DidRegisterPlugins &notice);

private:

    // This barrier ensures singleton access returns a fully-constructed
    // instance. This is the case for GetInstance(), but not required for
    // _GetInstanceForRegistration() which is called by exec definition registry
    // functions.
    std::unique_ptr<Exec_RegistrationBarrier> _registrationBarrier;

    // Comparator for ordering plugin computation definitions in a set.
    struct _PluginComputationDefinitionComparator {
        bool operator()(
            const Exec_PluginComputationDefinition &a,
            const Exec_PluginComputationDefinition &b) const {
            return a.GetComputationName() < b.GetComputationName();
        }
    };

    // Map from schemaType to plugin prim computation definitions.
    //
    // This is a concurrent map to allow lazy caching of composed prim
    // definitions (which reads from these maps) to happen in parallel with
    // loading of plugin computations (which writes to them).
    using _PluginComputationMap =
        tbb::concurrent_unordered_map<
            TfType,
            std::set<
                Exec_PluginComputationDefinition,
                _PluginComputationDefinitionComparator>,
            TfHash>;

    _PluginComputationMap _pluginPrimComputationDefinitions;
    _PluginComputationMap _pluginDispatchedPrimComputationDefinitions;

    // Map from schemaType to map from attribute name to plugin attribute
    // computation definitions.
    //
    // This is a concurrent map to allow lazy caching of composed prim
    // defintitions (which reads from these maps) to happen in parallel with
    // loading of plugin computations (which writes to them).
    using _PluginAttributeComputationMap =
        tbb::concurrent_map<
            std::pair<TfType, TfToken>,
            std::set<
                Exec_PluginComputationDefinition,
                _PluginComputationDefinitionComparator>,
            std::less<>>;

    _PluginAttributeComputationMap _pluginAttributeComputationDefinitions;
    _PluginComputationMap _pluginDispatchedAttributeComputationDefinitions;

    // Map from an opaque key to composed prim exec definition.
    //
    // This is a concurrent map to allow computation lookup to happen in
    // parallel with lazy caching of composed prim definitions.
    mutable tbb::concurrent_unordered_map<
        EsfSchemaConfigKey,
        _ComposedPrimDefinition,
        TfHash>
    _composedPrimDefinitions;

    // Map from schema type to a bool that indicates whether or not any plugin
    // computations are registered for the schema.
    //
    // This is a concurrent map to allow computation lookup to happen in
    // parallel with lazy loading of plugin computations; and also to allow
    // multiple threads to safely race when ensuring that plugins are loaded
    mutable tbb::concurrent_unordered_map<TfType, bool, TfHash>
    _computationsRegisteredForSchema;

    using _BuiltinComputationMap =
        std::unordered_map<
            TfToken,
            std::unique_ptr<Exec_ComputationDefinition>,
            TfHash>;

    // Map from computationName to builtin stage computation
    // definition.
    _BuiltinComputationMap _builtinStageComputationDefinitions;

    // Map from computationName to builtin prim computation
    // definition.
    _BuiltinComputationMap _builtinPrimComputationDefinitions;

    // Map from computationName to builtin attribute computation
    // definition.
    _BuiltinComputationMap _builtinAttributeComputationDefinitions;

    // Map from constant value to the unique key used to identify it.
    pxr_tsl::robin_map<VtValue, TfToken, TfHash> _constantValueToToken;

    // Map from constant value unique key to the constant value it identifies.
    pxr_tsl::robin_map<TfToken, VtValue, TfHash> _tokenToConstantValue;

    // An index that is used to generate unique keys for registered constant
    // input values.
    unsigned int _constantValueIndex = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
