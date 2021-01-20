//
// Copyright 2020 Pixar
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
#ifndef PXR_IMAGING_HGI_HANDLE_H
#define PXR_IMAGING_HGI_HANDLE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"

#include <stdint.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiHandle
///
/// Handle that contains a hgi object and unique id.
///
/// The unique id is used to compare two handles to gaurd against pointer
/// aliasing, where the same memory address is used to create a similar object,
/// but it is not actually the same object.
///
/// Handle is not a shared or weak_ptr and destruction of the contained object
/// should be explicitely managed by the client via the HgiDestroy*** functions.
///
/// If shared/weak ptr functionality is desired, the client creating Hgi objects
/// can wrap the returned handle in a shared_ptr.
///
template<class T>
class HgiHandle
{
public:
    HgiHandle() : _ptr(nullptr), _id(0) {}
    HgiHandle(T* obj, uint64_t id) : _ptr(obj), _id(id) {}

    T*
    Get() const {
        return _ptr;
    }

    // Note this only checks if a ptr is set, it does not offer weak_ptr safety.
    explicit operator bool() const {return _ptr!=nullptr;}

    // Pointer access operator
    T* operator ->() const {return _ptr;}

    bool operator==(const HgiHandle& other) const {
        return _id == other._id;
    }

    bool operator!=(const HgiHandle& other) const {
        return !(*this == other);
    }

private:
    T* _ptr;
    uint64_t _id;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
