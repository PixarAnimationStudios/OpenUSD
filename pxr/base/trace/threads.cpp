//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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