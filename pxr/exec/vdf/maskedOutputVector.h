//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_MASKED_OUTPUT_VECTOR_H
#define PXR_EXEC_VDF_MASKED_OUTPUT_VECTOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/hash.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfNetwork;

/// A vector of VdfMaskedOutputs.
///
typedef std::vector<VdfMaskedOutput> VdfMaskedOutputVector;

/// Hashing functor for VdfMaskedOutputVectors.
///
struct VdfMaskedOutputVector_Hash {
    size_t operator()(const VdfMaskedOutputVector& vector) const 
    {
        size_t hash = TfHash::Combine(vector.size());

        // Instead of hashing on the complete request we just do it on the
        // first three outputs (if any).
        size_t num = std::min<size_t>(vector.size(), 3);

        for(size_t i = 0; i < num; ++i) {
            hash = TfHash::Combine(hash, VdfMaskedOutput::Hash()(vector[i]));
        }

        // Also add the last entry.
        if (vector.size() > 3) {
            hash = TfHash::Combine(
                hash, VdfMaskedOutput::Hash()(vector.back()));
        }

        return hash;
    }
};

/// Sorts and uniques the given vector.
///
VDF_API
void VdfSortAndUniqueMaskedOutputVector(VdfMaskedOutputVector* vector);

/// Returns a pointer to the network if the vector is not empty.  Otherwise 
/// returns a nullptr.  This method assumes that all outputs in the vector
/// come from the same network.
///
VDF_API
const VdfNetwork* VdfGetMaskedOutputVectorNetwork(
    const VdfMaskedOutputVector& vector);

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXR_EXEC_VDF_MASKED_OUTPUT_VECTOR_H */
