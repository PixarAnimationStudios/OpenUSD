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
#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/hash.h"

#include "pxr/base/tf/hashmap.h"
#include <algorithm>
#include <iostream>
#include <mutex>

using std::string;
using std::sort;
using std::vector;

typedef TfHashMap<string, TfStopwatch*, TfHash> NameMap;

static NameMap* nameMap = 0;
static std::mutex mapMutex;

TfStopwatch::TfStopwatch(const string& name, bool share)
    : _nTicks(0)
    , _name(name)
    , _shared(share)
{
    Reset();

    if (share) {
        std::lock_guard<std::mutex> lock(mapMutex);

        if (!nameMap) {
            nameMap = new NameMap();
        }

        if (nameMap->count(name) == 0) {
            nameMap->insert(make_pair(name, this));
        } else {
            // The name was already in the map.  The second one
            // is simply not shared.
            //
            _shared = false;
        }
    }
}

TfStopwatch::TfStopwatch(const TfStopwatch& other)
    : _nTicks(other._nTicks)
    , _startTick(other._startTick)
    , _sampleCount(other._sampleCount)
    , _name(other._name)
    , _shared(false)
{
    // Nothing
}

TfStopwatch& TfStopwatch::operator=(const TfStopwatch& other)
{
    if (_shared) {
        // If this stopwatch was shared, it will no longer be shared
        // when it becomes a copy of other
        //
        std::lock_guard<std::mutex> lock(mapMutex);
        nameMap->erase(_name);
    }

    _nTicks = other._nTicks;
    _startTick = other._startTick;
    _sampleCount = other._sampleCount;
    _name = other._name;
    _shared = false;

    return *this;
}

/*virtual*/
TfStopwatch::~TfStopwatch()
{
    if (_shared) {
        std::lock_guard<std::mutex> lock(mapMutex);
        nameMap->erase(_name);
    }
}

/*static*/
TfStopwatch TfStopwatch::GetNamedStopwatch(const std::string& name)
{
    if (!nameMap) {
        // There is no nameMap and so there is obviously no stopwatch
        // with the given name.
        //
        return TfStopwatch();
    }

    TfStopwatch result;

    // Make sure that the stopwatch does not disappear out from under us.
    //
    std::lock_guard<std::mutex> lock(mapMutex);

    NameMap::iterator iter = nameMap->find(name);

    if (iter != nameMap->end()) {
        result = *(iter->second);
    }

    return result;
}

/*static*/
vector<string> TfStopwatch::GetStopwatchNames()
{
    vector<string> result;

    if (!nameMap) {
        // There is no nameMap so just return an empty vector
        //
        return result;
    }

    // Lock the map while we traverse it.
    //
    std::lock_guard<std::mutex> lock(mapMutex);

    result.reserve(nameMap->size());

    NameMap::iterator iter;
    for (iter = nameMap->begin(); iter != nameMap->end(); ++iter) {
        result.push_back(iter->first);
    }

    sort(result.begin(), result.end());

    return result;
}


std::ostream &
operator<<(std::ostream& out, const TfStopwatch& s)
{
    return out << s.GetSeconds() << " seconds";
}
