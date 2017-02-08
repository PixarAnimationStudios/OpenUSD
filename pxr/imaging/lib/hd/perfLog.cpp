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
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/stl.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_INSTANTIATE_SINGLETON(HdPerfLog);

static
bool
_IsEnabledPerfLog()
{
    static bool isEnabledPerfLog=
        TfGetenv("HD_ENABLE_PERFLOG", "0") == "1";
    return isEnabledPerfLog;
}

HdPerfLog::HdPerfLog()
    : _enabled(_IsEnabledPerfLog())
{
    /*NOTHING*/
}

HdPerfLog::~HdPerfLog()
{
    /*NOTHING*/
}

void
HdPerfLog::AddCacheHit(TfToken const& name,
                 SdfPath const& id,
                 TfToken const& tag)
{
    if (ARCH_LIKELY(!_enabled))
        return;
    _Lock lock(_mutex);
    _cacheMap[name].AddHit();
    TF_DEBUG(HD_CACHE_HITS).Msg("Cache hit: %s %s %s hits: %lu\n",
            name.GetText(),
            id.GetText(),
            tag.GetText(),
            _cacheMap[name].GetHits());
}

void
HdPerfLog::AddCacheMiss(TfToken const& name,
                  SdfPath const& id,
                  TfToken const& tag)
{
    if (ARCH_LIKELY(!_enabled))
        return;
    _Lock lock(_mutex);
    _cacheMap[name].AddMiss();
    TF_DEBUG(HD_CACHE_MISSES).Msg("Cache miss: %s %s %s Total misses: %lu\n",
            name.GetText(),
            id.GetText(),
            tag.GetText(),
            _cacheMap[name].GetMisses());
}

void
HdPerfLog::ResetCache(TfToken const& name)
{
    if (ARCH_LIKELY(!_enabled))
        return;
    _Lock lock(_mutex);
    _cacheMap[name].Reset();
}
 
double
HdPerfLog::GetCacheHitRatio(TfToken const& name)
{
    _Lock lock(_mutex);
    if (HdPerfLog::_CacheEntry* value = TfMapLookupPtr(_cacheMap, name)) {
        return value->GetHitRatio();
    }
    return 0.0;
}

size_t
HdPerfLog::GetCacheHits(TfToken const& name)
{
    _Lock lock(_mutex);
    if (HdPerfLog::_CacheEntry* value = TfMapLookupPtr(_cacheMap, name))
        return value->GetHits();
    return 0;
}

size_t
HdPerfLog::GetCacheMisses(TfToken const& name)
{
    _Lock lock(_mutex);
    if (HdPerfLog::_CacheEntry* value = TfMapLookupPtr(_cacheMap, name))
        return value->GetMisses();
    return 0;
}

TfTokenVector
HdPerfLog::GetCacheNames()
{
    _Lock lock(_mutex);
    TfTokenVector names;
    names.reserve(_cacheMap.size());
    TF_FOR_ALL(tokCacheIt, _cacheMap) {
        names.push_back(tokCacheIt->first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

TfTokenVector 
HdPerfLog::GetCounterNames()
{
    _Lock lock(_mutex);
    TfTokenVector names;
    names.reserve(_counterMap.size());
    TF_FOR_ALL(it, _counterMap) {
        names.push_back(it->first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

void
HdPerfLog::IncrementCounter(TfToken const& name)
{
    if (ARCH_LIKELY(!_enabled)) 
        return;
    _Lock lock(_mutex);
    TF_DEBUG(HD_COUNTER_CHANGED).Msg("Counter changed %s: %f -> %f\n",
            name.GetText(),
            _counterMap[name],
            _counterMap[name] + 1.0);
    _counterMap[name] += 1.0;
}

void
HdPerfLog::DecrementCounter(TfToken const& name)
{
    if (ARCH_LIKELY(!_enabled)) 
        return;
    _Lock lock(_mutex);
    TF_DEBUG(HD_COUNTER_CHANGED).Msg("Counter changed %s: %f -> %f\n",
            name.GetText(),
            _counterMap[name],
            _counterMap[name] - 1.0);
    _counterMap[name] -= 1.0;
}

void
HdPerfLog::SetCounter(TfToken const& name, double value)
{
    if (ARCH_LIKELY(!_enabled)) 
        return;
    _Lock lock(_mutex);
    TF_DEBUG(HD_COUNTER_CHANGED).Msg("Counter changed %s: %f -> %f\n",
            name.GetText(),
            _counterMap[name],
            value);
    _counterMap[name] = value;
}

void
HdPerfLog::AddCounter(TfToken const &name, double value)
{
    if (ARCH_LIKELY(!_enabled))
        return;
    _Lock lock(_mutex);
    TF_DEBUG(HD_COUNTER_CHANGED).Msg("Counter changed %s %f -> %f\n",
            name.GetText(),
            _counterMap[name],
            _counterMap[name] + value);
    _counterMap[name] += value;
}

void
HdPerfLog::SubtractCounter(TfToken const &name, double value)
{
    if (ARCH_LIKELY(!_enabled))
        return;
    _Lock lock(_mutex);
    TF_DEBUG(HD_COUNTER_CHANGED).Msg("Counter changed %s %f -> %f\n",
            name.GetText(),
            _counterMap[name],
            _counterMap[name] - value);
    _counterMap[name] -= value;
}

double
HdPerfLog::GetCounter(TfToken const& name)
{
    _Lock lock(_mutex);
    return TfMapLookupByValue(_counterMap, name, 0.0);
}

void
HdPerfLog::ResetCounters()
{
    if (ARCH_LIKELY(!_enabled))
        return;
    _Lock lock(_mutex);
    TF_FOR_ALL(counterIt, _counterMap) {
        TF_DEBUG(HD_COUNTER_CHANGED).Msg("Counter reset %s: %f -> 0\n",
                                         counterIt->first.GetText(),
                                         counterIt->second);
        counterIt->second = 0;
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

