//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
/// The unique id is used to compare two handles to guard against pointer
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

    uint64_t GetId() const {
        return _id;
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
