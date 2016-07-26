#ifndef TF_ATOMICINTEGER_H
#define TF_ATOMICINTEGER_H


#include "pxr/base/arch/atomicOperations.h"

/*!
 * \file atomicInteger.h
 * \ingroup group_tf_Multithreading
 */



/*!
 * \class TfAtomicInteger AtomicInteger.h pxr/base/tf/atomicInteger.h
 * \ingroup group_tf_Multithreading
 * \brief Atomic integer class.
 *
 * This class is used to modify shared (i.e. global) integer variables
 * in a thread-safe manner, without the need for locking.  Atomic
 * operations are typically an order of magnitude faster than locking
 * a mutex, changing the value, and unlocking the mutex.
 * */

class TfAtomicInteger {
public:
    /*! \brief Leaves object's value uninitialized. */
    TfAtomicInteger() {
    }

    /*! \brief Initializes object to value \c value. */
    TfAtomicInteger(int value) {
        Set(value);
    }

    /*! \brief Returns object's value. */
    int Get() const {
        return _value;
    }

    /*! \brief Returns object's value. */
    operator int() const {
        return _value;
    }

    /*! \brief Sets object's value to \c value. */
    void Set(int value) {
        ArchAtomicIntegerSet(&_value, value);
    }

    /*!
     * \brief Increment the integer by 1.
     */
    void Increment() {
        return ArchAtomicIntegerIncrement(&_value);
    }

    /*!
     * \brief Decrement the integer by 1.
     */
    void Decrement() {
        ArchAtomicIntegerDecrement(&_value);
    }

    /*!
     * \brief Decrement the integer by 1 and return true if the result is zero.
     */
    bool DecrementAndTestIfZero() {
        return ArchAtomicIntegerDecrementAndTestIfZero(&_value);
    }

    /*!
     * \brief Add \c amount to object's value.
     */
    void Add(int amount) {
        ArchAtomicIntegerAdd(&_value, amount);
    }

    /*!
     * \brief Add \c amount to object's value, returning initial value.
     */
    int FetchAndAdd(int amount) {
        return ArchAtomicIntegerFetchAndAdd(&_value, amount);
    }

private:
    volatile int _value;
};


#endif
