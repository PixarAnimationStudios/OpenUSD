//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/wrapUtils.h"

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

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

PXR_NAMESPACE_CLOSE_SCOPE

