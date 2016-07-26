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
#include "pxr/usd/ar/pyResolverContext.h"

#include "pxr/base/tf/iterator.h"

#include <vector>

class Ar_PythonConverterRegistry
{
public:
    std::vector<Ar_MakeResolverContextFromPythonFn> _convertFromPython;
    std::vector<Ar_ResolverContextToPythonFn> _convertToPython;
};

static 
Ar_PythonConverterRegistry&
_GetRegistry()
{
    static Ar_PythonConverterRegistry registry;
    return registry;
}

void
Ar_RegisterResolverContextPythonConversion(
    const Ar_MakeResolverContextFromPythonFn& convertFunc,
    const Ar_ResolverContextToPythonFn& getObjectFunc)
{
    _GetRegistry()._convertFromPython.push_back(convertFunc);
    _GetRegistry()._convertToPython.push_back(getObjectFunc);
}

bool
Ar_CanConvertResolverContextFromPython(PyObject* pyObj)
{
    Ar_PythonConverterRegistry& reg = _GetRegistry();
    TF_FOR_ALL(canMakeContextFrom, reg._convertFromPython) {
        if ((*canMakeContextFrom)(pyObj, NULL)) {
            return true;
        }
    }
    return false;
}

ArResolverContext
Ar_ConvertResolverContextFromPython(PyObject* pyObj)
{
    ArResolverContext context;

    Ar_PythonConverterRegistry& reg = _GetRegistry();
    TF_FOR_ALL(makeContextFrom, reg._convertFromPython) {
        if ((*makeContextFrom)(pyObj, &context)) {
            break;
        }
    }
    return context;
}

TfPyObjWrapper
Ar_ConvertResolverContextToPython(const ArResolverContext& context)
{
    TfPyObjWrapper pyObj;
    
    Ar_PythonConverterRegistry& reg = _GetRegistry();
    TF_FOR_ALL(convertToPython, reg._convertToPython) {
        if ((*convertToPython)(context, &pyObj)) {
            break;
        }
    }
    return pyObj;
}
