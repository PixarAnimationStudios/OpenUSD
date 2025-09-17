//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_READ_WRITE_ITERATOR_H
#define PXR_EXEC_VDF_READ_WRITE_ITERATOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/allocateBoxedValue.h"
#include "pxr/exec/vdf/boxedContainer.h"
#include "pxr/exec/vdf/input.h"
#include "pxr/exec/vdf/iterator.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/vector.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfReadWriteIterator
/// 
/// This iterator provides read access to input values, and write access to the
/// associated output values. If the output does not have an associated input,
/// read/write access is provided to the output values.
/// 
/// On construction, VdfReadWriteIterator will look for an input with the
/// specified name. If the specified name does not refer to a valid input, or
/// if the input does not have an associated output, VdfReadWriteIterator will
/// look for an output with the specified name. If no valid output is available,
/// a runtime error will be emitted.
/// 
/// When constructed without an explicit input/output name, VdfReadWriteIterator
/// will look for the single output on the current node. If the node has more
/// than one output, a runtime error will be emitted.
///
/// For outputs with an affects mask, the data elements visited by the iterator
/// will be limited to those set in the affects mask. All data elements will
/// be visited for outputs without an affects mask.
/// 
/// \note This is a mutable ForwardIterator with the exception of a missing
/// post-increment operator. The implementation of a post-increment operator
/// would be slower than that of pre-increment and to prevent erroneous use it
/// has been omitted entirely.
///
template<typename T>
class VdfReadWriteIterator final : public VdfIterator
{
public:

    /// Type of the elements this iterator gives access to.
    ///
    using value_type = T;

    /// The type used to identify distance between instances of this iterator.
    ///
    using difference_type = int;

    /// Type of a reference to a value of this iterator.
    ///
    using reference = value_type &;

    /// The type of a pointer to a value of this iterator.
    ///
    using pointer = value_type *;

    /// The STL category of this iterator type.
    ///
    using iterator_category = std::forward_iterator_tag;

    /// Constructs a read/write iterator for the given input or output. If no
    /// input with the specified \p name exists on the current node, or if the
    /// input does not have an associated output, attempt to find an output
    /// named \p name. Emits a coding error if \p name does not name an input
    /// or an output.
    ///
    VdfReadWriteIterator(const VdfContext &context, const TfToken &name);

    /// Constructs a read/write iterator for the only output on the current
    /// node. If the node has more than a single output, a coding error will
    /// be emitted.
    ///
    explicit VdfReadWriteIterator(const VdfContext &context) :
        VdfReadWriteIterator(context, TfToken())
    {}

    /// Allocates storage for \p count elements at the given input or output
    /// and returns a read/write iterator at the beginning of that newly
    /// allocated storage. The elements in the storage will be default
    /// initialized.
    /// 
    /// If no input with the specified \p name exists on the current node, or if
    /// the input does not have an associated output, attempt to find an output
    /// named \p name. Emits a coding error if \p name does not name an input
    /// or an output.
    ///
    static VdfReadWriteIterator Allocate(
        const VdfContext &context,
        const TfToken &name,
        size_t count);

    /// Allocates storage for \p count elements at the only output on the
    /// current node and returns a read/write iterator at the beginning of that
    /// newly allocated storage. The elements in the storage will be default
    /// initialized.
    /// 
    /// If the node has more than a single output, a coding error will
    /// be emitted.
    ///
    static VdfReadWriteIterator Allocate(
        const VdfContext &context,
        size_t count);

    /// Returns \c true if this iterator and \p rhs compare equal.
    ///
    bool operator==(const VdfReadWriteIterator &rhs) const;

    /// Returns \c true if this iterator and \p rhs do not compare equal.
    ///
    bool operator!=(const VdfReadWriteIterator &rhs) const {
        return !operator==(rhs);
    }

    /// Increment operator to point to the next element. Calling this on an
    /// iterator that IsAtEnd() is invalid and will lead to undefined behavior.
    ///
    VdfReadWriteIterator &operator++();

    /// Returns reference to current element. Calling this on an iterator that
    /// IsAtEnd() is invalid and will lead to undefined behavior.
    ///
    reference operator*() const {
        TF_DEV_AXIOM( !IsAtEnd() );
        TF_DEV_AXIOM( *_iterator < _accessor.GetNumValues() );
        return _accessor[*_iterator];
    }

    /// Returns pointer to current element. Calling this on an iterator that
    /// IsAtEnd() is invalid and will lead to undefined behavior.
    ///
    pointer operator->() const {
        TF_DEV_AXIOM( !IsAtEnd() );
        TF_DEV_AXIOM( *_iterator < _accessor.GetNumValues() );
        return &_accessor[*_iterator];
    }
    
    /// Returns true if the iterator is done iterating and false otherwise.
    ///
    bool IsAtEnd() const {
        return _iterator.IsAtEnd();
    }

    /// Advance the iterator to the end.
    ///
    void AdvanceToEnd() {
        _iterator = VdfMask::Bits::AllSetView::const_iterator();
    }

private:

    // Default constructs a read/write iterator that is at end.
    VdfReadWriteIterator() : _output(nullptr) {}

    // Returns the current index into the data source.
    friend int Vdf_GetIteratorIndex(const VdfReadWriteIterator &it) {
        return *it._iterator;
    }

    // Initialize the iterator.
    void _Initialize(const VdfContext &context);

    // The output data accessor.
    VdfVector::ReadWriteAccessor<T> _accessor;
    
    // The mask iterator. Will iterate over the affects mask or the optional
    // bitset.
    VdfMask::Bits::AllSetView::const_iterator _iterator;

    // The optional bitset used for iteration over values with an empty
    // affects mask. A bitset is used instead of a mask, in order to avoid
    // contention on the mask registry lock.
    std::shared_ptr<VdfMask::Bits> _bits;

    // The source output.
    const VdfOutput *_output;
    
};

////////////////////////////////////////////////////////////////////////////////

template<typename T>
VdfReadWriteIterator<T>::VdfReadWriteIterator(
    const VdfContext &context,
    const TfToken &name)
{
    // Get the required output, if available. This will issue a coding error if
    // the output is not available.
    _output = _GetRequiredOutputForWriting(context, name);

    // Initialize with the required output.
    if (_output) {
        _Initialize(context);
    }
}

template<typename T>
VdfReadWriteIterator<T>
VdfReadWriteIterator<T>::Allocate(
    const VdfContext &context,
    const TfToken &name,
    size_t count)
{
    return Vdf_AllocateBoxedValue<T>(context, name, count)
        ? VdfReadWriteIterator(context, name)
        : VdfReadWriteIterator();
}

template<typename T>
VdfReadWriteIterator<T>
VdfReadWriteIterator<T>::Allocate(
    const VdfContext &context,
    size_t count)
{
    return Vdf_AllocateBoxedValue<T>(context, TfToken(), count)
        ? VdfReadWriteIterator(context)
        : VdfReadWriteIterator();
}

template<typename T>
bool
VdfReadWriteIterator<T>::operator==(const VdfReadWriteIterator &rhs) const
{
    // The source outputs must match.
    if (_output != rhs._output) {
        return false;
    }

    // If either one iterate is at-end, the other one must be at-end too.
    const bool atEnd = IsAtEnd();
    const bool rhsAtEnd = rhs.IsAtEnd();
    if (atEnd || rhsAtEnd) {
        return atEnd == rhsAtEnd;
    }

    // If neither iterator is at-end, we can dereference the mask iterators and
    // compare the resulting indices.
    return *_iterator == *rhs._iterator;
}

template<typename T>
VdfReadWriteIterator<T> &
VdfReadWriteIterator<T>::operator++()
{
    ++_iterator;
    return *this;
}

template<typename T>
void
VdfReadWriteIterator<T>::_Initialize(const VdfContext &context)
{
    // Retrieve the affects and request masks.
    const VdfMask *requestMask = nullptr;
    const VdfMask *affectsMask = nullptr;
    if (!_GetOutputMasks(context, *_output, &requestMask, &affectsMask)) {
        return;
    }

    // Get the output value for writing. We always expect there to be one. It
    // should have been prepared by the executor engine.
    VdfVector *v = _GetOutputValueForWriting(context, *_output);
    if (!TF_VERIFY(
            v, "Output '%s' is missing buffer.",
            _output->GetName().GetText())) {
        return;
    }

    // Get the accessor to the data, and bail out if there is no data to
    // iterate over.
    _accessor = v->GetReadWriteAccessor<T>();
    if (_accessor.IsEmpty()) {
        return;
    }

    // If the affects mask size mismatches the number of data elements, iterate
    // over all of the available data. This includes the case where the affects
    // mask is empty (output does not have an affects mask) and where the value
    // is boxed.
    if (ARCH_UNLIKELY(affectsMask->GetSize() != _accessor.GetNumValues())) {
        TF_DEV_AXIOM(
            (affectsMask->IsEmpty()) ||
            (affectsMask->GetSize() == 1 && _accessor.IsBoxed()));

        _bits = std::make_shared<VdfMask::Bits>(_accessor.GetNumValues());
        _bits->Complement();
        _iterator = _bits->GetAllSetView().begin();
    }

    // If there is a valid affects mask, let's use it for iteration.
    else {
        _iterator = affectsMask->GetBits().GetAllSetView().begin();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
