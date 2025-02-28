//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_FILTERING_SCENE_INDEX_H
#define PXR_IMAGING_HD_FILTERING_SCENE_INDEX_H

#include "pxr/pxr.h"

#include <set>
#include <unordered_map>

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/singleton.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdFilteringSceneIndexBase);

///
/// \class HdFilteringSceneIndexBase
///
/// An abstract base class for scene indexes that have one or more input scene
/// indexes which serve as a basis for their own scene.
///
class HdFilteringSceneIndexBase : public HdSceneIndexBase
{
public:
    virtual std::vector<HdSceneIndexBaseRefPtr> GetInputScenes() const = 0;
};

///
/// \class HdEncapsulatingSceneIndexBase
///
/// A mix-in class for scene indices that implement their behaviour by
/// creating other scene indices (internally).
///
/// Note that this can be combined with the HdFilteringSceneIndexBase.
///
/// The intention here is that we can traverse the scene index topology
/// at different levels of detail in, e.g., a piece of software to display
/// the scene index graph.
///
/// More precisely, the topology of scene indices should be imagined as a
/// nested directed acyclic graph, that is, each node of the graph itself
/// contains a graph. The high-level directed acyclic graph structure is
/// obtained by recursing GetInputScenes. A node itself contains a graph
/// if the node corresponds to an encapsulating scene index. This nested
/// graph consists of the scene indices internal to the encapsulating scene
/// index as defined below. We also need some extra information in how
/// some of the internal scene indices are connected to the external scene
/// indices to completely describe the scene index topology.
///
/// Given a scene index that is both an HdFilteringSceneIndexBase and an
/// HdEncapsulatingSceneIndexBase, we call the result of GetInputScenes()
/// the "external" scene indices. Now consider the scene indices that can be
/// reached by first calling GetEncapsulatingScenes and then recursing
/// GetInputScenes until we hit an external scene index. We call these scene
/// indices "internal". If the scene index is not subclassing from
/// HdFilteringSceneIndexBase, we compute the internal scene indices in the
/// same way under the premise that there are no external scene indices.
///
/// If this mix-in class is combined with HdFilteringSceneIndexBase, then
/// GetInputScenes() should be a subset of the scene indices obtained by
/// recursively calling GetInputScenes or GetEncapsulatedScenes (or a mix of
/// those).
///
///
/// Example:
/// B filtering scene index with inputs {A}
/// C filtering scene index with inputs {B}
/// D filtering scene index with inputs {B}
/// E filtering scene index with inputs {C, D}
/// F filtering and encapsulating scene index with inputs {B} and
///                 encapsulated scenes {E}
/// G filtering scene index with inputs {F}
///
/// Nested scene index Graph:
///
///                A
///                |
///                B
///               / \                                  |
///      -F------/---\--------
///      |      /     \      |
///      |     C       D     |
///      |      \     /      |
///      |       \   /       |
///      |        \ /        |
///      |         E         |
///      |                   |
///      ---------------------
///                |
///                G
///
class HdEncapsulatingSceneIndexBase
{
public:
    virtual std::vector<HdSceneIndexBaseRefPtr>
    GetEncapsulatedScenes() const = 0;

    static HdEncapsulatingSceneIndexBase * Cast(const HdSceneIndexBaseRefPtr &);
};

TF_DECLARE_WEAK_AND_REF_PTRS(HdSingleInputFilteringSceneIndexBase);

///
/// \class HdSingleInputFilteringSceneIndexBase
///
/// An abstract base class for a filtering scene index that observes a single 
/// input scene index.
///
class HdSingleInputFilteringSceneIndexBase : public HdFilteringSceneIndexBase
{
public:
    HD_API
    std::vector<HdSceneIndexBaseRefPtr> GetInputScenes() const final;

protected:
    HD_API
    HdSingleInputFilteringSceneIndexBase(
            const HdSceneIndexBaseRefPtr &inputSceneIndex);

    virtual void _PrimsAdded(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::AddedPrimEntries &entries) = 0;

    virtual void _PrimsRemoved(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::RemovedPrimEntries &entries) = 0;

    virtual void _PrimsDirtied(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::DirtiedPrimEntries &entries) = 0;

    // Base implementation converts prim removed messages.
    HD_API
    virtual void _PrimsRenamed(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::RenamedPrimEntries &entries);

    /// Returns the input scene.  
    ///
    /// It is always safe to call and dereference this return value.  If this
    /// was constructed with a null scene index, a fallback one will be used.
    const HdSceneIndexBaseRefPtr &_GetInputSceneIndex() const {
        return _inputSceneIndex;
    }

private:

    HdSceneIndexBaseRefPtr _inputSceneIndex;

    friend class _Observer;

    class _Observer : public HdSceneIndexObserver
    {
    public:
        _Observer(HdSingleInputFilteringSceneIndexBase *owner)
        : _owner(owner) {}

        HD_API
        void PrimsAdded(
                const HdSceneIndexBase &sender,
                const AddedPrimEntries &entries) override;

        HD_API
        void PrimsRemoved(
                const HdSceneIndexBase &sender,
                const RemovedPrimEntries &entries) override;

        HD_API
        void PrimsDirtied(
                const HdSceneIndexBase &sender,
                const DirtiedPrimEntries &entries) override;

        HD_API
        void PrimsRenamed(
                const HdSceneIndexBase &sender,
                const RenamedPrimEntries &entries) override;
    private:
        HdSingleInputFilteringSceneIndexBase *_owner;
    };

    _Observer _observer;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_FILTERING_SCENE_INDEX_H
