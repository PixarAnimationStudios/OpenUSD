//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_REF_BASE_H
#define PXR_BASE_TF_REF_BASE_H

/// \file tf/refBase.h
/// \ingroup group_tf_Memory

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"

#include <atomic>
#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

template <class T> class TfRefPtr;
template <class T> class TfWeakPtr;

/// \class TfRefBase
/// \ingroup group_tf_Memory
///
/// Enable a concrete base class for use with \c TfRefPtr.
///
/// You should be familiar with the \c TfRefPtr type before reading further.
///
/// A class (but not an interface class) is enabled for reference
/// counting via the \c TfRefPtr type by publicly deriving from \c
/// TfRefBase.
///
/// For example,
/// \code
///     #include "pxr/base/tf/refPtr.h"
///
///     class Simple : public TfRefBase {
///     public:
///         TfRefPtr<Simple> New() {
///             return TfCreateRefPtr(new Simple);
///         }
///     private:
///         Simple();
///     };
/// \endcode
///
/// The class \c Simple can now only be manipulated in terms of
/// a \c TfRefPtr<Simple>.
///
/// To disable the cost of the "unique changed" system, derive
/// from TfSimpleRefBase instead.
///
class TfRefBase {
public:

    typedef void (*UniqueChangedFuncPtr)(TfRefBase const *, bool);
    struct UniqueChangedListener {
        void (*lock)();
        UniqueChangedFuncPtr func;
        void (*unlock)();
    };

    // This mimics the old TfRefCount's default ctor behavior, which set
    // _refCount to 1.
    TfRefBase() : _refCount(1) {}

    // This mimics the old TfRefCount's copy ctor behavior, which set _refCount
    // to 1 on copy construction.
    TfRefBase(TfRefBase const &) : _refCount(1) {}

    // This mimics the old TfRefCount's copy assignment behavior, which took no
    // action.
    TfRefBase &operator=(TfRefBase const &) {
        return *this;
    }

    /// Return the current reference count of this object.
    size_t GetCurrentCount() const {
        // Return the absolute value since the sign encodes whether or not this
        // TfRefBase invokes the UniqueChangedListener.
        return std::abs(_refCount.load(std::memory_order_relaxed));
    }

    /// Return true if only one \c TfRefPtr points to this object.
    bool IsUnique() const {
        return GetCurrentCount() == 1;
    }

    void SetShouldInvokeUniqueChangedListener(bool shouldCall) {
        int curValue = _refCount.load(std::memory_order_relaxed);
        while ((curValue > 0 && shouldCall) ||
               (curValue < 0 && !shouldCall)) {
            if (_refCount.compare_exchange_weak(curValue, -curValue)) {
                return;
            }                    
        }
    }

    TF_API static void SetUniqueChangedListener(UniqueChangedListener listener);

protected:
    /*
     * Prohibit deletion through a TfRefBase pointer.
     */
    TF_API virtual ~TfRefBase();

private:
    // For TfRefPtr's use.
    std::atomic_int &_GetRefCount() const {
        return _refCount;
    }
    
    // Note! Counts can be both positive or negative.  Negative counts indicate
    // that we must invoke the _uniqueChangedListener if the count goes 1 -> 2
    // or 2 -> 1 (which is really -1 -> -2 or -2 -> -1).
    mutable std::atomic_int _refCount;

    static UniqueChangedListener _uniqueChangedListener;
    template <typename T> friend class TfRefPtr;
    friend struct Tf_RefPtr_UniqueChangedCounter;
    friend struct Tf_RefPtr_Counter;

    template <typename T> friend TfRefPtr<T>
    TfCreateRefPtrFromProtectedWeakPtr(TfWeakPtr<T> const &);
};

/// \class TfSimpleRefBase
/// \ingroup group_tf_Memory
///
/// Enable a concrete base class for use with \c TfRefPtr that inhibits the
/// "unique changed" facility of TfRefPtr.
///
/// Derive from this class if you don't plan on wrapping your
/// reference-counted object via pxr_boost::python.
///
class TfSimpleRefBase : public TfRefBase {
public:
    TF_API virtual ~TfSimpleRefBase();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_REF_BASE_H
