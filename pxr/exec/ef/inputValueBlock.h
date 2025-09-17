//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_INPUT_VALUE_BLOCK_H
#define PXR_EXEC_EF_INPUT_VALUE_BLOCK_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/ef/api.h"

#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/maskedOutputVector.h"
#include "pxr/exec/vdf/typedVector.h"

#include <vector>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// Forward declarations
class VdfExecutorInterface;

///////////////////////////////////////////////////////////////////////////////
///
/// \class EfInputValueBlock
///
/// \brief An input value block is a vector of (output, value) pairs, each
/// of which will be used to initialize a network before execution.
///
class EfInputValueBlock
{

private:

    // The type representing the vector of (output, value) pairs.
    typedef std::vector< std::pair<VdfMaskedOutput, VdfVector *> > _VectorType;

public:

    /// Constructs an empty input value block.
    ///
    EfInputValueBlock() = default;

    /// Copy constructor.
    ///
    EF_API
    EfInputValueBlock(const EfInputValueBlock &rhs);

    /// Move constructor.
    ///
    EfInputValueBlock(EfInputValueBlock &&rhs) = default;

    /// Destructor.
    ///
    EF_API
    ~EfInputValueBlock();

    /// Copy assignment operator.
    ///
    EF_API
    EfInputValueBlock &operator=(const EfInputValueBlock &rhs);

    /// Move assignment operator.
    ///
    EfInputValueBlock &operator=(EfInputValueBlock &&rhs) = default;

    /// Adds an (output, value) pair to this block.
    ///
    /// Note that this API currently only supports single valued outputs and
    /// does not yet support vectorized outputs.
    ///
    template <typename T>
    void AddOutputValuePair(const VdfMaskedOutput &output, const T &value)
    {
        _values.push_back(std::make_pair( output, new VdfTypedVector<T>()));
        _values.back().second->Set(value);
    }

    /// Adds an (output, VdfVector) pair to this block.
    ///
    void AddOutputVectorPair(const VdfMaskedOutput &output, 
                             const VdfVector &value)
    {
        _values.push_back(std::make_pair( output, new VdfVector(value) ) );
    }

    /// Applies the input value block to an executor, by setting the output
    /// values and pushing through invalidation for each one of the output
    /// values set.
    ///
    /// \p invalidationRequest is an output parameter, which will return the
    /// request used for invalidation.
    ///
    EF_API
    void Apply(VdfExecutorInterface *executor, 
               VdfMaskedOutputVector *invalidationRequest = NULL) const;

    /// Pushes invalidation into the \p executor using the supplied
    /// \p invalidationRequest. Contrary to the Apply method, this method does
    /// not infer the invalidation request from the set input values. Instead,
    /// the \p invalidationRequest may be specified by the caller.
    ///    
    EF_API
    void InvalidateAndApply(VdfExecutorInterface *executor,
                            const VdfMaskedOutputVector &invalidationRequest) const;

    /// A const iterator into a block vector.
    ///
    typedef _VectorType::const_iterator const_iterator;

    /// Returns a const_iterator to the beginning of the values vector.
    ///
    const_iterator begin() const {
        return _values.begin();
    }

    /// Returns a const_iterator to the end of the values vector.
    ///
    const_iterator end() const {
        return _values.end();
    }

    /// Returns the number of outputs in this block.
    ///
    size_t GetSize() const {
        return _values.size();
    }

private:

    // Push invalidation on to the executor.
    //
    void _Invalidate(VdfExecutorInterface *executor,
                     const VdfMaskedOutputVector &invalidationRequest) const;

    // Sets the output values on the executor.
    //
    void _SetValues(VdfExecutorInterface *executor) const;

    // Clears the input value block.
    //
    void _Clear();

    // Appends the contents of \p to this object.
    //
    void _Append(const EfInputValueBlock &rhs);

private:

    // The value pairs that make up this block.
    _VectorType _values;

};

/// A vector of EfInputValueBlocks.
///
typedef std::vector< EfInputValueBlock > EfInputValueBlockVector;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
