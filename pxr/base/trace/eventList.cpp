//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/trace/eventList.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TraceEventList::TraceEventList()
{
    // Make sure the list always has at least one set in it.
    _caches.emplace_back();
}

void TraceEventList::Append(TraceEventList&& other)
{
    // We use splice to keep the keys in the same memory location since the 
    // events reference dynamic key by pointer.
    _caches.splice(_caches.end(), std::move(other._caches));
    _events.Append(std::move(other._events));
}

 PXR_NAMESPACE_CLOSE_SCOPE
