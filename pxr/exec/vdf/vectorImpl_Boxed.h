//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_VECTOR_IMPL_BOXED_H
#define PXR_EXEC_VDF_VECTOR_IMPL_BOXED_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/vectorDataTyped.h"

#include "pxr/exec/vdf/boxedContainer.h"
#include "pxr/exec/vdf/estimateSize.h"
#include "pxr/exec/vdf/forEachCommonType.h"
#include "pxr/exec/vdf/mask.h"

#include "pxr/base/tf/mallocTag.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Implements a Vdf_VectorData storage that holds a boxed element.
///
template <typename T>
class VDF_API_TYPE Vdf_VectorImplBoxed final
    : public Vdf_VectorDataTyped<T>
{
public:

    explicit Vdf_VectorImplBoxed(const Vdf_BoxedContainer<T> &box)
        : _box(box)
    {}

    explicit Vdf_VectorImplBoxed(Vdf_BoxedContainer<T> &&box)
        : _box(std::move(box))
    {}

    Vdf_VectorImplBoxed(const Vdf_VectorImplBoxed &o)
        : Vdf_VectorImplBoxed(o._box)
    {}

    Vdf_VectorImplBoxed(Vdf_VectorImplBoxed &&o)
        : Vdf_VectorImplBoxed(std::move(o._box))
    {}

    ~Vdf_VectorImplBoxed() override = default;

    void MoveInto(Vdf_VectorData::DataHolder *destData) override {
        TfAutoMallocTag tag("Vdf", __ARCH_PRETTY_FUNCTION__);
        destData->Destroy();
        destData->New< Vdf_VectorImplBoxed >(std::move(*this));
    }

    void Clone(Vdf_VectorData::DataHolder *destData) const override {
        TfAutoMallocTag tag("Vdf", __ARCH_PRETTY_FUNCTION__);
        destData->Destroy();
        destData->New< Vdf_VectorImplBoxed >(*this);
    }

    void CloneSubset(
        const VdfMask &mask,
        Vdf_VectorData::DataHolder *destData) const override {

        // We only have one element, not much point in looking at the mask.
        Clone(destData);
    }

    void Box(
        const VdfMask::Bits &bits,
        Vdf_VectorData::DataHolder *destData) const override {
        TfAutoMallocTag tag("Vdf", __ARCH_PRETTY_FUNCTION__);

        destData->Destroy();
        if (bits.GetSize() == 1 && bits.AreAllSet()) {
            destData->New< Vdf_VectorImplBoxed >(*this);
        } else {
            destData->New< Vdf_VectorImplEmpty<T> >(1);
        }
    }

    void Merge(
        const VdfMask::Bits &bits,
        Vdf_VectorData::DataHolder *destData) const override {

        if (bits.AreAllSet()) {
            Clone(destData);
        }
    }

    /// The size of a boxed vector is always 1, since that's the size of the
    /// mask used to represent data held in a boxed vector.
    size_t GetSize() const override {
        return 1;
    }

    /// The number of elements stored in a boxed vector is eiter 0 (if the box
    /// is empty) or 1. We don't return the size of the box because the number
    /// of stored elements is the same as the number of bits set in a
    /// corresponding mask.
    size_t GetNumStoredElements() const override {
        return _box.empty() ? 0 : 1;
    }

    bool IsSharable() const override {
        return _box.size() >= Vdf_VectorData::_VectorSharingSize;
    }

    size_t EstimateElementMemory() const override {
        // This is somewhat tricky to think about.  For boxed impls, the
        // "element" is a Vdf_BoxedContainer<T> that may hold many Ts.
        size_t elementSize = VdfEstimateSize(_box);
        if (!_box.empty()) {
            elementSize +=
                VdfEstimateSize(_box[0]) * _box.size() +
                VdfEstimateSize(Vdf_BoxedRanges::Range{}) *
                _box.GetRanges().GetNumRanges();
        }
        return elementSize;
    }

    Vdf_VectorData::Info GetInfo() override {
        return Vdf_VectorData::Info(
            /* data = */ &_box,
            /* size = */ 1,
            /* first = */ 0,
            /* last = */ 0,
            /* compressedIndexMapping = */ nullptr,
            /* layout = */ Vdf_VectorData::Info::Layout::Boxed);
    }

private:

    Vdf_BoxedContainer<T> _box;
};

#define VDF_DECLARE_EXTERN_VECTOR_IMPL_BOXED(type)      \
     extern template class VDF_API_TYPE Vdf_VectorImplBoxed<type>;
VDF_FOR_EACH_COMMON_TYPE(VDF_DECLARE_EXTERN_VECTOR_IMPL_BOXED)
#undef VDF_DECLARE_EXTERN_VECTOR_IMPL_BOXED

PXR_NAMESPACE_CLOSE_SCOPE

#endif
