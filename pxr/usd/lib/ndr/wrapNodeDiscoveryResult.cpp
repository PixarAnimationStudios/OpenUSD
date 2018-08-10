//
// Copyright 2018 Pixar
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
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/ndr/nodeDiscoveryResult.h"

#include <boost/python.hpp>

namespace bp = boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static
std::string
_Repr(const NdrNodeDiscoveryResult& x)
{
    return TF_PY_REPR_PREFIX +
        TfStringPrintf("NodeDiscoveryResult(%s, %s, %s, %s, %s, %s, %s, %s%s%s)",
                       TfPyRepr(x.identifier).c_str(),
                       TfPyRepr(x.version).c_str(),
                       TfPyRepr(x.name).c_str(),
                       TfPyRepr(x.family).c_str(),
                       TfPyRepr(x.discoveryType).c_str(),
                       TfPyRepr(x.sourceType).c_str(),
                       TfPyRepr(x.uri).c_str(),
                       TfPyRepr(x.resolvedUri).c_str(),
                       x.blindData.empty() ? "" :", ",
                       x.blindData.empty() ? "" :TfPyRepr(x.blindData).c_str());
}

// XXX: WBN if Tf provided this sort of converter for stl maps.
template <typename MAP>
struct MapConverter
{
    typedef MAP Map;
    typedef typename Map::key_type Key;
    typedef typename Map::mapped_type Value;

    MapConverter()
    {
        boost::python::type_info info = boost::python::type_id<Map>();
        boost::python::converter::registry::push_back(&convertible, &construct, 
                                                      info);

        const boost::python::converter::registration* reg =
                boost::python::converter::registry::query(info);
        if (reg == NULL || reg->m_to_python == NULL) {
            boost::python::to_python_converter<Map, MapConverter<Map>>();
        }
    }
    static PyObject* convert(const Map& map)
    {
        boost::python::dict result;
        for (const auto& entry : map) {
            result[entry.first] = entry.second;
        }
        return boost::python::incref(result.ptr());
    }

    static void* convertible(PyObject* obj_ptr)
    {
        if (!PyDict_Check(obj_ptr)) {
            return nullptr;
        }

        boost::python::dict map = boost::python::extract<boost::python::dict>(
                obj_ptr);
        boost::python::list keys = map.keys();
        boost::python::list values = map.values();
        for (int i = 0; i < len(keys); ++i) {

            boost::python::object keyObj = keys[i];
            if (!boost::python::extract<Key>(keyObj).check()) {
                return nullptr;
            }

            boost::python::object valueObj = values[i];
            if (!boost::python::extract<Value>(valueObj).check()) {
                return nullptr;
            }
        }

        return obj_ptr;
    }
    static void construct(PyObject* obj_ptr,
        boost::python::converter::rvalue_from_python_stage1_data* data)
    {
        void* storage = (
                (boost::python::converter::rvalue_from_python_storage<Map>*)data
            )->storage.bytes;
        new (storage)Map();
        data->convertible = storage;

        Map& result = *((Map*)storage);

        boost::python::dict map = 
                boost::python::extract<boost::python::dict>(obj_ptr);
        boost::python::list keys = map.keys();
        boost::python::list values = map.values();
        for (int i = 0; i < len(keys); ++i) {

            boost::python::object keyObj = keys[i];
            boost::python::object valueObj = values[i];
            result.emplace(boost::python::extract<Key>(keyObj),
                           boost::python::extract<Value>(valueObj));
        }
    }
};

}

void wrapNodeDiscoveryResult()
{
    MapConverter<NdrTokenMap>();

    using namespace boost::python;

    typedef NdrNodeDiscoveryResult This;
    class_<This>("NodeDiscoveryResult", no_init)
        .def(init<NdrIdentifier, NdrVersion, std::string, TfToken, TfToken, 
                  TfToken, std::string, std::string, std::string,
                  NdrTokenMap, std::string>(
                  (arg("identifier"), arg("version"), arg("name"), 
                   arg("family"), arg("discoveryType"), arg("sourceType"), 
                   arg("uri"), arg("resolvedUri"), 
                   arg("sourceCode")=std::string(), 
                   arg("metadata")=NdrTokenMap(),
                   arg("blindData")=std::string())))
        .add_property("identifier", &This::identifier)
        .add_property("version", &This::version)
        .add_property("name", &This::name)
        .add_property("family", &This::family)
        .add_property("discoveryType", &This::discoveryType)
        .add_property("sourceType", &This::sourceType)
        .add_property("uri", &This::uri)
        .add_property("resolvedUri", &This::resolvedUri)
        .add_property("sourceCode", &This::sourceCode)
        .add_property("metadata", &This::metadata)
        .add_property("blindData", &This::blindData)
        .def("__repr__", _Repr)
        ;

    TfPyContainerConversions::from_python_sequence<
        std::vector<This>,
        TfPyContainerConversions::variable_capacity_policy >();
    boost::python::to_python_converter<
        std::vector<This>, 
        TfPySequenceToPython<std::vector<This> > >();
}
