//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_VECTOR_IMPL_SINGLE_H
#define PXR_EXEC_VDF_VECTOR_IMPL_SINGLE_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/boxedContainerTraits.h"
#include "pxr/exec/vdf/estimateSize.h"
#include "pxr/exec/vdf/fixedSizeHolder.h"
#include "pxr/exec/vdf/forEachCommonType.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/vectorDataTyped.h"
#include "pxr/exec/vdf/vectorImpl_Empty.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Implements a Vdf_VectorData storage that is holds a single element.
///
template<typename TYPE>
class VDF_API_TYPE Vdf_VectorImplSingle final
    : public Vdf_VectorDataTyped<TYPE>
{
    static_assert(
        !Vdf_IsBoxedContainer<TYPE>,
        "Only Vdf_VectorImplBoxed may hold boxed values");

public:

    Vdf_VectorImplSingle() : _data(TYPE()) {
    }

    explicit Vdf_VectorImplSingle(const TYPE &value) :
        _data(value) {
    }

    explicit Vdf_VectorImplSingle(TYPE &&value) :
        _data(std::move(value)) {
    }

    Vdf_VectorImplSingle(const Vdf_VectorImplSingle &rhs) :
        _data(rhs._data) {
    }

    Vdf_VectorImplSingle(Vdf_VectorImplSingle &&rhs) :
        _data(std::move(rhs._data)) {
    }

    ~Vdf_VectorImplSingle() override = default;

    void MoveInto(Vdf_VectorData::DataHolder *destData) override
    {
        destData->Destroy();
        destData->New< Vdf_VectorImplSingle >(std::move(*this));
    }

    void Clone(Vdf_VectorData::DataHolder *destData) const override
    {
        // XXX:optimization
        // Here, since we have destData, we can dynamic_cast it to a
        // Vdf_SingleElement and if it is, we can assign the element directly
        // without having to delete and Clone().  So far that hasn't shown up
        // in any profile.
        destData->Destroy();
        destData->New< Vdf_VectorImplSingle >(*this);
    }

    void CloneSubset(const VdfMask &mask,
                     Vdf_VectorData::DataHolder *destData) const override
    {
        // We only have one element, not much point in looking at the mask.
        Clone(destData);
    }

    void Box(const VdfMask::Bits &bits,
             Vdf_VectorData::DataHolder *destData) const override
    {
        // We should never box single values.  Attempting to do so will yield
        // either a copy of this impl, if the mask is suitable, or an empty
        // impl.  There is no circumstance which will yield a boxed impl.
        TF_VERIFY(false, "Attempted to box single-element vector");

        if (bits.GetSize() == 1 && bits.AreAllSet()) {
            destData->Destroy();
            destData->New< Vdf_VectorImplSingle >(*this);
        } else {
            destData->Destroy();
            destData->New< Vdf_VectorImplEmpty<TYPE> >(1);
        }
    }

    void Merge(const VdfMask::Bits &bits,
               Vdf_VectorData::DataHolder *destData) const override
    {
        if (bits.AreAllSet()) {
            Clone(destData);
        }
    }

    size_t GetSize() const override
    {
        return 1;
    }

    size_t GetNumStoredElements() const override
    {
        return 1;
    }

    size_t EstimateElementMemory() const override
    {
        // Clients of execution may overload VdfEstimateSize to provide a more
        // accurate estimate based on the held value.
        return VdfEstimateSize(_data.Get());
    }

    Vdf_VectorData::Info GetInfo() override
    {
        return Vdf_VectorData::Info(
            /* data = */ &_data.GetMutable(), 
            /* size = */ 1);
    }

private :

    Vdf_FixedSizeHolder<TYPE, Vdf_VectorData::_SmallBufferSize> _data;
};

#define VDF_DECLARE_EXTERN_VECTOR_IMPL_SINGLE(type)     \
    extern template class VDF_API_TYPE Vdf_VectorImplSingle<type>;
VDF_FOR_EACH_COMMON_TYPE(VDF_DECLARE_EXTERN_VECTOR_IMPL_SINGLE)
#undef VDF_DECLARE_EXTERN_VECTOR_IMPL_SINGLE

PXR_NAMESPACE_CLOSE_SCOPE

#endif
