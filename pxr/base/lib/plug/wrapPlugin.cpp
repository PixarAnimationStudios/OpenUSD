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
#include <boost/python.hpp>

#include "pxr/base/plug/plugin.h"

#include "pxr/base/js/converter.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/iterator.h"

#include <string>

using namespace boost::python;
using std::string;
using std::vector;

static dict
_ConvertDict( const JsObject & dictionary )
{
    dict result;
    TF_FOR_ALL(i, dictionary) {
        const string & key = i->first;
        const JsValue & val = i->second;

        result[key] = JsConvertToContainerType<object, dict>(val);
    }
    return result;
}

static dict
_GetMetadata(PlugPluginPtr plugin)
{
    return _ConvertDict(plugin->GetMetadata());
}

static dict
_GetMetadataForType(PlugPluginPtr plugin, const TfType &type)
{
    return _ConvertDict(plugin->GetMetadataForType(type));
}

void wrapPlugin()
{
    typedef PlugPlugin This;
    typedef PlugPluginPtr ThisPtr;

    class_<This, ThisPtr, boost::noncopyable> ( "Plugin", no_init )
        .def(TfPyWeakPtr())
        .def("Load", &This::Load)

        .add_property("isLoaded", &This::IsLoaded)
        .add_property("isPythonModule", &This::IsPythonModule)
        .add_property("isResource", &This::IsResource)

        .add_property("metadata", ::_GetMetadata)

        .add_property("name",
                      make_function(&This::GetName,
                                    return_value_policy<return_by_value>()))
        .add_property("path",
                      make_function(&This::GetPath,
                                    return_value_policy<return_by_value>()))
        .add_property("resourcePath",
                      make_function(&This::GetResourcePath,
                                    return_value_policy<return_by_value>()))

        .def("GetMetadataForType", ::_GetMetadataForType)
        .def("DeclaresType", &This::DeclaresType,
             (arg("type"), 
              arg("includeSubclasses") = false))

        .def("MakeResourcePath", &This::MakeResourcePath)
        .def("FindResource", &This::FindResource,
             (arg("path"), 
              arg("verify") = true))
        ;

    // The call to JsConvertToContainerType in _ConvertDict creates
    // vectors of boost::python::objects for array values, so register
    // a converter that turns that vector into a Python list.
    boost::python::to_python_converter<std::vector<object>,
        TfPySequenceToPython<std::vector<object> > >();
}

TF_REFPTR_CONST_VOLATILE_GET(PlugPlugin)
