//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_REQUEST_H
#define PXR_EXEC_VDF_REQUEST_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/maskedOutputVector.h"

#include "pxr/base/tf/bits.h"
#include "pxr/base/tf/hash.h"

#include <cstddef>
#include <iterator>
#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNetwork;

class VdfRequest 
{
public:
    /// Constructor for empty VdfRequest.
    ///
    VDF_API
    VdfRequest();

    /// Convenience constructor for a VdfRequest with a singular output.
    ///
    VDF_API
    explicit VdfRequest(const VdfMaskedOutput& output);

    /// Constructor from a vector.  Automatically sorts and uniques the vector.
    /// NOTE : The sort does not distinguish between masked outputs that 
    ///        contain the same VdfOutput pointer.  
    ///
    VDF_API
    explicit VdfRequest(const VdfMaskedOutputVector& vector);

    /// Move constructor from a vector.  Automatically sorts and uniques the 
    /// vector.
    /// NOTE : The sort does not distinguish between masked outputs that 
    ///        contain the same VdfOutput pointer.  
    ///
    VDF_API
    explicit VdfRequest(VdfMaskedOutputVector&& vector);

    /// Destructor.
    ///
    VDF_API
    ~VdfRequest();

    ////////////////////////////////////////////////////////////////////////////
    // Queries
    ////////////////////////////////////////////////////////////////////////////

    /// Returns true if the size of the vector is 0.
    ///
    VDF_API
    bool IsEmpty() const;

    /// Returns the size of the internally held vector.
    ///
    VDF_API
    size_t GetSize() const;

    /// Returns the network pointer of the first masked output.  It assumes
    /// that all the masked outputs are from the same network. If the request
    /// is empty, returns NULL.
    ///
    VDF_API
    const VdfNetwork* GetNetwork() const;

    ////////////////////////////////////////////////////////////////////////////
    // Iterators
    ////////////////////////////////////////////////////////////////////////////

    class const_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const VdfMaskedOutput;
        using reference = const VdfMaskedOutput &;
        using pointer = const VdfMaskedOutput *;
        using difference_type = std::ptrdiff_t;

        VDF_API
        const_iterator();

        reference operator*() const { return _Dereference(); }
        pointer operator->() const { return &(_Dereference()); }

        const_iterator &operator++() {
            _Increment();
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator r(*this);
            _Increment();
            return r;
        }

        bool operator==(const const_iterator& rhs) const {
            return _Equal(rhs);
        }

        bool operator!=(const const_iterator& rhs) const {
            return !_Equal(rhs);
        }

    private:
        friend class VdfRequest;

        VDF_API
        const_iterator(
            const VdfMaskedOutput* mo,
            const VdfMaskedOutput* first,
            const TfBits* bits);

        VDF_API
        void _Increment();

        bool _Equal(const const_iterator& rhs) const {
            return _mo == rhs._mo;
        }

        const VdfMaskedOutput& _Dereference() const {
            return *_mo;
        }

        VDF_API
        size_t _ComputeIndex() const;

        // Data members
        const VdfMaskedOutput *_mo;
        const VdfMaskedOutput *_firstMo;
        const TfBits *_bits;
    };

    /// Returns a vector iterator to the beginning of the internally held vector.
    ///
    VDF_API
    const_iterator begin() const;

    /// Returns a vector iterator to the end of the internally held vector.
    ///
    VDF_API
    const_iterator end() const;

    ////////////////////////////////////////////////////////////////////////////
    // Random access using integer indices
    ////////////////////////////////////////////////////////////////////////////

    class IndexedView
    {
    public:
        /// Construct an indexed view ontop of the \p request.
        ///
        IndexedView(const VdfRequest &request) : _r(&request) {}

        /// Returns the size of the indexed view.
        ///
        size_t GetSize() const {
            return _r->_request->size();
        }

        /// Returns a pointer to the element stored at index \p i or
        /// nullptr if the element stored at index \p i is removed from the
        /// request.
        ///
        const VdfMaskedOutput *Get(const size_t i) const {
            return !_r->_bits.GetSize() || _r->_bits.IsSet(i)
                ? &(*_r->_request)[i]
                : nullptr;
        }

    private:
        const VdfRequest *_r;
    };

    ////////////////////////////////////////////////////////////////////////////
    // Request subset operators
    ////////////////////////////////////////////////////////////////////////////

    /// Marks the element at the index of the VdfMaskedOutput that 
    /// \p iterator points to as added.
    ///
    VDF_API
    void Add(const const_iterator& iterator);

    /// Marks the element at the index of the VdfMaskedOutput that \p iterator
    /// points to as removed.
    ///
    VDF_API
    void Remove(const const_iterator& iterator);

    /// Marks all the elements in the request as being "added".  The bit set
    /// should be empty.
    ///
    VDF_API
    void AddAll();

    /// Marks all the elements in the request as being "removed".  The bit set
    /// should be the same size as the length of the vector, but all the bits
    /// should be unset.
    ///
    VDF_API
    void RemoveAll();

    /// Returns true if the internally held vector is the same, either by 
    /// pointing to the same vector or by containing the same contents.
    ///
    friend bool operator==(const VdfRequest& lhs, const VdfRequest& rhs)
    {
        return (lhs._request == rhs._request || 
            *lhs._request == *rhs._request) &&
            lhs._bits == rhs._bits;
    }

    /// Returns false if the internally held vector is the same, either by 
    /// pointing to the same vector or by containing the same contents.
    ///
    friend bool operator!=(const VdfRequest& lhs, const VdfRequest& rhs)
    {
        return !(lhs == rhs);
    }

    struct Hash {
        size_t operator()(const VdfRequest &request) const
        {
            size_t hash = TfBits::Hash()(request._bits);

            // Instead of hashing on the complete request we just do it on the
            // first three outputs (if any).
            const size_t num = std::min<size_t>(request.GetSize(), 3);

            const VdfMaskedOutputVector &outputs = *request._request;
            for(size_t i = 0; i < num; i++) {
                hash = TfHash::Combine(
                    hash, VdfMaskedOutput::Hash()(outputs[i]));
            }

            // Also add the last entry.
            if (request.GetSize() > 3) {
                hash = TfHash::Combine(
                    hash, VdfMaskedOutput::Hash()(outputs.back()));
            }

            return hash;
        }
    };

private:
    /// Marks the element at \p index in the request as being "added". The bit 
    /// set should be the same size as the length of the vector and the bit 
    /// at \p index should be set.
    ///
    void _Add(size_t index);

    /// Marks the element at \p index in the request as being "removed". The bit
    /// set should be the same size as the length of the vector and the bit at 
    /// \p index should be unset.
    ///
    void _Remove(size_t index);

private:
    // Internally held vector that is guaranteed to be sorted and uniqued.
    std::shared_ptr<const VdfMaskedOutputVector> _request;

    // Used for holding "subsets" without changing the vector.  
    // An empty bit set is a sentinel for a full vector (i.e. all the elements
    // in the internally held vector are part of the request). This bit set
    // should only ever be of size 0 or the size of the vector.
    //
    TfBits _bits;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
