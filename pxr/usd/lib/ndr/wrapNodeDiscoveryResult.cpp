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

using namespace boost::python;

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

}

void wrapNodeDiscoveryResult()
{
    typedef NdrNodeDiscoveryResult This;

    class_<This>("NodeDiscoveryResult", no_init)
        .def(init<NdrIdentifier, NdrVersion, std::string, TfToken,
                  TfToken, TfToken, std::string, std::string>())
        .def(init<NdrIdentifier, NdrVersion, std::string, TfToken,
                  TfToken, TfToken, std::string, std::string, std::string>())
        .add_property("identifier", &This::identifier)
        .add_property("version", &This::version)
        .add_property("name", &This::name)
        .add_property("family", &This::family)
        .add_property("discoveryType", &This::discoveryType)
        .add_property("sourceType", &This::sourceType)
        .add_property("uri", &This::uri)
        .add_property("resolvedUri", &This::resolvedUri)
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
