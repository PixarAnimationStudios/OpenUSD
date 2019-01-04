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
#ifndef TF_REFBASE_H
#define TF_REFBASE_H

/// \file tf/refBase.h
/// \ingroup group_tf_Memory

#include "pxr/pxr.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/refCount.h"
#include "pxr/base/tf/api.h"

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

    TfRefBase() : _shouldInvokeUniqueChangedListener(false) { }

    /// Return the current reference count of this object.
    size_t GetCurrentCount() const {
        return GetRefCount().Get();
    }

    /// Return true if only one \c TfRefPtr points to this object.
    bool IsUnique() const {
        return GetRefCount().Get() == 1;
    }

    const TfRefCount& GetRefCount() const {
        return _refCount;
    }

    void SetShouldInvokeUniqueChangedListener(bool shouldCall) {
        _shouldInvokeUniqueChangedListener = shouldCall;
    }

    TF_API static void SetUniqueChangedListener(UniqueChangedListener listener);

protected:
    /*
     * Prohibit deletion through a TfRefBase pointer.
     */
    TF_API virtual ~TfRefBase();

private:
    TfRefCount _refCount;
    bool _shouldInvokeUniqueChangedListener;

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
/// reference-counted object via boost::python.
///
class TfSimpleRefBase : public TfRefBase {
public:
    TF_API virtual ~TfSimpleRefBase();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_REFBASE_H
