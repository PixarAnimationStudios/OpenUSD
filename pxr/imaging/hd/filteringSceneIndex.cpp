//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include "pxr/base/tf/instantiateSingleton.h"

PXR_NAMESPACE_OPEN_SCOPE

HdEncapsulatingSceneIndexBase *
HdEncapsulatingSceneIndexBase::Cast(const HdSceneIndexBaseRefPtr &scene)
{
    return
        dynamic_cast<HdEncapsulatingSceneIndexBase *>(
            boost::get_pointer(scene));
}

// This is a fallback scene index that we use in case an invalid sceneIndex is
// passed in to the filtering scene index c'tor.
class _NoOpSceneIndex final : public HdSceneIndexBase
{
public:
    static HdSceneIndexBaseRefPtr New()
    {
        return TfCreateRefPtr(new _NoOpSceneIndex());
    }

    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override final
    {
        return { TfToken(), nullptr };
    }

    SdfPathVector
    GetChildPrimPaths(const SdfPath& priMPath) const override final
    {
        return {};
    }
};

HdSingleInputFilteringSceneIndexBase::HdSingleInputFilteringSceneIndexBase(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
    : _inputSceneIndex(inputSceneIndex)
    , _observer(this)
{
    if (inputSceneIndex) {
        inputSceneIndex->AddObserver(HdSceneIndexObserverPtr(&_observer));
    }
    else {
        TF_CODING_ERROR("Invalid input sceneIndex.");
        _inputSceneIndex = _NoOpSceneIndex::New();
    }
}

std::vector<HdSceneIndexBaseRefPtr>
HdSingleInputFilteringSceneIndexBase::GetInputScenes() const
{
    return {_inputSceneIndex};
}

void HdSingleInputFilteringSceneIndexBase::_PrimsRenamed(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RenamedPrimEntries &entries)
{
    HdSceneIndexObserver::RemovedPrimEntries removed;
    HdSceneIndexObserver::AddedPrimEntries added;
    HdSceneIndexObserver::ConvertPrimsRenamedToRemovedAndAdded(
        sender, entries, &removed, &added);

    _PrimsRemoved(sender, removed);
    _PrimsAdded(sender, added);
}

void
HdSingleInputFilteringSceneIndexBase::_Observer::PrimsAdded(
        const HdSceneIndexBase &sender,
        const AddedPrimEntries &entries)
{
    _owner->_PrimsAdded(sender, entries);
}

void
HdSingleInputFilteringSceneIndexBase::_Observer::PrimsRemoved(
        const HdSceneIndexBase &sender,
        const RemovedPrimEntries &entries)
{
    _owner->_PrimsRemoved(sender, entries);
}

void
HdSingleInputFilteringSceneIndexBase::_Observer::PrimsDirtied(
        const HdSceneIndexBase &sender,
        const DirtiedPrimEntries &entries)
{
    _owner->_PrimsDirtied(sender, entries);
}

void
HdSingleInputFilteringSceneIndexBase::_Observer::PrimsRenamed(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RenamedPrimEntries &entries)
{
    _owner->_PrimsRenamed(sender, entries);
}


PXR_NAMESPACE_CLOSE_SCOPE
