//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_VECTOR_H
#define PXR_EXEC_VDF_VECTOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/boxedContainerTraits.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/vectorAccessor.h"
#include "pxr/exec/vdf/vectorImpl_Boxed.h"
#include "pxr/exec/vdf/vectorImpl_Compressed.h"
#include "pxr/exec/vdf/vectorImpl_Contiguous.h"
#include "pxr/exec/vdf/vectorImpl_Empty.h"
#include "pxr/exec/vdf/vectorImpl_Shared.h"
#include "pxr/exec/vdf/vectorImpl_Single.h"

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/safeTypeCompare.h"
#include "pxr/base/vt/value.h"

#include <iosfwd>
#include <new>
#include <type_traits>
#include <typeinfo>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

template <typename T>
class Vdf_VectorSubrangeAccessor;

/// \class VdfVector
///
/// This class is used to abstract away knowledge of the cache data used for
/// each node.
///
/// Note that data can be put into a VdfVector only atomically, no incremental
/// adding of elements is possible.
///
/// Note that Vdf requires the availability of the default and copy constructor
/// for the given template parameter TYPE. Additional Vdf provides default
/// fallback values via the VdfExecutionTypeRegistry. That is to give types the
/// ability to have empty default constructors (for speed) but at the same time
/// have well definied values to use in case we need to provide a "default".
///
class VdfVector
{
public:

    /// Copy constructor.
    ///
    VdfVector(const VdfVector &rhs)
    {
        // We need to new an empty impl because Clone() always expects
        // a valid _data to clone into.
        rhs._data.Get()->NewEmpty(0, &_data);
        rhs._data.Get()->Clone(&_data);
    }

    /// Copy constructor with subset copying.
    ///
    VdfVector(
        const VdfVector &rhs,
        const VdfMask &mask)
    {
        // CloneSubset expects valid _data to copy into, so first create
        // an empty vector.
        rhs._data.Get()->NewEmpty(0, &_data);

        // If the mask is all ones, take advange of the potentially faster 
        // Clone() method.
        if (mask.IsAllOnes()) {
            rhs._data.Get()->Clone(&_data);
        } else if (mask.IsAnySet()) {
            rhs._data.Get()->CloneSubset(mask, &_data);
        }
    }

    /// Copy constructor with boxing.
    ///
    enum ConstructBoxedCopyTag {
        ConstructBoxedCopy
    };

    VdfVector(
        const VdfVector &rhs,
        const VdfMask &mask,
        ConstructBoxedCopyTag)
    {
        if (rhs.GetSize() != mask.GetSize()) {
            TF_CODING_ERROR(
                "size mismatch: rhs.GetSize() (%zu) != mask.GetSize() (%zu)",
                rhs.GetSize(), mask.GetSize());
        }

        // Box expects valid _data to copy into, so first create an
        // empty vector.
        rhs._data.Get()->NewEmpty(0, &_data);
        if (mask.IsAnySet()) {
            rhs._data.Get()->Box(mask.GetBits(), &_data);
        }
    }

    /// Construct a vector with the same element type as \p rhs and of size
    /// \p size. All elements in this new vector are default constructed.
    ///
    VdfVector(
        const VdfVector &rhs,
        size_t size)
    {
        if (size == 0) {
            rhs._data.Get()->NewEmpty(0, &_data);
        } else if (size == 1){
            rhs._data.Get()->NewSingle(&_data);
        } else{
            rhs._data.Get()->NewDense(size, &_data);
        }
    }

    /// Move constructor.
    ///
    VdfVector(VdfVector &&rhs)
    {
        rhs._data.Get()->NewEmpty(0, &_data);
        rhs._data.Get()->MoveInto(&_data);
    }

    /// Destructor.
    ///
    ~VdfVector()
    {
        _data.Destroy();
    }

    /// Returns the number of elements held in this vector.
    ///
    size_t GetSize() const { return _data.Get()->GetSize(); }

    /// Returns whether or not this vector is empty.
    /// 
    bool IsEmpty() const { return GetSize() == 0; }

    /// Returns the number of elements for which this vector has storage.
    ///
    size_t GetNumStoredElements() const
    {
        return _data.Get()->GetNumStoredElements();
    }

    /// Forwards \p data into the vector.
    ///
    template<typename TYPE>
    void Set(TYPE &&data)
    {
        // We need to decay because TYPE is deduced as T& when l-values are
        // passed and T may also be cv-qualified.  This decayed type is what
        // is held by the underlying vector impl that provides storage.
        using T = typename std::decay<TYPE>::type;

        _CheckType<T>();
        _data.Destroy();
        _data.New<Vdf_VectorImplSingle<T>>(std::forward<TYPE>(data));
    }

    /// Copy boxed values into the vector.
    ///
    template <typename TYPE>
    void Set(const Vdf_BoxedContainer<TYPE> &data)
    {
        _CheckType<TYPE>();
        _data.Destroy();
        _data.New<Vdf_VectorImplBoxed<TYPE>>(data);
    }
    // Unfortunately, we have to provide overloads for all combinations of
    // const-qualification & reference category due to the unconstrained Set
    // overload.  Everything other than non-const rvalue reference gets
    // copied.
    //
    template <typename TYPE>
    void Set(Vdf_BoxedContainer<TYPE> &data)
    {
        const Vdf_BoxedContainer<TYPE> &cdata = data;
        this->Set(cdata);
    }
    template <typename TYPE>
    void Set(const Vdf_BoxedContainer<TYPE> &&data)
    {
        const Vdf_BoxedContainer<TYPE> &cdata = data;
        this->Set(cdata);
    }

    /// Move boxed values into the vector.
    ///
    template <typename TYPE>
    void Set(Vdf_BoxedContainer<TYPE> &&data)
    {
        _CheckType<TYPE>();
        _data.Destroy();
        _data.New<Vdf_VectorImplBoxed<TYPE>>(std::move(data));
    }

    /// Allocates space for \p size number of elements.
    ///
    /// The vector will be initialized with the default ctor. Note that if this 
    /// doesn't do anything meaningful (cf. Gf types), memory will be left
    /// uninitialized.
    ///
    template<typename TYPE>
    void Resize(size_t size)
    {
        _CheckType<TYPE>();

        _data.Destroy();

        // Note that we never construct a compressed vector impl, here. The
        // purpose of this function is to resize the vector to be able to
        // accommodate all the data denoted in \p mask, but we do not support
        // merging data into a compressed vector without first uncompressing.

        if (size == 0)
            _data.New< Vdf_VectorImplEmpty<TYPE> >(0);
        else if (size == 1)
            _data.New< Vdf_VectorImplSingle<TYPE> >();
        else
            _data.New< Vdf_VectorImplContiguous<TYPE> >(size);
    }

    /// Allocates space for the elements denoted by \p bits.
    ///
    /// The vector will be initialized with the default ctor. Note that if this 
    /// doesn't do anything meaningful (cf. Gf types), memory will be left
    /// uninitialized.
    ///
    template<typename TYPE>
    void Resize(const VdfMask::Bits &bits)
    {
        _CheckType<TYPE>();

        _data.Destroy();

        // Note that we never construct a compressed vector impl, here. The
        // purpose of this function is to resize the vector to be able to
        // accommodate all the data denoted in \p bits, but we do not support
        // merging data into a compressed vector without first uncompressing.

        const size_t size = bits.GetSize();

        if (size == 0)
            _data.New< Vdf_VectorImplEmpty<TYPE> >(0);
        else if (size == 1)
            _data.New< Vdf_VectorImplSingle<TYPE> >();
        else if (bits.AreAllUnset())
            _data.New< Vdf_VectorImplEmpty<TYPE> >(size);
        else
            _data.New< Vdf_VectorImplContiguous<TYPE> >(bits);
    }

    /// Copies the contents of \p rhs into this vector.
    ///
    /// \p rhs and this vector must be type compatible.
    ///
    /// Use this instead of operator= when you want to take advantage of
    /// only copying the elements set in \p mask from the \p rhs vector.
    ///
    void Copy(const VdfVector &rhs, const VdfMask &mask)
    {
        _CheckType(rhs);

        // Need to detach local data before copying into it if we are shared.
        if (ARCH_UNLIKELY(_data.Get()->GetInfo().ownership ==
            Vdf_VectorData::Info::Ownership::Shared)) {
            Vdf_VectorImplShared::Detach(&_data);
        }

        // If the mask is all ones, take advange of the potentially faster 
        // Clone() method.
        if (mask.IsAllOnes()) {
            rhs._data.Get()->Clone(&_data);
        }

        else if (mask.IsAnySet()) {
            rhs._data.Get()->CloneSubset(mask, &_data);
        } 
        
        // If the mask is all zeros, create an empty vector instead of
        // duplicating the rhs vector's implementation with an empty data
        // section. For compressed vectors, for example, this would cause
        // problems, because the index mapping would remain uninitialized,
        // essentially leaving the implementation in a broken state.
        else {
            _data.Destroy();
            rhs._data.Get()->NewEmpty(mask.GetSize(), &_data);
        }
    }

    /// Merges the contents of \p rhs into this vector. The elements copied
    /// from \p rhs are determined by the \p mask.
    ///
    /// \p rhs and this vector must be type compatible. Also note, that this
    /// vector (the destination vector), must NOT be a compressed vector.
    ///
    VDF_API
    void Merge(const VdfVector &rhs, const VdfMask::Bits &bits);

    /// Same as Merge(), but takes a VdfMask instead of a bitset.
    ///
    void Merge(const VdfVector &rhs, const VdfMask &mask) {
        Merge(rhs, mask.GetBits());
    }

    /// Embeds the current vector's existing implementaion into a reference 
    /// counted implementaion so that the data can be shared without copying.
    /// Mutating the contents of the data holder once shared will cause
    /// detachment. Returns true if the sharing was successful.
    ///
    /// Note: This method is not thread safe.
    ///
    bool Share() const  
    {
        // Bail out if not sharable
        if (!_data.Get()->IsSharable()) {
            return false;
        }

        // Create the new shared impl in a temp DataHolder, _data is moved
        // into and held by a SharedSource.
        Vdf_VectorData::DataHolder tmp;
        tmp.New<Vdf_VectorImplShared>(&_data);

        // Move the new shared data into this vector's DataHolder.
        tmp.Get()->MoveInto(&_data);
        tmp.Destroy();

        return true;
    }

    /// Returns \c true if the vector has been shared.
    /// 
    /// Note: Only used in tests currently.
    ///
    bool IsShared() const
    {
        return _data.Get()->GetInfo().ownership == 
            Vdf_VectorData::Info::Ownership::Shared;
    }

    /// Returns \c true if the vector can be shared.
    ///
    bool IsSharable() const
    {
        return _data.Get()->IsSharable();
    }

    /// Extracts this vector's values into a VtArray<T>.
    ///
    /// If the data has been shared previously, no copying occurs.  Otherwise,
    /// the data is copied into a new VtArray.
    ///
    template <typename T>
    VtArray<T>
    ExtractAsVtArray(const size_t size, const int offset) const
    {
        Vdf_VectorData* data = _data.Get();
        const Vdf_VectorData::Info& info = data->GetInfo();

        if (ARCH_UNLIKELY(info.compressedIndexMapping)) {
            return _DecompressAsVtArray(
                reinterpret_cast<T *>(info.data),
                *info.compressedIndexMapping, size, offset);
        }

        // Need to get a typed pointer to the first element. The memory layout
        // depends on if the vector is boxed or not. This is what 
        // Vdf_VectorAccessor does under the hood to provide element access.
        T *access;
        if (info.layout == Vdf_VectorData::Info::Layout::Boxed) {
            using BoxedVectorType = Vdf_BoxedContainer<T>;
            access = reinterpret_cast<BoxedVectorType *>(info.data)->data();
        } else {
            access = reinterpret_cast<T *>(info.data) - info.first;
        }

        access += offset;

        return info.ownership == Vdf_VectorData::Info::Ownership::Shared
            ? VtArray<T>(data->GetSharedSource(), access, size)
            : VtArray<T>(access, access + size);
    }

    /// A read/write accessor for low-level access to the contents
    /// of the VdfVector.
    ///
    template < typename TYPE >
    class ReadWriteAccessor {
    public:

        /// Default constructor.
        ///
        ReadWriteAccessor() = default;

        /// Returns \c true of the vector is empty.
        ///
        bool IsEmpty() const { return _accessor.IsEmpty(); }

        /// Returns the size of the vector, i.e. the nu,ber of values it holds.
        ///
        size_t GetNumValues() const { return _accessor.GetNumValues(); }

        /// Returns \c true if this accessor is providing element-wise access
        /// into a boxed container.
        ///
        bool IsBoxed() const { return _accessor.IsBoxed(); }

        /// Returns a mutable reference to an element.
        ///
        TYPE &operator[](size_t i) const { return _accessor[i]; }

    private:

        // Only VdfVector is allowed to create instances of this class.
        friend class VdfVector;

        // The constructor used by VdfVector.
        ReadWriteAccessor(
            Vdf_VectorData *data,
            const Vdf_VectorData::Info &info) : 
            _accessor(data, info) 
        {}

        // The underlying accessor type.
        Vdf_VectorAccessor<TYPE> _accessor;
        
    };

    /// GetReadWriteAccessor() allows low level access to the content of the 
    /// VdfVector via the Vdf_VectorData base class. In order to get the handle 
    /// to that this method will do the correct type checking for you.
    ///
    template<typename TYPE>
    ReadWriteAccessor<TYPE> GetReadWriteAccessor() const
    {
        Vdf_VectorData::Info info = _data.Get()->GetInfo();

        if (ARCH_UNLIKELY(info.ownership == 
            Vdf_VectorData::Info::Ownership::Shared)) {

            Vdf_VectorImplShared::Detach(&_data);
        
            // Update the info after detaching.
            info = _data.Get()->GetInfo();
        }

        return ReadWriteAccessor<TYPE>(_data.Get(), info);
    }

    /// A read-only accessor for low-level acces to the contents of the 
    /// VdfVector.
    ///
    template <typename TYPE>
    class ReadAccessor {
    public:

        /// Default constructor.
        ///
        ReadAccessor() = default;

        /// Returns \c true if the vector is empty.
        ///
        bool IsEmpty() const { return _accessor.IsEmpty(); }

        /// Returns the size of the vector, i.e. the number of values it holds. 
        ///
        size_t GetNumValues() const { return _accessor.GetNumValues(); }

        /// Returns \c true if this accessor is providing element-wise access
        /// into a boxed container. 
        ///
        bool IsBoxed() const { return _accessor.IsBoxed(); }

        /// Returns a const reference to an element. 
        ///
        const TYPE& operator[](size_t i) const { return _accessor[i]; }

    private:

        // Only VdfVector is allowed to create instances of this class.
        friend class VdfVector;

        // The constructor used by VdfVector. 
        ReadAccessor(
            Vdf_VectorData* data,
            const Vdf_VectorData::Info& info): 
            _accessor(data, info)
        {}

        // The underlying accessor type. 
        Vdf_VectorAccessor<TYPE> _accessor;
    };

    /// GetReadAccessor() allows low level read-only access to the content of
    /// of the VdfVector via the Vdf_VectorData base class. In order to get the 
    /// handle to that, this method will do the correct type checking for you. 
    ///
    template <typename TYPE>
    ReadAccessor<TYPE> GetReadAccessor() const
    {
        return ReadAccessor<TYPE>(_data.Get(), _data.Get()->GetInfo());
    }

    /// Provide read-only access to the boxed subranges held by this vector.
    ///
    /// While this is a public method, only VdfSubrangeView can make use of
    /// the returned Vdf_VectorSubrangeAccessor.
    ///
    template <typename TYPE>
    Vdf_VectorSubrangeAccessor<TYPE> GetSubrangeAccessor() const
    {
        return Vdf_VectorSubrangeAccessor<TYPE>(
            _data.Get(), _data.Get()->GetInfo());
    }

    /// Checks if a vector holds a specific type.
    ///
    template<typename TYPE>
    bool Holds() const
    {
        static_assert(
            !Vdf_IsBoxedContainer<TYPE>,
            "VdfVector::Holds cannot check for boxed-ness");

        return TfSafeTypeCompare(_GetTypeInfo(), typeid(TYPE));
    }

    /// Copies the content of \p rhs into this vector. This is an expensive
    /// operation if rhs has not been shared.
    ///
    /// This method does runtime type checking to ensure that both vectors
    /// have compatible types.
    ///
    VdfVector &operator=(const VdfVector &rhs)
    {
        if (&rhs == this)
            return *this;

        _CheckType(rhs);

        rhs._data.Get()->Clone(&_data);

        return *this;
    }

    /// Moves the content of \p rhs into this vector.
    ///
    /// This method does runtime type checking to ensure that both vectors
    /// have compatible types.
    ///
    VdfVector &operator=(VdfVector &&rhs)
    {
        if (&rhs == this)
            return *this;

        _CheckType(rhs);

        rhs._data.Get()->MoveInto(&_data);

        return *this;
    }

    /// Returns the number of bytes necessary to store a single element of
    /// this VdfVector.
    ///
    /// Note that this method estimates the allocated memory, which may not be
    /// accurate if the held data is not a value type, or has fields that are
    /// not value types.
    ///
    size_t EstimateElementMemory() const
    {
        return _data.Get()->EstimateElementMemory();
    }

    /// An ostream-able object wrapping a VdfVector instance, as well as a mask
    /// indicating which elements in the wrapped vector should be streamed out.
    ///
    /// Note that this class only outputs meaningful information if the object
    /// held by the VdfVector implements the ostream operator. For types that
    /// do not support the ostream operator, the type name will be printed
    /// instead.
    ///
    class DebugPrintable {
    public:

        /// Ostream operator.
        ///
        VDF_API
        friend std::ostream &operator<<(std::ostream &, const DebugPrintable &);

    private:

        // Only VdfVector is allowed to create instances of this class.
        friend class VdfVector;

        // Constructor.
        DebugPrintable(const Vdf_VectorData *data, const VdfMask &mask) :
            _data(data), _mask(mask)
        {}

        // The vector to be printed.
        const Vdf_VectorData *_data;

        // The mask denoting which elements in the vector should be printed.
        const VdfMask _mask;

    };

    /// Returns an ostream-able object, which can be used to debug print the
    /// contents of this VdfVector, filtered by \p mask.
    ///
    DebugPrintable GetDebugPrintable(const VdfMask &mask) const
    {
        return DebugPrintable(_data.Get(), mask);
    }

// -----------------------------------------------------------------------------

protected:

    // Constructs an empty VdfVector.  Note that publicly we're only allowed
    // to create a VdfTypedVector.  See also
    // VdfExecutionTypeRegistry::CreateEmptyVector(const TfType&).
    //
    VdfVector()
    {
        // We rely on VdfTypedVector to make an empty data of the correct
        // type for the default construction case.
    }

private:

    // Helper for ExtractAsVtArray.
    //
    // Decompresses the contents of a compressed vector into a VtArray.
    // Compressed vectors are always copied because they're never sharable.
    template <typename T>
    static VtArray<T> _DecompressAsVtArray(
        const T* const access,
        const Vdf_CompressedIndexMapping &indexMapping,
        const size_t size,
        const int offset) {

        VtArray<T> array;

        // This is not a general purpose compressed vector copy.  VtArray
        // extraction requests a contiguous range of logical indices and we
        // assume that this cannot span multiple blocks of data.
        if (size_t dataIdx; _ComputeCompressedExtractionIndex(
                                indexMapping, size, offset, &dataIdx)) {
            const T* const src = access + dataIdx;
            array.assign(src, src+size);
        }
        return array;
    }

    // Computes the data index into a compressed vector impl for the logical
    // offset.
    //
    // Returns true if (offset, size) is contained in a single block of data
    // and a valid index was written to *dataIdx.  Otherwise, returns false.
    VDF_API
    static bool _ComputeCompressedExtractionIndex(
        const Vdf_CompressedIndexMapping &indexMapping,
        size_t size,
        int offset,
        size_t *dataIdx);

    // Helper function that delivers an error message when type checking fails.
    VDF_API
    static void _PostTypeError(
        const std::type_info &thisTypeInfo,
        const std::type_info &otherTypeInfo);

    void _CheckType(
        const std::type_info &otherTypeInfo) const
    {
        const std::type_info &thisTypeInfo = _GetTypeInfo();
        if (!TfSafeTypeCompare(thisTypeInfo, otherTypeInfo)) {
            _PostTypeError(thisTypeInfo, otherTypeInfo);
        }
    }

    void _CheckType(const VdfVector &rhs) const
    {
        _CheckType(rhs._GetTypeInfo());
    }

    template <typename TYPE>
    void _CheckType() const
    {
        _CheckType(typeid(TYPE));
    }

    const std::type_info &_GetTypeInfo() const
    {
        return _data.Get()->GetTypeInfo();
    }

protected:

    // Holder of the actual implementation of Vdf_VectorData that holds this 
    // vector's data.  This is protected so that it can be initialized from
    // our only derived class VdfTypedVector.
    mutable Vdf_VectorData::DataHolder _data;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
