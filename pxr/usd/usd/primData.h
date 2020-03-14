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
#ifndef PXR_USD_USD_PRIM_DATA_H
#define PXR_USD_USD_PRIM_DATA_H

/// \file usd/primData.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/primFlags.h"
#include "pxr/usd/usd/primDefinition.h"
#include "pxr/usd/usd/primTypeInfo.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/pointerAndBits.h"
#include "pxr/base/tf/token.h"

#include "pxr/usd/sdf/path.h"

#include <boost/range/iterator_range.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/intrusive_ptr.hpp>

#include <atomic>
#include <cstdint>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(UsdStage);

// Private class that stores cached prim information and defines the prim tree
// on a UsdStage.
//
// Usd_PrimData objects are arranged in a tree structure, represented as a
// binary tree.  See the _firstChild and _nextSiblingOrParent members.
//
// UsdStage builds and manages the tree structure of Usd_PrimData objects.  The
// Usd_PrimData objects lifetimes are governed by an internal reference count
// (see _refCount).  Two objects mutate this reference count: UsdStage owns
// references to all the Usd_PrimData objects that represent the scene graph,
// and UsdObject (and by inheritance its subclasses) owns a reference to its
// prim data object via Usd_PrimDataHandle.
//
// Usd_PrimData has a 'dead' flag (see _IsDead and _MarkDead).  UsdStage sets
// this when a prim data object is removed from the scene graph.
// Usd_PrimDataHandle, which is a smart pointer to Usd_PrimData consults this
// dead flag to determine prim validity, and to issue informative crash messages
// on invalid use.  See USD_CHECK_ALL_PRIM_ACCESSES.
//
class Usd_PrimData
{
public:

    // --------------------------------------------------------------------- //
    /// \name Prim Data & Behavior
    // --------------------------------------------------------------------- //

    /// Returns the composed path for the prim.
    ///
    /// This path is absolute with respect to the current stage and may require
    /// translation when used in the context of individual layers of which the 
    /// current stage is composed.
    /// This always returns a cached result.
    const SdfPath &GetPath() const { return _path; }

    const TfToken &GetName() const { return GetPath().GetNameToken(); }

    UsdStage *GetStage() const { return _stage; }

    /// Returns the prim definition for this prim.
    const UsdPrimDefinition &GetPrimDefinition() const {
        return _primTypeInfo->GetPrimDefinition();
    }

    /// Returns the composed type name for the prim.
    /// Note that this value is cached and is efficient to query.
    const TfToken& GetTypeName() const { 
        return _primTypeInfo->GetTypeName(); 
    }

    /// Return true if this prim is active, meaning neither it nor any of its
    /// ancestors have active=false.  Return false otherwise.
    bool IsActive() const { return _flags[Usd_PrimActiveFlag]; }

    /// Return true if this prim is active, and \em either it is loadable and
    /// it is loaded, \em or its nearest loadable ancestor is loaded, \em or it
    /// has no loadable ancestor; false otherwise.
    bool IsLoaded() const { return _flags[Usd_PrimLoadedFlag]; }

    /// Return true if this prim is a model based on its kind metadata, false
    /// otherwise.
    bool IsModel() const { return _flags[Usd_PrimModelFlag]; }

    /// Return true if this prim is a model group based on its kind metadata,
    /// false otherwise.  If this prim is a group, it is also necessarily a
    /// model.
    bool IsGroup() const { return _flags[Usd_PrimGroupFlag]; }

    /// Return true if this prim or any of its ancestors is a class.
    bool IsAbstract() const { return _flags[Usd_PrimAbstractFlag]; }

    /// Return true if this prim and all its ancestors have defining specifiers,
    /// false otherwise. \sa SdfIsDefiningSpecifier.
    bool IsDefined() const { return _flags[Usd_PrimDefinedFlag]; }

    /// Return true if this prim has a specifier of type SdfSpecifierDef
    /// or SdfSpecifierClass.
    bool HasDefiningSpecifier() const {
        return _flags[Usd_PrimHasDefiningSpecifierFlag]; 
    }

    /// Return true if this prim has one or more payload composition arcs.
    bool HasPayload() const { return _flags[Usd_PrimHasPayloadFlag]; }

    /// Return true if attributes on this prim may have opinions in clips, 
    /// false otherwise. If true, the relevant clips will be examined for
    /// opinions during value resolution.
    bool MayHaveOpinionsInClips() const { return _flags[Usd_PrimClipsFlag]; }

    /// Return this prim's composed specifier.
    USD_API
    SdfSpecifier GetSpecifier() const;

    // --------------------------------------------------------------------- //
    /// \name Parent & Stage
    // --------------------------------------------------------------------- //

    /// Return this prim's parent prim.  Return nullptr if this is a root prim.
    USD_API
    Usd_PrimDataConstPtr GetParent() const;

    // --------------------------------------------------------------------- //
    // PrimIndex access.
    // --------------------------------------------------------------------- //

    /// Return a const reference to the PcpPrimIndex for this prim.
    ///
    /// The prim's PcpPrimIndex can be used to examine the scene description
    /// sites that contribute to the prim's property and metadata values in
    /// minute detail.
    ///
    /// For master prims this prim index will be empty; this ensures
    /// that these prims do not provide any attribute or metadata
    /// values. 
    ///
    /// For all other prims in masters, this is the prim index for the 
    /// instance that was chosen to serve as the master for all other 
    /// instances.  
    ///
    /// In either of the above two cases, this prim index will not have the 
    /// same path as the prim's path.
    USD_API
    const class PcpPrimIndex &GetPrimIndex() const;

    /// Return a const reference to the source PcpPrimIndex for this prim.
    ///
    /// For all prims in masters (which includes the master prim itself), 
    /// this is the prim index for the instance that was chosen to serve
    /// as the master for all other instances.  This prim index will not
    /// have the same path as the prim's path.
    USD_API
    const class PcpPrimIndex &GetSourcePrimIndex() const;

    // --------------------------------------------------------------------- //
    // Tree Structure
    // --------------------------------------------------------------------- //

    // Return this prim data's first child if it has one, nullptr otherwise.
    Usd_PrimDataPtr GetFirstChild() const { return _firstChild; }

    // Return this prim data's next sibling if it has one, nullptr otherwise.
    Usd_PrimDataPtr GetNextSibling() const {
        return !_nextSiblingOrParent.BitsAs<bool>() ?
            _nextSiblingOrParent.Get() : nullptr;
    }

    // Return this prim data's parent if this prim data is the last in its chain
    // of siblings.  That is, if the _nextSiblingOrParent field is pointing to
    // its parent.  Return nullptr otherwise.
    Usd_PrimDataPtr GetParentLink() const {
        return _nextSiblingOrParent.BitsAs<bool>() ?
            _nextSiblingOrParent.Get() : nullptr;
    }

    // Return the next prim data "to the right" of this one.  That is, this
    // prim's next sibling if it has one, otherwise the next sibling of the
    // nearest ancestor with a sibling, if there is one, otherwise null.
    inline Usd_PrimDataPtr GetNextPrim() const {
        if (Usd_PrimDataPtr sibling = GetNextSibling())
            return sibling;
        for (Usd_PrimDataPtr p = GetParentLink(); p; p = p->GetParentLink()) {
            if (Usd_PrimDataPtr sibling = p->GetNextSibling())
                return sibling;
        }
        return nullptr;
    }
    
    // Return the prim data at \p path.  If \p path indicates a prim
    // beneath an instance, return the prim data for the corresponding 
    // prim in the instance's master.
    USD_API Usd_PrimDataConstPtr 
    GetPrimDataAtPathOrInMaster(const SdfPath &path) const;

    // --------------------------------------------------------------------- //
    // Instancing
    // --------------------------------------------------------------------- //

    /// Return true if this prim is an instance of a shared master prim,
    /// false otherwise.
    bool IsInstance() const { return _flags[Usd_PrimInstanceFlag]; }

    /// Return true if this prim is a shared master prim, false otherwise.
    bool IsMaster() const { return IsInMaster() && GetPath().IsRootPrimPath(); }

    /// Return true if this prim is a child of a shared master prim,
    /// false otherwise.
    bool IsInMaster() const { return _flags[Usd_PrimMasterFlag]; }

    /// If this prim is an instance, return the prim data for the corresponding
    /// master.  Otherwise, return nullptr.
    USD_API Usd_PrimDataConstPtr GetMaster() const;

    // --------------------------------------------------------------------- //
    // Private Members
    // --------------------------------------------------------------------- //
private:

    USD_API
    Usd_PrimData(UsdStage *stage, const SdfPath& path);
    USD_API
    ~Usd_PrimData();

    // Compute and store type info and cached flags.
    void _ComposeAndCacheTypeAndFlags(
        Usd_PrimDataConstPtr parent, bool isMasterPrim);

    // Flags direct access for Usd_PrimFlagsPredicate.
    friend class Usd_PrimFlagsPredicate;
    const Usd_PrimFlagBits &_GetFlags() const {
        return _flags;
    }

    // --------------------------------------------------------------------- //
    // Prim Children
    // --------------------------------------------------------------------- //

    // Composes the prim children, reporting errors as they occur. Returns true
    // on success false on failure.
    bool _ComposePrimChildNames(TfTokenVector* nameOrder);

    void _SetSiblingLink(Usd_PrimDataPtr sibling) {
        _nextSiblingOrParent.Set(sibling, /* isParent */ false);
    }

    void _SetParentLink(Usd_PrimDataPtr parent) {
        _nextSiblingOrParent.Set(parent, /* isParent */ true);
    }

    // Set the dead bit on this prim data object.
    void _MarkDead() {
        _flags[Usd_PrimDeadFlag] = true;
        _stage = nullptr;
        _primIndex = nullptr;
    }

    // Return true if this prim's dead flag is set, false otherwise.
    bool _IsDead() const { return _flags[Usd_PrimDeadFlag]; }

    // Set whether this prim or any of its namespace ancestors had clips
    // specified.
    void _SetMayHaveOpinionsInClips(bool hasClips) {
        _flags[Usd_PrimClipsFlag] = hasClips;
    }

    typedef boost::iterator_range<
        class Usd_PrimDataSiblingIterator> SiblingRange;

    inline class Usd_PrimDataSiblingIterator _ChildrenBegin() const;
    inline class Usd_PrimDataSiblingIterator _ChildrenEnd() const;
    inline SiblingRange _GetChildrenRange() const;

    typedef boost::iterator_range<
        class Usd_PrimDataSubtreeIterator> SubtreeRange;

    inline class Usd_PrimDataSubtreeIterator _SubtreeBegin() const;
    inline class Usd_PrimDataSubtreeIterator _SubtreeEnd() const;
    inline SubtreeRange _GetSubtreeRange() const;

    // Data members.
    UsdStage *_stage;
    const PcpPrimIndex *_primIndex;
    SdfPath _path;
    const Usd_PrimTypeInfo *_primTypeInfo;
    Usd_PrimData *_firstChild;
    TfPointerAndBits<Usd_PrimData> _nextSiblingOrParent;
    mutable std::atomic<int64_t> _refCount;
    Usd_PrimFlagBits _flags;

    // intrusive_ptr core primitives implementation.
    friend void intrusive_ptr_add_ref(const Usd_PrimData *prim) {
        prim->_refCount.fetch_add(1, std::memory_order_relaxed);
    }
    friend void intrusive_ptr_release(const Usd_PrimData *prim) {
        if (prim->_refCount.fetch_sub(1, std::memory_order_release) == 1)
            delete prim;
    }

    USD_API
    friend void Usd_IssueFatalPrimAccessError(Usd_PrimData const *p);
    friend std::string
    Usd_DescribePrimData(const Usd_PrimData *p, SdfPath const &proxyPrimPath);

    friend inline bool Usd_IsDead(Usd_PrimData const *p) {
        return p->_IsDead();
    }

    friend class UsdPrim;
    friend class UsdStage;
};

// Sibling iterator class.
class Usd_PrimDataSiblingIterator : public boost::iterator_adaptor<
    Usd_PrimDataSiblingIterator,                  // crtp.
    Usd_PrimData *,                               // base iterator.
    Usd_PrimData *,                               // value.
    boost::forward_traversal_tag,                 // traversal.
    Usd_PrimData *                                // reference.
    >
{
public:
    // Default ctor.
    Usd_PrimDataSiblingIterator() {}

private:
    friend class Usd_PrimData;

    // Constructor used by Prim.
    Usd_PrimDataSiblingIterator(const base_type &i)
        : iterator_adaptor_(i) {}

    // Core primitives implementation.
    friend class boost::iterator_core_access;
    reference dereference() const { return base(); }
    void increment() {
        base_reference() = base_reference()->GetNextSibling();
    }
};

// Sibling range.
typedef boost::iterator_range<
    class Usd_PrimDataSiblingIterator> Usd_PrimDataSiblingRange;

// Inform TfIterator it should feel free to make copies of the range type.
template <>
struct Tf_ShouldIterateOverCopy<
    Usd_PrimDataSiblingRange> : boost::true_type {};
template <>
struct Tf_ShouldIterateOverCopy<
    const Usd_PrimDataSiblingRange> : boost::true_type {};

Usd_PrimDataSiblingIterator
Usd_PrimData::_ChildrenBegin() const
{
    return Usd_PrimDataSiblingIterator(_firstChild);
}

Usd_PrimDataSiblingIterator
Usd_PrimData::_ChildrenEnd() const
{
    return Usd_PrimDataSiblingIterator(0);
}

Usd_PrimData::SiblingRange
Usd_PrimData::_GetChildrenRange() const
{
    return Usd_PrimData::SiblingRange(_ChildrenBegin(), _ChildrenEnd());
}


// Tree iterator class.
class Usd_PrimDataSubtreeIterator : public boost::iterator_adaptor<
    Usd_PrimDataSubtreeIterator,                  // crtp.
    Usd_PrimData *,                               // base iterator.
    Usd_PrimData *,                               // value.
    boost::forward_traversal_tag,                 // traversal.
    Usd_PrimData *                                // reference.
    >
{
public:
    // Default ctor.
    Usd_PrimDataSubtreeIterator() {}

private:
    friend class Usd_PrimData;
    friend class UsdPrimSubtreeIterator;

    // Constructor used by Prim.
    Usd_PrimDataSubtreeIterator(const base_type &i)
        : iterator_adaptor_(i) {}

    // Core primitives implementation.
    friend class boost::iterator_core_access;
    reference dereference() const { return base(); }
    void increment() {
        base_type &b = base_reference();
        b = b->GetFirstChild() ? b->GetFirstChild() : b->GetNextPrim();
    }
};

// Tree range.
typedef boost::iterator_range<
    class Usd_PrimDataSubtreeIterator> Usd_PrimDataSubtreeRange;

// Inform TfIterator it should feel free to make copies of the range type.
template <>
struct Tf_ShouldIterateOverCopy<
    Usd_PrimDataSubtreeRange> : boost::true_type {};
template <>
struct Tf_ShouldIterateOverCopy<
    const Usd_PrimDataSubtreeRange> : boost::true_type {};

Usd_PrimDataSubtreeIterator
Usd_PrimData::_SubtreeBegin() const
{
    return Usd_PrimDataSubtreeIterator(
        _firstChild ? _firstChild : GetNextPrim());
}

Usd_PrimDataSubtreeIterator
Usd_PrimData::_SubtreeEnd() const
{
    return Usd_PrimDataSubtreeIterator(GetNextPrim());
}

Usd_PrimData::SubtreeRange
Usd_PrimData::_GetSubtreeRange() const
{
    return Usd_PrimData::SubtreeRange(_SubtreeBegin(), _SubtreeEnd());
}

// Helpers for instance proxies.

// Return true if the prim with prim data \p p and proxy prim path
// \p proxyPrimPath represents an instance proxy.
template <class PrimDataPtr>
inline bool
Usd_IsInstanceProxy(const PrimDataPtr &p, const SdfPath &proxyPrimPath)
{
    return !proxyPrimPath.IsEmpty();
}

// Helpers for subtree traversals.

// Create a predicate based on \p pred for use when traversing the
// siblings or descendants of the prim with prim data \p p and proxy
// prim path \p proxyPrimPath. This is used by prim traversal functions 
// like UsdPrim::GetFilteredChildren, UsdPrim::GetFilteredDescendants, 
// UsdPrim::GetFilteredNextSibling, and UsdPrimRange.
template <class PrimDataPtr>
inline Usd_PrimFlagsPredicate
Usd_CreatePredicateForTraversal(const PrimDataPtr &p, 
                                const SdfPath &proxyPrimPath,
                                Usd_PrimFlagsPredicate pred)
{
    // Don't allow traversals beneath instances unless the client has
    // explicitly requested it or the starting point is already beneath
    // an instance (i.e., the starting point is an instance proxy).
    if (!Usd_IsInstanceProxy(p, proxyPrimPath) && 
        !pred.IncludeInstanceProxiesInTraversal()) {
        pred.TraverseInstanceProxies(false);
    }
    return pred;
}

// Move \p p to its parent.  If \p proxyPrimPath is not empty, set it to 
// its parent path.  If after this \p p is a master prim, move \p p to
// the prim indicated by \p proxyPrimPath.  If \p p's path is then equal
// to \p proxyPrimPath, set \p proxyPrimPath to the empty path.
template <class PrimDataPtr>
inline void
Usd_MoveToParent(PrimDataPtr &p, SdfPath &proxyPrimPath)
{
    p = p->GetParent();

    if (!proxyPrimPath.IsEmpty()) {
        proxyPrimPath = proxyPrimPath.GetParentPath();

        if (p && p->IsMaster()) {
            p = p->GetPrimDataAtPathOrInMaster(proxyPrimPath);
            if (TF_VERIFY(p, "No prim at <%s>", proxyPrimPath.GetText()) &&
                p->GetPath() == proxyPrimPath) {
                proxyPrimPath = SdfPath();
            }
        }
    }
}

// Search for the next sibling that matches \p pred (up to \p end).  If such a
// sibling exists, move \p p to it and return false.  If no such sibling exists
// then move \p p to its parent and return true.  If \p end is reached while 
// looking for siblings, move \p p to \p end and return false.
//
// If \p proxyPrimPath is not empty, update it based on the new value of \p p:
// - If \p p was moved to \p end, set \p proxyPrimPath to the empty path.
// - If \p p was moved to a sibling, set the prim name for \p proxyPrimPath
//   to the sibling's name.  
// - If \p p was moved to a parent, set \p proxyPrimPath and \p p the same
//   way as Usd_MoveToParent.
template <class PrimDataPtr>
inline bool
Usd_MoveToNextSiblingOrParent(PrimDataPtr &p, SdfPath &proxyPrimPath,
                              PrimDataPtr end,
                              const Usd_PrimFlagsPredicate &pred)
{
    // Either all siblings are instance proxies or none are. We can just 
    // compute this once and reuse it as we scan for the next sibling.
    const bool isInstanceProxy = Usd_IsInstanceProxy(p, proxyPrimPath);

    PrimDataPtr next = p->GetNextSibling();
    while (next && next != end && 
           !Usd_EvalPredicate(pred, next, isInstanceProxy)) {
        p = next;
        next = p->GetNextSibling();
    }
    p = next ? next : p->GetParentLink();

    if (!proxyPrimPath.IsEmpty()) {
        if (p == end) {
            proxyPrimPath = SdfPath();
        }
        else if (p == next) {
            proxyPrimPath = 
                proxyPrimPath.GetParentPath().AppendChild(p->GetName());
        }
        else {
            proxyPrimPath = proxyPrimPath.GetParentPath();
            if (p && p->IsMaster()) {
                p = p->GetPrimDataAtPathOrInMaster(proxyPrimPath);
                if (TF_VERIFY(p, "No prim at <%s>", proxyPrimPath.GetText()) &&
                    p->GetPath() == proxyPrimPath) {
                    proxyPrimPath = SdfPath();
                }
            }
        }
    }

    // Return true if we successfully moved to a parent, otherwise false.
    return !next && p;
}

// Convenience method for calling the above with \p end = \c nullptr.
template <class PrimDataPtr>
inline bool
Usd_MoveToNextSiblingOrParent(PrimDataPtr &p, SdfPath &proxyPrimPath,
                              const Usd_PrimFlagsPredicate &pred)
{
    return Usd_MoveToNextSiblingOrParent(p, proxyPrimPath, 
                                         PrimDataPtr(nullptr), pred);
}

// Search for the first direct child of \p p that matches \p pred (up to
// \p end).  If the given \p p is an instance, search for direct children 
// on the  corresponding master prim.  If such a direct child exists, 
// move \p p to it, and return true.  Otherwise leave the iterator 
// unchanged and return false.  
template <class PrimDataPtr>
inline bool
Usd_MoveToChild(PrimDataPtr &p, SdfPath &proxyPrimPath,
                PrimDataPtr end,
                const Usd_PrimFlagsPredicate &pred)
{
    bool isInstanceProxy = Usd_IsInstanceProxy(p, proxyPrimPath);

    PrimDataPtr src = p;
    if (src->IsInstance()) {
        src = src->GetMaster();
        isInstanceProxy = true;
    }

    if (PrimDataPtr child = src->GetFirstChild()) {
        if (isInstanceProxy) {
            proxyPrimPath = proxyPrimPath.IsEmpty() ?
                p->GetPath().AppendChild(child->GetName()) :
                proxyPrimPath.AppendChild(child->GetName());
        }

        p = child;

        if (Usd_EvalPredicate(pred, p, isInstanceProxy) || 
            !Usd_MoveToNextSiblingOrParent(p, proxyPrimPath, end, pred)) {
            return true;
        }
    }
    return false;
}

// Convenience method for calling the above with \p end = \c nullptr.
template <class PrimDataPtr>
inline bool
Usd_MoveToChild(PrimDataPtr &p, SdfPath &proxyPrimPath,
                const Usd_PrimFlagsPredicate &pred) 
{
    return Usd_MoveToChild(p, proxyPrimPath, PrimDataPtr(nullptr), pred);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_PRIM_DATA_H
