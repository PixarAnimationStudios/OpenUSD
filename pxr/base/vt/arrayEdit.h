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
#include "pxr/base/tf/functionRef.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/span.h"
#include "pxr/base/trace/trace.h"

#include <iosfwd>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

template <class ELEM>
class VtArrayEditBuilder; // fwd

/// \class VtArrayEdit
///
/// An array edit represents a sequence of per-element modifications to a
/// VtArray.
///
/// The member function ComposeOver(strong, weak) applies strong's edits to
/// `weak` and returns the resulting VtArray or VtArrayEdit, depending on
/// whether `weak` is a VtArray or VtArrayEdit.
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

    /// Equality comparison.
    friend bool operator==(VtArrayEdit const &x, VtArrayEdit const &y) {
        return std::tie(x._literals, x._ops) == std::tie(y._literals, y._ops);
    }
    /// Inequality comparison.
    friend bool operator!=(VtArrayEdit const &x, VtArrayEdit const &y) {
        return !(x == y);
    }
    
    /// Return true if this edit is the identity edit.  The identity edit
    /// performs no edits.  Composing the identity with another edit returns
    /// that edit unmodified.
    bool IsIdentity() const {
        return _ops.IsEmpty();
    }

    /// Return a view of the literal elements that this edit makes use of.
    TfSpan<const ElementType> GetLiterals() const {
        return _literals;
    }

    /// Return a mutable view of the literal elements that this edit makes use
    /// of.  This can be useful in case elements need to be transformed or
    /// translated.
    TfSpan<ElementType> GetMutableLiterals() {
        return _literals;
    }

    /// Compose this edit over \p weaker and return a new result representing
    /// the function composition, where \p weaker is the "inner" function and \p
    /// *this is the "outer" function.  In other words, return an edit that
    /// represents the action of the edits in \p weaker followed by those in \p
    /// *this.
    VtArrayEdit ComposeOver(VtArrayEdit const &weaker) && {
        if (IsIdentity()) {
            return weaker;
        }
        return _ComposeEdits(weaker);
    }

    /// \overload
    VtArrayEdit ComposeOver(VtArrayEdit &&weaker) && {
        if (IsIdentity()) {
            return std::move(weaker);
        }
        return _ComposeEdits(std::move(weaker));
    }

    /// \overload
    VtArrayEdit ComposeOver(VtArrayEdit const &weaker) const & {
        if (IsIdentity()) {
            return weaker;
        }
        return _ComposeEdits(weaker);
    };
    
    /// \overload
    VtArrayEdit ComposeOver(VtArrayEdit &&weaker) const & {
        if (IsIdentity()) {
            return std::move(weaker);
        }
        return _ComposeEdits(std::move(weaker));
    }

    /// Apply the edits in \p *this to \p weaker and return the resulting array.
    Array ComposeOver(Array const &weaker) const {
        if (IsIdentity()) {
            return weaker;
        }
        return _ApplyEdits(weaker);
    }

    /// \overload
    Array ComposeOver(Array &&weaker) const {
        if (IsIdentity()) {
            return std::move(weaker);
        }
        return _ApplyEdits(weaker);
    }

    /// Insert \p self to the stream \p out using the following format:
    ///
    /// ```
    /// edit [<op_1>; <op_2>; ... <op_N>]
    /// ```
    ///
    /// Where each op is one of:
    ///
    /// ```
    /// write <literal> to <index>
    /// write <index> to <index>
    /// insert <literal> at <index>
    /// insert <index> at <index>
    /// prepend <literal>
    /// prepend <index>
    /// append <literal>
    /// append <index>
    /// erase <index>
    /// minsize N [fill <literal>]
    /// resize N [fill <literal>]
    /// maxsize N
    /// ```
    ///
    /// An `<index>` is an integer enclosed in square brackets, and a
    /// `<literal>` is an element value serialized by `VtStreamOut()`.
    ///
    friend std::ostream &
    operator<<(std::ostream &out, const VtArrayEdit self) {
        auto streamElem = [&](int64_t index) -> std::ostream & {
            return VtStreamOut(self._literals[index], out);
        };
        return Vt_ArrayEditStreamImpl(
            self._ops, self._literals.size(), streamElem, out);
    }

    /// Insert \p self to the stream \p out, but call \p unaryOp on each
    /// contained value-type element and pass the result to \p VtStreamOut().
    /// An example where this is useful is when the element-type is string and
    /// the destination format requires quoting.  For more details on the
    /// overall output format, see the regular stream insertion operator.
    template <class UnaryOp>
    std::ostream &
    StreamCustom(std::ostream &out, UnaryOp &&unaryOp) const {
        auto streamElem = [&](int64_t index) -> std::ostream & {
            return VtStreamOut(
                std::forward<UnaryOp>(unaryOp)(_literals[index]),
                out);
        };
        return Vt_ArrayEditStreamImpl(_ops, _literals.size(), streamElem, out);
    }

private:
    friend class VtArrayEditBuilder<ELEM>;
    friend struct Vt_ArrayEditHashAccess;

    friend
    std::ostream &Vt_ArrayEditStreamImpl(
        Vt_ArrayEditOps const &ops, size_t literalsSize,
        TfFunctionRef<std::ostream &(int64_t index)> elemToStr,
        std::ostream &out);
    
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
    
    Array _literals;
    _Ops _ops;
};

VT_API
std::ostream &Vt_ArrayEditStreamImpl(
    Vt_ArrayEditOps const &ops, size_t literalsSize,
    TfFunctionRef<std::ostream &(int64_t index)> streamElem,
    std::ostream &out);

// Declare basic arrayEdit instantiations as extern templates.  They are
// explicitly instantiated in arrayEdit.cpp.
#define VT_ARRAY_EDIT_EXTERN_TMPL(unused, elem) \
    VT_API_TEMPLATE_CLASS(VtArrayEdit< VT_TYPE(elem) >);
TF_PP_SEQ_FOR_EACH(VT_ARRAY_EDIT_EXTERN_TMPL, ~, VT_SCALAR_VALUE_TYPES)
#undef VT_ARRAY_EDIT_EXTERN_TMPL

struct Vt_ArrayEditHashAccess
{
    template <class HashState, class Edit>
    static void Append(HashState &h, Edit const &edit) {
        h.Append(edit._literals, edit._ops);
    }
};

template <class HashState, class ELEM>
std::enable_if_t<VtIsHashable<ELEM>()>
TfHashAppend(HashState &h, VtArrayEdit<ELEM> const &edit) {
    Vt_ArrayEditHashAccess::Append(h, edit);
}

template <class ELEM>
VtArray<ELEM>
VtArrayEdit<ELEM>::_ApplyEdits(Array &&weaker) const
{
    TRACE_FUNCTION();

    // weaker is an array that we edit.
    Array result = std::move(weaker);
    Array const &cresult = result;

    Array const &literals = _literals;
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
    // result._literals =
    //     weaker._literals + result._literals;
    result._literals.insert(
        result._literals.begin(),
        weaker._literals.begin(), weaker._literals.end());

    // Bump the literal indexes in result._ops to account for weaker's
    // literals.
    const auto numWeakerLiterals = weaker._literals.size();
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

    const auto numWeakerLiterals = weaker._literals.size();

    // Append the stronger literals to weaker.
    weaker._literals.insert(
        weaker._literals.end(),
        std::make_move_iterator(result._literals.begin()),
        std::make_move_iterator(result._literals.end()));
    
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

// VtArrayEdit can transform if the underlying element type can.
template <class ELEM>
struct VtValueTypeCanTransform<VtArrayEdit<ELEM>> :
    VtValueTypeCanTransform<ELEM> {};

// VtArrayEdit can compose over itself, VtArray, and the VtBackground.
template <class T>
struct VtValueTypeCanCompose<VtArrayEdit<T>> : std::true_type {};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_ARRAY_EDIT_H
