//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TRACE_THREADS_H
#define PXR_BASE_TRACE_THREADS_H

#include "pxr/pxr.h"
#include "pxr/base/trace/api.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
/// \class TraceThreadId
///
/// This class represents an identifier for a thread.
///
class TraceThreadId {
public:
    /// Constructor which creates an identifier based on std::thread_id. .
    /// It is either"Main Thread" if this id is marked as the main thread or
    ///  "Thread XXX" where XXX is the string representation of the thread id.
    TRACE_API TraceThreadId();

    /// Constructor which creates an identifier from \p id.
    TRACE_API explicit TraceThreadId(const std::string& id);

    /// Returns the string representation of the id.
    const std::string& ToString() const { return _id; }

    /// Equality operator.
    TRACE_API bool operator==(const TraceThreadId&) const;

    /// Less than operator.
    TRACE_API bool operator<(const TraceThreadId&) const;
private:
    std::string _id;
};

inline TraceThreadId TraceGetThreadId() {
    return  TraceThreadId();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_THREADS_H
