//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_READ_ITERATOR_H
#define PXR_EXEC_VDF_READ_ITERATOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/iterator.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/poolChainIndex.h"
#include "pxr/exec/vdf/vector.h"

#include "pxr/base/tf/diagnostic.h"

#include <iterator>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfReadIterator
///
/// An iterator that provides read access to input values using a context.
/// 
/// \note This is a ForwardIterator with the exception of a missing
/// post-increment operator. The implementation of a post-increment operator
/// would be slower than that of pre-increment and to prevent erroneous use it
/// has been omitted entirely.
///
template<typename T>
class VdfReadIterator : public VdfIterator
{
public:

    /// Type of the elements this iterator gives access to.
    ///
    using value_type = const T;

    /// The type used to identify distance between instances of this iterator.
    ///
    using difference_type = int;

    /// Type of a reference to a value of this iterator.
    ///
    using reference = value_type &;

    /// Type of a pointer to a value of this iterator.
    ///
    using pointer = value_type *;

    /// The STL category of this iterator type.
    ///
    using iterator_category = std::forward_iterator_tag;

    /// Constructs a read iterator
    ///
    VdfReadIterator(const VdfContext &context, const TfToken &inputName);

    /// Returns \c true if this iterator and \p rhs compare equal.
    ///
    bool operator==(const VdfReadIterator &rhs) const;

    /// Returns \c true if this iterator and \p rhs do not compare equal.
    ///
    bool operator!=(const VdfReadIterator &rhs) const {
        return !operator==(rhs);
    }

    /// Increment operator to point to the next element. Calling this on an
    /// iterator that IsAtEnd() is invalid and will lead to undefined behavior.
    ///
    VdfReadIterator &operator++();

    /// Returns reference to current element. Calling this on an iterator that
    /// IsAtEnd() is invalid and will lead to undefined behavior.
    ///
    reference operator*() const {
        return *operator->();
    }

    /// Returns pointer to current element. Calling this on an iterator that
    /// IsAtEnd() is invalid and will lead to undefined behavior.
    ///
    pointer operator->() const;

    /// Returns true if the iterator is done iterating and false otherwise.
    ///
    bool IsAtEnd() const {
        return _connectionIndex == -1;
    }

    /// Returns the total number of data elements that will be iterated over.
    ///
    size_t ComputeSize() const;

    /// Advance the iterator to the end.
    ///
    void AdvanceToEnd();

private:

    // VdfSubrangeView has access to the private constructor.
    template<typename> friend class VdfSubrangeView;

    // Construct a read iterator beginning at the specified connection and boxed
    // index. Will seek to the next valid connection if the specified connection
    // does not provide data.
    VdfReadIterator(
        const VdfContext &context,
        const TfToken &inputName,
        int connectionIndex,
        unsigned int boxedIndex);

    // Returns the current index into the data source.
    friend int Vdf_GetIteratorIndex(const VdfReadIterator &it) {
        return *it._iterator + it._boxedIndex;
    }

    // Returns the name of the current data source.
    friend bool Vdf_IsIteratorSourcePool(const VdfReadIterator &it) {
        return Vdf_IsPoolOutput(
            (*it._input)[it._connectionIndex].GetSourceOutput());
    }

    // Sets the current connection from our input connector and returns true if
    // the connection is valid. If the connection is not valid, we need to jump
    // to the next connection. Returns true if the specified connection is
    // a valid data source.
    bool _SetCurrentConnection(int connectionIndex);

    // Sets the current boxed index to the specified index, or advances to the
    // next valid connection if the index exceeds the number of input values
    // available on this connection. Returns true if a valid data source is
    // available.
    bool _SetCurrentBoxedIndex(unsigned int boxedIndex);

    // Advances to the next input and returns true if one exists,
    // otherwise returns false to indicate at end. Returns true if there is
    // a valid data element to advance to.
    bool _Advance();

    // Advances to the next input connection with scheduled output data to
    // source from.
    bool _AdvanceConnection(int connectionIndex);

    // The context this iterator is bound to.
    const VdfContext *_context;

    // The input connector for this iterator.  This is where all the connections
    // that we are iterating through are connected.
    const VdfInput *_input;

    // The index of the current connection.
    int _connectionIndex;

    // The index of the current boxed value.
    unsigned int _boxedIndex;

    // The iterator for the connection mask.
    VdfMask::iterator _iterator;

    // The accessor into the current output value.
    VdfVector::ReadAccessor<T> _accessor;

};

////////////////////////////////////////////////////////////////////////////////

template<typename T>
VdfReadIterator<T>::VdfReadIterator(
    const VdfContext &context, 
    const TfToken &inputName) :
    _context(&context),
    _input(_GetNode(context).GetInput(inputName)),
    _connectionIndex(-1),
    _boxedIndex(0)
{
    // If the input is not valid, or there are no input connections, this
    // iterator remains empty.
    if (!_input || _input->GetNumConnections() == 0) {
        return;
    }

    // Advance connections starting at the current connection index -1, to find
    // the first valid connection with data.
    if (!_AdvanceConnection(-1)) {
        AdvanceToEnd();
    }
}

template<typename T>
VdfReadIterator<T>::VdfReadIterator(
    const VdfContext &context, 
    const TfToken &inputName,
    int connectionIndex,
    unsigned int boxedIndex) :
    _context(&context),
    _input(_GetNode(context).GetInput(inputName)),
    _connectionIndex(-1),
    _boxedIndex(0)
{
    // If the input is not valid, or the specified connection index is beyond
    // the number of available connections, this iterator is at-end.
    if (!_input ||
        connectionIndex < 0 ||
        static_cast<size_t>(connectionIndex) >= _input->GetNumConnections()) {
        return;
    }

    // Advance to the next valid connection starting at the previous connection
    // index. If the previous connection index is -1, find the first valid
    // connection.
    if (!_AdvanceConnection(connectionIndex - 1) ||
        !_SetCurrentBoxedIndex(boxedIndex)) {
        AdvanceToEnd();
    }
}

template<typename T>
bool
VdfReadIterator<T>::operator==(const VdfReadIterator &rhs) const
{
    return
        _input == rhs._input &&
        _connectionIndex == rhs._connectionIndex &&
        _boxedIndex == rhs._boxedIndex &&
        _iterator == rhs._iterator;
}

template<typename T>
VdfReadIterator<T> &
VdfReadIterator<T>::operator++()
{
    // Are we already at end()?
    if (IsAtEnd()) {
        return *this;
    }

    // If not, advance to the next input.
    if (!_Advance()) {
        AdvanceToEnd();
    }

    return *this;
}

template<typename T>
typename VdfReadIterator<T>::pointer
VdfReadIterator<T>::operator->() const
{
    TF_DEV_AXIOM(!IsAtEnd());
    const size_t idx = *_iterator + _boxedIndex;

    TF_DEV_AXIOM(idx < _accessor.GetNumValues());
    return &_accessor[idx];
}

template<typename T>
size_t
VdfReadIterator<T>::ComputeSize() const
{
    // Bail out immediately if there is no data to iterate over.
    if (!_input || _input->GetNumConnections() == 0) {
        return 0;
    }

    // Fast path for single connection, boxed values.
    if (_input->GetNumConnections() == 1 && _accessor.IsBoxed()) {
        return _accessor.GetNumValues();
    }

    // Iterate over all the connections on the input connector.
    size_t size = 0;
    for (const VdfConnection *connection : _input->GetConnections()) {

        // For connections with 1x1 masks, we could be dealing with boxed
        // values, so we need to look at the stored value to figure out the
        // actual number of data values provided.
        const VdfMask &mask = connection->GetMask();
        if (mask.GetSize() == 1 && mask.IsAllOnes()) {

            // If this is a scheduled input value, account for the total number
            // of data values provided by the boxed container.
            if (_IsRequiredInput(*_context, *connection)) {  

                if (const VdfVector *v =
                    _GetInputValue(*_context, *connection, mask)) {
                    VdfVector::ReadAccessor<T> accessor = 
                        v->template GetReadAccessor<T>();
                    size += accessor.GetNumValues();
                }
            }
        }

        // For all other masks, the number of entries set in the mask is the
        // number of input values provided.
        else {
            size += mask.GetNumSet();
        }
    }

    // Return the number of input values provided.
    return size;
}

template<typename T>
void
VdfReadIterator<T>::AdvanceToEnd()
{
    _connectionIndex = -1;
    _boxedIndex = 0;
    _iterator = VdfMask::iterator();
}

template<typename T>
bool 
VdfReadIterator<T>::_SetCurrentConnection(int connectionIndex)
{
    const VdfConnection &connection = (*_input)[connectionIndex];
    const VdfMask &mask = connection.GetMask();

    // Reset all indices.
    _connectionIndex = connectionIndex;
    _boxedIndex = 0;
    _iterator = mask.begin();

    // Reset the accessor
    _accessor = VdfVector::ReadAccessor<T>();

    // See if the connection's source output is scheduled. This is not a
    // valid connection, if the source output is not scheduled.
    if (!_IsRequiredInput(*_context, connection)) {
        return false;
    }

    // Get the accessor to the data.
    const VdfVector *data = _GetInputValue(*_context, connection, mask);
    if (!data) {
        return false;
    }
    _accessor = data->GetReadAccessor<T>();

    // Return false if the connection does not provide any values.
    if (!_accessor.GetNumValues()) {
        return false;
    }

    // This is a valid connection if the iterator is valid.
    return !_iterator.IsAtEnd();
}

template<typename T>
bool
VdfReadIterator<T>::_SetCurrentBoxedIndex(unsigned int boxedIndex)
{
    // If the boxed index exceeds the number of available input values, move on
    // to the next valid connection, if any.
    if (boxedIndex >= _accessor.GetNumValues()) {
        return _AdvanceConnection(_connectionIndex);
    }

    // Apply the boxed index.
    _boxedIndex = boxedIndex;

    // Success!
    return true;
}

template<typename T>
bool
VdfReadIterator<T>::_Advance()
{
    // When iterating over boxed values, increment the index into the boxed
    // container.
    if (_accessor.IsBoxed()) {
        ++_boxedIndex;

        // After the last element in the boxed container, move on to the next
        // connection.
        if (_boxedIndex >= _accessor.GetNumValues()) {
            return _AdvanceConnection(_connectionIndex);
        }
    }

    // When iterating over values that are not boxed, increment the mask
    // iterator.
    else {
        ++_iterator;

        // After the last entry in the mask, move on to the next connection.
        if (_iterator.IsAtEnd()) {
            return _AdvanceConnection(_connectionIndex);
        }
    }

    // Successfully reached the next input value.
    return true;
}

template<typename T>
bool
VdfReadIterator<T>::_AdvanceConnection(int connectionIndex)
{
    // Note that we start at '_connectionIndex + 1' because we have already
    // iterated through the mask of the current connection.
    const int numConnections = _input->GetNumConnections();
    for (int i = connectionIndex + 1; i < numConnections; ++i) {

        // If the next connection has a scheduled data source, we are done.
        // Skip any data sources that provide no values.
        if (_SetCurrentConnection(i)) {
            return true;
        }
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
