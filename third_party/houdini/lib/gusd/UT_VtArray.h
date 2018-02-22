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
#ifndef _GUSD_UT_VTARRAY_H_
#define _GUSD_UT_VTARRAY_H_

#include <SYS/SYS_Types.h>
#include <UT/UT_Assert.h>

#include <pxr/pxr.h>
#include "pxr/base/vt/array.h"

PXR_NAMESPACE_OPEN_SCOPE

/** Read-only wrapper to assist in read operations on a VtArray.

    This container is not meant to be an owner of the VtArray --
    only a temporary structure used when iterating over the array.
    Hence, it retains a reference to the array, rather than holding
    a shared pointer. The array must remain in memory while the
    container is in use.

    Since this container is read-only on the referenced array,
    it is guaranteed that none of its method will trigger copying
    of that data.*/
template <class T>
class GusdUT_VtArrayRO
{
public:
    typedef T           ValueType;
    typedef VtArray<T>  ArrayType;

    typedef typename ArrayType::const_iterator          const_iterator;
    typedef typename ArrayType::const_reverse_iterator  const_reverse_iterator;

    GusdUT_VtArrayRO(const ArrayType& array)
        : _array(array), _size(array.size()), _data(array.cdata())
        { UT_ASSERT(_size == 0 || _data != NULL); }

    const T&                operator()(exint i) const
                            {
                                UT_ASSERT_P(i >= 0 && i < _size);
                                return _data[i];
                            }

    const ArrayType&        operator*() const   { return _array; }

    const ArrayType&        array() const       { return _array; }

    const T*                data() const        { return _data; }
    
    exint                   size() const        { return _size; }

    const_iterator          begin() const   { return _array.begin(); }
    const_iterator          end() const     { return _array.end(); }
    const_reverse_iterator  rbegin() const  { return _array.rbegin(); }
    const_reverse_iterator  rend() const    { return _array.rend(); }

private:
    const ArrayType&    _array;
    const T*            _data;  /*! Cached as an optimization.*/
    exint               _size;
};


/** Read-write wrapper on a VtArray.

    As with GusdUT_VtArrayRW, this does not own the VtArray, but rather
    is a general tool for editing one, and so retains a reference to
    the underyling array. The array must remain in memory while the
    container is in use. */
template <class T>
class GusdUT_VtArrayRW
{
public:
    typedef T           ValueType;
    typedef VtArray<T>  ArrayType;

    typedef typename ArrayType::iterator                iterator;
    typedef typename ArrayType::reverse_iterator        reverse_iterator;
    typedef typename ArrayType::const_iterator          const_iterator;
    typedef typename ArrayType::const_reverse_iterator  const_reverse_iterator;

    GusdUT_VtArrayRW(const ArrayType& array) : _array(array)
        { update(); }

    const T&                operator()(exint i) const
                            {
                                UT_ASSERT_P(i >= 0 && i < _size);
                                UT_ASSERT_P(_size == _array.size());
                                return _data[i];
                            }

    T&                      operator()(exint i)
                            {
                                UT_ASSERT_P(i >= 0 && i < _size);
                                UT_ASSERT_P(_size == _array.size());
                                return _data[i];
                            }
    
    /** Access to the underlying array.
        @warning If you modify the arrays externally, you should    
        call update() before attempting to access the array again
        through this container. 
        @{ */
    const ArrayType&        operator*() const   { return _array; }  
    ArrayType&              operator*()         { return _array; }

    const ArrayType&        array() const       { return _array; }
    ArrayType&              array()             { return _array; }
    /** @} */
                        
    const T*                data() const        { return _data; }
    T*                      data()              { return _data; }

    exint                   size() const        { return _size; }
    
    void                    resize(exint size)
                            {
                                UT_ASSERT(size >= 0);
                                _array.resize(size);
                                update();
                            }

    void                    reserve(exint size)
                            {
                                UT_ASSERT(size >= 0);
                                _array.reserve(size);
                                update();
                            }

    void                    swap(ArrayType& array)
                            {
                                _array.swap(array);
                                update();
                            }

    /** Update the state of the array to reflect changes
        in the underlying array (eg., change in size).*/
    void                    update()
                            {
                                _data = _array.data();
                                _size = _array.size();
                                UT_ASSSERT(_size == 0 || _data != NULL);
                            }

    const_iterator          begin() const   { return _array.begin(); }
    const_iterator          end() const     { return _array.end(); }
    iterator                begin()         { return _array.begin(); }
    iterator                end()           { return _array.end(); }

    const_reverse_iterator  rbegin() const  { return _array.rbegin(); }
    const_reverse_iterator  rend() const    { return _array.rend(); }
    reverse_iterator        rbegin()        { return _array.rbegin(); }
    reverse_iterator        rend()          { return _array.rend(); }

private:
    ArrayType&  _array;
    T*          _data;  /*! Cache as an optimization
                            (avoid referencing checks on every value lookup)*/
    exint       _size;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_UT_VTARRAY_H_*/
