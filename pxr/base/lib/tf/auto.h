#ifndef TF_AUTO_H
#define TF_AUTO_H

#include "pxr/base/tf/preprocessorUtilsLite.h"

/*! 
 * \file auto.h
 * \ingroup group_tf_Multithreading
 */


/*!
 * \class TfAuto Auto.h pxr/base/tf/auto.h
 * \brief Helper class to automatically call Start()/Stop() methods at
 * the beginning/end of a scope.
 * \ingroup group_tf_Multithreading
 *
 * A declaration
 * \code
 *     TfAuto<T> obj(t);
 * \endcode
 * is used for exactly one purpose: when \c obj is created, it calls
 * \c t.Start() and when \c obj is destroyed, it calls \c t.Stop().
 * The \c TfAuto<T> template works well with the "birth is acquisition" and
 * "death is release" philosophy of resources. As an example:
 *
 * \code
 *     // using a TfMutex mutex directly:
 *     int func(TfMutex& mutex) {
 *          mutex.Lock();       // wait for lock on mutex
 *          int value = func2();
 *          mutex.Unlock();     // release lock on mutex
 *          return value;
 *     }
 * \endcode
 *
 * If an exception occurs before \c Unlock() is called,
 * deadlock is likely since the lock on \c mutex is not released.
 * Similarly, if there are multiple return paths out of \c func(),
 * each path (and any newly added paths) must be careful to unlock the mutex.
 * But using a TfAuto<TfMutex>, the code is simply
 *
 * \code
 *     int func(TfMutex& mutex) {
 *          TfAuto<TfMutex> dummy(mutex);    // wait for lock on mutex
 *          return func2();                  
 *     }                                     // lock released
 * \endcode
 *
 * Note that the \c TfMutex class is designed so that \c Start() and \c Lock()
 * are synonyms, as are \c Stop() and \c Unlock().
 *
 * <B> Warning! Warning! Warning! </B>
 *
 * The following code fragments will \e not do what you want; each of these
 * is actually a function declaration, meaning that it doesn't create an object.
 * Be wary of these, because there will be no warning from the compiler:
 * \code
 *     TfAuto<SomeType> noname(SomeType);    // noname declared to be a function,
 *                                           // returning a TfAuto<SomeType>.
 *
 *     TfAuto<SomeType> noname(SomeType());  // still a function declaration;
 *                                           // this does NOT mean to create
 *                                           // a temporary object of type
 *                                           // SomeType()
 * \endcode
 */

class Tf_AutoBase {};

template <class T>
class TfAuto : public Tf_AutoBase {
public:
    explicit TfAuto(T& object) : _object(&object)
    {
        _object->Start();
    }

    ~TfAuto() {
        if (_object)
            _object->Stop();
    }

    // Copy ctor *moves* stop responsibility into this object.
    inline TfAuto(TfAuto const &other) : _object(other._object) {
        // Disable the copied-from auto.
        other._object = 0;
    }

private:

    void operator=(TfAuto const &);

    mutable T* _object;
};

template <class T>
inline TfAuto<T> TfMakeAuto(T &object) {
    return TfAuto<T>(object);
}

/// Helper for creating a local TfAuto<T> for \p obj.
#define TF_SCOPED_AUTO(obj)                                                  \
    Tf_AutoBase const & TF_PP_CAT(Tf_auto_var_, __LINE__) = TfMakeAuto(obj); \
    (void)TF_PP_CAT(Tf_auto_var_, __LINE__)

#endif
