//
// Copyright 2018 Pixar
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

#ifndef TRACE_THREADS_H
#define TRACE_THREADS_H

#include "pxr/pxr.h"
#include "pxr/base/trace/api.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
/// \class ThraceThreadId
///
/// This class reperesents an identifier for a thread.
///
class TraceThreadId {
public:
    /// Contructor which creates an identifier based on std::thread_id. .
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

#endif // TRACE_THREADS_H
