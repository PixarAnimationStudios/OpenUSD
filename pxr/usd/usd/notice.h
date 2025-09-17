//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_NOTICE_H
#define PXR_USD_USD_NOTICE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/object.h"

#include "pxr/usd/sdf/changeList.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/notice.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdNotice
///
/// Container class for Usd notices
///
class UsdNotice {
public:

    /// Base class for UsdStage notices.
    class StageNotice : public TfNotice {
    public:
        USD_API
        StageNotice(const UsdStageWeakPtr &stage);
        USD_API
        ~StageNotice() override;

        /// Return the stage associated with this notice.
        const UsdStageWeakPtr &GetStage() const { return _stage; }

    private:
        UsdStageWeakPtr _stage;
    };

    /// \class StageContentsChanged
    ///
    /// Ultra-conservative notice sent when the given UsdStage's contents
    /// have changed in any way.  This notice is sent when \em any authoring
    /// is performed in any of the stage's participatory layers, in the
    /// thread performing the authoring, \em after the affected UsdStage
    /// has reconfigured itself in response to the authored changes.
    ///
    /// Receipt of this notice should cause clients to disregard any cached
    /// values for properties or metadata.  It does not \em necessarily imply
    /// invalidation of UsdPrim s.
    ///
    class StageContentsChanged : public StageNotice {
    public:
        explicit StageContentsChanged(const UsdStageWeakPtr& stage)
            : StageNotice(stage) {}
        USD_API ~StageContentsChanged() override;
    };

    /// \class ObjectsChanged
    ///
    /// Notice sent in response to authored changes that affect UsdObjects.
    ///
    /// The kinds of object changes are divided into these categories: 
    ///
    /// - Object resync:
    /// \parblock
    /// "Resyncs" are potentially structural changes that invalidate entire
    /// subtrees of UsdObjects (including prims and properties).  For example,
    /// if the path "/foo" is resynced, then all subpaths like "/foo/bar" and
    /// "/foo/bar.baz" may be arbitrarily changed.  
    ///
    /// When a prim is resynced, say "/foo/bar", it might have been created or
    /// destroyed. Indication of possible changes flows down the resynced prim 
    /// namespace, implicitly via prim resync notices. We *do not* consider the
    /// parent "/foo" to be resynced, as this would incorrectly imply that
    /// some or all of "/foo/bar"'s siblings (and their descendants) have 
    /// also changed. Additionally, we do not propagate change indication to
    /// objects associated with the changed object through relationships or 
    /// connections.
    /// \endparblock
    ///
    /// - Resolved asset path resync:
    /// \parblock
    /// "Resolved asset path resyncs" invalidate asset paths in a subtree of
    /// objects. Asset paths authored anywhere in this subtree of objects
    /// (e.g. as attribute or metadata values) may now resolve to different
    /// locations, even though the asset path authored in scene description
    /// has not changed.
    ///
    /// \endparblock
    ///
    /// - Changed info:
    /// \parblock
    /// "Changed-info" means that a nonstructural change has occurred, like an
    /// attribute value change or a value change to a metadata field not related
    /// to composition. Unlike resyncs, changed-info notices for an object do 
    /// not imply that the subtree beneath that object have changed.
    ///
    /// \endparblock
    ///
    /// This notice provides API for two client use-cases.  Clients interested
    /// in testing whether specific objects are affected by the changes should
    /// use the methods that return a bool, like AffectedObject(). Clients that
    /// wish to reason about all changes as a whole should use the methods that
    /// return a PathRange, like GetResyncedPaths().
    /// 
    class ObjectsChanged : public StageNotice {

    public:
        /// A type for further classifying objects that have may have been
        /// resynced because of namespace edits. 
        enum class PrimResyncType {
            /// These six types indicate that a resynced object was moved to
            /// a new path, via a UsdNamespaceEditor, and that object at the
            /// new path has the same computed prim stack as the objects at 
            /// the original path did (i.e. the new object composes the 
            /// exact same layer opinions in the exact same order as the 
            /// original object). The old path resync will be classified as a
            /// Source and the new path resync will be classified as a 
            /// Destination.
            RenameSource,
            RenameDestination,
            ReparentSource,
            ReparentDestination,
            RenameAndReparentSource,
            RenameAndReparentDestination,

            /// This Delete type indicates that an object has been removed
            /// from the stage without an indication that it was the source of a
            /// rename and/or a reparent operation.
            Delete,

            /// The UnchangedPrimStack type indicates that the resynced object 
            /// still exists and is effectively unchanged in that it has the 
            /// same composed prim stack as before it was resynced. This can 
            /// occur when composition arcs are changed or dependent layer specs
            /// are moved to maintain prims of dependent stages in a
            /// UsdNamespaceEditor edit.
            UnchangedPrimStack,

            /// This type indicates all other resyncs that we cannot classify
            /// based on namespace edit information. This other type does not 
            /// necesarily imply that we don't have a rename, reparent, noop, 
            /// etc. but rather that we cannot determine the type and need to 
            /// treat it as a full resync.
            Other,

            /// Invalid indicates that the object has not been resynced.
            Invalid
        };

        /// Value type holding a list of property paths that have been renamed
        /// via the UsdNamespaceEditor paired with the new name of the property.
        using RenamedProperties = std::vector<std::pair<SdfPath, TfToken>>;      

    private:
        using _PathsToChangesMap = 
            std::map<SdfPath, std::vector<const SdfChangeList::Entry*>>;

        struct _PrimResyncInfo {
            PrimResyncType resyncType;
            SdfPath associatePath;
        };
        using _PrimResyncInfoMap = std::map<SdfPath, _PrimResyncInfo>;

        struct _NamespaceEditsInfo {
            _PrimResyncInfoMap primResyncsInfo;
            RenamedProperties renamedProperties;
        };

        static const _PathsToChangesMap& _GetEmptyChangesMap();
        static const _NamespaceEditsInfo& _GetEmptyNamespaceEditsInfo();

        friend class UsdStage;
        ObjectsChanged(const UsdStageWeakPtr &stage,
                       const _PathsToChangesMap *resyncChanges,
                       const _PathsToChangesMap *infoChanges,
                       const _PathsToChangesMap *assetPathChanges,
                       const _NamespaceEditsInfo *namespaceEditsInfo)
            : StageNotice(stage)
            , _resyncChanges(resyncChanges)
            , _infoChanges(infoChanges)
            , _assetPathChanges(assetPathChanges)
            , _namespaceEditsInfo(namespaceEditsInfo) {}

        ObjectsChanged(const UsdStageWeakPtr &stage,
                       const _PathsToChangesMap *resyncChanges);

    public:
        USD_API ~ObjectsChanged() override;

        /// Return true if \p obj was possibly affected by the layer changes
        /// that generated this notice.  This is the case if either the object
        /// is subject to a resync or has changed info.  Equivalent to:
        /// \code
        /// ResyncedObject(obj) || ResolvedAssetPathsResynced(obj) || ChangedInfoOnly(obj)
        /// \endcode
        bool AffectedObject(const UsdObject &obj) const {
            return ResyncedObject(obj) || ResolvedAssetPathsResynced(obj) 
                || ChangedInfoOnly(obj);
        }

        /// Return true if \p obj was resynced by the layer changes that
        /// generated this notice.  This is the case if the object's path or an
        /// ancestor path is present in GetResyncedPaths().
        USD_API bool ResyncedObject(const UsdObject &obj) const;

        /// Return true if asset path values in \p obj were resynced by the
        /// layer changes that generated this notice.  This is the case if the
        /// object's path or an ancestor path is present in
        /// GetResolvedAssetPathsResyncedPaths().
        USD_API bool ResolvedAssetPathsResynced(const UsdObject &obj) const;

        /// Return true if \p obj was changed but not resynced by the layer
        /// changes that generated this notice. This is the case if this
        /// object's exact path is present in GetChangedInfoOnlyPaths().
        USD_API bool ChangedInfoOnly(const UsdObject &obj) const;

        /// \class PathRange
        /// An iterable range of paths to objects that have changed.
        ///
        /// Users may use this object in range-based for loops, or use the
        /// iterators to access additional information about each changed
        /// object.
        class PathRange
        {
        public:
            /// \class iterator
            class iterator {
                using _UnderlyingIterator = _PathsToChangesMap::const_iterator;
            public:
                using iterator_category = std::forward_iterator_tag;
                using value_type = const SdfPath&;
                using reference = const SdfPath&;
                using pointer = const SdfPath*;
                using difference_type =
                    typename _UnderlyingIterator::difference_type;

                iterator() = default;
                reference operator*() const { return dereference(); }
                pointer operator->() const { return &(dereference()); }

                iterator& operator++() {
                    ++_underlyingIterator;
                    return *this;
                }

                iterator operator++(int) {
                    iterator result = *this;
                    ++_underlyingIterator;
                    return result;
                }

                bool operator==(const iterator& other) const{
                    return _underlyingIterator == other._underlyingIterator;
                }

                bool operator!=(const iterator& other) const{
                    return _underlyingIterator != other._underlyingIterator;
                }

                /// Return the set of changed fields in layers that affected 
                /// the object at the path specified by this iterator.  See
                /// UsdNotice::ObjectsChanged::GetChangedFields for more
                /// details.
                USD_API TfTokenVector GetChangedFields() const;

                /// Return true if the object at the path specified by this
                /// iterator has any changed fields, false otherwise.  See
                /// UsdNotice::ObjectsChanged::HasChangedFields for more
                /// details.
                USD_API bool HasChangedFields() const;

                /// Returns the underlying iterator
                _UnderlyingIterator GetBase() const {
                    return _underlyingIterator;
                }

                /// \deprecated Use GetBase() instead.
                _UnderlyingIterator base() const {
                    return GetBase();
                }

            private:
                friend class PathRange;

                explicit iterator(_UnderlyingIterator baseIter)
                    : _underlyingIterator(baseIter) {}

                inline reference dereference() const { 
                    return _underlyingIterator->first;
                }

                _UnderlyingIterator _underlyingIterator;
            };

            using const_iterator = iterator;

            PathRange() : _changes(nullptr) { }

            /// Explicit conversion to SdfPathVector for convenience
            explicit operator SdfPathVector() const {
                return SdfPathVector(begin(), end());
            }
        
            /// Return true if this range contains any paths, false otherwise.
            bool empty() const { 
                return !_changes || _changes->empty(); 
            }

            /// Return the number of paths in this range.
            size_t size() const { 
                return _changes ? _changes->size() : 0; 
            }

            /// Return iterator to the start of this range.
            iterator begin() const { 
                return iterator(_changes->cbegin()); 
            }

            /// Return iterator to the start of this range.
            const_iterator cbegin() const { 
                return iterator(_changes->cbegin()); 
            }

            /// Return the end iterator for this range.
            iterator end() const { 
                return iterator(_changes->cend()); 
            }

            /// Return the end iterator for this range.
            const_iterator cend() const { 
                return iterator(_changes->cend()); 
            }

            /// Return an iterator to the specified \p path in this range if
            /// it exists, or end() if it does not.  This is potentially more
            /// efficient than std::find(begin(), end()).
            const_iterator find(const SdfPath& path) const {
                return const_iterator(_changes->find(path));
            }

        private:
            friend class ObjectsChanged;
            explicit PathRange(const _PathsToChangesMap* changes)
                : _changes(changes)
            { }

            const _PathsToChangesMap* _changes;
        };

        /// Return the set of paths that are resynced in lexicographical order.
        /// Resyncs imply entire subtree invalidation of all descendant prims
        /// and properties, so this set is minimal regarding ancestors and
        /// descendants.  For example, if the path '/foo' appears in this set,
        /// the entire subtree at '/foo' is resynced so the path '/foo/bar' will
        /// not appear, but it should be considered resynced.
        ///
        /// Since object resyncs fully invalidate entire subtrees, this set of
        /// paths subsumes all other paths. For example, if the path '/foo'
        /// appears in this set, but an attribute value was changed at
        /// '/foo/bar.x', this notice will only contain '/foo' in the set
        /// returned by this path and empty sets from all other functions. This
        /// is because the change to '/foo/bar.x' is implied by the resync of
        /// '/foo'.
        USD_API PathRange GetResyncedPaths() const;

        /// Return the set of paths that have only info changes (those that do
        /// not affect the structure of cached UsdPrims on a UsdStage) in
        /// lexicographical order.  Info changes do not imply entire subtree
        /// invalidation, so this set is not minimal regarding ancestors and
        /// descendants, as opposed to GetResyncedPaths().  For example, both
        /// the paths '/foo' and '/foo/bar' may appear in this set.
        ///
        /// \note
        /// The "only" in "changed info only paths" was historically meant to 
        /// distinguish these paths from the object resync paths returned by
        /// GetResyncedPaths, since the former is subsumed by the latter. It
        /// is now slightly misleading; paths in "changed info only" are still
        /// subsumed by "object resync" paths, but are *not* subsumed by other
        /// types of changes, like "resolved asset path resyncs".
        USD_API PathRange GetChangedInfoOnlyPaths() const;

        /// Return the set of paths affected by changes that may cause asset
        /// path values to resolve to different locations, even though the asset
        /// path authored in scene description has not changed.  For example,
        /// asset paths using expression variables may be invalidated when a
        /// variable value is modified, even though the authored asset paths
        /// have not changed. The set of paths are returned in lexicographical
        /// order.
        ///
        /// Resolved asset path resyncs imply invalidation of asset paths within
        /// entire subtrees including all descendant prims and properties, so
        /// this set is minimal regarding ancestors and descendants.  For
        /// example, if the path '/foo' appears in this set, all asset paths in
        /// the entire subtree at '/foo' are invalidated, so the path '/foo/bar'
        /// will not appear, but asset paths on that prim should be considered
        /// invalidated.
        USD_API PathRange GetResolvedAssetPathsResyncedPaths() const;

        /// Return the set of changed fields in layers that affected \p obj. 
        ///
        /// This set will be empty for objects whose paths are not in
        /// GetResyncedPaths() or GetChangedInfoOnlyPaths().  
        ///
        /// If a field is present in this set, it does not necessarily mean the 
        /// composed value of that field on \p obj has changed.  For example, if
        /// a metadata value on \p obj is overridden in a stronger layer and
        /// is changed in a weaker layer, that field will appear in this set.
        /// However, since the value in the stronger layer did not change,
        /// the composed value returned by GetMetadata() will not have changed.
        USD_API TfTokenVector GetChangedFields(const UsdObject &obj) const;

        /// \overload
        USD_API TfTokenVector GetChangedFields(const SdfPath &path) const;

        /// Return true if there are any changed fields that affected \p obj,
        /// false otherwise.  See GetChangedFields for more details.
        USD_API bool HasChangedFields(const UsdObject &obj) const;
        
        /// \overload
        USD_API bool HasChangedFields(const SdfPath &path) const;

        /// Returns the type of resync that has occurred for the prim at 
        /// \p primPath. 
        ///
        /// When prims are edited through the UsdNamespaceEditor we'll have
        /// additional information about whether the prim resyncs that have 
        /// occurred are for prims that have been renamed, reparented, or 
        /// just adjusted to maintain composition without changing the prim 
        /// path itself. These are all resyncs that are expected to have no net
        /// effect on the prim's composed contents relative to the original 
        /// prim. This function returns the the prim's resync type based on that
        /// information.
        /// 
        /// If the prim path is the source of a rename and/or reparent operation
        /// the returned type will be a "Source" type and the 
        /// \p associatedPrimPath, if provided, will be set to the path of the 
        /// corresponding destination prim. Likewise, if the prim path is the
        /// destination of a rename and/or reparent operation the returned type
        /// will be a "Destination" type and the \p associatedPrimPath, if 
        /// provided, will be set to the path of the corresponding source prim. 
        /// See PrimSyncType for more information about the other return types.
        USD_API PrimResyncType GetPrimResyncType(
            const SdfPath &primPath,
            SdfPath *associatedPrimPath = nullptr) const;       

        /// Return the list of property paths that have been renamed via a 
        /// UsdNamespaceEditor ApplyEdits operation along with the new names of
        /// those properties. When multiple properties have been edited, this
        /// list will contain them in no particular order.
        const RenamedProperties &GetRenamedProperties() const {
            return _namespaceEditsInfo->renamedProperties;
        }

    private:
        const _PathsToChangesMap *_resyncChanges;
        const _PathsToChangesMap *_infoChanges;
        const _PathsToChangesMap *_assetPathChanges;
        const _NamespaceEditsInfo *_namespaceEditsInfo;
    };

    /// \class StageEditTargetChanged
    ///
    /// Notice sent when a stage's EditTarget has changed.  Sent in the
    /// thread that changed the target.
    ///
    class StageEditTargetChanged : public StageNotice {
    public:
        explicit StageEditTargetChanged(const UsdStageWeakPtr &stage)
            : StageNotice(stage) {}
        USD_API ~StageEditTargetChanged() override;
    };

    /// \class LayerMutingChanged
    ///
    /// Notice sent after a set of layers have been newly muted or unmuted.
    /// Note this does not necessarily mean the specified layers are currently 
    /// loaded.
    ///
    /// LayerMutingChanged notice is sent before any UsdNotice::ObjectsChanged 
    /// or UsdNotice::StageContentsChanged notices are sent resulting from
    /// muting or unmuting of layers.
    /// 
    /// Note that LayerMutingChanged notice is sent even if the
    /// muting/unmuting layer does not belong to the current stage, or is a 
    /// layer that does belong to the current stage but is not yet loaded 
    /// because it is behind an unloaded payload or unselected variant.
    class LayerMutingChanged : public StageNotice {
    public:
        explicit LayerMutingChanged(const UsdStageWeakPtr &stage, 
                const std::vector<std::string>& mutedLayers, 
                const std::vector<std::string>& unmutedLayers)
            : StageNotice(stage),
             _mutedLayers(mutedLayers),
             _unMutedLayers(unmutedLayers) {}

        USD_API ~LayerMutingChanged() override;

        /// Returns the identifier of the layers that were muted.
        ///
        /// The stage's resolver context must be bound when looking up
        /// layers using the returned identifiers to ensure the same layers
        /// that would be used by the stage are found.
        const std::vector<std::string>& GetMutedLayers() const {
            return _mutedLayers;
        }

        /// Returns the identifier of the layers that were unmuted.
        ///
        /// The stage's resolver context must be bound when looking up
        /// layers using the returned identifiers to ensure the same layers
        /// that would be used by the stage are found.
        const std::vector<std::string>& GetUnmutedLayers() const {
            return _unMutedLayers;
        }

    private:
        const std::vector<std::string>& _mutedLayers;
        const std::vector<std::string>& _unMutedLayers;
    };

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_NOTICE_H
