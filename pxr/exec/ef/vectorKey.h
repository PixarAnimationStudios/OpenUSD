//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_VECTOR_KEY_H
#define PXR_EXEC_EF_VECTOR_KEY_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/ef/api.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/exec/vdf/vector.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class Ef_VectorKey
///
/// \brief This class wraps a VdfVector adding equality comparison and hashing
///        capabilities to the vector, without requiring all types stored in
///        VdfVector to implement the corresponding operators. Only the types
///        wrapped in Ef_VectorKey must provide these operators.
///
///        This is an abstract base class.
///
class EF_API_TYPE Ef_VectorKey
{
public:
    /// The type which must be used to store Ef_VectorKeys, for example as
    /// keys in a hash map.
    ///
    using StoredType = std::shared_ptr<Ef_VectorKey>;

    /// Non-copyable.
    ///
    Ef_VectorKey(const Ef_VectorKey&) = delete;
    Ef_VectorKey& operator=(const Ef_VectorKey&) = delete;

    /// Destructor.
    ///
    EF_API
    virtual ~Ef_VectorKey();

    /// Returns the wrapped VdfVector.
    ///
    const VdfVector &GetValue() const {
        return _value;
    }

    /// Generates a hash from the VdfVector.
    ///
    /// This must be implemented by the derived Ef_TypedVectorKey.
    ///
    virtual size_t CreateHash() const = 0;

    /// Equality compares this Ef_VectorKey with another one.
    ///
    /// This must be implemented by the derived Ef_TypedVectorKey.
    ///
    virtual bool IsEqual(const Ef_VectorKey &) const = 0;

    /// Equality operator.
    ///
    bool operator==(const Ef_VectorKey &rhs) const {
        return IsEqual(rhs);
    }

    bool operator!=(const Ef_VectorKey &rhs) const {
        return !(*this == rhs);
    }

    /// Hashing.
    ///
    template <class HashState>
    friend void TfHashAppend(HashState &h, const Ef_VectorKey::StoredType &v) {
        h.Append(v->CreateHash());
    }

    /// Equality compare functor.
    ///
    struct Equal {
        size_t operator()(const StoredType &lhs, const StoredType &rhs) const {
            return lhs->IsEqual(*rhs);
        }
    };

    /// Type of a hash map with Ef_VectorKey as key.
    ///
    template < typename T >
    struct Map {
        using Type = TfHashMap<
            Ef_VectorKey::StoredType, T, TfHash, Ef_VectorKey::Equal>;
    };

protected:
    // Constructor.
    explicit Ef_VectorKey(const VdfVector &value) :
        _value(value)
    {}

    // The wrapped VdfVector.
    const VdfVector _value;

};


///////////////////////////////////////////////////////////////////////////////
///
/// \class Ef_TypedVectorKey
///
/// \brief The derived Ef_VectorKey type, which implements the methods for
///        generating hashes and equality comparing Ef_VectorKeys with wrapped
///        VdfVectors of type T.
///
template < typename T >
class Ef_TypedVectorKey final : public Ef_VectorKey
{
public:

    /// Constructor.
    ///
    explicit Ef_TypedVectorKey(const VdfVector &value) :
        Ef_VectorKey(value) 
    {}

    /// Non-copyable.
    ///
    Ef_TypedVectorKey(const Ef_TypedVectorKey&) = delete;
    Ef_TypedVectorKey &operator=(const Ef_TypedVectorKey&) = delete;

    /// Implementation of the method that generates a hash from the
    /// VdfVector holding data of type T.
    ///
    virtual size_t CreateHash() const {
        VdfVector::ReadAccessor<T> a = _value.GetReadAccessor<T>();
        size_t hash = TfHash::Combine(a.GetNumValues());
        for (size_t i = 0; i < a.GetNumValues(); ++i) {
            hash = TfHash::Combine(hash, a[i]);
        }
        return hash;
    }

    /// Implementation of the method that equality compares two Ef_VectorKeys
    /// of type T.
    ///
    /// Note, that if two Ef_VectorKeys do not hold the same type T, they
    /// will be considered unequal by design.
    ///
    virtual bool IsEqual(const Ef_VectorKey &rhs) const {
        if (const Ef_TypedVectorKey<T> *rhsDerived =
                dynamic_cast<const Ef_TypedVectorKey<T>*>(&rhs)) {

            // Get the accessor to this vector
            VdfVector::ReadAccessor<T> a = _value.GetReadAccessor<T>();

            // Get the accessor to the right-hand-side vector
            const VdfVector &rhsV = rhsDerived->_value;
            VdfVector::ReadAccessor<T> rhsA = rhsV.GetReadAccessor<T>();

            // Compare each stored element. If two vectors do not hold
            // the same number of elements, they are considered not equal.
            if (a.GetNumValues() == rhsA.GetNumValues()) {
                for (size_t i = 0; i < a.GetNumValues(); ++i) {
                    if (a[i] != rhsA[i]) {
                        return false;
                    }
                }
                return true;
            }
        }

        // The two Ef_VectorKeys do not hold the same type T or their sizes do
        // not match, so we consider them unequal.
        return false;
    }

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
