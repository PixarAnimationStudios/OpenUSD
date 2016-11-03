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
#include "pxr/base/tf/flyweight.h"

#include <string>
#include <utility>

// Flyweight pools must be globally unique across the whole system.  We can't
// just put static data members in the template, since there may be one instance
// per shared library.  Instead, the global data is all stored here, in this one
// translation unit, which ensures that there will be one unique pool for each
// flyweight type.

Tf_FlyweightDataBase *
Tf_TrySetFlyweightData(std::string const &poolName, Tf_FlyweightDataBase *data)
{
    using std::string;
    using std::make_pair;
    using DataHashMap = tbb::concurrent_hash_map<
        string, Tf_FlyweightDataBase *>;

    static DataHashMap _globalDataMap;

    DataHashMap::accessor acc;
    _globalDataMap.insert(acc, make_pair(poolName, data));
    return acc->second;
}
