//
// Copyright 2017 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef _GUSD_GT_VTARRAY_H_
#define _GUSD_GT_VTARRAY_H_


#include <GT/GT_DataArray.h>
#include <GT/GT_DANumeric.h>
#include <SYS/SYS_Compiler.h>
#include <SYS/SYS_Math.h>

#include <pxr/pxr.h>
#include "pxr/base/vt/array.h"

#include "gusd/UT_TypeTraits.h"
#include "gusd/GT_Utils.h"

PXR_NAMESPACE_OPEN_SCOPE

/** GT_DataArray implementation that wraps a VtArray.

    This allows, in some cases, for arrays read in from USD to be
    pushed into GT prims without having to incur copying.

    Example:

    @code
    VtArray<int> valsFromUSD;
    GT_DataArrayHandle hnd(new GusdGT_VtArray<int>(valsFromUSD));
    @endcode

    These arrays are designed to be read-only.
    If you need to make edits, use the following pattern:

    @code
    GusdGT_VtArray<int> srcData;
    // swap data into tmp array, modify.
    VtArray<int> tmp;
    srcData.swap(tmp);
    tmp[10] = 37;
    // swap data back into place.
    srcData.swap(tmp);
    @encode
    
    Note that this kind of swapping trick does *not* require the
    full array to be copied; only the internal references are swapped.*/
template <class T>
class GusdGT_VtArray : public GT_DataArray
{
public:
    SYS_STATIC_ASSERT(GUSDUT_IS_PODTUPLE(T));

    typedef GusdGT_VtArray<T>   This;
    typedef T                   ValueType;
    typedef VtArray<T>          ArrayType;

    typedef typename GusdUT_TypeTraits::PODTuple<T> PODTuple;
    typedef typename PODTuple::ValueType            PODType;
    
    static const int        tupleSize = PODTuple::tupleSize;
    static const GT_Storage storage   = GusdGT_Utils::StorageByType<PODType>::value;


    GusdGT_VtArray(const ArrayType& array, GT_Type type=GT_TYPE_NONE);
    GusdGT_VtArray(GT_Type type=GT_TYPE_NONE);

    virtual ~GusdGT_VtArray() {}

    virtual const char* className() const   { return "GusdGT_VtArray"; }
    
    const T&            operator()(GT_Offset o) const
                        {
                            UT_ASSERT_P(o >= 0 && o <= _size);
                            return reinterpret_cast<T*>(_data)[o];
                        }

    PODType             operator()(GT_Offset o, int idx) const
                        { return getT<PODType>(o, idx); }

    const ArrayType&    operator*() const   { return _array; }

    const PODType*      data() const        { return _data; }

    /** Swap our array contents with another array.*/
    void                swap(ArrayType& o);

    virtual GT_DataArrayHandle  harden() const;

    const PODType*      getData(GT_Offset o) const
                        {   
                            UT_ASSERT_P(o >= 0 && o <= _size);
                            return _data + o*tupleSize;
                        }

    /** Access to individual elements as given POD type.
        For performance, this is preferred to the virtual getXX() methods.*/
    template <typename PODT>
    PODT                getT(GT_Offset o, int idx=0) const;

    /** Get access to a raw array of data.
        If @a OTHERPODT is not the same as the array's underlying type,
        the raw array will be stored in the given @a buf.*/
    template <typename PODT>
    const PODT*         getArrayT(GT_DataArrayHandle& buf) const;

    /** Extract a tuple into @a dst.*/
    template <typename PODT>
    void                importT(GT_Offset o, PODT* dst, int tsize=-1) const;

    /** Extract data for entire array into @a dst.*/
    template <typename PODT>
    void                fillArrayT(PODT* dst, GT_Offset start, GT_Size length,
                                   int tsize=-1, int stride=-1) const;

    /** Extended form of array extraction that supports repeated elems.*/
    template <typename PODT>
    void                extendedFillT(PODT* dst, GT_Offset start,
                                      GT_Size length, int tsize=-1,
                                      int nrepeats=1, int stride=-1) const;

    virtual GT_Storage  getStorage() const      { return storage; }
    virtual GT_Size     getTupleSize() const    { return tupleSize; }
    virtual GT_Size     entries() const         { return _size; }
    virtual GT_Type     getTypeInfo() const     { return _type; }
    virtual int64       getMemoryUsage() const
                        { return sizeof(*this) + sizeof(T)*_size; }

    /** Type-specific virtual getters.*/

#define _DECL_GETTERS(suffix,podt)                                      \
public:                                                                 \
    virtual podt        SYS_CONCAT(get,suffix)(GT_Offset o, int idx=0) const\
                        { return getT<podt>(o, idx); }                  \
                                                                        \
    virtual const podt* SYS_CONCAT(get,SYS_CONCAT(suffix,Array))        \
                        (GT_DataArrayHandle& buf) const                 \
                        { return getArrayT<podt>(buf); }                \
                                                                        \
protected:                                                              \
    virtual void        doImport(GT_Offset o, podt* dst, GT_Size tsize) const\
                        { importT(o, dst, tsize); }                     \
                                                                        \
    virtual void        doFillArray(podt* dst, GT_Offset start, GT_Size length,\
                                    int tsize, int stride) const        \
                        { fillArrayT(dst, start, length, tsize, stride); }\
                                                                        \
    virtual void        extendedFill(podt* dst, GT_Offset start,        \
                                     GT_Size length, int tsize,         \
                                     int nrepeats, int stride=-1) const \
                        { extendedFillT(dst, start, length,             \
                                        tsize, nrepeats, stride); }

    _DECL_GETTERS(U8,  uint8);
    _DECL_GETTERS(I32, int32);
    _DECL_GETTERS(I64, int64);
    _DECL_GETTERS(F16, fpreal16);
    _DECL_GETTERS(F32, fpreal32);
    _DECL_GETTERS(F64, fpreal64);

#undef _DECL_GETTER

protected:
    /** Update our @a _data member to point at the array data.
        This must be called after any operation that changes
        the contents of @a _array.*/
    void                _UpdateDataPointer(bool makeUnique);

private:
    /* No string support. For strings, use GusdGT_VtStringArray.*/
    virtual GT_String   getS(GT_Offset, int) const              { return NULL; }
    virtual GT_Size     getStringIndexCount() const             { return -1; }
    virtual GT_Offset   getStringIndex(GT_Offset,int) const     { return -1; }
    virtual void        getIndexedStrings(UT_StringArray&,
                                          UT_IntArray&) const {}

protected:
    ArrayType       _array;
    const GT_Type   _type;
    GT_Size         _size;
    const PODType*  _data;  /*! Raw pointer to the underlying data.
                                Held separately as an optimization.*/
};


template <class T>
GusdGT_VtArray<T>::GusdGT_VtArray(const ArrayType& array, GT_Type type)
    :  _array(array), _type(type), _size(array.size())
{
    _UpdateDataPointer(false);
}


template <class T>
GusdGT_VtArray<T>::GusdGT_VtArray(GT_Type type)
    : _type(type), _size(0), _data(NULL)
{}


template <class T>
void
GusdGT_VtArray<T>::_UpdateDataPointer(bool makeUnique)
{
    /* Access a non-const pointer to make the array unique.*/
    _data = reinterpret_cast<const PODType*>(
        makeUnique ? _array.data() : _array.cdata());
    UT_ASSERT(_size == 0 || _data != NULL);
}


template <class T>
void
GusdGT_VtArray<T>::swap(ArrayType& o)
{
    _array.swap(o);
    _size = _array.GetSize();
    _UpdateDataPointer(false);
}


template <class T>
GT_DataArrayHandle
GusdGT_VtArray<T>::harden() const
{
    This* copy = new This(_array, _type);
    copy->_UpdateDataPointer(true);
    return GT_DataArrayHandle(copy);
}


template <class T>
template <typename PODT>
PODT
GusdGT_VtArray<T>::getT(GT_Offset o, int idx) const
{
    UT_ASSERT_P(o >= 0 && o < _size);
    UT_ASSERT_P(idx >= 0 && idx < tupleSize);
    return static_cast<PODT>(_data[tupleSize*o + idx]);
}


template <class T>
template <typename PODT>
const PODT*
GusdGT_VtArray<T>::getArrayT(GT_DataArrayHandle& buf) const
{
    if(SYS_IsSame<PODType,PODT>::value)
        return reinterpret_cast<const PODT*>(_data);

#if HDK_API_VERSION < 16050000
    typedef GT_DANumeric<PODT,
        GusdGT_Utils::StorageByType<PODT>::value> _GTArrayType;
#else
    typedef GT_DANumeric<PODT> _GTArrayType;
#endif
    
    _GTArrayType* tmpArray = new _GTArrayType(_size, tupleSize, _type);
    fillArrayT(tmpArray->data(), 0, _size, tupleSize);
    buf = tmpArray;
    return tmpArray->data();
}


template <class T>
template <typename PODT>
void
GusdGT_VtArray<T>::importT(GT_Offset o, PODT* dst, int tsize) const
{
    tsize = tsize < 1 ? tupleSize : SYSmin(tsize, tupleSize);
    const PODType* src = getData(o);
    if( src )
    {
        for(int i = 0; i < tsize; ++i)
            dst[i] = static_cast<PODT>(src[i]);
    }
}


template <class T>
template <typename PODT>
void
GusdGT_VtArray<T>::fillArrayT(PODT* dst, GT_Offset start, GT_Size length,
                              int tsize, int stride) const
{
    if(_size == 0)
        return;
    if(tsize < 1)
        tsize = tupleSize;
    /* Stride is >= the unclamped tuple size.
       This seems wrong, but is consistent with GT_DANumeric.*/
    stride = SYSmax(stride, tsize);
    tsize = SYSmin(tsize, tupleSize);
    if(SYS_IsSame<PODT,PODType>::value &&
       tsize == tupleSize && stride == tupleSize) {
        /* direct copy is safe */
        memcpy(dst, getData(start), length*tsize*sizeof(PODT));
    } else {
        const PODType* src = getData(start*tupleSize);
        for(GT_Offset i = 0; i < length; ++i, src += tupleSize, dst += stride) {
            for(int j = 0; j < tsize; ++j)
                dst[j] = static_cast<PODT>(src[j]);
        }
    }
}


template <class T>
template <typename PODT>
void
GusdGT_VtArray<T>::extendedFillT(PODT* dst, GT_Offset start,
                                 GT_Size length, int tsize,
                                 int nrepeats, int stride) const
{
    if(nrepeats == 1)
        return fillArrayT(dst, start, length, tsize, stride);
    if(_size == 0)
        return;
    if(tsize < 1)
        tsize = tupleSize;
    stride = SYSmax(stride, tsize);
    tsize = SYSmin(tsize, tupleSize);
    
    const PODType* src = getData(start*tupleSize);
    for(GT_Offset i = 0; i < length; ++i, src += tupleSize) {
        for(int r = 0; r < nrepeats; ++r, dst += stride) {
            for(int j = 0; j < tsize; ++j)
                dst[j] = static_cast<PODT>(src[j]);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_GT_VTARRAY_H_*/
