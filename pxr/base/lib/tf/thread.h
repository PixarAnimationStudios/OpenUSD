#ifndef TF_THREAD_H
#define TF_THREAD_H


#include "pxr/base/tf/threadBase.h"
#include "pxr/base/tf/traits.h"
#include "pxr/base/tf/typeFunctions.h"

#include <boost/function.hpp>

/*!
 * \file thread.h
 * \ingroup group_tf_Multithreading
 */



class TfThreadDispatcher;

/*!
 * \class TfThread Thread.h pxr/base/tf/Thread.h
 * \ingroup group_tf_Multithreading
 * \brief Thread descriptor returned by \c TfThreadDispatcher::Start().
 *
 * When threads are launched by a \c TfThreadDispatcher, information about
 * the thread (completion status and return value) are accessed via a
 * \c TfThread structure returned by the dispatcher.  A request to
 * run a function with retun type \p RET yields a pointer to a
 * \c TfThread<RET>.  It is never the user's responsibility to delete
 * a \c TfThread (and in fact, the destructor is private).
 *
 * Note that users are never given a \c TfThread directly; they receive
 * a \c TfRefPtr of appropriate type.
 */
template <class RET>
class TfThread : public TfThreadBase {
public:
    /*!
     * \brief Handle \c TfThread via \c TfRefPtr mechanism.
     *
     * When threads are launched from a \c TfThreadDispatcher,
     * the dispatcher returns a \c TfThread by means of a \c TfRefPtr.
     * When the last reference to a \c TfThread disappears, memory is
     * reclaimed.
     */
    typedef TfRefPtr<TfThread<RET> > Ptr;
    
    /*!
     * \brief Block until the thread is completed and then return result.
     *
     * This call will not return until the thread being waited on finishes.
     * A reference to the return value of the function executed by the thread
     * is returned, unless the function in question returns \c void.
     * The storage reserved for the return value lasts until the last
     * reference to the \c TfThread returned by \c TfThreadDispatcher is gone.
     * Repeated calls to \c GetResult() will immediately the same reference.
     *
     * Note that you cannot call this function on a non-immediate thread from
     * another non-immediate thread if both threads are run from the
     * same dispatcher (because of the potential for deadlock, given that
     * the thread-pool is of finite size).  Attempting such a wait results
     * in run-time termination of the program.
     *    
     * Finally, you \e cannot call \c Wait() on a thread that you have
     * canceled, since there is no return value for the thread.
     * Calling \c Wait() on a canceled thread will result in an
     * assertion failure and termination of your program.  Thus, if
     * there is the possibility that the thread might have been
     * canceled, call \c IsCanceled() to check cancellation status
     * first.
     */
#if defined(doxygen)
    RET&
#else
    typename TfTraits::Type<RET>::RefType
#endif
    GetResult() {
        if (ARCH_UNLIKELY(_canceled))
            TF_FATAL_ERROR("cannot wait on cancelled thread with "
                           "return value");
        TfThreadBase::Wait();
        return *_resultPtr;
    }
    
    virtual ~TfThread();

private:
    TfThread(const boost::function<RET ()>& func, TfThreadInfo* info)
        : TfThreadBase(info), _func(func), _resultPtr(NULL)
    {
    }

    virtual void _ExecuteFunc();

    boost::function<RET ()>  _func;
                                                    // pointer to return value
    typename TfTraits::Type<RET>::PointerType _resultPtr;

    friend class TfThreadDispatcher;
};

template <class RET>
TfThread<RET>::~TfThread()
{
    if (!TfTraits::Type<RET>::isReference)
        delete _resultPtr;
}

template <class RET>
void
TfThread<RET>::_ExecuteFunc()
{
    /*
     * If the delayed function returns by value, we need to copy it.
     * Otherwise, we want the address of the object returned; this is
     * exactly what TfCopyIfNotReference does.  The destructor
     * (above) will delete _resultPtr in the case when RET is NOT a
     * reference.
     */
    _StoreThreadInfo();

    // note: if cancellation occurs, _func() does not return
    _resultPtr = TfCopyIfNotReference<RET>::Apply(_func());
}

#if !defined(doxygen)
// Specialization for void return type

template <>
class TfThread<void> : public TfThreadBase {
public:
    typedef TfRefPtr<TfThread<void> > Ptr;

    virtual ~TfThread();

private:
    TfThread(const boost::function<void ()>& func, TfThreadInfo* info)
        : TfThreadBase(info), _func(func)
    {
    }
    
    boost::function<void ()> _func;
    virtual void _ExecuteFunc();
    friend class TfThreadDispatcher;
};

#endif  /* !doxygen */





#endif
