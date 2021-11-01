//
// Copyright 2021 Pixar
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
#ifndef PXR_IMAGING_HD_SCENE_INDEX_H
#define PXR_IMAGING_HD_SCENE_INDEX_H

#include "pxr/pxr.h"

#include <set>
#include <unordered_map>

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/singleton.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"

PXR_NAMESPACE_OPEN_SCOPE


///
/// Small struct representing a 'prim' in the Hydra scene index. A prim is
/// represented by a container data source which contains a tree of properties.
///
struct HdSceneIndexPrim
{
    TfToken primType;
    HdContainerDataSourceHandle dataSource;
};

///
/// \class HdSceneIndexBase
///
/// Abstract interface to scene data. This class can be queried for scene
/// data directly, and it can also register observers to be notified about
/// scene changes (see HdSceneIndexObserver).
///
class HdSceneIndexBase : public TfRefBase, public TfWeakBase
{
public:

    HD_API
    virtual ~HdSceneIndexBase();

    // ------------------------------------------------------------------------
    // Scene Observer API
    // ------------------------------------------------------------------------

    /// Adds an observer to this scene index. The given observer will be sent
    /// notices for prims added, removed, or dirtied after it is added as an
    /// observer.  It will not be sent notices for prims already in the scene
    /// index; the calling code is responsible for updating observer state
    /// if the scene index has already been populated. This function is not
    /// threadsafe.
    HD_API
    void AddObserver(const HdSceneIndexObserverPtr &observer);

    /// Removes an observer from this scene index; the given observer will no
    /// longer be forwarded notices. Note that the observer won't get any
    /// notices as a result of being detached from this scene index. If
    /// \p observer is not registered on this scene index, this call does
    /// nothing. This function is not threadsafe.
    HD_API
    void RemoveObserver(const HdSceneIndexObserverPtr &observer);

    // ------------------------------------------------------------------------
    // Scene Data API
    // ------------------------------------------------------------------------

    /// Returns a pair of (prim type, datasource) for the object at
    /// \p primPath. If no such object exists, the type will be the empty
    /// token and the datasource will be null. This function is expected to
    /// be threadsafe.
    virtual HdSceneIndexPrim GetPrim(const SdfPath &primPath) const = 0;

    /// Returns the paths of all scene index prims located immediately below
    /// \p primPath. This function can be used to traverse
    /// the scene by recursing from \p SdfPath::AbsoluteRootPath(); such a
    /// traversal is expected to give the same set of prims as the
    /// flattening of the scene index's \p PrimsAdded and \p PrimsRemoved
    /// messages. This function is expected to be threadsafe.
    virtual SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const = 0;

    /// A convenience function: look up the object at \p primPath, and if
    /// successful return the datasource at \p locator within that prim. This
    /// is equivalent to calling \p GetPrim(primPath), and then calling
    /// \p HdContainerDataSource::Get(prim.dataSource, locator).
    HdDataSourceBaseHandle GetDataSource(
            const SdfPath &primPath,
            const HdDataSourceLocator &locator) const
    {
        return HdContainerDataSource::Get(
                GetPrim(primPath).dataSource, locator);
    }

protected:

    /// Notify attached observers of prims added to the scene. The set of
    /// scene prims compiled from added/removed notices should match the set
    /// from a traversal based on \p GetChildPrimNames. Each prim has a path
    /// and type. It's possible for \p PrimsAdded to be called for prims that
    /// already exist; in that case, observers should be sure to update the
    /// prim type, in case it changed, and resync the prim. This function is
    /// not threadsafe; some observers expect it to be called from a single
    /// thread.
    void _SendPrimsAdded(
        const HdSceneIndexObserver::AddedPrimEntries &entries);

    /// Notify attached observers of prims removed from the scene. Note that
    /// this message is considered hierarchical: if \p /Path is removed,
    /// \p /Path/child is considered removed as well. This function is not
    /// threadsafe; some observers expect it to be called from a single thread.
    void _SendPrimsRemoved(
        const HdSceneIndexObserver::RemovedPrimEntries &entries);

    /// Notify attached observers of datasource invalidations from the scene.
    /// This message is not considered hierarchical on \p primPath; if
    /// \p /Path is dirtied, \p /Path/child is not necessarily dirtied.
    /// However, locators are considered hierarchical: if \p primvars is
    /// dirtied on a prim, \p primvars/color is considered dirtied as well.
    /// This function is not threadsafe; some observers expect it to be called
    /// from a single thread.
    void _SendPrimsDirtied(
        const HdSceneIndexObserver::DirtiedPrimEntries &entries);

    /// Returns whether the scene index has any registered observers; this
    /// information can be used to skip work preparing notices when there are
    /// no observers.
    bool _IsObserved() const;

private:

    using _ObserverSet = std::set<HdSceneIndexObserverPtr>;
    _ObserverSet _observers;
};


///
/// \class HdSceneIndexNameRegistry
///
/// A registry containing named instances of Hydra indexes. Scene Indexes
/// are not automatically registered here, and must be manually added
/// (generally by the application).
///
class HdSceneIndexNameRegistry
    : public TfSingleton<HdSceneIndexNameRegistry> 
{
    friend class TfSingleton<HdSceneIndexNameRegistry>;

    HdSceneIndexNameRegistry() = default;

public:

    /// Returns the singleton-instance of this registry.
    ///
    static HdSceneIndexNameRegistry &GetInstance()  {
        return TfSingleton<HdSceneIndexNameRegistry>::GetInstance();
    }

    /// Registers an \p instance of a scene index with a given \p name.
    ///
    void RegisterNamedSceneIndex(
        const std::string &name, HdSceneIndexBasePtr instance);

    /// Returns the names of all registered scene indexes.
    ///
    std::vector<std::string> GetRegisteredNames();

    /// Returns the scene index that was registered with the given \p name.
    ///
    HdSceneIndexBaseRefPtr GetNamedSceneIndex(const std::string &name);

private:

    using _NamedInstanceMap =
        std::unordered_map<std::string, HdSceneIndexBasePtr>;

    _NamedInstanceMap _namedInstances;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_SCENE_INDEX_H
