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
#ifndef TF_REFCOUNT_H
#define TF_REFCOUNT_H


#include "pxr/base/arch/inttypes.h"
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/tf/api.h"
#include <atomic>

/*!
 * \file refCount.h
 * \ingroup group_tf_Memory
 */



template <typename T> class TfRefPtr;

/*!
 * \class TfRefCount RefCount.h pxr/base/tf/refCount.h
 * \ingroup group_tf_Memory
 * \brief Reference counter class
 *
 * This class is intended to be embedded in other classes, for use as
 * a reference counter.  Unless you need to provide extraordinary
 * customization, you should forgo direct use of this class and instead
 * make use of the base class \c TfRefBase.
 *
 * Initialization of a reference counter is somewhat counterintuitive.
 * Consider an object T with a reference counter R.  When T is
 * initialized, R should be initialized to one, even if T is
 * copy-constructed.  This implies that \e all constructors of \c
 * TfRefCount set the counter to one, even the copy constructor.
 *
 * Conversely, if T is assigned to, the reference counter R in T should not
 * change.  This implies that the assignment operator for \c TfRefCount
 * does not change the counter's value.
 *
 * Finally, for thread-safety, the counter should be atomic.
 *
 * This class was written primarily for use in classes whose access is
 * encapsulated by means of the \c TfRefPtr interface; such classes require
 * reference counting semantics as described above.  Note that
 * the behavior of a \c TfRefCount in a class T is invariant with respect
 * to T's copy constructors and assignment operators.
 *
 * Again, please do not directly embed a \c TfRefCount in a structure
 * unless the functionality of \c TfRefBase is insufficient for your needs.
 */

class TfRefCount {
public:
    /*!
     * \brief Initialize counter to one.
     */
    TfRefCount() : _counter(1) {
    }

    /*!
     * \brief Initialize counter to one.
     *
     * Even if you copy from a reference counter, you want the
     * newly constructed counter to start at one.
     */

    TfRefCount(const TfRefCount&) : _counter(1) {
    }
    
    /*! \brief Returns counter's value. */
    int Get() const {
        return _counter;
    }

    /*!
     * \brief Assignment to a reference counter has no effect.
     */
    const TfRefCount& operator=(const TfRefCount&) const {
        return *this;
    }
private:
    /*!
     * \brief Decrements counter by \c 1, returning true if the result is 0.
     */
    bool _DecrementAndTestIfZero() const {
        return (--_counter == 0);
    }

    /*!
     * \brief Adds \c amount to the count, returning the prior value.
     */
    int _FetchAndAdd(int amount) const {
        return _counter.fetch_add(amount);
    }

private:
    mutable std::atomic<int> _counter;
    template <typename T> friend class TfRefPtr;
    friend struct Tf_RefPtr_UniqueChangedCounter;
    friend struct Tf_RefPtr_Counter;
};





#endif
