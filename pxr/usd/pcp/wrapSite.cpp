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
#include "pxr/usd/pcp/site.h"
#include "pxr/base/tf/stringUtils.h"
#include <boost/python.hpp>


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdPcpWrapSite {

static std::string
_PcpSiteStr(const PcpSite& x)
{
    return TfStringify(x);
}

static std::string
_PcpLayerStackSiteStr(const PcpLayerStackSite& x)
{
    return TfStringify(x);
}

} // anonymous namespace 

void wrapSite()
{    
    boost::python::class_<PcpSite>
        ("Site", "", boost::python::no_init)
        .add_property("layerStack", 
                      boost::python::make_getter(&PcpSite::layerStackIdentifier,
                                  boost::python::return_value_policy<boost::python::return_by_value>()),
                      boost::python::make_setter(&PcpSite::layerStackIdentifier))
        .add_property("path", 
                      boost::python::make_getter(&PcpSite::path,
                                  boost::python::return_value_policy<boost::python::return_by_value>()),
                      boost::python::make_setter(&PcpSite::path))
        .def("__str__", &pxrUsdPcpWrapSite::_PcpSiteStr)
        ;

    boost::python::class_<PcpLayerStackSite>
        ("LayerStackSite", "", boost::python::no_init)
        .add_property("layerStack", 
                      boost::python::make_getter(&PcpLayerStackSite::layerStack,
                                  boost::python::return_value_policy<boost::python::return_by_value>()),
                      boost::python::make_setter(&PcpLayerStackSite::layerStack))
        .add_property("path", 
                      boost::python::make_getter(&PcpLayerStackSite::path,
                                  boost::python::return_value_policy<boost::python::return_by_value>()),
                      boost::python::make_setter(&PcpLayerStackSite::path))
        .def("__str__", &pxrUsdPcpWrapSite::_PcpLayerStackSiteStr)
        ;
}
