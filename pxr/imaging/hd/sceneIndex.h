//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_SCENE_INDEX_H
#define PXR_IMAGING_HD_SCENE_INDEX_H

#include "pxr/pxr.h"

#include <set>
#include <unordered_map>

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/denseHashSet.h"


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
    HdSceneIndexBase();

    HD_API
    ~HdSceneIndexBase() override;

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

    // ------------------------------------------------------------------------
    // System-wide API
    // ------------------------------------------------------------------------

    /// Sends a message with optional arguments to this and any upstream input
    /// scene indices. Scene indices may implement _SystemMessage to provide
    /// custom handling. See systemMessages.h for common message definitions.
    HD_API
    void SystemMessage(
        const TfToken &messageType,
        const HdDataSourceBaseHandle &args);

    // ------------------------------------------------------------------------
    // User Interface Utilities
    // ------------------------------------------------------------------------

    /// Returns a value previously set by SetDisplayName. If no value (or an
    /// empty string) was last set, this returns a symbol-demangled version of
    /// the class type itself. This is in service of user interfaces with views
    /// of scene index chains or graphs.
    HD_API
    std::string GetDisplayName() const;

    /// Allows for scene index instances to be identified in a more contextually
    /// relevant way. This is in service of user interfaces with views of scene
    /// index chains or graphs.
    HD_API
    void SetDisplayName(const std::string &n);

    /// Adds a specified tag token to a scene index instance. This is in service
    /// of user interfaces which want to filter views of a scene index chain
    /// or graph.
    HD_API
    void AddTag(const TfToken &tag);

    /// Removes a specified tag token to a scene index instance.
    /// This is in service of user interfaces which want to filter views of a
    /// scene index chain or graph.
    HD_API
    void RemoveTag(const TfToken &tag);

    /// Returns true if a specified tag token has been added to a scene index
    /// instance. This is in service of user interfaces which want to filter
    /// views of a scene index chain or graph.
    HD_API
    bool HasTag(const TfToken &tag) const;

    /// Returns all tag tokens currently added to a scene index instance. This
    /// is in service of user interfaces which want to filter views of a scene
    /// index chain or graph.
    HD_API
    TfTokenVector GetTags() const;

protected:

    /// Notify attached observers of prims added to the scene. The set of
    /// scene prims compiled from added/removed notices should match the set
    /// from a traversal based on \p GetChildPrimPaths. Each prim has a path
    /// and type. It's possible for \p PrimsAdded to be called for prims that
    /// already exist; in that case, observers should be sure to update the
    /// prim type, in case it changed, and resync the prim. This function is
    /// not threadsafe; some observers expect it to be called from a single
    /// thread.
    HD_API
    void _SendPrimsAdded(
        const HdSceneIndexObserver::AddedPrimEntries &entries);

    /// Notify attached observers of prims removed from the scene. Note that
    /// this message is considered hierarchical: if \p /Path is removed,
    /// \p /Path/child is considered removed as well. This function is not
    /// threadsafe; some observers expect it to be called from a single thread.
    HD_API
    void _SendPrimsRemoved(
        const HdSceneIndexObserver::RemovedPrimEntries &entries);

    /// Notify attached observers of datasource invalidations from the scene.
    /// This message is not considered hierarchical on \p primPath; if
    /// \p /Path is dirtied, \p /Path/child is not necessarily dirtied.
    /// However, locators are considered hierarchical: if \p primvars is
    /// dirtied on a prim, \p primvars/color is considered dirtied as well.
    /// This function is not threadsafe; some observers expect it to be called
    /// from a single thread.
    HD_API
    void _SendPrimsDirtied(
        const HdSceneIndexObserver::DirtiedPrimEntries &entries);


    /// Notify attached observers of prims (and their descendents) which have
    /// been renamed or reparented.
    /// This function is not threadsafe; some observers expect it to be called
    /// from a single thread.
    HD_API
    void _SendPrimsRenamed(
        const HdSceneIndexObserver::RenamedPrimEntries &entries);


    /// Returns whether the scene index has any registered observers; this
    /// information can be used to skip work preparing notices when there are
    /// no observers.
    HD_API
    bool _IsObserved() const;

    /// Implement in order to react directly to system messages sent from
    /// downstream.
    HD_API
    virtual void _SystemMessage(
        const TfToken &messageType,
        const HdDataSourceBaseHandle &args);

private:
    void _RemoveExpiredObservers();

    // Scoped (RAII) helper to manage tracking recursion depth,
    // and to remove expired observers after completing delivery.
    struct _NotifyScope;

    // Registered observers, in order of registration.
    using _Observers = std::vector<HdSceneIndexObserverPtr>;
    _Observers _observers;

    // Count of in-flight observer notifications
    int _notifyDepth;

    // Flag hinting that expired observers may exist.
    bool _shouldRemoveExpiredObservers;

    // User-visible label for this scene index
    std::string _displayName;

    // Tags used to categorize this scene index
    using _TagSet = TfDenseHashSet<TfToken, TfHash, std::equal_to<TfToken>, 8>;
    _TagSet _tags;
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
    HD_API
    static HdSceneIndexNameRegistry &GetInstance()  {
        return TfSingleton<HdSceneIndexNameRegistry>::GetInstance();
    }

    /// Registers an \p instance of a scene index with a given \p name.
    ///
    HD_API
    void RegisterNamedSceneIndex(
        const std::string &name, HdSceneIndexBasePtr instance);

    /// Returns the names of all registered scene indexes.
    ///
    HD_API
    std::vector<std::string> GetRegisteredNames();

    /// Returns the scene index that was registered with the given \p name.
    ///
    HD_API
    HdSceneIndexBaseRefPtr GetNamedSceneIndex(const std::string &name);

private:

    using _NamedInstanceMap =
        std::unordered_map<std::string, HdSceneIndexBasePtr>;

    _NamedInstanceMap _namedInstances;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_SCENE_INDEX_H
