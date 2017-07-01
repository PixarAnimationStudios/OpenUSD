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
#ifndef _GUSD_GT_VTSTRINGARRAY_H_
#define _GUSD_GT_VTSTRINGARRAY_H_


#include <GT/GT_DataArray.h>

#include <pxr/pxr.h>
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

/** GT_DataArray implementation wrapping VtArray for
    string-like types. I.e., std::string, TfToken, et. al.

    @warning: The GT_String (const char*) pointer returned for all empty
    strings is always the NULL pointer. This includes std::string, which
    can't be constructed from a NULL string. Be careful when reconstring
    source objects! */
template <class T>
class GusdGT_VtStringArray : public GT_DataArray
{
public:
    typedef GusdGT_VtStringArray<T> This;
    typedef T                        ValueType;
    typedef VtArray<T>               ArrayType;


    GusdGT_VtStringArray();
    GusdGT_VtStringArray(const ArrayType& array);

    virtual ~GusdGT_VtStringArray() {}

    virtual const char* className() const   { return "GusdGT_VtStringArray"; }

    const T&            operator()(GT_Offset o) const
                        {
                            UT_ASSERT_P(o >= 0 && o <= _size);
                            return _data[0];
                        }

    const ArrayType&    operator*() const   { return _array; }

    /* Non-virtual string accessor.*/
    GT_String           getString(GT_Offset o) const
                        { return _GetString((*this)(o)); }

    /** Swap our array contents with another array.*/
    void                swap(ArrayType& o);

    virtual GT_DataArrayHandle  harden() const;
    
    virtual GT_String   getS(GT_Offset o, int idx=0) const
                        { return getString(o); }
                            
    /** Indexed strings are not currently supported in Vt.*/
    virtual GT_Size     getStringIndexCount() const             { return -1; }
    virtual GT_Offset   getStringIndex(GT_Offset, int) const    { return -1; }
    virtual void        getIndexedStrings(UT_StringArray&,
                                          UT_IntArray&) const {}

    virtual GT_Storage  getStorage() const      { return GT_STORE_STRING; }
    virtual GT_Size     getTupleSize() const    { return 1; }
    virtual GT_Size     entries() const         { return _size; }
    virtual GT_Type     getTypeInfo() const     { return GT_TYPE_NONE; }
    virtual int64       getMemoryUsage() const
                        { return sizeof(*this) + sizeof(T)*_size; }

protected:
    /** Update our @a _data member to point at the array data.
        This must be called after any operation that changes
        the contents of @a _array.*/
    void                _UpdateDataPointer(bool makeUnique); 

    /** Return a GT_String from one of our elems.
        This must be specialized per element type.*/
    GT_String           _GetString(const T& o) const;

    GT_String           _GetStringFromStdString(const std::string& o) const
                        { return o.empty() ? NULL : o.c_str(); }

private:
    /** No numeric accessors supported.*/
    virtual uint8       getU8(GT_Offset, int idx) const     { return 0; }
    virtual int32       getI32(GT_Offset, int idx) const    { return 0; }
    virtual fpreal32    getF32(GT_Offset, int idx) const    { return 0; }

private:
    ArrayType   _array;
    GT_Size     _size;
    const T*    _data;  /*! Raw pointer to the underlying data.
                            This is held in order to avoid referencing
                            checks on every array lookup.*/
};


template <class T>
GusdGT_VtStringArray<T>::GusdGT_VtStringArray(const ArrayType& array)
    :  _array(array), _size(array.size())
{
    _UpdateDataPointer(false);
}


template <class T>
GusdGT_VtStringArray<T>::GusdGT_VtStringArray()
    : _size(0), _data(NULL)
{}


template <class T>
void
GusdGT_VtStringArray<T>::_UpdateDataPointer(bool makeUnique)
{
    /* Access a non-const pointer to make the array unique.*/
    _data = makeUnique ? _array.data() : _array.cdata();
    UT_ASSERT(_size == 0 || _data != NULL);
}


template <class T>
void
GusdGT_VtStringArray<T>::swap(ArrayType& o)
{
    _array.swap(o);
    _size = _array.size();
    _UpdateDataPointer(false);
}


template <class T>
GT_DataArrayHandle
GusdGT_VtStringArray<T>::harden() const
{
    This* copy = new This(_array);
    copy->_UpdateDataPointer(true);
    return GT_DataArrayHandle(copy);
}


template <>
GT_String
GusdGT_VtStringArray<std::string>::_GetString(const std::string& o) const
{ return _GetStringFromStdString(o); }


template <>
GT_String
GusdGT_VtStringArray<TfToken>::_GetString(const TfToken& o) const
{ return _GetStringFromStdString(o.GetString()); }


template <>
GT_String
GusdGT_VtStringArray<SdfPath>::_GetString(const SdfPath& o) const
{ return _GetStringFromStdString(o.GetString()); }


template <>
GT_String
GusdGT_VtStringArray<SdfAssetPath>::_GetString(const SdfAssetPath& o) const
{ return _GetStringFromStdString(o.GetAssetPath()); }



template <>
int64
GusdGT_VtStringArray<std::string>::getMemoryUsage() const
{
    int64 sz = sizeof(*this) + sizeof(ValueType)*_size;
    for(GT_Size i = 0; i < _size; ++i)
        sz += _data[i].size()*sizeof(char);
    return sz;
}


template <>
int64
GusdGT_VtStringArray<SdfAssetPath>::getMemoryUsage() const
{
    int64 sz = sizeof(*this) + sizeof(ValueType)*_size;
    for(GT_Size i = 0; i < _size; ++i) {
        const SdfAssetPath& p = _data[i];
        sz += (p.GetAssetPath().size() +
               p.GetResolvedPath().size())*sizeof(char);
    }
    return sz;
}

/** TODO: Specialization of getMemoryUsage for SdPath? */


typedef GusdGT_VtStringArray<std::string>   GusdGT_VtStdStringArray;
typedef GusdGT_VtStringArray<TfToken>       GusdGT_VtTokenArray;
typedef GusdGT_VtStringArray<SdfPath>       GusdGT_VtPathArray;
typedef GusdGT_VtStringArray<SdfAssetPath>  GusdGT_VtAssetPathArray;

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_GT_VTSTRINGARRAY_H_*/
