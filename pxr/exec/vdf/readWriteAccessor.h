//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_READ_WRITE_ACCESSOR_H
#define PXR_EXEC_VDF_READ_WRITE_ACCESSOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/iterator.h"

#include "pxr/base/arch/hints.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfReadWriteAccessor
/// 
/// VdfReadWriteAccessor allows for random access to output data. The index
/// into the data is in iteration space, i.e. access to index N returns the
/// value of the N-th element as visited by the VdfReadWriteIterator.
/// 
/// \warning Due to performance caveats described below, accessing values
/// through an iterator (e.g. VdfReadWriteIterator) is preferred, if the data
/// is accessed in a forward iterating pattern.
/// 
/// If the memory layout of the output values is not contiguous in the output
/// buffer (e.g. a non-contiguous affects mask), the accessor will redirect
/// access to the underlying data. This indirection can be costly. If the data
/// is contiguous in memory, fast access will be provided through what is
/// essentially access through pointer indirection / indexing of array elements.
/// 
/// Note that the memory layout of output buffers is an implementation detail
/// of the system influenced by many factors. Subsequently, no assumptions can
/// be made about whether access will take the fast- or the slow-path.
/// 
/// The only way to guarantee fast indirection is by accessing data through
/// iterators (e.g. VdfReadWriteIterator). The use of iterators instead
/// of using VdfReadWriteAccessor is strongly encouraged.
///
template < typename T >
class VdfReadWriteAccessor : public VdfIterator
{
public:

    /// Constructs a read/write accessor for the given input or output. If no
    /// input with the specified \p name exists on the current node, or if the
    /// input does not have an associated output, attempt to find an output
    /// named \p name. Emits a coding error if \p name does not name an input
    /// or an output.
    ///
    VdfReadWriteAccessor(const VdfContext &context, const TfToken &name);

    /// Constructs a read/write accessor for the only output on the current
    /// node. If the node has more than a single output, a coding error will be
    /// emitted.
    ///
    VdfReadWriteAccessor(const VdfContext &context) :
        VdfReadWriteAccessor(context, TfToken())
    {}

    /// Provides constant random access to the data stored at the output. Note
    /// that \p index must be within [0, GetSize()). Out of bounds access will
    /// lead to undefined behavior.
    /// 
    const T &operator[](size_t index) const {
        return const_cast<VdfReadWriteAccessor *>(this)->operator[](index);
    }

    /// Provides mutable random access to the data stored at the output. Note
    /// that \p index must be within [0, GetSize()). Out of bounds access will
    /// lead to undefined behavior.
    ///
    T &operator[](size_t index);

    /// Returns \c true if there is no data stored at the output. 
    ///
    bool IsEmpty() const {
        return GetSize() == 0;
    }

    /// Returns the size of the data stored at the output.
    ///
    size_t GetSize() const {
        return _size;
    }

private:

    // The accessor to the output data.
    VdfVector::ReadWriteAccessor<T> _accessor;

    // The mask with accessible data elements. All elements are accessible if
    // this mask is empty.
    VdfMask _mask;

    // The offset into the data.
    size_t _offset;

    // The size of the data.
    size_t _size;

};

///////////////////////////////////////////////////////////////////////////////

template < typename T >
VdfReadWriteAccessor<T>::VdfReadWriteAccessor(
    const VdfContext &context,
    const TfToken &name) :
    _offset(0),
    _size(0)
{
    // Get the required output for writing. This will emit a coding error if
    // there is no valid output.
    const VdfOutput *output = _GetRequiredOutputForWriting(context, name);
    if (!output) {
        return;
    }

    // Retrieve the relevant masks at the output. This will return false if the
    // output is not scheduled, i.e. the accessor will remain empty.
    const VdfMask *requestMask = nullptr;
    const VdfMask *affectsMask = nullptr;
    if (!_GetOutputMasks(context, *output, &requestMask, &affectsMask)) {
        return;
    }

    // Get the value to write to. It is an error for this value not to be
    // available. The executor engine is responsible for creating it.
    VdfVector *v = _GetOutputValueForWriting(context, *output);
    if (!TF_VERIFY(v, "Output '%s' is missing buffer.",
            output->GetName().GetText())) {
        return;
    }

    // Get the accessor to the value.
    _accessor = v->GetReadWriteAccessor<T>();

    // If there is an affects mask on this output, and that mask is not
    // all-ones, use the mask to redirect data access. If the mask is
    // contiguous, we can simply use the first index as an offset into the data.
    // The size is the number of bits set on the mask.
    if (affectsMask && !affectsMask->IsAllOnes()) {
        if (affectsMask->IsContiguous()) {
            _offset = affectsMask->GetFirstSet();
        } else {
            _mask = *affectsMask;
        }
        _size = affectsMask->GetNumSet();
    }

    // If there is no affects mask, or if the affects mask is all ones we can
    // provide access without redirection. The size is the number of values on
    // the vector accessor.
    else {
        _size = _accessor.GetNumValues();
    }
}

template < typename T >
T &
VdfReadWriteAccessor<T>::operator[](size_t index)
{
    // Perform out of bounds check in debug builds.
    TF_DEV_AXIOM(index < GetSize());

    // The fast-path is for data that is contiguous in memory. The offset is
    // often 0, but the addition is fast enough to perform indiscriminately and
    // instead of a branch.
    if (ARCH_LIKELY(_mask.IsEmpty())) {
        return _accessor[index + _offset];
    }

    // If a mask is used to redirect data access, we need to map the provided
    // index to the n-th set bit in the mask. This is the slow-path.
    return _accessor[_mask.GetBits().FindNthSet(index)];
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
