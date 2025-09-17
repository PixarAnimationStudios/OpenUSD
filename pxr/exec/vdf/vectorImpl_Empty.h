//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_VECTOR_IMPL_EMPTY_H
#define PXR_EXEC_VDF_VECTOR_IMPL_EMPTY_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/boxedContainerTraits.h"
#include "pxr/exec/vdf/forEachCommonType.h"
#include "pxr/exec/vdf/vectorDataTyped.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Implements a Vdf_VectorData storage that is always empty.
///
/// Note that the reason this empty holder is templated is because VdfVector
/// always needs to know the type and it gets that information from its
/// Vdf_VectorData's.
///
template<typename TYPE>
class VDF_API_TYPE Vdf_VectorImplEmpty final
    : public Vdf_VectorDataTyped<TYPE>
{
    static_assert(
        !Vdf_IsBoxedContainer<TYPE>,
        "Only Vdf_VectorImplBoxed may hold boxed values");

public :
    
    explicit Vdf_VectorImplEmpty(size_t size) : _size(size) {}

    Vdf_VectorImplEmpty(const Vdf_VectorImplEmpty &sourceData) :
        _size(sourceData._size)
    {}

    void MoveInto(Vdf_VectorData::DataHolder *destData) override
    {
        destData->Destroy();
        destData->New< Vdf_VectorImplEmpty >(*this);
    }

    void Clone(Vdf_VectorData::DataHolder *destData) const override
    {
        destData->Destroy();
        destData->New< Vdf_VectorImplEmpty >(_size);
    }

    void CloneSubset(const VdfMask &mask,
                     Vdf_VectorData::DataHolder *destData) const override
    {
        Clone(destData);
    }

    void Box(const VdfMask::Bits &bits,
             Vdf_VectorData::DataHolder *destData) const override
    {
        destData->Destroy();
        destData->New< Vdf_VectorImplEmpty >(_size);
    }

    void Merge(const VdfMask::Bits &bits,
               Vdf_VectorData::DataHolder *destData) const override
    {
        // Nothing to do here.
    }

    size_t GetSize() const override
    {
        return _size;
    }

    size_t GetNumStoredElements() const override
    {
        return 0;
    }

    Vdf_VectorData::Info GetInfo() override
    {
        return Vdf_VectorData::Info(
            /* data = */ NULL,
            /* size = */ _size);
    }

private:

    size_t _size;

};

#define VDF_DECLARE_EXTERN_VECTOR_IMPL_EMPTY(type)      \
    extern template class VDF_API_TYPE Vdf_VectorImplEmpty<type>;
VDF_FOR_EACH_COMMON_TYPE(VDF_DECLARE_EXTERN_VECTOR_IMPL_EMPTY)
#undef VDF_DECLARE_EXTERN_VECTOR_IMPL_EMPTY

PXR_NAMESPACE_CLOSE_SCOPE

#endif
