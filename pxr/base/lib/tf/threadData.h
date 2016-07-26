#ifndef TF_THREAD_DATA_H
#define TF_THREAD_DATA_H

#include "pxr/base/tf/threadInfo.h"

/*!
 * \file threadData.h
 * \ingroup group_tf_Multithreading
 */


/*!
 * \class TfThreadData ThreadData.h pxr/base/tf/ThreadData.h
 * \ingroup group_tf_Multithreading
 * \brief Thread-specific data.
 *
 * Threads sometimes need access to data that is specific to their own
 * thread.  A \c TfThreadData<T> object can be thought of as a pointer
 * to data; however, two distinct threads that derefence the same \c
 * TfThreadData<T> object will access different objects.
 *
 * Creation of a \c TfThreadData<T> object does not create a \c T
 * object initially.  The first dereference of the data-object causes
 * a \c T to be created; this \c T object persists until the thread
 * terminates. (If the size of \c T is large and you need to control
 * its lifetime, consider making a \c TfThreadData<T*> object instead;
 * each thread can then new/delete a \c T object, with the pointer
 * stored as thread-specific data.)
 *
 * However, in the case of pool-mode threads, it may be useful for the
 * created \c T object to persist until the actual physical (as
 * opposed to virtual) pthread created by the \c TfThreadDispatcher
 * terminates.  To support this, a \c TfThreadData<T> can be created
 * as a short-term or long-term data-object.  For non-pool mode
 * threads, the distinction between short-term and long-term is
 * meaningless.  For pool-mode threads however, a given \c
 * TfThreadDispatcher::Launch() call can involve the "reuse" of a
 * previously existent thread; in this case, the newly launched task
 * will find that long-term data-objects which have already been
 * created are "inherited" by the new task.  For this reason, one
 * should use long-term data-objects with care, because their created
 * \c T objects are only destroyed when the dispatcher shuts down its
 * pool-threads (i.e. when the dispatcher is destroyed).
 *
 * A shorter translation of the above might be: use short-term mode,
 * unless you are convinced you need long-term mode, and in that case,
 * check with an expert before proceeding.
 *
 * Here is a simple example:
 * \code
 *    struct MyStuff {
 *        TfThreadData<GfVec3d> _tsdPoint;
 *        ...
 *        void _Initialize() {
 *             *_tsdPoint = GfVec3d(0,0,0);
 *        }
 *
 *        void _Task() {
 *            GfVec3d oldValue = *_tsdPoint;
 *            ...
 *            *_tsdPoint = newValue;
 *            _tsdPoint->Normalize();
 *        }
 *    };
 * \endcode
 *
 * In this example, multiple threads invoking \c _Initialize() or \c
 * _Task() on the same \c MyStuff object will write/read into
 * different variables.
 */

template <typename T>
class TfThreadData {
public:
    /*!
     * \brief Create a thread-data object.
     *
     * The type \c T must have a default constructor.  The first
     * derefence of \c *this causes a \c T to be constructed in a
     * thread-specific location.  The newly constructed \c T value
     * is initialized to \p defaultValue.
     *
     * Destruction of \c *this itself does \e not cause the \c T
     * object to be destroyed (even though it is no longer
     * accessible).  Also, if \c T is of pointer type, the pointer
     * itself is destroyed, but \e not the data pointed to.
     *
     * The default behavior is to create the data as a short-term
     * object.  This means that the \c T object is destroyed when the
     * thread is destroyed.
     *
     * However, if \p type is \c TfThreadInfo::LONG_TERM, then the
     * data-object created is a long-term object, whose behavior
     * differs from a short-term object \e only for pool-mode threads.
     * For a pool-mode thread, long-term data objects are not
     * destroyed when a thread finishes.  Rather, the next launched
     * thread which runs in the same phyiscal thread of the pool will
     * reuse the created \c T object.  Use this option with caution,
     * since you are essentially polluting the thread-specific data
     * environment.
     */
    explicit TfThreadData(const T& defaultValue,
                          TfThreadInfo::DataLifetime type = TfThreadInfo::SHORT_TERM)
        : _defaultValue(defaultValue),
          _key(TfThreadInfo::_GetNextThreadDataKey()),
          _shortTerm(type == TfThreadInfo::SHORT_TERM)
    {
    }

    /*!
     * \overload
     *
     * The default value is unspecified; newly constructed per-thread data
     * is essentially uninitialized.
     */
    explicit TfThreadData(TfThreadInfo::DataLifetime type = TfThreadInfo::SHORT_TERM)
        : _key(TfThreadInfo::_GetNextThreadDataKey()),
          _shortTerm(type == TfThreadInfo::SHORT_TERM)
    {
    }
    
    //! Return a reference to a thread-specific \c T object.
    T& operator* () const {
        typedef TfThreadInfo::_ThreadDataTable _Table;

        _Table& table = TfThreadInfo::Find()->_GetThreadDataTable(_shortTerm);
        _Table::iterator i = table.find(_key);
        TfThreadInfo::_UntypedThreadData* data;

        if (i == table.end())
            data = (table[_key] = new TfThreadInfo::_ThreadData<T>(_defaultValue));
        else
            data = i->second;
            
        return *(reinterpret_cast<T*>(data->_Get()));
    }   

    //! Accessor to thread-specific object's public members.
    T* operator-> () const {
        return &(operator* ());
    }

private:
    T _defaultValue;
    int _key;
    bool _shortTerm;
};

#endif
