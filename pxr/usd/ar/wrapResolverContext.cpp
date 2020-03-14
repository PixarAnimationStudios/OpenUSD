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
#include <boost/python/to_python_converter.hpp>
#include <boost/python/object.hpp>

#include "pxr/pxr.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/ar/pyResolverContext.h"

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

struct Ar_ResolverContextToPython
{
    Ar_ResolverContextToPython()
    {
        to_python_converter<
            ArResolverContext, Ar_ResolverContextToPython>();
    }

    static PyObject* convert(const ArResolverContext& context)
    {
        return incref(Ar_ConvertResolverContextToPython(context).ptr());
    }
};

struct Ar_ResolverContextFromPython
{
    Ar_ResolverContextFromPython() 
    {
        converter::registry::push_back(
            &_convertible, &_construct,
            boost::python::type_id<ArResolverContext>());
    }

    static void* _convertible(PyObject* obj_ptr) 
    {
        if (obj_ptr == Py_None ||
            Ar_CanConvertResolverContextFromPython(obj_ptr)) {
            return obj_ptr;
        }
        return 0;
    }
    
    static void _construct(
        PyObject* obj_ptr, converter::rvalue_from_python_stage1_data *data) 
    {
        void *storage = ((converter::rvalue_from_python_storage<ArResolverContext>*)data)
            ->storage.bytes;

        ArResolverContext context;
        if (obj_ptr != Py_None) {
            context = Ar_ConvertResolverContextFromPython(obj_ptr);
        }

        new (storage) ArResolverContext(context);
        data->convertible = storage;
    }
};

} // anonymous namespace 

void
wrapResolverContext()
{
    Ar_ResolverContextToPython();
    Ar_ResolverContextFromPython();
}
