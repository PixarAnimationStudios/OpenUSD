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
#ifndef PXR_BASE_TF_REF_PTR_TRACKER_H
#define PXR_BASE_TF_REF_PTR_TRACKER_H

/// \file tf/refPtrTracker.h

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/singleton.h"
#include <boost/noncopyable.hpp>
#include <iosfwd>
#include <mutex>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class TfRefBase;
template <class T> class TfRefPtr;

/// \class TfRefPtrTracker
///
/// Provides tracking of \c TfRefPtr objects to particular objects.
///
/// Clients can enable, at compile time, tracking of \c TfRefPtr objects that
/// point to particular instances of classes derived from \c TfRefBase.
/// This is useful if you have a ref counted object with a ref count that
/// should've gone to zero but didn't.  This tracker can tell you every
/// \c TfRefPtr that's holding the \c TfRefBase and a stack trace where it
/// was created or last assigned to.
///
/// Clients can get a report of all watched instances and how many \c TfRefPtr
/// objects are holding them using \c ReportAllWatchedCounts() (in python use
/// \c Tf.RefPtrTracker().GetAllWatchedCountsReport()).  You can see all of
/// the stack traces using \c ReportAllTraces() (in python use
/// \c Tf.RefPtrTracker().GetAllTracesReport()).
///
/// Clients will typically enable tracking using code like this:
///
/// \code
/// #include "pxr/base/tf/refPtrTracker.h"
///
/// class MyRefBaseType;
/// typedef TfRefPtr<MyRefBaseType> MyRefBaseTypeRefPtr;
///
/// TF_DECLARE_REFPTR_TRACK(MyRefBaseType);
///
/// class MyRefBaseType {
/// ...
/// public:
///     static bool _ShouldWatch(const MyRefBaseType*);
/// ...
/// };
///
/// TF_DEFINE_REFPTR_TRACK(MyRefBaseType, MyRefBaseType::_ShouldWatch);
/// \endcode
///
/// Note that the \c TF_DECLARE_REFPTR_TRACK() macro must be invoked before
/// any use of the \c MyRefBaseTypeRefPtr type.
///
/// The \c MyRefBaseType::_ShouldWatch() function returns \c true if the
/// given instance of \c MyRefBaseType should be tracked.  You can also
/// use \c TfRefPtrTracker::WatchAll() to watch every instance (but that
/// might use a lot of memory and time).
///
/// If you have a base type, \c B, and a derived type, \c D, and you hold
/// instances of \c D in a \c TfRefPtr<\c B> (i.e. a pointer to the base) then
/// you must track both type \c B and type \c D.  But you can use
/// \c TfRefPtrTracker::WatchNone() when tracking \c B if you're not
/// interested in instances of \c B.
///
class TfRefPtrTracker : public TfWeakBase, boost::noncopyable {
public:
    enum TraceType { Add, Assign };

    TF_API static TfRefPtrTracker& GetInstance()
    {
         return TfSingleton<TfRefPtrTracker>::GetInstance();
    }

    /// Returns the maximum stack trace depth.
    TF_API
    size_t GetStackTraceMaxDepth() const;

    /// Sets the maximum stack trace depth.
    TF_API
    void SetStackTraceMaxDepth(size_t);

    /// A track trace.
    struct Trace {
        /// The stack trace when the \c TfRefPtr was created or assigned to.
        std::vector<uintptr_t> trace;

        /// The object being pointed to.
        const TfRefBase* obj;

        /// Whether the \c TfRefPtr was created or assigned to.
        TraceType type;
    };

    /// Maps a \c TfRefPtr address to the most recent trace for it.
    typedef TfHashMap<const void*, Trace, TfHash> OwnerTraces;

    /// Maps a \c TfRefBase object pointer to the number of \c TfRefPtr
    /// objects using it.  This should be the ref count on the \c TfRefBase
    /// but it's tracked separately.
    typedef TfHashMap<const TfRefBase*, size_t, TfHash> WatchedCounts;

    /// Returns the watched objects and the number of owners of each.
    /// Returns a copy for thread safety.
    TF_API
    WatchedCounts GetWatchedCounts() const;

    /// Returns traces for all owners.  Returns a copy for thread safety.
    TF_API
    OwnerTraces GetAllTraces() const;

    /// Writes all watched objects and the number of owners of each
    /// to \p stream.
    TF_API
    void ReportAllWatchedCounts(std::ostream& stream) const;

    /// Writes all traces to \p stream.
    TF_API
    void ReportAllTraces(std::ostream& stream) const;

    /// Writes traces for all owners of \p watched.
    TF_API
    void ReportTracesForWatched(std::ostream& stream,
                                const TfRefBase* watched) const;

    /// Handy function to pass as second argument to \c TF_DEFINE_REFPTR_TRACK.
    /// No objects of the type will be watched but you can watch derived types.
    /// This is important if you'll be holding TfRefPtr objects to base types;
    /// if you don't track the base types, you'll fail to track all uses of
    /// the derived objects.
    static bool WatchNone(const void*)
    {
        return false;
    }

    /// Handy function to pass as second argument to \c TF_DEFINE_REFPTR_TRACK.
    /// All objects of the type will be watched.
    static bool WatchAll(const void*)
    {
        return true;
    }

private:
    TfRefPtrTracker();
    ~TfRefPtrTracker();

    /// Start watching \p obj.  Only watched objects are traced.
    void _Watch(const TfRefBase* obj);

    /// Stop watching \p obj.  Existing traces for \p obj are kept.
    void _Unwatch(const TfRefBase* obj);

    /// Add a trace for a new owner \p owner of object \p obj if \p obj
    /// is being watched.
    void _AddTrace(const void* owner, const TfRefBase* obj, TraceType = Add);

    /// Remove traces for owner \p owner.
    void _RemoveTraces(const void* owner);

private:
    typedef std::mutex _Mutex;
    typedef std::lock_guard<std::mutex> _Lock;
    mutable _Mutex _mutex;
    size_t _maxDepth;
    WatchedCounts _watched;
    OwnerTraces _traces;

    friend class Tf_RefPtrTrackerUtil;
    friend class TfSingleton<TfRefPtrTracker>;
};

TF_API_TEMPLATE_CLASS(TfSingleton<TfRefPtrTracker>);

// For internal use only.
class Tf_RefPtrTrackerUtil {
public:
    /// Start watching \p obj.  Only watched objects are traced.
    static void Watch(const TfRefBase* obj)
    {
        TfRefPtrTracker::GetInstance()._Watch(obj);
    }

    /// Stop watching \p obj.  Existing traces for \p obj are kept.
    static void Unwatch(const TfRefBase* obj)
    {
        TfRefPtrTracker::GetInstance()._Unwatch(obj);
    }

    /// Add a trace for a new owner \p owner of object \p obj if \p obj
    /// is being watched.
    static void AddTrace(const void* owner, const TfRefBase* obj,
                         TfRefPtrTracker::TraceType type = TfRefPtrTracker::Add)
    {
        TfRefPtrTracker::GetInstance()._AddTrace(owner, obj, type);
    }

    /// Remove traces for owner \p owner.
    static void RemoveTraces(const void* owner)
    {
        TfRefPtrTracker::GetInstance()._RemoveTraces(owner);
    }
};

#define TF_DECLARE_REFPTR_TRACK(T)                                          \
inline void Tf_RefPtrTracker_FirstRef(const void*, T* obj);                 \
inline void Tf_RefPtrTracker_LastRef(const void*, T* obj);                  \
inline void Tf_RefPtrTracker_New(const void* owner, T* obj);                \
inline void Tf_RefPtrTracker_Delete(const void* owner, T* obj);             \
inline void Tf_RefPtrTracker_Assign(const void* owner, T* obj, T* oldObj);

#define TF_DEFINE_REFPTR_TRACK(T, COND)                                     \
inline void Tf_RefPtrTracker_FirstRef(const void*, T* obj) {                \
    if (obj && COND(obj)) Tf_RefPtrTrackerUtil::Watch(obj);                \
}                                                                           \
inline void Tf_RefPtrTracker_LastRef(const void*, T* obj) {                 \
    Tf_RefPtrTrackerUtil::Unwatch(obj);                                     \
}                                                                           \
inline void Tf_RefPtrTracker_New(const void* owner, T* obj) {               \
    Tf_RefPtrTrackerUtil::AddTrace(owner, obj);                             \
}                                                                           \
inline void Tf_RefPtrTracker_Delete(const void* owner, T* obj) {            \
    Tf_RefPtrTrackerUtil::RemoveTraces(owner);                              \
}                                                                           \
inline void Tf_RefPtrTracker_Assign(const void* owner, T* obj, T* oldObj) { \
    if (oldObj != obj) {                                                    \
        Tf_RefPtrTrackerUtil::AddTrace(owner, obj, TfRefPtrTracker::Assign);\
    }                                                                       \
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
