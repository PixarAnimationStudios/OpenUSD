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
/// \file refPtrTracker.h

#include "pxr/base/tf/refPtrTracker.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/stackTrace.h"
#include <ostream>

// Helper function for reporting.
static
std::string
_GetDemangled(const TfRefBase* watched)
{
    return watched ? ArchGetDemangled(typeid(*watched)) :
                     std::string("<unknown>");
}

// Trace type to string table.
static const char* _type[] = { "Add", "Assign" };

// The number of levels in stack traces that are for TfRefPtrTracker itself.
// Currently these are:
//   ArchGetStackFrames()
//   TfRefPtrTracker::AddTrace()
static const size_t _NumInternalStackLevels = 2;

TF_INSTANTIATE_SINGLETON(TfRefPtrTracker);

TfRefPtrTracker::TfRefPtrTracker() : _maxDepth(20)
{
    // Do nothing
}

TfRefPtrTracker::~TfRefPtrTracker()
{
    // Do nothing
}

void
TfRefPtrTracker::_Watch(const TfRefBase* obj)
{
    _Lock lock(_mutex);

    // We're now watching the TfRefBase at obj but there are no TfRefPtr
    // objects using it yet.
    _watched.insert(std::make_pair(obj, (size_t)0));
}

void
TfRefPtrTracker::_Unwatch(const TfRefBase* obj)
{
    _Lock lock(_mutex);

    // Stop watching the TfRefBase at obj.
    _watched.erase(obj);
}

void
TfRefPtrTracker::_AddTrace(
    const void* owner,
    const TfRefBase* obj,
    TraceType type)
{
    _Lock lock(_mutex);

    // The owner (TfRefPtr) is no longer pointing to the TfRefBase it
    // had been pointing to.  Decrement the number of uses of that
    // TfRefBase.  If we can't find the owner then it wasn't pointing
    // to a TfRefBase we were watching or it was just created (and so
    // isn't pointing to a TfRefBase we're watching).  If we can't find
    // the use count then we're no longer watching the object;  this
    // shouldn't happen.
    OwnerTraces::iterator i = _traces.find(owner);
    if (i != _traces.end()) {
        // Stop watching previous object.
        WatchedCounts::iterator j = _watched.find(i->second.obj);
        if (j != _watched.end()) {
            --j->second;
        }
    }

    // See if the new TfRefBase is being watched.
    WatchedCounts::iterator j = _watched.find(obj);
    if (j != _watched.end()) {
        // TfRefBase is being watched.  Increment the number of uses of
        // that TfRefBase.
        ++j->second;

        // Grab a stack trace and save it, along with the new object being
        // watched and whether this is a new owner or an assignment to an
        // existing owner.
        Trace& trace = _traces[owner];
        ArchGetStackFrames(_maxDepth, _NumInternalStackLevels, &trace.trace);
        trace.obj  = obj;
        trace.type = type;
    }

    else if (i != _traces.end()) {
        // We assigned a TfRefBase that we're not watching.  This owner is
        // no longer relevant so discard it.
        _traces.erase(i);
    }
}

void
TfRefPtrTracker::_RemoveTraces(const void* owner)
{
    _Lock lock(_mutex);

    // See if we have this owner.
    OwnerTraces::iterator i = _traces.find(owner);
    if (i != _traces.end()) {
        // We have the owner.  Decrement the number of uses of the object
        // it's pointing to.  If we can't find the use count then we're
        // no longer watching that object;  this shouldn't happen.
        WatchedCounts::iterator j = _watched.find(i->second.obj);
        if (j != _watched.end()) {
            --j->second;
        }

        // Discard the owner.
        _traces.erase(i);
    }
}

TfRefPtrTracker::WatchedCounts
TfRefPtrTracker::GetWatchedCounts() const
{
    _Lock lock(_mutex);
    return _watched;
}

TfRefPtrTracker::OwnerTraces
TfRefPtrTracker::GetAllTraces() const
{
    _Lock lock(_mutex);
    return _traces;
}

void
TfRefPtrTracker::ReportAllWatchedCounts(std::ostream& stream) const
{
    stream << "TfRefPtrTracker watched counts:" << std::endl;
    TF_FOR_ALL(i, _watched) {
        stream << "  " << i->first << ": " << i->second
               << " (type " << _GetDemangled(i->first) << ")"
               << std::endl;
    }
}

void
TfRefPtrTracker::ReportAllTraces(std::ostream& stream) const
{
    stream << "TfRefPtrTracker traces:" << std::endl;
    _Lock lock(_mutex);
    TF_FOR_ALL(i, _traces) {
        const Trace& trace = i->second;
        stream << "  Owner: " << i->first
               << " " << _type[trace.type] << " " << trace.obj << ":"
               << std::endl;
        stream << "=============================================================="
               << std::endl;
        ArchPrintStackFrames(stream, trace.trace);
        stream << std::endl;
    }
}

void
TfRefPtrTracker::ReportTracesForWatched(
    std::ostream& stream,
    const TfRefBase* watched) const
{
    _Lock lock(_mutex);

    // Report if not watched.
    if (_watched.find(watched) == _watched.end()) {
        stream << "TfRefPtrTracker traces for " << watched
               << ":  not watched" << std::endl;
        return;
    }

    // Report what watched really is.
    stream << "TfRefPtrTracker traces for " << watched
           << " (type " << _GetDemangled(watched) << ")" << std::endl;

    // Loop over all traces and report the ones that are watching watched.
    TF_FOR_ALL(i, _traces) {
        const Trace& trace = i->second;
        if (trace.obj == watched) {
            stream << "  Owner: " << i->first
                   << " " << _type[trace.type] << ":"
                   << std::endl;
            stream << "=============================================================="
                   << std::endl;
            ArchPrintStackFrames(stream, trace.trace);
            stream << std::endl;
        }
    }

    stream << "=============================================================="
           << std::endl;
}
