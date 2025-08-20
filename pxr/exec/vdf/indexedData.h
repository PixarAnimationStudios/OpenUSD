//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_INDEXED_DATA_H
#define PXR_EXEC_VDF_INDEXED_DATA_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/indexedDataIterator.h"
#include "pxr/exec/vdf/traits.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/tf.h"

#include <algorithm>
#include <ostream>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
template<typename T> 
class VdfIndexedData
{
public:
    /// The type of data held in the indexed data object.
    ///
    using DataType = T;

    /// Adds a corresponding (index, data) pair to this object.  The index
    /// values must be added in strictly increasing order.
    ///
    void Add(int index, VdfByValueOrConstRef<T> data) {
        TfAutoMallocTag2 tag("Vdf", "VdfIndexedData::Add");
        TfAutoMallocTag tag2(__ARCH_PRETTY_FUNCTION__);
        if (!_indices.empty() && index <= _indices.back()) {
            TF_CODING_ERROR("Indexed data must be added in sorted order.");
            return;
        }
        _indices.push_back(index);
        _data.push_back(data);
    }

    /// Returns the number of indexed data.
    ///
    size_t GetSize() const {
        return _indices.size();
    }

    /// Reserve a size for the arrays.
    ///
    /// Reserving a non-zero size avoids repeated resizing.
    void Reserve(size_t size) {
        _indices.reserve(size);
        _data.reserve(size);
    }

    /// Returns true if this structure is empty, and false otherwise.
    ///
    bool IsEmpty() const {
        return _indices.empty();
    }

    /// Returns the i'th index.
    ///
    int GetIndex(size_t i) const {
        return _indices[i];
    }

    /// Returns the i'th data.
    ///
    VdfByValueOrConstRef<T> GetData(size_t i) const {
        return _data[i];
    }

    /// Returns a non-const reference to the i'th data.
    ///
    T &operator[](size_t i) {
        // This used to be called GetData(), but that caused a const/non-const
        // ambiguity (which in turn was problamatic for std::vector<bool>). 
        return _data[i];
    }

    /// Returns the first index that is greater than or equal to
    /// \p currentIndex.
    ///
    /// Returns size (c.f., GetSize()) if no such index has been found.
    ///
    size_t GetFirstDataIndex(size_t currentIndex) const;

    /// Returns the first index that is greater than or equal to
    /// \p currentIndex.
    /// 
    /// Performs a linear search starting at \p startIndex. Returns size
    // (c.f., GetSize()) if no such index has been found.
    /// 
    size_t GetFirstDataIndex(size_t currentIndex, size_t startIndex) const {
        const size_t size = _indices.size();
        for (size_t i = startIndex; i < size; ++i) {
            if (_indices[i] >= 0 &&
                static_cast<unsigned int>(_indices[i]) >= currentIndex) {
                return i;
            }
        }
        return size;
    }

    /// Composes indexedData with an over composition, strong over weak.
    ///
    /// Indices are the union of weak and strong, strong wins if they
    /// both have data for the same index.
    ///
    /// Result needs to be different than weak and strong, we're modifying
    /// result indexedData in place.
    ///
    /// XXX:optimization
    /// Try to re-use result indexedData many times, onced it's been
    /// expanded in size this composition doesn't do any mallocs. The
    /// first time through there are mallocs due to push_backs
    ///
    static void Compose(VdfIndexedData *result,
                        const VdfIndexedData &weak,
                        const VdfIndexedData &strong); 

    // CODE_COVERAGE_OFF_VDF_DIAGNOSTICS
    /// Returns the amount of memory used by this data structure.
    ///
    size_t GetMemoryUsage() const {
        return sizeof(*this) +
               sizeof(int)*_indices.capacity() +
               sizeof(T)*_data.capacity();
    }
    // CODE_COVERAGE_ON

    /// Overload for TfHashAppend.  This makes VdfIndexedData hashable by
    /// TfHash.
    /// 
    template <class HashState>
    friend void TfHashAppend(HashState &h, const VdfIndexedData &d) {
        h.Append(d._indices, d._data);
    }

    /// Returns true, if two objects are equal.
    ///
    bool operator==(const VdfIndexedData &rhs) const {
        return _indices == rhs._indices &&
               _data == rhs._data;
    }

    /// Returns false if two objects are equal.
    ///
    bool operator!=(const VdfIndexedData &rhs) const {
        return !(*this == rhs);
    }

    /// Support for swapping two VdfIndexedData instances.
    ///
    friend void swap(VdfIndexedData &lhs, VdfIndexedData &rhs) {
        lhs._indices.swap(rhs._indices);
        lhs._data.swap(rhs._data);
    }

    /// The following types are const forward iterators which provide limited
    /// read access to the "index" and "data" member containers of this class.
    ///
    typedef VdfIndexedDataIterator<int> IndexIterator;
    typedef VdfIndexedDataIterator<T> DataIterator;

    // XXX
    // Supplying a range of iterators as a pair is consistent with other usages
    // of iterator ranges in the STL and our code base, but it would be
    // preferred to replace that idiom entirely with a dedicated range type.
    typedef std::pair<IndexIterator, IndexIterator> IndexIteratorRange;
    typedef std::pair<DataIterator, DataIterator> DataIteratorRange;

    /// Return a pair of STL-compliant forward input iterators that bracket the
    /// full range of the unspecified container holding the "index" values in
    /// this object.  For read-only use by STL algorithms.
    ///
    /// Note that the "index" values held by this object are guaranteed to be in
    /// sorted incrementing-value order.
    ///
    IndexIteratorRange GetIndexIterators() const {
        return std::make_pair(IndexIterator(_indices.begin()),
                              IndexIterator(_indices.end()));
    }

    /// Return a pair of STL-compliant forward input iterators that bracket the
    /// full range of the unspecified container holding the "data" values in
    /// this object.  For read-only use by STL algorithms.
    ///
    /// Note that the "data" values held by this object have no defined order.
    ///
    DataIteratorRange GetDataIterators() const {
        return std::make_pair(DataIterator(_data.begin()),
                              DataIterator(_data.end()));
    }

protected:
    // Returns the non-const indices.
    std::vector<int> &_GetWriteIndices() {
        return _indices;
    }

    // Returns the indices.
    const std::vector<int> &_GetReadIndices() const {
        return _indices;
    }

    // Returns the non-const data.
    std::vector<T> &_GetWriteData() {
        return _data;
    }

    // Returns the data.
    const std::vector<T> &_GetReadData() const {
        return _data;
    }

    // Provided access to the above protected member functions for derived
    // classes that need to call them on instances other than themselves.
    static std::vector<int> &_GetWriteIndices(
        VdfIndexedData<T> *o) {
        return o->_GetWriteIndices();
    }

    static const std::vector<int> &_GetReadIndices(
        const VdfIndexedData<T> *o) {
        return o->_GetReadIndices();
    }

    static std::vector<T> &_GetWriteData(
        VdfIndexedData<T> *o) {
        return o->_GetWriteData();
    }

    static const std::vector<T> &_GetReadData(
        const VdfIndexedData<T> *o) {
        return o->_GetReadData();
    }

private:

    // The indices corresponding to the data in _data.
    std::vector<int> _indices;

    // The data corresponding to the indices in _indices.
    std::vector<T> _data;

};

template<class T>
std::ostream &
operator<<(std::ostream &os, const VdfIndexedData<T> &data)
{
    size_t sz = data.GetSize();

    // Output the VdfIndexedData<T> as a python like ordered list of tuples.
    // This seems to be the most reasonable thing to do.
    os << '[';
    for(size_t i=0; i<sz; i++) {
        os << '(' << data.GetIndex(i) << ", " << data.GetData(i) << ')';
        if (i+1 < sz)
            os << ", ";
    }
    os << ']';
    return os;
}

template<typename T>
size_t 
VdfIndexedData<T>::GetFirstDataIndex(size_t currentIndex) const
{
    std::vector<int>::const_iterator it = std::lower_bound(
        _indices.begin(), _indices.end(), currentIndex);
    return std::distance(_indices.begin(), it);
}

template<typename T>
void
VdfIndexedData<T>::Compose(VdfIndexedData<T> *result,
                           const VdfIndexedData<T> &weak,
                           const VdfIndexedData<T> &strong) 
{

    if ((&weak == result) ||
        (&strong == result)) {
        TF_CODING_ERROR("Result indexData must be different than strong or weak.");
        return;
    }
    
    // Quick returns in the case where one of the vectors is empty
    //
    if (strong._indices.empty()) {
        result->_indices = weak._indices;
        result->_data = weak._data;
        return;
    }
    
    if (weak._indices.empty()) {
        result->_indices = strong._indices;
        result->_data = strong._data;
        return;
    }

    std::vector<int> &resultIndices = result->_indices;
    std::vector<T> &resultData = result->_data;

    // clear vector but don't free memory, should be fast once
    // result has been resized
    resultIndices.clear();
    resultData.clear();
    
    // Compose vectors in a single pass, take advantage of the fact that
    // _indices is sorted, this is verified in Add.
    size_t strongIndex = 0; // index into both vectors in strong
    bool strongDone = false;// becomes true when strongIndex >= strong.size()
    
    size_t weakIndex = 0; // index into both vectors in weak
    bool weakDone = false;// becomes true when weakIndex >= weak.size()

    // Set up local variables for accessing the indices.
    const std::vector<int> &strongIndices = strong._indices;
    const std::vector<int> &weakIndices = weak._indices;

    size_t numStrongIndices = strongIndices.size();
    size_t numWeakIndices = weakIndices.size();

    // Iterate from start of both indexedData, stop this iteration
    // when we hit the end of either vector
    while ( (weakDone == false) || (strongDone == false)) {

        // default to appending value from strong indexedData
        // onto result indexedData;
        bool useStrongValue = true;

        int strongIndexValue = -1;
        int weakIndexValue = -1;
            
        if (strongDone) {
            
            // if strong indexedData is empty, use weak value            
            useStrongValue = false;
            weakIndexValue = weakIndices[weakIndex];            
            
        } else if (weakDone) {

            // No need to set useStrongValue,  initialized = true
            strongIndexValue = strongIndices[strongIndex];
            
        } else {

            // we're still iterating through both vectors, do
            // the composition depending on current index values

            strongIndexValue = strongIndices[strongIndex];
            weakIndexValue = weakIndices[weakIndex];

            if (weakIndexValue < strongIndexValue) {

                // Only case where we use weak value.
                //
                // If the values are equal then use data from strong
                // because both indexedData have data for the same index,
                // strong wins in this case as this is an "over"
                // composition.
                //
                // if strongIndexValue > weakIndexValue, then use value
                // from strong because weak doesn't have data for the index.
                //
                useStrongValue = false;
            }
        }

        // Push back on the two std::vectors within result that we're
        // accumulating into.
        //
        // XXX:optimization
        // Hopefully result indexedData is re-used as a buffer and these
        // push_backs don't have to malloc.
        //
        if (useStrongValue) {

            TF_DEV_AXIOM(strongIndexValue >= 0);
            
            resultIndices.push_back(strongIndexValue);
            resultData.push_back(strong._data[strongIndex]);
            ++strongIndex;
            if (weakIndexValue == strongIndexValue) {
                ++weakIndex;
            }
            
        } else {

            TF_DEV_AXIOM(weakIndexValue >= 0);
            
            // Use weak value
            resultIndices.push_back(weakIndexValue);
            resultData.push_back(weak._data[weakIndex]);
            ++weakIndex;
        }
        
        // check if we've hit the end of either vector
        if (strongIndex >= numStrongIndices) {
            strongDone = true;
        }
        if (weakIndex >= numWeakIndices) {
            weakDone = true;
        }
    }
}

// VdfIndexedData<T> is equality comparable iff T is equality comparable.
template <typename T>
constexpr bool VdfIsEqualityComparable<VdfIndexedData<T>> =
    VdfIsEqualityComparable<T>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
