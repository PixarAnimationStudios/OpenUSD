//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_PRIM_DATA_HANDLE_H
#define PXR_USD_USD_PRIM_DATA_HANDLE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/base/tf/delegatedCountPtr.h"
#include "pxr/base/tf/hash.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;

// To start we always validate.
#define USD_CHECK_ALL_PRIM_ACCESSES

// Forward declare TfDelegatedCountPtr requirements.  Defined in primData.h.
void TfDelegatedCountIncrement(const class Usd_PrimData *prim) noexcept;
void TfDelegatedCountDecrement(const class Usd_PrimData *prim) noexcept;

// Forward declarations for Usd_PrimDataHandle's use.  Defined in primData.h.
USD_API
void Usd_ThrowExpiredPrimAccessError(Usd_PrimData const *p);
bool Usd_IsDead(Usd_PrimData const *p);

// convenience typedefs for raw ptrs.
typedef Usd_PrimData *Usd_PrimDataPtr;
typedef const Usd_PrimData *Usd_PrimDataConstPtr;

// convenience typedefs for TfDelegatedCountPtr.
using Usd_PrimDataIPtr = TfDelegatedCountPtr<Usd_PrimData>;
using Usd_PrimDataConstIPtr = TfDelegatedCountPtr<const Usd_PrimData>;

// Private helper class that holds a reference to prim data.  UsdObject (and by
// inheritance its subclasses) hold an instance of this class.  It lets
// UsdObject detect prim expiry, and provides access to cached prim data.
class Usd_PrimDataHandle
{
public:
    // smart ptr element_type typedef.
    typedef Usd_PrimDataConstIPtr::element_type element_type;

    // Construct a null handle.
    Usd_PrimDataHandle() {}
    // Convert/construct a handle from a prim data delegated count ptr.
    Usd_PrimDataHandle(const Usd_PrimDataIPtr &primData)
        : _p(primData) {}
    // Convert/construct a handle from a prim data delegated count ptr.
    Usd_PrimDataHandle(const Usd_PrimDataConstIPtr &primData)
        : _p(primData) {}
    // Convert/construct a handle from a prim data raw ptr.
    Usd_PrimDataHandle(Usd_PrimDataPtr primData)
        : _p(TfDelegatedCountIncrementTag, primData) {}
    // Convert/construct a handle from a prim data raw ptr.
    Usd_PrimDataHandle(Usd_PrimDataConstPtr primData)
        : _p(TfDelegatedCountIncrementTag, primData) {}

    // Reset this handle to null.
    void reset() { _p.reset(); }

    // Swap this handle with \p other.
    void swap(Usd_PrimDataHandle &other) { _p.swap(other._p); }

    // Dereference this handle.  If USD_CHECK_ALL_PRIM_ACCESSES is defined, this
    // will issue a fatal error if the handle is invalid.
    element_type *operator->() const {
        element_type *p = _p.get();
#ifdef USD_CHECK_ALL_PRIM_ACCESSES
        if (!p || Usd_IsDead(p)) {
            Usd_ThrowExpiredPrimAccessError(p);
        }
#endif
        return p;
    }

    // Explicit bool conversion operator. Returns \c true if this handle points 
    // to a valid prim instance that is not marked dead, \c false otherwise.
    explicit operator bool() const {
        element_type *p = _p.get();
        return p && !Usd_IsDead(p);
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
        return TfHash()(h._p.get());
    }

    friend element_type *get_pointer(const Usd_PrimDataHandle &h) {
        return h._p.get();
    }

    Usd_PrimDataConstIPtr _p;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_PRIM_DATA_HANDLE_H
