//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_VECTOR_DATA_TYPED_H
#define PXR_EXEC_VDF_VECTOR_DATA_TYPED_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/forEachCommonType.h"
#include "pxr/exec/vdf/vectorData.h"

PXR_NAMESPACE_OPEN_SCOPE

template<typename TYPE> class Vdf_VectorImplEmpty;
template<typename TYPE> class Vdf_VectorImplSingle;
template<typename TYPE> class Vdf_VectorImplContiguous;
template<typename TYPE> class Vdf_VectorImplBoxed;

// Vdf_VectorDataTypedBase is used as templated base class for
// Vdf_VectorDataTyped implementing methods that are the same for
// all types.
//
template<typename TYPE>
class VDF_API_TYPE Vdf_VectorDataTyped : public Vdf_VectorData
{
public:
    const std::type_info &GetTypeInfo() const override final
    {
        return typeid(TYPE);
    }

    void NewEmpty(size_t size, DataHolder *destData) const override final
    {
        destData->New< Vdf_VectorImplEmpty<TYPE> >(size);
    }

    void NewSingle(DataHolder *destData) const override final
    {
        destData->New< Vdf_VectorImplSingle<TYPE> >();
    }

    void NewSparse(
        size_t size, size_t first, size_t last,
        DataHolder *destData) const override final
    {
        destData->New< Vdf_VectorImplContiguous<TYPE> >(size, first, last);
    }

    void NewDense(size_t size, DataHolder *destData) const override final
    {
        destData->New< Vdf_VectorImplContiguous<TYPE> >(size);
    }

    size_t EstimateElementMemory() const override
    {
        // We estimate the size of the element based on the type only. Note
        // that this is an approximation, as individual instances of TYPE may
        // heap allocate memory.
        return sizeof(TYPE);
    }
};

#if !defined(ARCH_OS_WINDOWS)
#define VDF_DECLARE_EXTERN_VECTOR_DATA_TYPED(type)      \
    extern template class VDF_API_TYPE Vdf_VectorDataTyped<type>;
VDF_FOR_EACH_COMMON_TYPE(VDF_DECLARE_EXTERN_VECTOR_DATA_TYPED)
#undef VDF_DECLARE_EXTERN_VECTOR_DATA_TYPED
#endif // !defined(ARCH_OS_WINDOWS)

PXR_NAMESPACE_CLOSE_SCOPE

#endif
