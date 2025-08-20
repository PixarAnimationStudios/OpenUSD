//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_VECTOR_SUBRANGE_ACCESSOR_H
#define PXR_EXEC_VDF_VECTOR_SUBRANGE_ACCESSOR_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/boxedContainer.h"
#include "pxr/exec/vdf/vectorData.h"

#include "pxr/base/tf/safeTypeCompare.h"

PXR_NAMESPACE_OPEN_SCOPE

template <typename T> class VdfIteratorRange;
template <typename T> class VdfReadIterator;
template <typename T> class VdfSubrangeView;

// Post a TF_FATAL_ERROR on behalf of Vdf_VectorSubrangeAccessor when a
// type-mismatch is detected.
[[noreturn]]
VDF_API
void
Vdf_VectorSubrangeAccessorPostFatalError(
    const std::type_info &haveType,
    const std::type_info &wantType);

/// Specialized vector accessor for read access to boxed containers.
///
/// Subrange accessors may be freely constructed for both boxed and non-boxed
/// vectors but clients must only call GetBoxedRanges() if the vector contains
/// boxed values.
///
/// Only VdfSubrangeView has access to the methods of this type.
///
template <typename T>
class Vdf_VectorSubrangeAccessor
{
public:
    /// Constructor.
    ///
    Vdf_VectorSubrangeAccessor(
        Vdf_VectorData *data,
        const Vdf_VectorData::Info &info)
        : _boxedRanges(nullptr)
    {
        const std::type_info &haveType = data->GetTypeInfo();
        const std::type_info &wantType = typeid(T);
        if (ARCH_UNLIKELY(!TfSafeTypeCompare(haveType, wantType))) {
            Vdf_VectorSubrangeAccessorPostFatalError(haveType, wantType);
        }

        if (info.layout == Vdf_VectorData::Info::Layout::Boxed) {
            const Vdf_BoxedContainer<T> *boxedContainer =
                static_cast<const Vdf_BoxedContainer<T>*>(info.data);
            _boxedRanges = &boxedContainer->GetRanges();
        }
    }

private:
    friend class VdfSubrangeView<VdfIteratorRange<VdfReadIterator<T>>>;

    /// Returns true if the vector is holding boxed values.
    ///
    bool IsBoxed() const {
        return _boxedRanges;
    }

    /// Returns a reference to the boxed container ranges.
    ///
    /// Clients must ensure that IsBoxed() returns true before calling this
    /// method.
    ///
    const Vdf_BoxedRanges &GetBoxedRanges() const {
        return *_boxedRanges;
    }

private:
    const Vdf_BoxedRanges *_boxedRanges;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
