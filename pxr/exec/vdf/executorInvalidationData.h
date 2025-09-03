//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTOR_INVALIDATION_DATA_H
#define PXR_EXEC_VDF_EXECUTOR_INVALIDATION_DATA_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \brief 
///
class VdfExecutorInvalidationData
{
    // The possible invalidation states.
    enum class _MaskState {
        AllZeros,   // All valid
        AllOnes,    // All invalid
        Sparse      // Mask denotes validity
    };

public:

    /// The value invalidation timestamps shall initially assume.
    ///
    constexpr static VdfInvalidationTimestamp InitialInvalidationTimestamp = 0;

    /// Noncopyable.
    ///
    VdfExecutorInvalidationData(const VdfExecutorInvalidationData &) = delete;
    VdfExecutorInvalidationData &operator=(
        const VdfExecutorInvalidationData &) = delete;

    /// Constructor.
    ///
    VdfExecutorInvalidationData() : _maskState(_MaskState::AllOnes) {}

    /// Destructor.
    ///
    ~VdfExecutorInvalidationData() {}

    /// Reset the data to its original, newly constructed state.
    ///
    VDF_API
    void Reset();

    /// Clones this VdfExecutorInvalidationData instance to \p dest.
    ///
    VDF_API
    void Clone(VdfExecutorInvalidationData *dest) const;

    /// Returns \c true if the corresponding output has been marked invalid for
    /// the elements identified by \p invalidationMask. \p wasTouched indicates
    /// whether the data has been touched during evaluation.
    ///
    inline bool IsInvalid(
        const VdfMask &invalidationMask,
        const bool wasTouched) const;

    /// Marks the corresponding output as invalid for the elements identified
    /// by \p invalidationMask. \p wasTouched indicates whether the data has
    /// been touched during evaluation.
    ///
    /// Returns \c true if the data has been invalidated.  If the data was
    /// already invalid for all bits in \p invalidationMask, this method
    /// returns \c false.
    ///
    inline bool Invalidate(
        const VdfMask &invalidationMask,
        const bool wasTouched);

private:

    // Apply the invalidation mask, setting the invalidation mask state
    // and sparse invalidation mask, if required.
    inline void _ApplyInvalidationMask(const VdfMask &invalidationMask);

    // Mask that remembers which elements have been invalidated.  This mask
    // is not an up-to-date record of which elements are valid.  It is purely
    // a log used to prevent subsequent redundant invalidation, and it is
    // sometimes conservatively reset so as to cause potentially redundant
    // invalidation.
    VdfMask _mask;

    // The invalidation mask state, denoting an entirely valid, entirely
    // invalid or sparsely invalid buffer. For sparsely invalid buffers,
    // the invalidation mask becomes relevant.
    _MaskState _maskState;

};

////////////////////////////////////////////////////////////////////////////////

void
VdfExecutorInvalidationData::_ApplyInvalidationMask(
    const VdfMask &invalidationMask)
{
    // If the invalidation mask is an all-ones mask, simply set the
    // corresponding invalidation mask state.
    if (invalidationMask.IsAllOnes()) {
        _maskState = _MaskState::AllOnes;
        return;
    } 

    // Set the passed in invalidation mask, if the current mask is all zeros,
    // or the mask sizes mismatch (note, this includes the current mask being
    // an empty mask).
    if (_maskState == _MaskState::AllZeros ||
        _mask.GetSize() != invalidationMask.GetSize()) {
        _mask = invalidationMask;
        _maskState = _MaskState::Sparse;
    }

    // Append the current invalidation mask, and set the invalidation mask
    // state based on whether the new mask is all ones or sparse.
    else {
        _mask |= invalidationMask;
        _maskState = _mask.IsAllOnes()
            ? _MaskState::AllOnes
            : _MaskState::Sparse;

    }
}

bool
VdfExecutorInvalidationData::IsInvalid(
    const VdfMask &invalidationMask,
    const bool wasTouched) const
{
    // If the buffer was touched, the corresponding output is not invalid.
    if (wasTouched) {
        return false;
    }

    // If everything is invalid, or if the passed invalidation mask is empty,
    // the corresponding output is invalid.
    if (_maskState == _MaskState::AllOnes ||
        invalidationMask.IsAllZeros()) {
        return true;
    }

    // If the bits set in the passed mask are all already set in the mask
    // we are holding, the corresponding output is invalid.
    else if (_maskState == _MaskState::Sparse &&
        _mask.GetSize() == invalidationMask.GetSize() &&
        _mask.Contains(invalidationMask)) {
        return true;
    }

    // The corresponding output is not invalid for the given mask.
    return false;
}

bool
VdfExecutorInvalidationData::Invalidate(
    const VdfMask &invalidationMask,
    const bool wasTouched)
{
    // If the buffer was touched, make sure to reset the invalidation mask.
    if (wasTouched) {
        _maskState = _MaskState::AllZeros;
    }

    // If everything is invalid or if the passed invalidation mask is empty,
    // there's no invalidation to do.
    if (_maskState == _MaskState::AllOnes || 
        invalidationMask.IsAllZeros()) {
        return false;
    }

    // If the bits set in the passed mask are all already set in the mask
    // we were holding, there's no invalidation to do.
    else if (_maskState == _MaskState::Sparse &&
        _mask.GetSize() == invalidationMask.GetSize() &&
        _mask.Contains(invalidationMask)) {
        return false;
    }

    // Update the invalidation state and invalidation mask.
    _ApplyInvalidationMask(invalidationMask);

    // Return true, indicating that we did some invalidation.
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
