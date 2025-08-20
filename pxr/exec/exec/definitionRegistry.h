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
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"

#include <tbb/concurrent_unordered_map.h>

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

    // Provides selective access for computation builder classes.
    class ComputationBuilderAccess
    {
        friend class Exec_ComputationBuilder;
        friend class Exec_PrimComputationBuilder;

        inline static void _RegisterPrimComputation(
            TfType schemaType,
            const TfToken &computationName,
            TfType resultType,
            ExecCallbackFn &&callback,
            Exec_InputKeyVectorRefPtr &&inputKeys,
            std::unique_ptr<ExecDispatchesOntoSchemas> &&dispatchesOntoSchemas);

        static void _SetComputationRegistrationComplete(
            const TfType schemaType) {
            _GetInstanceForRegistration()._SetComputationRegistrationComplete(
                schemaType);
        }
    };

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

    // Looks for a dispatched prim computation using the given \p
    // dispatchingConfigKey.
    //
    const Exec_ComputationDefinition *_LookUpDispatchedPrimComputation(
        const EsfPrimInterface &providerPrim,
        const TfToken &computationName,
        EsfSchemaConfigKey dispatchingConfigKey,
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
        // Map from computation name to plugin prim computation definition.
        using _ComposedPrimDefinitionMap =
            std::unordered_map<
                TfToken,
                const Exec_PluginComputationDefinition *,
                TfHash>;

        _ComposedPrimDefinitionMap primComputationDefinitions;
        _ComposedPrimDefinitionMap dispatchedPrimComputationDefinitions;

        // TODO: Add plugin attribute computation definitions.
    };

    // Creates and returns the composed prim definition for a prim on \p stage
    // with typed schema \p schemaType and API schemas \p appliedSchemas.
    //
    _ComposedPrimDefinition _ComposePrimDefinition(
        const EsfStage &stage,
        TfType schemaType,
        const TfTokenVector &appliedSchemas) const;

    // Registers a prim computation on \p schemaType.
    //
    // If \p dispatchesOntoSchemas is null, the computation is local
    // (non-dispatched). Otherwise, it is a dispatched computation that
    // dispatches onto prims with the given list of schemas, or onto all prims,
    // if the list is empty.
    // 
    void _RegisterPrimComputation(
        TfType schemaType,
        const TfToken &computationName,
        TfType resultType,
        ExecCallbackFn &&callback,
        Exec_InputKeyVectorRefPtr &&inputKeys,
        std::unique_ptr<ExecDispatchesOntoSchemas> &&dispatchesOntoSchemas);

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
    bool _IsComputationRegistrationComplete(const TfType schemaType);

    // Should be called when plugin computation registration for \p schemaType
    // is complete.
    //
    void _SetComputationRegistrationComplete(const TfType schemaType);

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
    // This is a concurrent map to allow computation lookup to happen in
    // parallel with loading of plugin computations.
    using _PrimPluginComputationMap =
        tbb::concurrent_unordered_map<
            TfType,
            std::set<
                Exec_PluginComputationDefinition,
                _PluginComputationDefinitionComparator>,
            TfHash>;

    _PrimPluginComputationMap _pluginPrimComputationDefinitions;
    _PrimPluginComputationMap _pluginDispatchedPrimComputationDefinitions;

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

    // Map from computationName to builtin stage computation
    // definition.
    std::unordered_map<
        TfToken,
        std::unique_ptr<Exec_ComputationDefinition>,
        TfHash>
    _builtinStageComputationDefinitions;

    // Map from computationName to builtin prim computation
    // definition.
    std::unordered_map<
        TfToken,
        std::unique_ptr<Exec_ComputationDefinition>,
        TfHash>
    _builtinPrimComputationDefinitions;

    // Map from computationName to builtin attribute computation
    // definition.
    std::unordered_map<
        TfToken,
        std::unique_ptr<Exec_ComputationDefinition>,
        TfHash>
    _builtinAttributeComputationDefinitions;
};

void
Exec_DefinitionRegistry::ComputationBuilderAccess::_RegisterPrimComputation(
    const TfType schemaType,
    const TfToken &computationName,
    const TfType resultType,
    ExecCallbackFn &&callback,
    Exec_InputKeyVectorRefPtr &&inputKeys,
    std::unique_ptr<ExecDispatchesOntoSchemas> &&dispatchesOntoSchemas)
{
    _GetInstanceForRegistration()._RegisterPrimComputation(
        schemaType,
        computationName,
        resultType,
        std::move(callback),
        std::move(inputKeys),
        std::move(dispatchesOntoSchemas));
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
