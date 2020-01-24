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
#ifndef PXR_USD_USD_PRIM_RANGE_H
#define PXR_USD_USD_PRIM_RANGE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primFlags.h"

#include <vector>
#include <iterator>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdPrimRange
///
/// An forward-iterable range that traverses a subtree of prims rooted at a
/// given prim in depth-first order.
///
/// In addition to depth-first order, UsdPrimRange provides the optional ability
/// to traverse in depth-first pre- and post-order wher prims appear twice in
/// the range; first before all descendants and then again immediately after all
/// descendants.  This is useful for maintaining state associated with subtrees,
/// in a stack-like fashion.  See UsdPrimRange::iterator::IsPostVisit() to
/// detect when an iterator is visiting a prim for the second time.
///
/// There are several constructors providing different levels of 
/// configurability; ultimately, one can provide a prim predicate for a custom
/// iteration, just as one would use UsdPrim::GetFilteredChildren() in a custom
/// recursion.
///
/// Why would one want to use a UsdPrimRange rather than just iterating
/// over the results of UsdPrim::GetFilteredDescendants() ?  Primarily, if
/// one of the following applies:
/// \li You need to perform pre-and-post-order processing
/// \li You may want to prune sub-trees from processing (see UsdPrimRange::iterator::PruneChildren())
/// \li You want to treat the root prim itself uniformly with its 
/// descendents (GetFilteredDescendants() will not return the root prim itself,
/// while UsdPrimRange will - see UsdPrimRange::Stage for an exception).
///
/// <b>Using UsdPrimRange in C++</b>
///
/// UsdPrimRange provides standard container-like semantics.  For example:
/// \code
/// // simple range-for iteration
/// for (UsdPrim prim: UsdPrimRange(rootPrim)) {
///     ProcessPrim(prim);
/// }
///
/// // using stl algorithms
/// std::vector<UsdPrim> meshes;
/// auto range = stage->Traverse();
/// std::copy_if(range.begin(), range.end(), std::back_inserter(meshes),
///              [](UsdPrim const &) { return prim.IsA<UsdGeomMesh>(); });
///
/// // iterator-based iterating, with subtree pruning
/// UsdPrimRange range(rootPrim);
/// for (auto iter = range.begin(); iter != range.end(); ++iter) {
///     if (UsdModelAPI(*iter).GetKind() == KindTokens->component) {
///         iter.PruneChildren();
///     }
///     else {
///         nonComponents.push_back(*iter);
///     }
/// }
/// \endcode
///
/// <b>Using Usd.PrimRange in python</b>
///
/// The python wrapping for PrimRange is python-iterable, so it can
/// used directly as the object of a "for x in..." clause; however in that
/// usage one loses access to PrimRange methods such as PruneChildren() and
/// IsPostVisit().  Simply create the iterator outside the loop to overcome
/// this limitation.  Finally, in python, prim predicates must be combined
/// with bit-wise operators rather than logical operators because the latter
/// are not overridable.
/// \code{.py}
/// # simple iteration
/// for prim in Usd.PrimRange(rootPrim):
///     ProcessPrim(prim)
///
/// # filtered range using iterator to invoke iterator methods
/// it = iter(Usd.PrimRange.Stage(stage, Usd.PrimIsLoaded & ~Usd.PrimIsAbstract))
/// for prim in it:
///     if Usd.ModelAPI(prim).GetKind() == Kind.Tokens.component:
///         it.PruneChildren()
///     else:
///         nonComponents.append(prim)
/// \endcode
///
/// Finally, since iterators in python are not directly dereferencable, we
/// provide the \em python \em only methods GetCurrentPrim() and IsValid(),
/// documented in the python help system.
///
class UsdPrimRange
{
public:
    class iterator;

    /// \class EndSentinel
    ///
    /// This class lets us represent past-the-end without the full weight of an
    /// iterator.
    class EndSentinel {
    private:
        friend class UsdPrimRange;
        explicit EndSentinel(UsdPrimRange const *range) : _range(range) {}
        friend class UsdPrimRange::iterator;
        UsdPrimRange const *_range;
    };

    /// \class iterator
    ///
    /// A forward iterator into a UsdPrimRange.  Iterators are valid for the
    /// range they were obtained from.  An iterator \em i obtained from a range
    /// \em r is not valid for a range \em c copied from \em r.
    class iterator : public boost::iterator_adaptor<
        iterator,                      // crtp base.
        Usd_PrimDataConstPtr,          // base iterator.
        UsdPrim,                       // value type.
        boost::forward_traversal_tag,  // traversal.
        UsdPrim>                       // reference type.
    {
    public:
        iterator() : iterator_adaptor_(nullptr) {}

        /// Allow implicit conversion from EndSentinel.
        iterator(EndSentinel e)
            : iterator_adaptor_(e._range->_end)
            , _range(e._range) {}
        
        /// Return true if the iterator points to a prim visited the second time
        /// (in post order) for a pre- and post-order iterator, false otherwise.
        bool IsPostVisit() const { return _isPost; }

        /// Behave as if the current prim has no children when next advanced.
        /// Issue an error if this is a pre- and post-order iterator that
        /// IsPostVisit().
        USD_API void PruneChildren();

        /// Return true if this iterator is equivalent to \p other.
        inline bool operator==(iterator const &other) const {
            return _range == other._range &&
                base() == other.base() &&
                _proxyPrimPath == other._proxyPrimPath &&
                _depth == other._depth &&
                _pruneChildrenFlag == other._pruneChildrenFlag &&
                _isPost == other._isPost;
        }

        /// Return true if this iterator is equivalent to \p other.
        inline bool operator==(EndSentinel const &other) const {
            return _range == other._range && base() == _range->_end;
        }

        /// Return true if this iterator is not equivalent to \p other.
        inline bool operator!=(iterator const &other) const {
            return !(*this == other);
        }

        /// Return true if this iterator is not equivalent to \p other.
        inline bool operator!=(EndSentinel const &other) const {
            return !(*this == other);
        }
         
    private:
        friend class UsdPrimRange;
        friend class boost::iterator_core_access;
        
        iterator(UsdPrimRange const *range,
                 Usd_PrimDataConstPtr prim,
                 SdfPath proxyPrimPath,
                 unsigned int depth)
            : iterator_adaptor_(prim)
            , _range(range)
            , _proxyPrimPath(proxyPrimPath)
            , _depth(depth) {}

        USD_API void increment();
        
        inline reference dereference() const { 
            return UsdPrim(base(), _proxyPrimPath);
        }

        UsdPrimRange const *_range = nullptr;
        SdfPath _proxyPrimPath;
        unsigned int _depth = 0;

        // True when the client has asked that the next increment skips the
        // children of the current prim.
        bool _pruneChildrenFlag = false;
        // True when we're on the post-side of a prim.  Unused if
        // _range->_postOrder is false.
        bool _isPost = false;
    };

    using const_iterator = iterator;

    UsdPrimRange()
        : _begin(nullptr)
        , _end(nullptr)
        , _initDepth(0)
        , _postOrder(false) {}

    /// Construct a PrimRange that traverses the subtree rooted at \p start in
    /// depth-first order, visiting prims that pass the default predicate (as
    /// defined by #UsdPrimDefaultPredicate).
    explicit UsdPrimRange(const UsdPrim &start) {
        Usd_PrimDataConstPtr p = get_pointer(start._Prim());
        _Init(p, p ? p->GetNextPrim() : nullptr, start._ProxyPrimPath());
    }

    /// Construct a PrimRange that traverses the subtree rooted at \p start in
    /// depth-first order, visiting prims that pass \p predicate.
    UsdPrimRange(const UsdPrim &start,
                 const Usd_PrimFlagsPredicate &predicate) {
        Usd_PrimDataConstPtr p = get_pointer(start._Prim());
        _Init(p, p ? p->GetNextPrim() : nullptr,
              start._ProxyPrimPath(), predicate);
    }

    /// Create a PrimRange that traverses the subtree rooted at \p start in
    /// depth-first order, visiting prims that pass the default predicate (as
    /// defined by #UsdPrimDefaultPredicate) with pre- and post-order
    /// visitation.
    ///
    /// Pre- and post-order visitation means that each prim appears
    /// twice in the range; not only prior to all its descendants as with an
    /// ordinary traversal but also immediately following its descendants.  This
    /// lets client code maintain state for subtrees.  See
    /// UsdPrimRange::iterator::IsPostVisit().
    static UsdPrimRange
    PreAndPostVisit(const UsdPrim &start) {
        UsdPrimRange result(start);
        result._postOrder = true;
        return result;
    }

    /// Create a PrimRange that traverses the subtree rooted at \p start in
    /// depth-first order, visiting prims that pass \p predicate with pre- and
    /// post-order visitation.
    ///
    /// Pre- and post-order visitation means that each prim appears
    /// twice in the range; not only prior to all its descendants as with an
    /// ordinary traversal but also immediately following its descendants.  This
    /// lets client code maintain state for subtrees.  See
    /// UsdPrimRange::iterator::IsPostVisit().
    static UsdPrimRange
    PreAndPostVisit(const UsdPrim &start,
                    const Usd_PrimFlagsPredicate &predicate) {
        UsdPrimRange result(start, predicate);
        result._postOrder = true;
        return result;
    }

    /// Construct a PrimRange that traverses the subtree rooted at \p start in
    /// depth-first order, visiting all prims (including deactivated, undefined,
    /// and abstract prims).
    static UsdPrimRange
    AllPrims(const UsdPrim &start) {
        return UsdPrimRange(start, UsdPrimAllPrimsPredicate);
    }

    /// Construct a PrimRange that traverses the subtree rooted at \p start in
    /// depth-first order, visiting all prims (including deactivated, undefined,
    /// and abstract prims) with pre- and post-order visitation.
    ///
    /// Pre- and post-order visitation means that each prim appears
    /// twice in the range; not only prior to all its descendants as with an
    /// ordinary traversal but also immediately following its descendants.  This
    /// lets client code maintain state for subtrees.  See
    /// UsdPrimRange::iterator::IsPostVisit().
    static UsdPrimRange
    AllPrimsPreAndPostVisit(const UsdPrim &start) {
        return PreAndPostVisit(start, UsdPrimAllPrimsPredicate);
    }

    /// Create a PrimRange that traverses all the prims on \p stage, and
    /// visits those that pass the default predicate (as defined by
    /// #UsdPrimDefaultPredicate).
    USD_API
    static UsdPrimRange
    Stage(const UsdStagePtr &stage,
          const Usd_PrimFlagsPredicate &predicate = UsdPrimDefaultPredicate);

    /// Return an iterator to the start of this range.
    iterator begin() const {
        return iterator(this, _begin, _initProxyPrimPath, _initDepth);
    }
    /// Return a const_iterator to the start of this range.
    const_iterator cbegin() const {
        return iterator(this, _begin, _initProxyPrimPath, _initDepth);
    }

    /// Return the first element of this range.  The range must not be empty().
    UsdPrim front() const { return *begin(); }

    // XXX C++11 & 14 require that c/end() return the same type as c/begin() for
    // range-based-for loops to work correctly.  C++17 relaxes that requirement.
    // Change the return type to EndSentinel once we are on C++17.

    /// Return the past-the-end iterator for this range.
    iterator end() const { return EndSentinel(this); }
    /// Return the past-the-end const_iterator for this range.
    const_iterator cend() const { return EndSentinel(this); }

    /// Modify this range by advancing the beginning by one.  The range must not
    /// be empty, and the range must not be a pre- and post-order range.
    void increment_begin() {
        set_begin(++begin());
    }

    /// Set the start of this range to \p newBegin.  The \p newBegin iterator
    /// must be within this range's begin() and end(), and must not have
    /// UsdPrimRange::iterator::IsPostVisit() be true.
    void set_begin(iterator const &newBegin) {
        TF_VERIFY(!newBegin.IsPostVisit());
        _begin = newBegin.base();
        _initProxyPrimPath = newBegin._proxyPrimPath;
        _initDepth = newBegin._depth;
    }

    /// Return true if this range contains no prims, false otherwise.
    bool empty() const { return begin() == end(); }

    /// Return true if this range contains one or more prims, false otherwise.
    explicit operator bool() const { return !empty(); }

    /// Return true if this range is equivalent to \p other.
    bool operator==(UsdPrimRange const &other) const {
        return this == &other ||
            (_begin == other._begin &&
             _end == other._end &&
             _initProxyPrimPath == other._initProxyPrimPath &&
             _predicate == other._predicate &&
             _postOrder == other._postOrder &&
             _initDepth == other._initDepth);
    }

    /// Return true if this range is not equivalent to \p other.
    bool operator!=(UsdPrimRange const &other) const {
        return !(*this == other);
    }

private:
    UsdPrimRange(Usd_PrimDataConstPtr begin,
                 Usd_PrimDataConstPtr end,
                 const SdfPath& proxyPrimPath,
                 const Usd_PrimFlagsPredicate &predicate =
                 UsdPrimDefaultPredicate) {
        _Init(begin, end, proxyPrimPath, predicate);
    }

    ////////////////////////////////////////////////////////////////////////
    // Helpers.
    void _Init(const Usd_PrimData *first,
               const Usd_PrimData *last,
               const SdfPath &proxyPrimPath,
               const Usd_PrimFlagsPredicate &predicate = 
               UsdPrimDefaultPredicate) {
        _begin = first;
        _end = last;
        _initProxyPrimPath = proxyPrimPath;
        _predicate = _begin ? 
            Usd_CreatePredicateForTraversal(_begin, proxyPrimPath, predicate) :
            predicate;
        _postOrder = false;
        _initDepth = 0;

        // Advance to the first prim that passes the predicate.
        iterator b = begin();
        if (b.base() != _end &&
            !Usd_EvalPredicate(_predicate, b.base(), proxyPrimPath)) {
            b._pruneChildrenFlag = true;
            set_begin(++b);
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // Data members.

    // These members are fixed for the life of the range.
    Usd_PrimDataConstPtr _begin;
    Usd_PrimDataConstPtr _end;
    SdfPath _initProxyPrimPath;
    Usd_PrimFlagsPredicate _predicate;
    unsigned int _initDepth;
    bool _postOrder;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_PRIM_RANGE_H
