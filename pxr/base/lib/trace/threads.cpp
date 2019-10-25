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

#include "pxr/base/trace/threads.h"

#include "pxr/pxr.h"
#include "pxr/base/arch/threads.h"

#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

TraceThreadId::TraceThreadId()
{
    if (std::this_thread::get_id() == ArchGetMainThreadId()) {
        _id = "Main Thread";
    } else {
        std::ostringstream threadName;
        threadName << "Thread " << std::this_thread::get_id();
        _id = threadName.str();
    }
}

TraceThreadId::TraceThreadId(const std::string& s)
    : _id(s)
{}

bool
TraceThreadId::operator==(const TraceThreadId& rhs) const
{
    return _id == rhs._id;
}

bool
TraceThreadId::operator<(const TraceThreadId& rhs) const
{
    // Because thread ids are stored in a string, sort the shorter strings to 
    // the front of the list. This results is a numerically sorted list rather
    // than an alphabetically sorted one, assuming all the thread ids are in 
    // the form of "Thread XXX" or "XXX".
    return _id.length() != rhs._id.length() ? 
        _id.length() < rhs._id.length() : _id < rhs._id;
}

PXR_NAMESPACE_CLOSE_SCOPE