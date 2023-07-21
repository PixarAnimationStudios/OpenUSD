//
// Copyright 2022 Pixar
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
#ifndef PXR_USD_IMAGING_USDVIEWQ_HYDRA_OBSERVER_H
#define PXR_USD_IMAGING_USDVIEWQ_HYDRA_OBSERVER_H

#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/usdImaging/usdviewq/api.h"


PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdviewqHydraObserver
///
/// Abstracts pieces necessary for implementing a Hydra Scene Browser in a
/// manner convenient for exposing to python.
/// 
/// For C++ code, this offers no benefits over directly implementing an
/// HdSceneIndexObserver. It exists solely in service of the python
/// implementation of Hydra Scene Browser present in usdview.
/// 
/// See extras/imaging/examples/hdui for an example of a C++ direct
/// implementation.
/// 
class UsdviewqHydraObserver
{
public:

    USDVIEWQ_API
    UsdviewqHydraObserver() = default;

    USDVIEWQ_API
    ~UsdviewqHydraObserver();

    /// Returns the names of scene indices previously registered with
    /// HdSceneIndexNameRegistry. It allows a browser to retrieve available
    /// instances without direct interaction with the application.
    USDVIEWQ_API
    static
    std::vector<std::string> GetRegisteredSceneIndexNames();


    /// Target this observer to a scene index with the given name previously
    /// registered via HdSceneIndexNameRegistry
    USDVIEWQ_API
    bool TargetToNamedSceneIndex(const std::string &name);

    using IndexList = std::vector<size_t> ;

    /// Starting from the currently targeted HdSceneIndex, each value in the
    /// \p inputIndices is treated as an index into the result of
    /// HdFilteringSceneIndexBase::GetInputScenes.
    ///
    /// Returns true if each followed index maps to a valid index into the
    /// input scenes of the previous.
    USDVIEWQ_API
    bool TargetToInputSceneIndex(const IndexList &inputIndices);


    /// Returns the display name of the actively targeted scene index.
    /// This display name is currently derived from the C++ typename.
    USDVIEWQ_API
    std::string GetDisplayName();

    /// Starting from the currently targeted HdSceneIndex, each value in the
    /// \p inputIndices is treated as an index into the result of
    /// HdFilteringSceneIndexBase::GetInputScenes.
    /// 
    /// If the scene index reached is a subclass of HdFilteringSceneIndexBase,
    /// the display names of the return value of GetInputScenes is returned.
    /// Otherwise, the return value is empty.
    USDVIEWQ_API
    std::vector<std::string> GetInputDisplayNames(
        const IndexList &inputIndices);

    /// Returns the paths of the immediate children of the specified
    /// \p primPath for the actively observer scene index.
    USDVIEWQ_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath);

    /// Returns the prim type and data source for the specified \p primPath for
    /// the actively observer scene index.
    USDVIEWQ_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath);

    /// Aggregate of HdSceneIndexObserver entry types for easier binding to
    /// python.
    struct NoticeEntry
    {
        NoticeEntry(const HdSceneIndexObserver::AddedPrimEntries &entries)
        : added(entries)
        {}

        NoticeEntry(const HdSceneIndexObserver::RemovedPrimEntries &entries)
        : removed(entries)
        {}

        NoticeEntry(const HdSceneIndexObserver::DirtiedPrimEntries &entries)
        : dirtied(entries)
        {}

        NoticeEntry(const NoticeEntry &other) = default;

        HdSceneIndexObserver::AddedPrimEntries added;
        HdSceneIndexObserver::RemovedPrimEntries removed;
        HdSceneIndexObserver::DirtiedPrimEntries dirtied;
    };

    using NoticeEntryVector = std::vector<NoticeEntry>;

    /// Returns true if there are pending scene change notices. Consumers
    /// of this follow a polling rather than callback pattern.
    USDVIEWQ_API
    bool HasPendingNotices();

    /// Returns (and clears) any accumulated scene change notices. Consumers
    /// of this follow a polling rather than callback pattern.
    USDVIEWQ_API
    NoticeEntryVector GetPendingNotices();

    /// Clears any accumulated scene change notices
    USDVIEWQ_API
    void ClearPendingNotices();

private:

    bool _Target(const HdSceneIndexBaseRefPtr &sceneIndex);

    class _Observer : public HdSceneIndexObserver
    {
    public:
        USDVIEWQ_API
        void PrimsAdded(
                const HdSceneIndexBase &sender,
                const AddedPrimEntries &entries) override;

        USDVIEWQ_API
        void PrimsRemoved(
                const HdSceneIndexBase &sender,
                const RemovedPrimEntries &entries) override;

        USDVIEWQ_API
        void PrimsDirtied(
                const HdSceneIndexBase &sender,
                const DirtiedPrimEntries &entries) override;

        USDVIEWQ_API
        void PrimsRenamed(
                const HdSceneIndexBase &sender,
                const RenamedPrimEntries &entries) override;


        NoticeEntryVector notices;
    };

    HdSceneIndexBaseRefPtr _sceneIndex;
    _Observer _observer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
