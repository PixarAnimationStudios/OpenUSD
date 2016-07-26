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


#include "pxr/base/tf/expiryNotifier.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/traits.h"

/*!
 * \file weakBase.h
 * \ingroup group_tf_Memory
 */

/*!
 * \class TfWeakBase WeakBase.h pxr/base/tf/weakBase.h
 * \ingroup group_tf_Memory
 * \brief Enable a concrete base class for use with \c TfWeakPtr.
 *
 * You should be familiar with the \c TfWeakPtr type
 * before reading further.
 *
 * A class is enabled for use with the \c TfWeakPtr type by publicly
 * deriving from \c TfWeakBase.  (Note that deriving from \c
 * TfWeakBase adds data to a structure, so the result is no longer a
 * "pure" interface class.)
 *
 * For example,
 * \code
 *     #include "pxr/base/tf/weakBase.h"
 *
 *     class Simple : public TfWeakBase {
 *           ...
 *     };
 * \endcode
 *
 * Given the above inheritance, a \c Simple* can now be used to
 * initialize an object of type \c TfWeakPtr<Simple>.
 */


/*
 * The _Remnant structure is simply a persisent memory of an object's address.
 * When the object dies, the pointer is set to NULL.  A _Remnant object is
 * destroyed when both the original whose address it was initialized with, and
 * there are no weak pointers left pointing to that remnant.
 */

class Tf_Remnant : public TfRefBase
{
public:

    //TF_ENABLE_CLASS_ALLOCATOR(Tf_Remnant);

    virtual ~Tf_Remnant();

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
    virtual void const *_GetUniqueIdentifier() const;

    // Note: this initializes a class member -- the parameter is a non-const
    // reference.
    static TfRefPtr<Tf_Remnant>
    Register(TfRefPtr<Tf_Remnant> &remnantPtr) {
        if (!remnantPtr) {
            Tf_Remnant *tmp = new Tf_Remnant;
            return Register(remnantPtr, tmp);
        }
        return remnantPtr;
    }

    // Note: this initializes a class member -- the parameter is a non-const
    // reference.
    template <class T>
    static TfRefPtr<Tf_Remnant>
    Register(TfRefPtr<Tf_Remnant> &remnantPtr, T *tempRmnt) {
        if (!remnantPtr) {
            /*
             * Note: This takes the place of TfCreateRefPtr: the reference
             * count starts at one, so this is correct.  Note that if we
             * don't delete tmp, we know that _remnantPtr._data is left
             * as nullptr.
             */
            TfRefBase *expectedNullPtr = nullptr;
            if (not remnantPtr._refBase.compare_exchange_strong(
                    expectedNullPtr,
                    static_cast<TfRefBase *>(tempRmnt),
                    std::memory_order_relaxed)) {
                delete tempRmnt;
            }
        }
        return remnantPtr;
    }

    // Mark this remnant to call the expiry notification callback function when
    // it dies.  See ExpiryNotifier.h
    virtual void EnableNotification() const;

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



class TfWeakBase {
public:
    TfWeakBase() {}

    TfWeakBase(const TfWeakBase&) {
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

    void const* GetUniqueIdentifier() const;
    
protected:
    /*
     * Prohibit deletion through a TfWeakBase pointer.
     */

    ~TfWeakBase() {
        if (_remnantPtr)
            _remnantPtr->_Forget();
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
        return bool(_remnantPtr);
    }

private:

    mutable TfRefPtr<Tf_Remnant> _remnantPtr;
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





#endif
