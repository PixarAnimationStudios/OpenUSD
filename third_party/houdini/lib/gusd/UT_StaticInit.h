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
#ifndef _GUSD_UT_STATICINIT_H_
#define _GUSD_UT_STATICINIT_H_

#include <pxr/pxr.h>

#include <UT/UT_Lock.h>

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/** Helper for creating a static value, whose construction
    is deferred and backed by a lock.
    This is similar to UT_SingletonWithLock, except that the value held
    is the result of calling method, rather than constructing an object.

    Example usage:
    
    @code
    T*  someFn();
    static auto staticVal(GusdUT_StaticVal(someFn));

    // Function is not exec'd until accessed.
    // To access:
    T* val = *staticVal;
    // Or:
    staticVal->method();
    @endcode */
template <typename Fn>
struct GusdUT_StaticValHolder
{
public:
    typedef GusdUT_StaticValHolder<Fn> This;
    using T = typename std::result_of<const Fn& ()>::type;

    GusdUT_StaticValHolder(const Fn& fn)
        : _val(NULL), _fn(fn), _lock() {}

    GusdUT_StaticValHolder(This&& o) noexcept
        : _fn(o._fn)
        {
            if(_val) delete _val;
            _val = o._val;
        }

    ~GusdUT_StaticValHolder()
        { if(_val) delete _val; }

    T&  operator*()    { return *get(); }
    T*  operator->()   { return get(); }

    T*  get()
        {
             if(!_val) {
                 UT_AutoLock lock(_lock);
                 if(!_val) {
                     _val = new T();
                     *_val = _fn();
                 }
             }
             return _val;
        }

private:
    const Fn&   _fn;
    UT_Lock     _lock;
    T*          _val;
};

/** Helper for constructing static values.
    This allows automatic template deduction of the function.*/
template <typename Fn>
GusdUT_StaticValHolder<Fn>
GusdUT_StaticVal(const Fn& fn)
{
    return GusdUT_StaticValHolder<Fn>(fn);
}

PXR_NAMESPACE_CLOSE_SCOPE


#endif /*_GUSD_UT_STATICINIT_H_*/
