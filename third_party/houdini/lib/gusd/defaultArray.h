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
#ifndef _GUSD_DEFAULTARRAY_H_
#define _GUSD_DEFAULTARRAY_H_

#include <pxr/pxr.h>

#include <UT/UT_Array.h>


PXR_NAMESPACE_OPEN_SCOPE


/// Simple array wrapper, providing an array that may either hold a
/// single constant value, or an array of values.
template <typename T>
class GusdDefaultArray
{
public:
    using value_type = T;
    using array_type = UT_Array<T>;

    GusdDefaultArray()
        {
            if(SYSisPOD<T>())
                memset((void*)&_default, 0, sizeof(T));
        }

    GusdDefaultArray(const T& defaultVal)
        : _default(defaultVal) {}

    exint               size() const
                        { return _array.size(); }
    
    void                Clear()
                        { return _array.clear(); }

    bool                IsConstant() const          { return size() == 0; }

    bool                IsVarying() const           { return size() > 0; }

    T&                  GetDefault()                { return _default; }
    const T&            GetDefault() const          { return _default; }

    void                SetDefault(const T& val)    {_default = val; }

    /// Turn this into a constant array, with value @a val.
    void                SetConstant(const T& val)
                        {
                            Clear();
                            SetDefault(val);
                        }

    T&                  operator()(exint i)
                        { return IsVarying() ? _array(i) : GetDefault(); }

    const T&            operator()(exint i) const
                        { return IsVarying() ? _array(i) : GetDefault(); }

    array_type&         GetArray()          { return _array; }

    const array_type&   GetArray() const    { return _array; }
    
private:
    array_type  _array;
    T           _default;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif /*_GUSD_DEFAULTARRAY_H_*/
