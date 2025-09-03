//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_INPUT_VALUES_POINTER_H
#define PXR_EXEC_VDF_INPUT_VALUES_POINTER_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/iterator.h"
#include "pxr/exec/vdf/readIterator.h"

#include "pxr/base/tf/span.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfInputValuesPointer
///
/// VdfInputValuesPointer is a smart pointer object that guarantees contiguous
/// memory access to the requested input values, regardless of the actual
/// memory layout in the output buffers.
/// 
/// \warning Due to performance caveats described below, accessing values
/// through an iterator (e.g. VdfReadIterator) or the VdfContext is generally
/// a better choice.
/// 
/// If the memory layout of input values is not contiguous in the output
/// buffers, this class will make a copy of the input values in order to
/// satisfy the contiguous access guarantees. Note that it can be expensive to
/// make this copy. If necessary, the copy will be produced at time of
/// construction.
/// 
/// If the memory layout of input values is already contiguous in the output
/// buffers, this class will provide contiguous access into those buffers
/// without making any copies.
/// 
/// Note that the memory layout of output buffers is an implementation detail
/// of the system influenced by many factors. Subsequently, no assumptions can
/// be made about whether copies will be made or not.
/// 
/// The only way to guarantee that no copies will be made is by accessing data
/// through iterators (e.g. VdfReadIterator) or the VdfContext (e.g.
/// VdfContext::GetInputValue). The use of iterators or the VdfContext instead
/// of using VdfInputValuesPointer is strongly encouraged. When calling into
/// functions, a good pattern is to parameterize said functions with iterator
/// ranges, rather than raw pointers or specific container types.
/// 
/// ```{.cpp}
/// template < Iterator >
/// MyFunction(Iterator begin, Iterator end)
/// {
///     for (Iterator it = begin; it != end; ++it) {
///         ...
///     }
/// }
/// ```
/// 
/// When using iterator ranges is not possible, for example when calling into
/// functions from a third party library, VdfInputValuesPointer may be used to
/// satisfy the required contiguous memory access guarantees.
/// 
/// ```{.cpp}
/// VdfInputValuesPointer<GfVec3d> points(context, _tokens->points);
/// ThirdPartyFunction(points.GetData(), points.GetSize(), ...);
/// ```
///
template < typename T >
class VdfInputValuesPointer : public VdfIterator
{
public:

    /// Construct a new instance of this class with access to the input values
    /// provided by the input named \p inputName. If the data provided by \p
    /// inputName is not contiguous in memory, the constructor will make a copy
    /// of the input values.
    ///
    VdfInputValuesPointer(const VdfContext &context, const TfToken &inputName);

    /// Destructor.
    ///
    ~VdfInputValuesPointer();

    /// Returns an immutable raw pointer to the data. Accessing data outside
    /// the bounds established by GetSize() is invalid and will lead to
    /// undefined behavior.
    ///
    const T *GetData() const {
        return _data;
    }

    /// Returns the size of the data in number of elements stored.
    ///
    size_t GetSize() const {
        return _size;
    }

    /// Construct a read-only TfSpan viewing this object's data. This enables
    /// the use of VdfInputValuesPointer with template methods that require STL
    /// container API. This user-defined conversion is necessary because the API
    /// on this class is incompatible with the STL requirements of the implicit
    /// container-conversion constructor on TfSpan.
    ///
    operator TfSpan<const T>() const {
        return TfSpan<const T>(GetData(), GetSize());
    }

private:

    // Noncopyable
    VdfInputValuesPointer(const VdfInputValuesPointer &) = delete;
    VdfInputValuesPointer &operator=(const VdfInputValuesPointer &) = delete;

    // Make a copy of the input values.
    void _CopyInputValues(const VdfContext &context, const TfToken &inputName);

    // A raw pointer to the data.
    const T *_data;

    // The number of elements in data.
    size_t _size;

    // Is this a copy of the data?
    bool _isCopy;

};

///////////////////////////////////////////////////////////////////////////////

template < typename T >
VdfInputValuesPointer<T>::VdfInputValuesPointer(
    const VdfContext &context,
    const TfToken &inputName) :
    _data(nullptr),
    _size(0),
    _isCopy(false)
{
    // Get the requested input.
    const VdfInput * const input = _GetNode(context).GetInput(inputName);

    // Bail out if the input is not available or if it has no connections.
    if (!input || input->GetNumConnections() == 0) {
        return;
    }

    // If there is only one connection targeting the requested input, and that
    // connection has a contiguous mask, we do not need to make a copy. This
    // is the fast path.
    if (input->GetNumConnections() == 1) {
        const VdfConnection &connection = (*input)[0];
        const VdfMask &mask = connection.GetMask();

        // Bail out if the single connection mask is all zeros.
        if (mask.IsAllZeros()) {
            return;
        }

        // If the connection mask is contiguous, we can retain a raw pointer to
        // the data stored in the output buffer.
        if (mask.IsContiguous()) {
            if (const VdfVector * const v =
                    _GetInputValue(context, connection, mask)) {
                const VdfVector::ReadAccessor<T> a = v->GetReadAccessor<T>();

                // If the VdfVector is empty, we have no data, so don't set the
                // data pointer.
                if (!a.IsEmpty()) {
                    _size = a.IsBoxed() ? a.GetNumValues() : mask.GetNumSet();
                    _data = &a[mask.GetFirstSet()];
                }
            }
            return;
        }
    }

    // If we were not able to retain a pointer pointing directly at the output
    // buffer, we need to fall back to making a copy of the input values. This
    // is the slow path.
    _CopyInputValues(context, inputName);
}

template < typename T >
void
VdfInputValuesPointer<T>::_CopyInputValues(
    const VdfContext &context,
    const TfToken &inputName)
{
    TRACE_FUNCTION();

    // Get a read iterator to the input values.
    VdfReadIterator<T> it(context, inputName);

    // Compute the size from the read iterator, and allocate an array large
    // enough to accommodate our copy of the input values.
    _isCopy = true;
    _size = it.ComputeSize();
    T *copy = new T[_size];

    // Iterate over the input values and copy them into our array, such that
    // the data is guaranteed to be laid out contiguously in memory.
    for (size_t i = 0; !it.IsAtEnd(); ++i, ++it) {
        copy[i] = *it;
    }

    // Assign the pointer to the copy to data.
    _data = copy;
}

template < typename T >
VdfInputValuesPointer<T>::~VdfInputValuesPointer()
{
    // If a copy was made during construction, we need to destruct that copy.
    if (_isCopy) {
        delete[] _data;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
