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
#ifndef USD_PRIMRANGE_H
#define USD_PRIMRANGE_H

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
/// An object with iterator semantics that will traverse the subtree of prims
/// rooted at a given prim.
///
/// In addition to providing an alternative to UsdPrim::GetChildren()-based
/// recursion, UsdPrimRange provides a compact expression for performing
/// post-order (prim is yielded after all of its descendents) iteration in
/// addition to "normal" pre-order (prim is yielded before its children)
/// iteration.  For iterations that include a post-order visitation, each
/// prim will be yielded twice, and client can call
/// UsdPrimRange::IsPostVisit() on the iterator to determine when to perform
/// the post-order processing.
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
/// \li You may want to prune sub-trees from processing (see PruneChildren())
/// \li You want to treat the root prim itself uniformly with its 
/// descendents (GetFilteredDescendants() will not return the root prim itself,
/// while UsdPrimRange will - see UsdPrimRange::Stage for the one
/// exception).
///
/// <b>Using UsdPrimRange in C++</b>
///
/// UsdPrimRange provides standard iterator semantics, so, for example:
/// \code
/// for (UsdPrimRange  it(rootPrim); it ; ++it){
///     // Good practice if you derefence 'it' more than once
///     const UsdPrim &prim = *it;
///
///     if (UsdModelAPI(prim).GetKind() == KindTokens->component){
///         it.PruneChildren();
///     }
///     else {
///         nonComponents.push_back(prim);
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
/// it = Usd.PrimRange.Stage(stage, Usd.PrimIsLoaded & ~Usd.PrimIsAbstract)
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
class UsdPrimRange : public boost::iterator_adaptor<
    UsdPrimRange,               // crtp base.
    Usd_PrimDataConstPtr,          // base iterator.
    UsdPrim,                       // value type.
    boost::forward_traversal_tag,  // traversal.
    UsdPrim>                       // reference type.
{
    typedef const Usd_PrimFlagsPredicate UsdPrimRange::*_UnspecifiedBoolType;

public:
    UsdPrimRange() : iterator_adaptor_(NULL) {}

    /// Construct a PrimRange that traverses the subtree rooted at \p start,
    /// and visits prims that pass the default predicate (as defined by
    /// #UsdPrimDefaultPredicate) with pre-order visitation.
    explicit UsdPrimRange(const UsdPrim &start)
        : iterator_adaptor_(get_pointer(start._Prim())) {
        _Init(base(), base() ? base()->GetNextPrim() : NULL, 
              start.GetPrimPath());
    }

    /// Construct a PrimRange that traverses the subtree rooted at \p start,
    /// and visits prims that pass \p predicate with pre-order visitation.
    UsdPrimRange(const UsdPrim &start,
                    const Usd_PrimFlagsPredicate &predicate)
        : iterator_adaptor_(get_pointer(start._Prim())) {
        _Init(base(), base() ? base()->GetNextPrim() : NULL, 
              start.GetPrimPath(), predicate);
    }

    /// Create a PrimRange that traverses the subtree rooted at \p start,
    /// and visits prims that pass the default predicate (as defined by
    /// #UsdPrimDefaultPredicate) with pre- and post-order visitation.
    static UsdPrimRange
    PreAndPostVisit(const UsdPrim &start) {
        UsdPrimRange result(start);
        result._postOrder = true;
        return result;
    }

    /// Create a PrimRange that traverses the subtree rooted at \p start, and
    /// visits prims that pass \p predicate with pre- and post-order visitation.
    static UsdPrimRange
    PreAndPostVisit(const UsdPrim &start,
                    const Usd_PrimFlagsPredicate &predicate) {
        UsdPrimRange result(start, predicate);
        result._postOrder = true;
        return result;
    }

    /// Create a PrimRange that traverses the subtree rooted at \p start, and
    /// visits all prims (including deactivated, undefined, and abstract prims)
    /// with pre-order visitation.
    static UsdPrimRange
    AllPrims(const UsdPrim &start) {
        return UsdPrimRange(start, Usd_PrimFlagsPredicate::Tautology());
    }

    /// Create a PrimRange that traverses the subtree rooted at \p start, and
    /// visits all prims (including deactivated, undefined, and abstract prims)
    /// with pre- and post-order visitation.
    static UsdPrimRange
    AllPrimsPreAndPostVisit(const UsdPrim &start) {
        return PreAndPostVisit(start, Usd_PrimFlagsPredicate::Tautology());
    }

    /// Create a PrimRange that traverses all the prims on \p stage, and
    /// visits those that pass the default predicate (as defined by
    /// #UsdPrimDefaultPredicate) with pre-order visitation.
    USD_API
    static UsdPrimRange
    Stage(const UsdStagePtr &stage,
          const Usd_PrimFlagsPredicate &predicate =
              UsdPrimDefaultPredicate);

#ifdef doxygen
    /// Explicit bool-conversion operator.  True if this iterator is not
    /// exhausted, false otherwise.
    operator unspecified-bool-type() const();
#else
    explicit operator bool() const {
        return base() != _end;
    }
#endif

    /// Return a UsdPrimRange that represents the end of this iterator's
    /// iteration.  This is useful for algorithms that require a range of
    /// [begin, end) iterators.
    UsdPrimRange GetEnd() const {
        UsdPrimRange r(*this);
        r.base_reference() = r._end;
        r._primPath = SdfPath();
        r._depth = 0;
        r._isPost = false;
        return r;
    }

    //
    // specialized accessors
    //

    /// Return true if the iterator points to a prim visited the second time (in
    /// post order) for a pre- and post-order iterator, false otherwise.
    bool IsPostVisit() const { return _isPost; }

    /// Behave as if the current prim has no children when next advanced.  Issue
    /// an error if this is a pre- and post-order iterator that IsPostVisit().
    USD_API
    void PruneChildren();

private:
    UsdPrimRange(Usd_PrimDataConstPtr start,
                    Usd_PrimDataConstPtr end,
                    const SdfPath& primPath,
                    const Usd_PrimFlagsPredicate &predicate =
                        UsdPrimDefaultPredicate)
        : iterator_adaptor_(start) {
        _Init(start, end, primPath, predicate);
    }

    ////////////////////////////////////////////////////////////////////////
    // Helpers.
    void _Init(const Usd_PrimData *start,
               const Usd_PrimData *end,
               const SdfPath &primPath,
               const Usd_PrimFlagsPredicate &predicate = 
                   UsdPrimDefaultPredicate) {
        _end = end;
        _primPath = primPath;
        _predicate = base() ? 
            Usd_CreatePredicateForTraversal(base(), _primPath, predicate) :
            predicate;
        _depth = 0;
        _postOrder = false;
        _pruneChildrenFlag = false;
        _isPost = false;

        // Advance to the first prim that passes the predicate.
        if (base() != _end && !Usd_EvalPredicate(_predicate, base(), primPath)) {
            _pruneChildrenFlag = true;
            increment();
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // Core implementation invoked by iterator_adaptor.
    friend class boost::iterator_core_access;
    bool equal(const UsdPrimRange &other) const {
        return
            base() == other.base() &&
            _end == other._end &&
            _primPath == other._primPath &&
            _predicate == other._predicate &&
            _depth == other._depth &&
            _postOrder == other._postOrder &&
            _pruneChildrenFlag == other._pruneChildrenFlag &&
            _isPost == other._isPost;
    }

    USD_API void increment();

    reference dereference() const { 
        return UsdPrim(base(), _primPath); 
    }

    ////////////////////////////////////////////////////////////////////////
    // Data members.

    // These members are fixed for the life of the iterator.
    base_type _end;
    SdfPath _primPath;
    Usd_PrimFlagsPredicate _predicate;
    unsigned int _depth;
    bool _postOrder;

    // True when the client has asked that the next increment skips the children
    // of the current prim.
    bool _pruneChildrenFlag;

    // True when we're on the post-side of a prim.  Unused if _postOrder is
    // false.
    bool _isPost;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_PRIMRANGE_H
