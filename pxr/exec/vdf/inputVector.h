//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_INPUT_VECTOR_H
#define PXR_EXEC_VDF_INPUT_VECTOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/tokens.h"
#include "pxr/exec/vdf/typedVector.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

class TfType;

////////////////////////////////////////////////////////////////////////////////

class VDF_API_TYPE Vdf_InputVectorBase : public VdfNode
{
public:

    /// Returns the number of values in the input vector.
    ///
    size_t GetSize() const {
        return _values.GetSize();
    }

    /// The computation of this node is simply setting the values we are
    /// holding.
    ///
    VDF_API
    void Compute(const VdfContext &context) const override final;

    /// Returns the amount of memory used by this node in bytes.
    ///
    VDF_API
    size_t GetMemoryUsage() const override final;

protected:
    VDF_API
    Vdf_InputVectorBase(
        VdfNetwork *network,
        const VdfOutputSpecs &outputSpecs,
        VdfVector &&values);

    VDF_API
    ~Vdf_InputVectorBase() override;

protected:

    // The values stored in this input vector.
    VdfVector _values;
};

template<typename T>
class VDF_API_TYPE VdfInputVector final : public Vdf_InputVectorBase
{
public:

    /// Creates an input vector with \p n elements.
    ///
    VdfInputVector(VdfNetwork *network, size_t n) :
        Vdf_InputVectorBase(
            network,
            VdfOutputSpecs()
                .Connector<T>(VdfTokens->out),
            VdfTypedVector<T>::CreateWithSize(n))
    {
    }

    /// Sets the value for the input stored at \p index.
    ///
    void SetValue(size_t index, const T &val)
    {
        if (!TF_VERIFY(index >= 0 && index < _values.GetSize()))
            return;

        VdfVector::ReadWriteAccessor<T> a = _values.GetReadWriteAccessor<T>();
        a[index] = val;
    }

    /// Returns \c true if the value stored at \p index is equal to \p val.
    ///
    bool IsValueEqual(size_t index, const T &val) const
    {
        if (const T *v = GetValue(index))
            return *v == val;
            
        return false;
    }

    /// Returns a pointer to the value at \p index.  If \p index is out of
    /// range an error will be raised and a nullptr returned.
    ///
    const T *GetValue(size_t index) const
    {
        if (!TF_VERIFY(index >= 0 && index < _values.GetSize()))
            return nullptr;

        const VdfVector::ReadAccessor<T> a = _values.GetReadAccessor<T>();
        return &a[index];
    }

protected:

    // Protected destructor.  Only the network is allowed to delete nodes.
    //
    ~VdfInputVector() override;

    typedef VdfInputVector<T> This;

    // Implements IsEqual() API of VdfNode
    bool _IsDerivedEqual(const VdfNode &rhs) const override
    {
        if (const This *other = dynamic_cast<const This *>(&rhs))
            return _ValuesAreEqual(other);

        return false;
    }

private:

    // Helper method to compare if two VdfInputVector are the same. Note that
    // this is factored out in order to prevent a performance regression due
    // to code generation.
    //
    bool _ValuesAreEqual(const This *rhs) const
    {
        const size_t size = _values.GetSize();
        if (size == rhs->_values.GetSize()) {
            const VdfVector::ReadAccessor<T> a = 
                _values.GetReadAccessor<T>();
            const VdfVector &rhsValues = rhs->_values;
            const VdfVector::ReadAccessor<T> b = 
                rhsValues.GetReadAccessor<T>();
            for (size_t i = 0; i < size; ++i) {
                if (a[i] != b[i]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
};

/// An empty, typed input vector.
class VDF_API_TYPE VdfEmptyInputVector final : public Vdf_InputVectorBase
{
public:

    /// Creates an empty input vector of type \p type.
    ///
    VDF_API
    VdfEmptyInputVector(VdfNetwork *network, const TfType &type);

protected:

    // Protected destructor.  Only the network is allowed to delete nodes.
    //
    VDF_API
    ~VdfEmptyInputVector() override;

    // Implements IsEqual() API of VdfNode
    //
    VDF_API
    bool _IsDerivedEqual(const VdfNode &rhs) const override;
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
VdfInputVector<T>::~VdfInputVector() = default;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
