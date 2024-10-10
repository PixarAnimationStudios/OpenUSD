//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/sceneIndexUtil.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

#include "pxr/base/trace/trace.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    HD_USE_ENCAPSULATING_SCENE_INDICES,
    false,
    "Whether to use encapsulating scene indices.");

namespace
{

class _EncapsulatingSceneIndexBase : public HdSceneIndexBase
{
public:
    _EncapsulatingSceneIndexBase(
        std::vector<HdSceneIndexBaseRefPtr> inputScenes)
    {
    }
};
    
class _FilteringEncapsulatingSceneIndexBase : public HdFilteringSceneIndexBase
{
public:
    _FilteringEncapsulatingSceneIndexBase(
        std::vector<HdSceneIndexBaseRefPtr> inputScenes)
      : _inputScenes(std::move(inputScenes))
    {
    }
    
    std::vector<HdSceneIndexBaseRefPtr> GetInputScenes() const override
    {
        return _inputScenes;
    }

private:
    const std::vector<HdSceneIndexBaseRefPtr> _inputScenes;
};

template<typename Base>
class _EncapsulatingSceneIndex : public Base, public HdEncapsulatingSceneIndexBase
{
public:
    using This = _EncapsulatingSceneIndex<Base>;

    static TfRefPtr<This> New(
        const std::vector<HdSceneIndexBaseRefPtr> &inputScenes,
        HdSceneIndexBaseRefPtr const &encapsulatedScene)
    {
        return TfCreateRefPtr(
            new _EncapsulatingSceneIndex(
                inputScenes, encapsulatedScene));
    }

    std::vector<HdSceneIndexBaseRefPtr> GetEncapsulatedScenes() const override {
        return { _encapsulatedScene };
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override {
        return _encapsulatedScene->GetPrim(primPath);
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override {
        return _encapsulatedScene->GetChildPrimPaths(primPath);
    }

private:
    _EncapsulatingSceneIndex(
        const std::vector<HdSceneIndexBaseRefPtr> &inputScenes,
        HdSceneIndexBaseRefPtr const &encapsulatedScene)
      : Base(inputScenes)
      , _encapsulatedScene(encapsulatedScene)
      , _observer(this)
    {
        _encapsulatedScene->AddObserver(HdSceneIndexObserverPtr(&_observer));
    }

    class _Observer : public HdSceneIndexObserver
    {
    public:
        _Observer(This *owner)
      : _owner(owner) {}

        void PrimsAdded(
            const HdSceneIndexBase &sender,
            const AddedPrimEntries &entries) override
        {
            _owner->_SendPrimsAdded(entries);
        }

        void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override
        {
            _owner->_SendPrimsRemoved(entries);
        }

        void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override
        {
            _owner->_SendPrimsDirtied(entries);
        }

        void PrimsRenamed(
            const HdSceneIndexBase &sender,
            const RenamedPrimEntries &entries) override
        {
            _owner->_SendPrimsRenamed(entries);
        }
    private:
        This * const _owner;
    };

    HdSceneIndexBaseRefPtr const _encapsulatedScene;

    _Observer _observer;
};

// Validation code.

using _SceneIndexSet = std::unordered_set<HdSceneIndexBasePtr, TfHash>;

void
_RecurseInputScenes(HdSceneIndexBasePtr const &sceneIndex,
                    const _SceneIndexSet &givenInputScenes,
                    _SceneIndexSet * const allInputScenes,
                    _SceneIndexSet * const externalScenes)
{
    const auto [it, inserted] = allInputScenes->insert(sceneIndex);
    if (!inserted) {
        return;
    }

    if (givenInputScenes.count(sceneIndex)) {
        externalScenes->insert(sceneIndex);
        return;
    }

    auto const filteringSceneIndex =
        TfDynamic_cast<HdFilteringSceneIndexBasePtr>(sceneIndex);
    if (!filteringSceneIndex) {
        return;
    }

    for (HdSceneIndexBaseRefPtr const &inputScene
             : filteringSceneIndex->GetInputScenes()) {
        _RecurseInputScenes(
            inputScene,
            givenInputScenes,
            allInputScenes,
            externalScenes);
    }
}

void
_ValidateInputScenesCanBeReached(
    const std::vector<HdSceneIndexBaseRefPtr> &inputScenes,
    HdSceneIndexBaseRefPtr const &encapsulatedScene)
{
    TRACE_FUNCTION();

    const _SceneIndexSet givenInputScenes(
        inputScenes.begin(), inputScenes.end());

    _SceneIndexSet allInputScenes;
    _SceneIndexSet externalScenes;

    _RecurseInputScenes(
        encapsulatedScene,
        givenInputScenes,
        &allInputScenes,
        &externalScenes);

    if (givenInputScenes.size() == externalScenes.size()) {
        return;
    }

    std::string missingInputs;
    for (HdSceneIndexBasePtr const &inputScene : givenInputScenes) {
        if (externalScenes.count(inputScene)) {
            continue;
        }
        if (!missingInputs.empty()) {
            missingInputs += ", ";
        }
        if (inputScene) {
            missingInputs += inputScene->GetDisplayName();
        } else {
            missingInputs += "[NULL]";
        }
    }

    TF_CODING_ERROR(
        "In HdMakeEncapsulatingSceneIndex, the following given input scenes "
        "could not be reached from the encapsulated scene (%s): %s.",
        encapsulatedScene->GetDisplayName().c_str(),
        missingInputs.c_str());
}

}

HdSceneIndexBaseRefPtr
HdMakeEncapsulatingSceneIndex(
    const std::vector<HdSceneIndexBaseRefPtr> &inputScenes,
    HdSceneIndexBaseRefPtr const &encapsulatedScene)
{
    if (!encapsulatedScene) {
        if (!inputScenes.empty()) {
            TF_CODING_ERROR("Expected encapsulatedScene");
        }
        return TfNullPtr;
    }

    if (inputScenes.empty()) {
        using Base = _EncapsulatingSceneIndexBase;
        return _EncapsulatingSceneIndex<Base>::New(
            inputScenes, encapsulatedScene);
    } else {
        // Raise coding error if the input scenes cannot be
        // reached from the encapsulated scene.
        _ValidateInputScenesCanBeReached(
            inputScenes, encapsulatedScene);

        using Base = _FilteringEncapsulatingSceneIndexBase;
        return _EncapsulatingSceneIndex<Base>::New(
            inputScenes, encapsulatedScene);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
