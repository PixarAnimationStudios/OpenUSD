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
#ifndef TF_WEAKBASE_H
#define TF_WEAKBASE_H

/// \file tf/weakBase.h
/// \ingroup group_tf_Memory

#include "pxr/pxr.h"

#include "pxr/base/tf/expiryNotifier.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/traits.h"
#include "pxr/base/tf/api.h"
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

// The _Remnant structure is simply a persistent memory of an object's
// address. When the object dies, the pointer is set to NULL.  A _Remnant
// object is destroyed when both the original whose address it was
// initialized with, and there are no weak pointers left pointing to that
// remnant.
class Tf_Remnant : public TfSimpleRefBase
{
public:

    TF_API virtual ~Tf_Remnant();

    void _Forget() {
        _alive = false;

        if (_notify2)
            Tf_ExpiryNotifier::Invoke2(this);
    }

    // Note that only "false" is of value in a multi-threaded world...
    bool _IsAlive() {
        return _alive;
    }

    // Must return an object's address whose lifetime is as long or longer than
    // this object.  Default implementation returns 'this'.
    TF_API virtual void const *_GetUniqueIdentifier() const;

    // Note: this initializes a class member -- the parameter is a non-const
    // reference.
    static TfRefPtr<Tf_Remnant>
    Register(std::atomic<Tf_Remnant*> &remnantPtr) {
        if (Tf_Remnant *remnant = remnantPtr.load()) {
            // Remnant exists.  Return additional reference.
            return TfRefPtr<Tf_Remnant>(remnant);
        } else {
            // Allocate a remnant and attempt to register it.
            return Register(remnantPtr, new Tf_Remnant);
        }
    }

    // Note: this initializes a class member -- the parameter is a non-const
    // reference.
    template <class T>
    static TfRefPtr<Tf_Remnant>
    Register(std::atomic<Tf_Remnant*> &remnantPtr, T *candidate) {
        Tf_Remnant *existing = nullptr;
        if (remnantPtr.compare_exchange_strong(
                existing,
                static_cast<Tf_Remnant*>(candidate))) {
            // Candidate registered.  Return additional reference.
            return TfRefPtr<Tf_Remnant>(candidate);
        } else {
            // Somebody beat us to it.
            // Discard candidate and return additional reference.
            delete candidate;
            return TfRefPtr<Tf_Remnant>(existing);
        }
    }

    // Mark this remnant to call the expiry notification callback function when
    // it dies.  See ExpiryNotifier.h
    TF_API virtual void EnableNotification() const;

protected:
    friend class TfWeakBase;

    Tf_Remnant()
        : _notify(false),
          _notify2(false),
          _alive(true)
    {
    }

private:

    mutable bool _notify, _notify2;
    bool _alive;
};

/// \class TfWeakBase
/// \ingroup group_tf_Memory
///
/// Enable a concrete base class for use with \c TfWeakPtr.
///
/// You should be familiar with the \c TfWeakPtr type before reading further.
///
/// A class is enabled for use with the \c TfWeakPtr type by publicly deriving
/// from \c TfWeakBase.  (Note that deriving from \c TfWeakBase adds data to a
/// structure, so the result is no longer a "pure" interface class.)
///
/// For example,
/// \code
///     #include "pxr/base/tf/weakBase.h"
///
///     class Simple : public TfWeakBase {
///           ...
///     };
/// \endcode
///
/// Given the above inheritance, a \c Simple* can now be used to initialize an
/// object of type \c TfWeakPtr<Simple>.
///
class TfWeakBase {
public:
    TfWeakBase() : _remnantPtr(nullptr) {}

    TfWeakBase(const TfWeakBase&) : _remnantPtr(nullptr) {
        // A newly created copy of a weak base doesn't start with a remnant
    }

    // Don't call this method -- only for Tf internal use.  The presence of this
    // method is used by TfWeakPtr and related classes to determine whether a
    // class may be pointed to by a TfWeakPtr.  It is named nonstandardly to
    // avoid any possible conflict with other names in derived classes.
    const TfWeakBase& __GetTfWeakBase__() const {
        return *this;
    }

    const TfWeakBase& operator= (const TfWeakBase&) {
        // Do nothing.  An assignment should NOT assign the other object's
        // remnant and should NOT create a new remnant.  We want to keep
        // the one we already have (if any).
        return *this;
    }

    // Don't call this.  Really.
    void EnableNotification2() const;

    TF_API void const* GetUniqueIdentifier() const;
    
protected:
    /*
     * Prohibit deletion through a TfWeakBase pointer.
     */

    ~TfWeakBase() {
        if (Tf_Remnant *remnant = _remnantPtr.load(std::memory_order_relaxed)) {
            remnant->_Forget();
            // Briefly forge a TfRefPtr to handle dropping our implied
            // reference to the remnant.
            TfRefPtr<Tf_Remnant> lastRef = TfCreateRefPtr(remnant);
        }
    }

    /*
     * This needs to be atomic, for multithreading.
     */
    TfRefPtr<Tf_Remnant> _Register() const {
        return Tf_Remnant::Register(_remnantPtr);
    }

    template <class T>
    TfRefPtr<Tf_Remnant> _Register(T *tempRmnt) const {
        return Tf_Remnant::Register<T>(_remnantPtr, tempRmnt);
    }

    bool _HasRemnant() const {
        return _remnantPtr.load(std::memory_order_relaxed) ? true : false;
    }

private:

    // XXX Conceptually this plays the same role as a TfRefPtr to the
    // Tf_Remnant, in the sense that we want TfWeakBase to participate
    // in the ref-counted lifetime tracking of its remnant.
    // However, we require atomic initialization of this pointer.
    mutable std::atomic<Tf_Remnant*> _remnantPtr;

    friend class Tf_WeakBaseAccess;
};

class Tf_WeakBaseAccess {
public:
    static TfRefPtr<Tf_Remnant> GetRemnant(TfWeakBase const &wb) {
        return wb._Register();
    }
private:
    Tf_WeakBaseAccess();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_WEAKBASE_H
