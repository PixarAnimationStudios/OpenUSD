//
// Copyright 2016 Pixar
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
#ifndef USD_PRIMDATAHANDLE_H
#define USD_PRIMDATAHANDLE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include <boost/functional/hash.hpp>
#include <boost/intrusive_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;

// To start we always validate.
#define USD_CHECK_ALL_PRIM_ACCESSES

// Forward declare boost::intrusive_ptr requirements.  Defined in primData.h.
void intrusive_ptr_add_ref(const class Usd_PrimData *prim);
void intrusive_ptr_release(const class Usd_PrimData *prim);

// Forward declarations for Usd_PrimDataHandle's use.  Defined in primData.h.
USD_API
void Usd_IssueFatalPrimAccessError(Usd_PrimData const *p);
bool Usd_IsDead(Usd_PrimData const *p);

// convenience typedefs for raw ptrs.
typedef Usd_PrimData *Usd_PrimDataPtr;
typedef const Usd_PrimData *Usd_PrimDataConstPtr;

// convenience typedefs for intrusive_ptr.
typedef boost::intrusive_ptr<Usd_PrimData> Usd_PrimDataIPtr;
typedef boost::intrusive_ptr<const Usd_PrimData> Usd_PrimDataConstIPtr;

// Private helper class that holds a reference to prim data.  UsdObject (and by
// inheritance its subclasses) hold an instance of this class.  It lets
// UsdObject detect prim expiry, and provides access to cached prim data.
class Usd_PrimDataHandle
{
    // safe-bool idiom.
    typedef const Usd_PrimDataConstIPtr
        Usd_PrimDataHandle::*_UnspecifiedBoolType;
public:
    // smart ptr element_type typedef.
    typedef Usd_PrimDataConstIPtr::element_type element_type;

    // Construct a null handle.
    Usd_PrimDataHandle() {}
    // Convert/construct a handle from a prim data intrusive ptr.
    Usd_PrimDataHandle(const Usd_PrimDataIPtr &primData)
        : _p(primData) {}
    // Convert/construct a handle from a prim data intrusive ptr.
    Usd_PrimDataHandle(const Usd_PrimDataConstIPtr &primData)
        : _p(primData) {}
    // Convert/construct a handle from a prim data raw ptr.
    Usd_PrimDataHandle(Usd_PrimDataPtr primData)
        : _p(Usd_PrimDataConstIPtr(primData)) {}
    // Convert/construct a handle from a prim data raw ptr.
    Usd_PrimDataHandle(Usd_PrimDataConstPtr primData)
        : _p(Usd_PrimDataConstIPtr(primData)) {}

    // Assignment.
    Usd_PrimDataHandle &operator=(const Usd_PrimDataHandle &other) {
        Usd_PrimDataHandle(other).swap(*this);
        return *this;
    }

    // Reset this handle to null.
    void reset() { _p.reset(); }

    // Swap this handle with \p other.
    void swap(Usd_PrimDataHandle &other) { _p.swap(other._p); }

    // Dereference this handle.  If USD_CHECK_ALL_PRIM_ACCESSES is defined, this
    // will issue a fatal error if the handle is invalid.
    element_type *operator->() const {
        element_type *p = _p.get();
#ifdef USD_CHECK_ALL_PRIM_ACCESSES
        if (!p || Usd_IsDead(p))
            Usd_IssueFatalPrimAccessError(p);
#endif
        return p;
    }

    // Safe-bool operator.  Return true if this handle points to a valid prim
    // instance that is not marked dead, false otherwise.
    operator _UnspecifiedBoolType() const {
        element_type *p = _p.get();
        return p && !Usd_IsDead(p) ? &Usd_PrimDataHandle::_p : NULL;
    }

    // Return a text description of this prim data, used primarily for
    // diagnostic purposes.
    std::string GetDescription(SdfPath const &proxyPrimPath) const;

private:
    // Equality comparison.
    friend bool operator==(const Usd_PrimDataHandle &lhs,
                           const Usd_PrimDataHandle &rhs) {
        return lhs._p == rhs._p;
    }

    // Inequality comparison.
    friend bool operator!=(const Usd_PrimDataHandle &lhs,
                           const Usd_PrimDataHandle &rhs) {
        return !(lhs == rhs);
    }

    // Swap \p lhs and \p rhs.
    friend void swap(Usd_PrimDataHandle &lhs, Usd_PrimDataHandle &rhs) {
        lhs.swap(rhs);
    }

    // Provide hash_value.
    friend size_t hash_value(const Usd_PrimDataHandle &h) {
        return boost::hash_value(h._p.get());
    }

    friend element_type *get_pointer(const Usd_PrimDataHandle &h) {
        return h._p.get();
    }

    Usd_PrimDataConstIPtr _p;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_PRIMDATAHANDLE_H
