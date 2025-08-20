//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_VT_ARRAY_EDIT_H
#define PXR_BASE_VT_ARRAY_EDIT_H

/// \file vt/arrayEdit.h

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/arrayEditOps.h"
#include "pxr/base/vt/streamOut.h"
#include "pxr/base/vt/traits.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

template <class ELEM>
class VtArrayEditBuilder; // fwd

/// \class VtArrayEdit
///
/// An array edit represents either a sequence of per-element modifications to a
/// VtArray, or simply a dense VtArray.  VtArray implicitly converts to
/// VtArrayEdit as a dense array.
///
/// The member function ComposeOver(strong, weak) applies strong's edits to weak
/// and returns the resulting VtArrayEdit, which may be either a dense array (if
/// one or both of strong & weak are dense) or a representation of the combined
/// edits.
///
/// VtArrayEdit under ComposeOver() forms an algebraic "monoid".  That is,
/// ComposeOver() is associative, where the default-constructed VtArrayEdit
/// (which represents no edits) is the identity element.
///
/// See the associated VtArrayEditBuilder class to understand the available edit
/// operations, and to build a VtArrayEdit from them.
///
template <class ELEM>
class VtArrayEdit
{
public:
    /// Shorthand for the corresponding VtArray type.
    using Array = VtArray<ELEM>;

    using ElementType = typename Array::ElementType;

    /// Construct an identity array edit that performs no edits.  ComposeOver()
    /// with an identity returns the other argument.
    VtArrayEdit() = default;

    /// Copy construct or implicitly convert from VtArray, producing a dense
    /// VtArrayEdit.
    VtArrayEdit(Array const &a) : _denseOrLiterals(a), _isDense(true) {}

    /// Move construct or implicitly convert from VtArray, producing a dense
    /// VtArrayEdit.
    VtArrayEdit(Array &&a) : _denseOrLiterals(std::move(a)), _isDense(true) {}

    /// Copy construct.
    VtArrayEdit(VtArrayEdit const &) = default;
    /// Move construct.
    VtArrayEdit(VtArrayEdit &&) = default;

    /// Copy assign.
    VtArrayEdit &operator=(VtArrayEdit const &) = default;
    /// Move assign.
    VtArrayEdit &operator=(VtArrayEdit &&) = default;

    /// Equality comparison.
    friend bool operator==(VtArrayEdit const &x, VtArrayEdit const &y) {
        return std::tie(x._isDense, x._denseOrLiterals, x._ops) ==
               std::tie(y._isDense, y._denseOrLiterals, y._ops);
    }
    /// Inequality comparison.
    friend bool operator!=(VtArrayEdit const &x, VtArrayEdit const &y) {
        return !(x == y);
    }
    
    /// Return true if this edit is the identity edit.  The identity edit
    /// performs no edits.  Composing the identity with another edit returns
    /// that edit unmodified.
    bool IsIdentity() const {
        return !_isDense && _ops.IsEmpty();
    }

    /// Return true if this edit represents a dense array.
    bool IsDenseArray() const {
        return _isDense;
    }

    /// Return the dense array if this edit represents one.  See IsDenseArray().
    /// If this edit does not represent a dense array issue an error and return
    /// an empty array.
    Array GetDenseArray() const & {
        if (!IsDenseArray()) {
            TF_CODING_ERROR("VtArrayEdit is not a dense array");
            return {};
        }
        return _denseOrLiterals;
    }

    /// Return the dense array if this edit represents one.  See IsDenseArray().
    /// If this edit does not represent a dense array issue an error and return
    /// an empty array.
    Array GetDenseArray() && {
        if (!IsDenseArray()) {
            TF_CODING_ERROR("VtArrayEdit is not a dense array");
            return {};
        }
        return std::move(_denseOrLiterals);
    }        

    /// Compose this edit over \p weaker and return a new result representing
    /// the function composition, where \p weaker is the "inner" function and \p
    /// *this is the "outer" function.
    /// 
    /// If \p *this represents a dense array, return \p *this unmodified.  If \p
    /// weaker represents a dense array, return an edit representing the dense
    /// array from \p weaker with the edits in \p *this applied to it.  If
    /// neither are dense, return an edit that represents the action of the
    /// edits in \p weaker followed by \p *this.
    VtArrayEdit ComposeOver(VtArrayEdit const &weaker) && {
        if (IsDenseArray()) {
            return std::move(*this);
        }
        if (IsIdentity()) {
            return weaker;
        }
        if (weaker.IsDenseArray()) {
            return _ApplyEdits(weaker._denseOrLiterals);
        }
        return _ComposeEdits(weaker);
    }

    VtArrayEdit ComposeOver(VtArrayEdit &&weaker) && {
        if (IsDenseArray()) {
            return std::move(*this);
        }
        if (IsIdentity()) {
            return std::move(weaker);
        }
        if (weaker.IsDenseArray()) {
            return _ApplyEdits(std::move(weaker._denseOrLiterals));
        }        
        return _ComposeEdits(std::move(weaker));
    }

    /// Compose this edit over \p weaker and return a new result representing
    /// the function composition, where \p weaker is the "inner" function and \p
    /// *this is the "outer" function.
    ///
    /// If \p *this represents a dense array, return \p *this unmodified.  If \p
    /// weaker represents a dense array, return an edit representing the dense
    /// array from \p weaker with the edits in \p *this applied to it.  If
    /// neither are dense, return an edit that represents the action of the
    /// edits in \p weaker followed by \p *this.
    VtArrayEdit ComposeOver(VtArrayEdit const &weaker) const & {
        if (IsDenseArray()) {
            return *this;
        }
        if (IsIdentity()) {
            return weaker;
        }
        if (weaker.IsDenseArray()) {
            return _ApplyEdits(weaker._denseOrLiterals);
        }
        return _ComposeEdits(weaker);
    };
    
    VtArrayEdit ComposeOver(VtArrayEdit &&weaker) const & {
        if (IsDenseArray()) {
            return *this;
        }
        if (IsIdentity()) {
            return std::move(weaker);
        }
        if (weaker.IsDenseArray()) {
            return _ApplyEdits(std::move(weaker._denseOrLiterals));
        }        
        return _ComposeEdits(std::move(weaker));
    }
    
private:
    friend class VtArrayEditBuilder<ELEM>;

    template <class HashState>
    friend void TfHashAppend(HashState &h, VtArrayEdit const &self) {
        h.Append(self._denseOrLiterals, self._ops, self._isDense);
    }
    
    using _Ops = Vt_ArrayEditOps;
    
    Array _ApplyEdits(Array &&weaker) const;
    Array _ApplyEdits(Array const &weaker) const {
        return _ApplyEdits(Array {weaker});
    }

    VtArrayEdit _ComposeEdits(VtArrayEdit &&weaker) &&;
    VtArrayEdit _ComposeEdits(VtArrayEdit const &weaker) &&;
    
    VtArrayEdit _ComposeEdits(VtArrayEdit &&weaker) const & {
        return VtArrayEdit(*this)._ComposeEdits(std::move(weaker));
    }
    VtArrayEdit _ComposeEdits(VtArrayEdit const &weaker) const & {
        return VtArrayEdit(*this)._ComposeEdits(weaker);
    }
    
    Array _denseOrLiterals;
    _Ops _ops;
    bool _isDense = false;
};

// Declare basic arrayEdit instantiations as extern templates.  They are
// explicitly instantiated in arrayEdit.cpp.
#define VT_ARRAY_EDIT_EXTERN_TMPL(unused, elem) \
    VT_API_TEMPLATE_CLASS(VtArrayEdit< VT_TYPE(elem) >);
TF_PP_SEQ_FOR_EACH(VT_ARRAY_EDIT_EXTERN_TMPL, ~, VT_SCALAR_VALUE_TYPES)
#undef VT_ARRAY_EDIT_EXTERN_TMPL

template <class ELEM>
VtArray<ELEM>
VtArrayEdit<ELEM>::_ApplyEdits(Array &&weaker) const
{
    TRACE_FUNCTION();

    // This is non-dense, weaker is an array that we edit.
    Array result = std::move(weaker);
    Array const &cresult = result;

    Array const &literals = _denseOrLiterals;
    const auto numLiterals = literals.size();

    // XXX: Note that this does not handle certain sequences of inserts and
    // erases (specifically those that insert or erase contiguous ranges of
    // elements) optimally.  This could be improved by detecting these cases and
    // doing a single batch insert or erase instead, to minimize shuffling the
    // other elements.
    
    _ops.ForEachValid(numLiterals, cresult.size(),
    [&](_Ops::Op op, int64_t a1, int64_t a2) {
        switch (op) {
        case _Ops::OpWriteLiteral:
            result[a2] = literals[a1];
            break;
        case _Ops::OpWriteRef: // a1: result index -> a2: result index.
            result[a2] = cresult[a1];
            break;
        case _Ops::OpInsertLiteral: // a1: literal index -> a2: result index.
            result.insert(result.cbegin() + a2, literals[a1]);
            break;
        case _Ops::OpInsertRef: // a1: result index -> a2: result index.
            result.insert(result.cbegin() + a2, cresult[a1]);
            break;
        case _Ops::OpEraseRef: // a1: result index, (a2: unused)
            result.erase(result.cbegin() + a1);
            break;
        case _Ops::OpMinSize:  // a1: minimum size, (a2: unused)
            if (result.size() < static_cast<size_t>(a1)) {
                result.resize(a1);
            }
            break;
        case _Ops::OpMinSizeFill:  // a1: minimum size, a2: literal index.
            if (result.size() < static_cast<size_t>(a1)) {
                result.resize(a1, literals[a2]);
            }
            break;
        case _Ops::OpSetSize:  // a1: explicit size, (a2: unused)
            result.resize(a1);
            break;
        case _Ops::OpSetSizeFill:  // a1: explicit size, a2: literal index.
            result.resize(a1, literals[a2]);
            break;
        case _Ops::OpMaxSize:  // a1: maximum size, a2: unused
            if (result.size() > static_cast<size_t>(a1)) {
                result.resize(a1);
            }
            break;
        };
    });
    return result;
}

template <class ELEM>
VtArrayEdit<ELEM>
VtArrayEdit<ELEM>::_ComposeEdits(VtArrayEdit const &weaker) &&
{
    TRACE_FUNCTION();

    // Both this and weaker consist of edits. We compose the edits and we can
    // steal our resources.

    // For now we just append the stronger literals, and update all the stronger
    // literal indexes with the offset.  We can do more in-depth analysis and
    // things like dead store elimination and deduplicating literals in the
    // future.

    VtArrayEdit result = std::move(*this);

    // Append the stronger literals to weaker.
    // result._denseOrLiterals =
    //     weaker._denseOrLiterals + result._denseOrLiterals;
    result._denseOrLiterals.insert(
        result._denseOrLiterals.begin(),
        weaker._denseOrLiterals.begin(), weaker._denseOrLiterals.end());

    // Bump the literal indexes in result._ops to account for weaker's
    // literals.
    const auto numWeakerLiterals = weaker._denseOrLiterals.size();
    result._ops.ModifyEach([&](_Ops::Op op, int64_t &a1, int64_t) {
        switch (op) {
        case _Ops::OpWriteLiteral: // a1: literal index -> a2: result index.
        case _Ops::OpInsertLiteral:
            a1 += numWeakerLiterals;
        default:
            break;
        };
    });

    result._ops._ins.insert(result._ops._ins.begin(),
                            weaker._ops._ins.begin(),
                            weaker._ops._ins.end());

    return result;
}

template <class ELEM>
VtArrayEdit<ELEM>
VtArrayEdit<ELEM>::_ComposeEdits(VtArrayEdit &&weaker) &&
{
    TRACE_FUNCTION();
    
    // Both this and weaker consist of edits. We compose the edits and we can
    // steal both our resources and weaker's.

    // For now we just append the stronger literals and stronger ops, and update
    // all the stronger literal indexes with the offset.  We can do more
    // in-depth analysis and things like dead store elimination and
    // deduplicating literals in the future.

    VtArrayEdit result = std::move(*this);

    const auto numWeakerLiterals = weaker._denseOrLiterals.size();

    // Append the stronger literals to weaker.
    weaker._denseOrLiterals.insert(
        weaker._denseOrLiterals.end(),
        std::make_move_iterator(result._denseOrLiterals.begin()),
        std::make_move_iterator(result._denseOrLiterals.end()));
    
    // Bump the literal indexes in the stronger _ops to account for weaker's
    // literals.
    result._ops.ModifyEach([&](_Ops::Op op, int64_t &a1, int64_t) {
        switch (op) {
        case _Ops::OpWriteLiteral: // a1: literal index -> a2: result index.
        case _Ops::OpInsertLiteral:
            a1 += numWeakerLiterals;
        default:
            break;
        };
    });

    // Append the stronger ops to weaker.
    weaker._ops._ins.insert(
        weaker._ops._ins.end(),
        std::make_move_iterator(result._ops._ins.begin()),
        std::make_move_iterator(result._ops._ins.end()));

    return std::move(weaker);
}

// Specialize traits for VtArrayEdit.
template <typename T>
struct VtIsArrayEdit<VtArrayEdit<T>> : public std::true_type {};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_ARRAY_EDIT_H
