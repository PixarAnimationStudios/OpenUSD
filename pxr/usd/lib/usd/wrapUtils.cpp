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
#include "pxr/usd/usd/wrapUtils.h"

using namespace boost::python;

static converter::to_python_function_t originalConverters[Usd_NumObjTypes];
static Usd_ObjectSubclass::DowncastFn downcastFns[Usd_NumObjTypes];

PyObject *
Usd_ObjectSubclass::_ConvertHelper(const UsdObject *obj)
{
    // Look up the cast function and conversion function based on dynamic type.
    // Run the cast function to get the pointer that the conversion function
    // expects, then invoke it to do the conversion.
    UsdObjType dynType = Usd_GetObjType(*obj);
    return originalConverters[dynType](downcastFns[dynType](obj));
}

void
Usd_ObjectSubclass::_ReplaceConverter(boost::python::type_info pti,
                                      UsdObjType objType,
                                      converter::to_python_function_t convert,
                                      DowncastFn downcast)
{
    // Store c++ downcast function.
    downcastFns[objType] = downcast;

    // Save boost.python's original converter for this type, and replace it with
    // the downcasting \a convert.
    converter::registration *reg =
        const_cast<converter::registration *>(converter::registry::query(pti));
    originalConverters[objType] = reg->m_to_python;
    reg->m_to_python = convert;
}
