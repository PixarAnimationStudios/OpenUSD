//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_MASKED_ITERATOR_H
#define PXR_EXEC_VDF_MASKED_ITERATOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/iterator.h"
#include "pxr/exec/vdf/mask.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// Enum to specify the behavior of VdfMaskedIterator as template parameter.
///
enum class VdfMaskedIteratorMode 
{
    /// The elements in the visitMask are skipped (default).
    ///
    VisitUnset = 0,

    /// Visit the elements in the visitMask instead of skipping them.
    ///
    VisitSet
};

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfMaskedIterator
///
/// An iterator that can be used to refine the given iterator \p IteratorType to
/// iterate over a given mask by either visiting or skipping over set bits
/// in the mask.
///
template<class IteratorType, VdfMaskedIteratorMode mode>
class VdfMaskedIterator final : public VdfIterator
{
public:

    /// The value type of this iterator.
    ///
    typedef typename IteratorType::value_type value_type;

    /// The dereference type of this iterator.
    ///
    typedef typename IteratorType::reference reference;

    /// Constructor.  Creates a masked iterator using \p visitMask and \p args.
    ///
    template<typename... Args>
    VdfMaskedIterator(
        const VdfContext &context, const VdfMask &visitMask, Args&&... args);

    /// Increment operator to point to the next element.
    ///
    VdfMaskedIterator &operator++();

    /// Returns reference to current element.
    ///
    reference operator*() const {
        return _iterator.operator*();
    }

    /// Returns true if the iterator is done iterating and false otherwise.
    ///
    bool IsAtEnd() const {
        return _iterator.IsAtEnd();
    }

    /// Advance the iterator to the end.
    ///
    void AdvanceToEnd() {
        _iterator.AdvanceToEnd();
    }
    
private:

    // Returns the current index into the data source.
    friend int Vdf_GetIteratorIndex(const VdfMaskedIterator &it) {
        return Vdf_GetIteratorIndex(it._iterator);
    }

    // Advance the underlying iterator as needed.
    void _AdvanceToIndexWithVisitMask();

    // The underlying iterator.
    IteratorType _iterator;

    // Mask of elements that are skipped during iteration.
    VdfMask::iterator _visitMaskIterator;
};

////////////////////////////////////////////////////////////////////////////////

template<class IteratorType, VdfMaskedIteratorMode mode>
template<typename... Args>
VdfMaskedIterator<IteratorType, mode>::VdfMaskedIterator(
    const VdfContext &context, const VdfMask &visitMask, Args&&... args) :
    _iterator(context, std::forward<Args>(args)...),
    _visitMaskIterator(visitMask.begin())
{
    // If we get an empty mask passed in, advance the iterator to the end.
    if (visitMask.GetSize() == 0) {
       _iterator.AdvanceToEnd();
    }
    
    _AdvanceToIndexWithVisitMask();
}

template<class IteratorType, VdfMaskedIteratorMode mode>
VdfMaskedIterator<IteratorType, mode> &
VdfMaskedIterator<IteratorType, mode>::operator++()
{
    _iterator.operator++();
    _AdvanceToIndexWithVisitMask();
    return *this;
}

template<class IteratorType, VdfMaskedIteratorMode mode>
void 
VdfMaskedIterator<IteratorType, mode>::_AdvanceToIndexWithVisitMask()
{
    // XXX: this could be more efficient by
    // - exposing a method to advance to the next element after a given 
    //   index in the base iterator.
    // - using those functions in the below while loop so that we skip one
    //   contiguous block of elements in visitMask at a time.
    // - this scheme also makes it hard to set the masked iterator to 
    //   "IsAtEnd" in the constructor, because advancing the mask iterator to
    //   the end (none set) doesn't mean that this iterator is at end due to
    //   the logic below.  That in turn makes it hard to check if the mask size
    //   and iterator data matches up.

    while (!_iterator.IsAtEnd()) {
        // Pull the visit mask iterator forward.
        const VdfMask::iterator::value_type idx(
            Vdf_GetIteratorIndex(_iterator));
        if (*_visitMaskIterator < idx) {
            _visitMaskIterator.AdvanceTo(idx);
        }

        const VdfMask::iterator::value_type visitIdx = *_visitMaskIterator;
        if (mode == VdfMaskedIteratorMode::VisitUnset) {
            // If we hit an element we don't want to skip, stop. Also stop, 
            // if we were unable to pull the visit mask forward (invalid visit 
            // mask case). 
            if (visitIdx > idx || visitIdx < idx) {
                break;
            }
        } else {
            // If we hit an element we want to visit, stop. Also stop, if we
            // were unable to pull the visit mask forward (invalid visit mask
            // case).
            if (visitIdx == idx || visitIdx < idx) {
                break;
            }
        }            
        // Otherwise continue iterating.
        _iterator.operator++();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
