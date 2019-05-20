//
// Copyright 2016 Pixar
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
#ifndef USD_NOTICE_H
#define USD_NOTICE_H

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
        virtual ~StageNotice();

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
        USD_API virtual ~StageContentsChanged();
    };

    /// \class ObjectsChanged
    ///
    /// Notice sent in response to authored changes that affect UsdObjects.
    ///
    /// The kinds of object changes are divided into two categories: "resync"
    /// and "changed-info".  "Resyncs" are potentially structural changes that
    /// invalidate entire subtrees of UsdObjects (including prims and
    /// properties).  For example, if the path "/foo" is resynced, then all
    /// subpaths like "/foo/bar" and "/foo/bar.baz" may be arbitrarily changed.
    /// In contrast, "changed-info" means that a nonstructural change has
    /// occurred, like an attribute value change or a value change to a metadata
    /// field not related to composition.
    ///
    /// When a prim is resynced, say "/foo/bar", it might have been created or
    /// destroyed.  In that case "/foo"'s list of children will have changed,
    /// but we *do not* consider "/foo" to be resynced.  If we did, it would
    /// mean clients would have to consider all of "/foo/bar"'s siblings (and
    /// their descendants) to be resynced which might be egregious
    /// overinvalidation.
    ///
    /// This notice provides API for two client use-cases.  Clients interested
    /// in testing whether specific objects are affected by the changes should
    /// use the AffectedObject() method (and the ResyncedObject() and
    /// ChangedInfoOnly() methods).  Clients that wish to reason about all
    /// changes as a whole should use the GetResyncedPaths() and
    /// GetChangedInfoOnlyPaths() methods.
    ///
    class ObjectsChanged : public StageNotice {
        using _PathsToChangesMap = 
            std::map<SdfPath, std::vector<const SdfChangeList::Entry*>>;

        friend class UsdStage;
        ObjectsChanged(const UsdStageWeakPtr &stage,
                       const _PathsToChangesMap *resyncChanges,
                       const _PathsToChangesMap *infoChanges)
            : StageNotice(stage)
            , _resyncChanges(resyncChanges)
            , _infoChanges(infoChanges) {}
    public:
        USD_API virtual ~ObjectsChanged();

        /// Return true if \p obj was possibly affected by the layer changes
        /// that generated this notice.  This is the case if either the object
        /// is subject to a resync or has changed info.  Equivalent to:
        /// \code
        /// ResyncedObject(obj) or ChangedInfoOnly(obj)
        /// \endcode
        bool AffectedObject(const UsdObject &obj) const {
            return ResyncedObject(obj) || ChangedInfoOnly(obj);
        }

        /// Return true if \p obj was resynced by the layer changes that
        /// generated this notice.  This is the case if the object's path or an
        /// ancestor path is present in GetResyncedPrimPaths().
        USD_API bool ResyncedObject(const UsdObject &obj) const;

        /// Return true if \p obj was changed but not resynced by the layer
        /// changes that generated this notice.
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
            class iterator : public boost::iterator_adaptor<
                iterator,                           // crtp base,
                _PathsToChangesMap::const_iterator, // base iterator
                const SdfPath&                      // value type
            >
            {
            public:
                iterator() 
                    : iterator_adaptor_(base_type()) {}

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

            private:
                friend class PathRange;
                friend class boost::iterator_core_access;

                iterator(base_type baseIter) 
                    : iterator_adaptor_(baseIter) {}
                inline reference dereference() const { 
                    return base()->first;
                }
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
        USD_API PathRange GetResyncedPaths() const;

        /// Return the set of paths that have only info changes (those that do
        /// not affect the structure of cached UsdPrims on a UsdStage) in
        /// lexicographical order.  Info changes do not imply entire subtree
        /// invalidation, so this set is not minimal regarding ancestors and
        /// descendants, as opposed to GetResyncedPaths().  For example, both
        /// the paths '/foo' and '/foo/bar' may appear in this set.
        USD_API PathRange GetChangedInfoOnlyPaths() const;

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

    private:
        const _PathsToChangesMap *_resyncChanges;
        const _PathsToChangesMap *_infoChanges;
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
        USD_API virtual ~StageEditTargetChanged();
    };

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_NOTICE_H
