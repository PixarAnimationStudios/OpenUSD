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

#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/object.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/notice.h"

/// \class UsdNotice
///
/// Container class for Usd notices
///
class UsdNotice {
public:

    /// Base class for UsdStage notices.
    class StageNotice : public TfNotice {
    public:
        StageNotice(const UsdStageWeakPtr &stage);
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
        virtual ~StageContentsChanged();
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
        friend class UsdStage;
        ObjectsChanged(const UsdStageWeakPtr &stage,
                       const SdfPathVector *resyncedPaths,
                       const SdfPathVector *changedInfoPaths)
            : StageNotice(stage)
            , _resyncedPaths(resyncedPaths)
            , _changedInfoPaths(changedInfoPaths) {}
    public:
        virtual ~ObjectsChanged();

        /// Return true if \p obj was possibly affected by the layer changes
        /// that generated this notice.  This is the case if either the object
        /// is subject to a resync or has changed info.  Equivalent to:
        /// \code
        /// ResyncedObject(obj) or ChangedInfoOnly(obj)
        /// \endcode
        bool AffectedObject(const UsdObject &obj) const {
            return ResyncedObject(obj) or ChangedInfoOnly(obj);
        }

        /// Return true if \p obj was resynced by the layer changes that
        /// generated this notice.  This is the case if the object's path or an
        /// ancestor path is present in GetResyncedPrimPaths().
        bool ResyncedObject(const UsdObject &obj) const {
            return SdfPathFindLongestPrefix(
                _resyncedPaths->begin(),
                _resyncedPaths->end(),
                obj.GetPath()) != _resyncedPaths->end();
        }

        /// Return true if \p obj was changed but not resynced by the layer
        /// changes that generated this notice.
        bool ChangedInfoOnly(const UsdObject &obj) const {
            return SdfPathFindLongestPrefix(
                _changedInfoPaths->begin(),
                _changedInfoPaths->end(),
                obj.GetPath()) != _changedInfoPaths->end();
        }

        /// Return the set of paths that are resynced in lexicographical order.
        /// Resyncs imply entire subtree invalidation of all descendant prims
        /// and properties, so this set is minimal regarding ancestors and
        /// descendants.  For example, if the path '/foo' appears in this set,
        /// the entire subtree at '/foo' is resynced so the path '/foo/bar' will
        /// not appear, but it should be considered resynced.
        const SdfPathVector &GetResyncedPaths() const {
            return *_resyncedPaths;
        }

        /// Return the set of paths that have only info changes (those that do
        /// not affect the structure of cached UsdPrims on a UsdStage) in
        /// lexicographical order.  Info changes do not imply entire subtree
        /// invalidation, so this set is not minimal regarding ancestors and
        /// descendants, as opposed to GetResyncedPaths().  For example, both
        /// the paths '/foo' and '/foo/bar' may appear in this set.
        const SdfPathVector &GetChangedInfoOnlyPaths() const {
            return *_changedInfoPaths;
        }
    private:
        const SdfPathVector *_resyncedPaths;
        const SdfPathVector *_changedInfoPaths;
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
        virtual ~StageEditTargetChanged();
    };

};

#endif // USD_NOTICE_H
