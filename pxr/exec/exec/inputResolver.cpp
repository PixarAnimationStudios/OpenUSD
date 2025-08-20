//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/inputResolver.h"

#include "pxr/exec/exec/computationDefinition.h"
#include "pxr/exec/exec/definitionRegistry.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/outputKey.h"
#include "pxr/exec/exec/providerResolution.h"

#include "pxr/base/trace/trace.h"
#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/journal.h"
#include "pxr/exec/esf/object.h"
#include "pxr/exec/esf/prim.h"
#include "pxr/exec/esf/relationship.h"
#include "pxr/exec/esf/stage.h"
#include "pxr/usd/sdf/path.h"

#include <utility>
#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

/// Helper class that performs input resolution.
///
/// Input resolution is implemented as a state machine. The "state" is
/// represented by an EsfObject, which begins at the resolution origin. Each
/// state transition is implemented by a private _TraverseToXxx method, which
/// updates the current object by traversing to a related scene object.
///
/// A single _InputResolver cannot be used to resolve multiple input keys. Each
/// must work with their own _InputResolver instance. To prevent misuse, this
/// class exposes a single static method that performs the entire resolution
/// process for a single input key.
///
class _InputResolver
{
public:
    /// Implements the global Exec_ResolveInput function.
    static Exec_OutputKeyVector ResolveInput(
        const EsfStage &stage,
        const EsfObject &origin,
        const EsfSchemaConfigKey dispatchingSchemaKey,
        const Exec_InputKey &inputKey,
        EsfJournal *const journal)
    {
        _InputResolver resolver(stage, origin, journal, dispatchingSchemaKey);

        return resolver._ResolveInput(inputKey);
    }

private:
    // Construct a new _InputResolver that begins at \p origin and logs
    // traversals to \p journal.
    // 
    _InputResolver(
        const EsfStage &stage,
        const EsfObject &origin,
        EsfJournal *const journal,
        const EsfSchemaConfigKey dispatchingSchemaKey)
        : _currentObject(nullptr)
        , _currentPrim(nullptr)
        , _currentAttribute(nullptr)
        , _journal(journal)
        , _dispatchingSchemaKey(dispatchingSchemaKey)
        , _stage(stage.Get())
        , _definitionRegistry(Exec_DefinitionRegistry::GetInstance())
    {
        // Initialize the current object by casting the origin to the most
        // appropriate derived type.
        if (origin->IsPrim()) {
            _SetPrim(origin->AsPrim());
            return;
        }

        if (origin->IsAttribute()) {
            _SetAttribute(origin->AsAttribute());
            return;
        }

        TF_VERIFY(false,
            "Cannot resolve inputs from non-prim, non-attribute origin <%s>.",
            origin->GetPath(nullptr).GetText());
    }

    // Updates the current object to the specified \p prim.
    void _SetPrim(EsfPrim &&prim)
    {
        _currentPrim = _currentObjectVariant.emplace<EsfPrim>(
            std::move(prim)).Get();
        _currentObject = _currentPrim;
        _currentAttribute = nullptr;
        _currentRelationship = nullptr;
    }

    // Updates the current object to the specified \p attribute.
    void _SetAttribute(EsfAttribute &&attribute)
    {
        _currentAttribute = _currentObjectVariant.emplace<EsfAttribute>(
            std::move(attribute)).Get();
        _currentObject = _currentAttribute;
        _currentPrim = nullptr;
        _currentRelationship = nullptr;
    }

    // Updates the current object to the specified \p relationship.
    void _SetRelationship(EsfRelationship &&relationship)
    {
        _currentRelationship = _currentObjectVariant.emplace<EsfRelationship>(
            std::move(relationship)).Get();
        _currentObject = _currentRelationship;
        _currentAttribute = nullptr;
        _currentPrim = nullptr;
    }

    // Updates the current object to its parent object.
    //
    // This does *not* check if the current object, or its parent is a valid
    // scene object. Such checks are left up to the caller. This only returns
    // false if the current object type is not supported.
    // 
    bool _TraverseToParent()
    {
        if (_currentPrim) {
            _SetPrim(_currentPrim->GetParent(_journal));
            return true;
        }
        else if (_currentAttribute) {
            _SetPrim(_currentAttribute->GetPrim(_journal));
            return true;
        }
    
        TF_VERIFY(false,
            "Cannot traverse to parent from unsupported scene object <%s>.",
        _currentObject->GetPath(nullptr).GetText());
        return false;
    }

    // Updates the current object to an attribute on the current object.
    //
    // This does *not* check if the current object or the resulting attribute
    // are valid scene objects. Such checks are left up to the caller. The
    // current object must be a prim, or else a TF_VERIFY is raised, and this
    // returns false.
    // 
    bool _TraverseToAttribute(const TfToken &attributeName)
    {
        if (!TF_VERIFY(_currentPrim)) {
            return false;
        }

        _SetAttribute(_currentPrim->GetAttribute(attributeName, _journal));

        return true;
    }

    // Updates the current object to a relationship on the current object.
    //
    // This does *not* check if the current object or the resulting relationship
    // are valid scene objects. Such checks are left up to the caller. The
    // current object must be a prim, or else a TF_VERIFY is raised, and this
    // returns false.
    // 
    bool _TraverseToRelationship(const TfToken &relationshipName)
    {
        if (!TF_VERIFY(_currentPrim)) {
            return false;
        }

        _SetRelationship(
            _currentPrim->GetRelationship(relationshipName, _journal));

        return true;
    }

    // Updates the current object by traversing along each component of the
    // relative path that is the `localTraversal` in \p providerResolution.
    //
    // The 'dynamicTraversal` in \p providerResolution is used to guide the
    // traversal, in some cases, as to what kind of provider we expect to find
    // at the relative path.
    //
    // The current object must be valid prior to calling this method.
    //
    // If this method returns true, then the current object is valid and is set
    // to the object indicated by the relative path.
    //
    // If this method returns false, then the current object is set to the first
    // invalid object encountered while performing the traversal - which may be
    // the final object, or some intermediate object.
    // 
    bool _TraverseToRelativePath(
        const ExecProviderResolution &providerResolution)
    {
        const SdfPath &relativePath = providerResolution.localTraversal;
        if (!TF_VERIFY(!relativePath.IsAbsolutePath())) {
            return false;
        }

        // SdfPath does not define a constant for this.
        static const SdfPath parentRelativePath("..");

        for (const SdfPath &prefix : relativePath.GetPrefixes()) {
            if (prefix == SdfPath::ReflexiveRelativePath()) {
                continue;
            }

            if (prefix == parentRelativePath) {
                if (!_TraverseToParent()) {
                    return false;
                }
            }
            
            else if (prefix.IsPropertyPath()) {
                const TfToken &propertyName = prefix.GetNameToken();

                // The dynamic traversal tells us whether we expect the property
                // path takes us to a relationship or an attribute.
                if (providerResolution.dynamicTraversal ==
                    ExecProviderResolution::DynamicTraversal::
                        RelationshipTargetedObjects) {
                    if (!_TraverseToRelationship(propertyName)) {
                        return false;
                    }
                } else if (!_TraverseToAttribute(propertyName)) {
                    return false;
                }
            }

            else {
                TF_VERIFY(false,
                    "Unable to traverse along relative path <%s>. Unhandled "
                    "prefix <%s>.",
                    relativePath.GetText(),
                    prefix.GetText());
                return false;
            }

            // After each hop, stop if we encountered an invalid object.
            if (!_currentObject->IsValid(_journal)) {
                return false;
            }
        }

        return true;
    }

    // Updates the current object by traversing to the object at the given
    // absolute path.
    //
    // If this method returns true, then the current object is valid and is set
    // to the object indicated by the absolute path.
    // 
    bool _TraverseToAbsolutePath(const SdfPath &path)
    {
        if (!TF_VERIFY(path.IsAbsolutePath()) ||
            !TF_VERIFY(!path.IsEmpty())) {
            return false;
        }

        const EsfObject targetObject = _stage->GetObjectAtPath(path, _journal);
        if (!targetObject->IsValid(_journal)) {
            return false;
        }

        if (targetObject->IsPrim()) {
            _SetPrim(targetObject->AsPrim());
        }
        else if (targetObject->IsAttribute()) {
            _SetAttribute(targetObject->AsAttribute());
        }
        else if (targetObject->IsRelationship()) {
            _SetRelationship(targetObject->AsRelationship());
        } else {
            TF_VERIFY(
                false,
                "Unable to traverse to path <%s>. Unhandled object type.",
                path.GetText());
            return false;
        }

        return true;
    }

    // Returns the schema config key to use in an output key for the given
    // computation definition; i.e., this config key will be used for
    // computation lookup for any inputs that request dispatched computations.
    //
    EsfSchemaConfigKey _GetDispatchingConfigKeyForOutputKey(
        const Exec_ComputationDefinition *const computationDefinition)
    {
        // If we found a dispatched computation, then we pass along the
        // dispatching schema config key, to support recursive dispatching.
        if (computationDefinition->IsDispatched()) {
            TF_VERIFY(_dispatchingSchemaKey != EsfSchemaConfigKey());
            return _dispatchingSchemaKey;
        }

        // Otherwise, return the current provider's schema config key.
        return _currentObject->GetSchemaConfigKey(_journal);
    }

    // Returns the ouput keys for the objects targeted by the forwarded targets
    // of the currrent relationship, for the computation of the given name and
    // result type.
    //
    // The current object must be a valid relationship prior to calling this
    // method.
    //
    Exec_OutputKeyVector _TraverseToRelationshipTargetedObjects(
        const TfToken &computationName,
        const TfType resultType)
    {
        if (!TF_VERIFY(_currentRelationship->IsValid(_journal))) {
            return {};
        }

        Exec_OutputKeyVector outputKeys;

        for (const SdfPath &path :
             _currentRelationship->GetForwardedTargets(_journal)) {
            if (!_TraverseToAbsolutePath(path)) {
                continue;
            }

            if (const Exec_ComputationDefinition *const computationDefinition =
                _FindComputationDefinition(computationName, resultType)) {
                outputKeys.emplace_back(
                    _currentObject->AsObject(),
                    _GetDispatchingConfigKeyForOutputKey(computationDefinition),
                    computationDefinition);
            }
        }

        // Clear the current object to make it clear that the traversal has
        // terminated.
        _currentObjectVariant = std::monostate{};
        _currentObject = nullptr;
        _currentRelationship = nullptr;

        return outputKeys;
    }

    // Updates the current object to the nearest namespace ancestor that 
    // a computation named \p computationName with the given \p resultType.
    //
    // The current object must already refer to a valid prim, or else this
    // returns false and the current object is not modified.
    //
    // If this returns true, then the current object is set to the ancestor that
    // defines the desired computation, and a pointer to the definition of that
    // computation will be stored in \p foundComputationDefinition.
    //
    // If no such ancestor can provide the requested computation, then the
    // current object is set to the pseudo-root prim, and this returns false.
    // 
    bool _TraverseToNamespaceAncestor(
        const TfToken &computationName,
        const TfType resultType,
        const Exec_ComputationDefinition **const foundComputationDefinition)
    {
        if (!TF_VERIFY(_currentPrim && !_currentPrim->IsPseudoRoot())) {
            return false;
        }

        if (!_currentPrim->IsValid(_journal)) {
            return false;
        }

        _SetPrim(_currentPrim->GetParent(_journal));
        while (!_currentPrim->IsPseudoRoot())
        {
            const Exec_ComputationDefinition *const computationDefinition =
                _definitionRegistry.GetComputationDefinition(
                    *_currentPrim,
                    computationName,
                    _dispatchingSchemaKey,
                    _journal);
            
            if (computationDefinition &&
                computationDefinition->GetResultType(*_currentPrim, _journal) ==
                resultType) {
                *foundComputationDefinition = computationDefinition;
                return true;
            }

            _SetPrim(_currentPrim->GetParent(_journal));
        }

        return false;
    }

    // Checks the Exec_DefinitionRegistry for a computation registered for the
    // current object.
    //
    // This finds a computation named \p computationName whose result type is
    // \p resultType, but if \p resultType unknown, then the found computaiton
    // may have any result type. (Note that leaf node compilation will request
    // computations of unknown result types).
    //
    // If found, the returned definition may refer to a prim computation or an
    // attribute computation. If not found, this returns nullptr.
    // 
    const Exec_ComputationDefinition *_FindComputationDefinition(
        const TfToken &computationName,
        const TfType resultType) const
    {
        const Exec_ComputationDefinition *computationDefinition = nullptr;

        if (_currentPrim) {
            computationDefinition =
                _definitionRegistry.GetComputationDefinition(
                    *_currentPrim,
                    computationName,
                    _dispatchingSchemaKey,
                    _journal);
        }
        else if (_currentAttribute) {
            computationDefinition =
                _definitionRegistry.GetComputationDefinition(
                    *_currentAttribute,
                    computationName,
                    _journal);
        }

        if (!computationDefinition) {
            return nullptr;
        }

        // If the input key result type is unknown, allow computations of any
        // result type. Otherwise, the expected result type must match the type
        // of the found definition.
        if (resultType.IsUnknown() ||
            resultType ==
            computationDefinition->GetResultType(*_currentObject, _journal)) {
            return computationDefinition;
        }

        return nullptr;
    }

    // Updates the current object by a traversal described by \p inputKey.
    //
    // Returns a vector of output keys, where each key's provider is a valid
    // object resulting from the traversal, and each key's computation is for
    // the requested computation in \p inputKey.
    //
    // If after traversal, the current object is valid, but does not define
    // the computation specified by \p inputKey, then the result does not
    // contain an output key for that object.
    // 
    Exec_OutputKeyVector _ResolveInput(const Exec_InputKey &inputKey)
    {
        if (!TF_VERIFY(_currentObject)) {
            return {};
        }

        const SdfPath &localTraversal =
            inputKey.providerResolution.localTraversal;

        // If the local traversal is the absolute root path, the stage
        // pseudo-root is the provider.
        if (localTraversal.IsAbsoluteRootPath()) {
            _SetPrim(_stage->GetPrimAtPath(localTraversal, _journal));
        }

        // Otherwise, verify we have a valid current object (and thereby journal
        // a dependency on it) and then perform the local traversal.
        else {
            if (!TF_VERIFY(_currentObject->IsValid(_journal))) {
                return {};
            }

            if (!_TraverseToRelativePath(inputKey.providerResolution)) {
                return {};
            }
        }

        const Exec_ComputationDefinition *computationDefinition = nullptr;

        // Perform the dynamic traversal.
        switch (inputKey.providerResolution.dynamicTraversal) {
        case ExecProviderResolution::DynamicTraversal::Local:
            computationDefinition = _FindComputationDefinition(
                inputKey.computationName,
                inputKey.resultType);
            break;
            
        case ExecProviderResolution::DynamicTraversal::
            RelationshipTargetedObjects:
            return _TraverseToRelationshipTargetedObjects(
                inputKey.computationName,
                inputKey.resultType);
            
        case ExecProviderResolution::DynamicTraversal::NamespaceAncestor:
            if (!_TraverseToNamespaceAncestor(
                inputKey.computationName,
                inputKey.resultType,
                &computationDefinition)) {
                return {};
            }
            break;
        }
        
        if (!computationDefinition) {
            return {};
        }

        return {
            {_currentObject->AsObject(),
             _GetDispatchingConfigKeyForOutputKey(computationDefinition),
             computationDefinition}
        };
    }

    // The state of the resolution process is represented by the current scene
    // object. The object may be a prim, an attribute, or a relationship, and it
    // lives inside a std::variant. Pointers to the derived type are maintained
    // so the object can be accessed without having to repeatedly inspect the
    // type held by the variant. The variant can also hold a std::monostate in
    // case the resolver was constructed with neither a prim nor an attribute.
    using _CurrentObjectVariant =
        std::variant<std::monostate, EsfPrim, EsfAttribute, EsfRelationship>;
    _CurrentObjectVariant _currentObjectVariant;
    const EsfObjectInterface *_currentObject;
    const EsfPrimInterface *_currentPrim;
    const EsfAttributeInterface *_currentAttribute;
    const EsfRelationshipInterface *_currentRelationship;

    // Scene traversals log entries to this journal.
    EsfJournal *const _journal;
    const EsfSchemaConfigKey _dispatchingSchemaKey;
    const EsfStageInterface *const _stage;
    const Exec_DefinitionRegistry &_definitionRegistry;
};

} // anonymous namespace

Exec_OutputKeyVector
Exec_ResolveInput(
    const EsfStage &stage,
    const EsfObject &origin,
    const EsfSchemaConfigKey dispatchingSchemaKey,
    const Exec_InputKey &inputKey,
    EsfJournal *const journal)
{
    TRACE_FUNCTION();
    return _InputResolver::ResolveInput(
        stage, origin, dispatchingSchemaKey, inputKey, journal);
}

PXR_NAMESPACE_CLOSE_SCOPE
