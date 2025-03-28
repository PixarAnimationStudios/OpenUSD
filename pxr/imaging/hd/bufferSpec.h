//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_BUFFER_SPEC_H
#define PXR_IMAGING_HD_BUFFER_SPEC_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


typedef std::vector<struct HdBufferSpec> HdBufferSpecVector;

/// \class HdBufferSpec
///
/// Describes each named resource of buffer array.
/// This specifies the buffer's value type as HdTupleType,
/// which specifies the value type, number of components, and
/// number of array entries (which may be 1).
///
/// for example:
/// HdBufferSpecVector
///    0: name = points,  tupleType = {HdTypeFloatVec3, 1}
///    1: name = normals, tupleType = {HdTypeFloatVec3, 1}
///    2: name = colors,  tupleType = {HdTypeFloatVec3, 1}
///
struct HdBufferSpec final {
    /// Constructor.
    HdBufferSpec(TfToken const &name, HdTupleType tupleType) :
        name(name), tupleType(tupleType) {}

    /// Util function for adding buffer specs of sources into bufferspecs.
    template<typename T>
    static void GetBufferSpecs(T const &sources,
                               HdBufferSpecVector *bufferSpecs) {
        for (auto const &src : sources) {
            if (src->IsValid()) {
                src->GetBufferSpecs(bufferSpecs);
            }
        }
    }

    /// Returns true if \p subset is a subset of \p superset.
    /// An empty set is considered a valid subset of the superset.
    HD_API
    static bool IsSubset(HdBufferSpecVector const &subset,
                         HdBufferSpecVector const &superset);

    /// Returns union set of \p spec1 and \p spec2. Duplicated entries are
    /// uniquified. The order of items in spec1 and spec2 are preserved relative
    /// to themselves respectively in the result, with items in spec1 appearing
    /// first.
    HD_API
    static HdBufferSpecVector ComputeUnion(HdBufferSpecVector const &spec1,
                                           HdBufferSpecVector const &spec2);
    
    /// Returns difference set of \p spec1 and \p spec2, i.e., entries in spec1
    /// that are not in spec2.  Duplicated entries are uniquified. The order of
    /// items in spec1 is preserved.  
    HD_API
    static HdBufferSpecVector ComputeDifference(HdBufferSpecVector const &spec1,
                                           HdBufferSpecVector const &spec2);

    /// Debug output.
    HD_API
    static void Dump(HdBufferSpecVector const &specs);

    /// Return a size_t hash for this spec.
    HD_API
    size_t Hash() const;

    /// Functor to use for unordered sets, maps.
    struct HashFunctor {
        size_t operator()(HdBufferSpec const& spec) const {
            return spec.Hash();
        }
    };

    /// Equality checks.
    bool operator == (HdBufferSpec const &other) const {
        return name == other.name && tupleType == other.tupleType;
    }
    bool operator != (HdBufferSpec const &other) const {
        return !(*this == other);
    }

    /// Ordering.
    bool operator < (HdBufferSpec const &other) const {
        return name < other.name || (name == other.name &&
            tupleType < other.tupleType);
    }

    TfToken name;
    HdTupleType tupleType;
};

// Support TfHash.
template <class HashState>
void
TfHashAppend(HashState &h, HdBufferSpec const &bs)
{
    h.Append(bs.name, bs.tupleType);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_BUFFER_SPEC_H
