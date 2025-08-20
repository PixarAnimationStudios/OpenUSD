//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_VT_ARRAY_EDIT_BUILDER_H
#define PXR_BASE_VT_ARRAY_EDIT_BUILDER_H

/// \file vt/arrayEditBuilder.h

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/arrayEditOps.h"
#include "pxr/base/vt/arrayEdit.h"
#include "pxr/base/vt/hash.h"
#include "pxr/base/vt/streamOut.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/span.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

// Non-template helper class for VtArrayEditBuilder.
class Vt_ArrayEditOpsBuilder
{
public:
    using Ops = Vt_ArrayEditOps;

    VT_API
    void AddOp(Ops::Op op, int64_t a1, int64_t a2);

    VT_API
    void AddOp(Ops::Op op, int64_t a1);
    
private:
    template <class ELEM>
    friend class VtArrayEditBuilder;

    void _AddOp(Ops::Op op);

    static void _IssueArityError(Ops::Op op, int count);
    static void _IssueNegativeSizeError(Ops::Op op, int64_t size);
    
    static inline bool _CheckArity(Ops::Op op, int count);

    std::vector<int64_t> _ins;
    int64_t _lastOpIdx = 0;
};

/// \class VtArrayEditBuilder
///
/// A builder type that produces instances of VtArrayEdit representing sequences
/// of array edit operations.
///
/// Callers will typically construct a builder and invoke its member functions,
/// like Write(), Insert(), Erase() repeatedly, then call FinalizeAndReset() to
/// produce a VtArrayEdit representing the sequence of operations.
///
template <class ELEM>
class VtArrayEditBuilder
{
public:
    using Ops = Vt_ArrayEditOps;
    using Edit = VtArrayEdit<ELEM>;
    using Array = typename Edit::Array;
    using ElementType = typename Edit::ElementType;
    using Self = VtArrayEditBuilder;

    /// A special index value meaning one-past-the-end of the array, for use in
    /// Insert() instructions.
    static constexpr auto EndIndex = Vt_ArrayEditOps::EndIndex;

    /// Default construct a builder with no instructions.
    VtArrayEditBuilder() = default;

    /// Add an instruction that writes \p elem to \p index.  The \p index may be
    /// negative in which case the array index is computed by adding to the
    /// array size to produce a final index.  When applied, if \p index is
    /// out-of-bounds, this instruction is ignored.
    Self &Write(ElementType const &elem, int64_t index) {
        _opsBuilder.AddOp(Ops::OpWriteLiteral, _FindOrAddLiteral(elem), index);
        return *this;
    }
    
    /// Add an instruction that writes the element at \p srcIndex to \p
    /// dstIndex.  These indexes may be negative in which case the array indexes
    /// are computed by adding to the array size to produce final indexes.  When
    /// applied, if \p index is out-of-bounds, this instruction is ignored.
    Self &WriteRef(int64_t srcIndex, int64_t dstIndex) {
        _opsBuilder.AddOp(Ops::OpWriteRef, srcIndex, dstIndex);
        return *this;
    }

    /// Add an instruction that inserts \p elem at \p index.  The \p index may
    /// be negative in which case the array index is computed by adding to the
    /// array size to produce a final index.  The index may also be \p EndIndex,
    /// which indicates insertion at the end.  When applied, if \p index is
    /// out-of-bounds and not EndIndex, this instruction is ignored.
    Self &Insert(ElementType const &elem, int64_t index) {
        _opsBuilder.AddOp(Ops::OpInsertLiteral, _FindOrAddLiteral(elem), index);
        return *this;
    }

    /// Add an instruction that inserts a copy of the element at \p srcIndex at
    /// \p dstIndex.  The indexes may be negative in which case the array
    /// indexes are computed by adding to the array size to produce the final
    /// indexes.  The dstIndex may also be \p EndIndex, which indicates
    /// insertion at the end.  When applied, if srcIndex is out-of-bounds or
    /// dstIndex is out of bounds and not EndIndex, this instruction is ignored.
    Self &InsertRef(int64_t srcIndex, int64_t dstIndex) {
        _opsBuilder.AddOp(Ops::OpInsertRef, srcIndex, dstIndex);
        return *this;
    }

    /// Equivalent to Insert(elem, 0)
    Self &Prepend(ElementType const &elem) {
        _opsBuilder.AddOp(Ops::OpInsertLiteral, _FindOrAddLiteral(elem), 0);
        return *this;
    }
    
    /// Equivalent to InsertRef(srcIndex, 0)
    Self &PrependRef(int64_t srcIndex) {
        _opsBuilder.AddOp(Ops::OpInsertRef, srcIndex, 0);
        return *this;
    }

    /// Equivalent to Insert(elem, EndIndex)
    Self &Append(ElementType const &elem) {
        _opsBuilder.AddOp(
            Ops::OpInsertLiteral, _FindOrAddLiteral(elem), EndIndex);
        return *this;
    }
    
    /// Equivalent to InsertRef(srcIndex, EndIndex)
    Self &AppendRef(int64_t srcIndex) {
        _opsBuilder.AddOp(Ops::OpInsertRef, srcIndex, EndIndex);
        return *this;
    }
    
    /// Add an instruction that erases the element at \p index.  The \p index
    /// may be negative in which case the array index is computed by adding to
    /// the array size to produce a final index.  When applied, if \p index is
    /// out-of-bounds, this instruction is ignored.
    Self &EraseRef(int64_t index) {
        _opsBuilder.AddOp(Ops::OpEraseRef, index);
        return *this;
    }

    /// Add an instruction that, if the array's size is less than \p size,
    /// appends value-initialized elements to the array until it has \p size.
    Self &MinSize(int64_t size) {
        _opsBuilder.AddOp(Ops::OpMinSize, size);
        return *this;
    }

    /// Add an instruction that, if the array's size is less than \p size,
    /// appends copies of \p fill to the array until it has \p size.
    Self &MinSize(int64_t size, ElementType const &fill) {
        _opsBuilder.AddOp(Ops::OpMinSizeFill, size, _FindOrAddLiteral(fill));
        return *this;
    }

    /// Add an instruction that, if the array's size is greater than \p size
    /// erases trailing elements until it has \p size.
    Self &MaxSize(int64_t size) {
        _opsBuilder.AddOp(Ops::OpMaxSize, size);
        return *this;
    }

    /// Add an instruction that, if the array's size is not equal to \p size,
    /// then items are either appended or erased as in MinSize() and MaxSize()
    /// until the array has size \p size.
    Self &SetSize(int64_t size) {
        _opsBuilder.AddOp(Ops::OpSetSize, size);
        return *this;
    }

    /// Add an instruction that, if the array's size is not equal to \p size,
    /// then items are either appended or erased as in MinSize() and MaxSize()
    /// until the array has size \p size.  If items are appended they are copies
    /// of \p fill.
    Self &SetSize(int64_t size, ElementType const &fill) {
        _opsBuilder.AddOp(Ops::OpSetSizeFill, size, _FindOrAddLiteral(fill));
        return *this;
    }

    /// Return a VtArrayEdit that performs the edits as specified by prior calls
    /// to this class's other member functions, then clear this builder's state,
    /// leaving it as if it was default constructed.
    VtArrayEdit<ELEM> FinalizeAndReset() {
        VtArrayEdit<ELEM> result;
        result._denseOrLiterals = std::move(_literals);
        result._ops._ins = std::move(_opsBuilder._ins);
        result._isDense = false;
        *this = {};
        return result;
    }

    /// Given a VtArrayEdit that may have been composed from several, attempt to
    /// produce a smaller, optimized edit that acts identically.  If \p in
    /// represents a dense array or is the identity, return it unmodified.
    static VtArrayEdit<ELEM> Optimize(VtArrayEdit<ELEM> &&in);

    // Return data for serializing `edit`, so it can be reconstructed later by
    // CreateFromSerializationData().  Note that VtArrayEdit::IsDense() is also
    // required, but can be obtained by calling that public API.
    //
    // This API is intended to be called only by storage/transmission
    // implementations.
    static void
    GetSerializationData(VtArrayEdit<ELEM> const &edit,
                         VtArray<ELEM> *valuesOut,
                         std::vector<int64_t> *indexesOut) {
        if (TF_VERIFY(valuesOut) && TF_VERIFY(indexesOut)) {
            *valuesOut = edit._denseOrLiterals;
            *indexesOut = edit._ops._ins;
        }
    }

    // Construct an array edit using serialization data previously obtained from
    // GetSerializationData() and VtArrayEdit::IsDense().
    //
    // This API is intended to be called only by storage/transmission
    // implementations.
    static VtArrayEdit<ELEM>
    CreateFromSerializationData(VtArray<ELEM> const &values,
                                TfSpan<int64_t> indexes,
                                bool isDense) {
        VtArrayEdit<ELEM> result;
        result._denseOrLiterals = values;
        result._ops._ins = std::vector<int64_t>(indexes.begin(), indexes.end());
        result._isDense = isDense;
        return result;
    }
    
private:
    int64_t _FindOrAddLiteral(ElementType const &elem) {
        auto iresult = _literalToIndex.emplace(elem, _literals.size());
        if (iresult.second) {
            _literals.push_back(elem);
        }
        return iresult.first->second;
    }
    
    Array _literals;
    Vt_ArrayEditOpsBuilder _opsBuilder;
    std::unordered_map<ElementType, int64_t, TfHash> _literalToIndex;
   
};

template <class ELEM>
VtArrayEdit<ELEM>
VtArrayEditBuilder<ELEM>::Optimize(VtArrayEdit<ELEM> &&in)
{
    // Minimal cases.
    if (in.IsDenseArray() || in.IsIdentity()) {
        return std::move(in);
    }
    
    VtArrayEditBuilder builder;

    // Walk all the instructions and rebuild.
    const Array literals = std::move(in._denseOrLiterals);
    const auto numLiterals = literals.size();
    Ops ops = std::move(in._ops);

    in._denseOrLiterals.clear();
    in._ops = {};
    
    ops.ForEach([&](Ops::Op op, int64_t a1, int64_t a2) {
        switch (op) {
        case Ops::OpWriteLiteral:
            // Ignore out-of-range literal indexes.
            if (a1 >= 0 && static_cast<size_t>(a1) < numLiterals) {
                builder.Write(literals[a1], a2);
            }
            break;
        case Ops::OpInsertLiteral:
            // Ignore out-of-range literal indexes.
            if (a1 >= 0 && static_cast<size_t>(a1) < numLiterals) {
                builder.Insert(literals[a1], a2);
            }
            break;
        case Ops::OpWriteRef:  builder.WriteRef(a1, a2);  break;
        case Ops::OpInsertRef: builder.InsertRef(a1, a2); break;
        case Ops::OpEraseRef:  builder.EraseRef(a1);      break;
        case Ops::OpMinSize:   builder.MinSize(a1);       break;
        case Ops::OpSetSize:   builder.SetSize(a1);       break;
        case Ops::OpMaxSize:   builder.MaxSize(a1);       break;
        case Ops::OpMinSizeFill:
            // Ignore out-of-range literal indexes.
            if (a1 >= 0 && static_cast<size_t>(a1) < numLiterals) {
                builder.MinSize(a1, literals[a2]);
            }
            break;
        case Ops::OpSetSizeFill:
            // Ignore out-of-range literal indexes.
            if (a1 >= 0 && static_cast<size_t>(a1) < numLiterals) {
                builder.SetSize(a1, literals[a2]);
            }
            break;
        };
    });
    
    return builder.FinalizeAndReset();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_ARRAY_EDIT_BUILDER_H
