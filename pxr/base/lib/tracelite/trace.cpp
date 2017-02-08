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
#include "pxr/pxr.h"
#include "pxr/base/tracelite/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

bool Tracelite_ScopeAuto::_active = false;
static TraceliteInitializeFunction _initializeFunction = NULL;
TraceliteBeginFunction Tracelite_ScopeAuto::_beginFunction = NULL;
TraceliteEndFunction Tracelite_ScopeAuto::_endFunction = NULL;

void TraceliteSetFunctions(TraceliteInitializeFunction initializeFunction,
			   TraceliteBeginFunction beginFunction,
			   TraceliteEndFunction endFunction)
{
    _initializeFunction = initializeFunction;
    Tracelite_ScopeAuto::_beginFunction = beginFunction;
    Tracelite_ScopeAuto::_endFunction = endFunction;
}

int TraceliteEnable(bool state)
{
    static int counter = 0;

    if (_initializeFunction) {
	if (state)
	    counter++;
	else
	    counter--;
	
	Tracelite_ScopeAuto::_active = counter > 0;
    }

    return counter;
}

void
Tracelite_ScopeAuto::_Initialize(std::atomic<TraceScopeHolder*>* siteData,
                                 const std::string& key)
{
    if (!*siteData)
        (*_initializeFunction)(siteData, &key, NULL, NULL);
}

void
Tracelite_ScopeAuto::_Initialize(std::atomic<TraceScopeHolder*>* siteData,
                                 char const* key1, char const* key2)
{
    if (!*siteData)
	(*_initializeFunction)(siteData, NULL, key1, key2);
}

PXR_NAMESPACE_CLOSE_SCOPE
