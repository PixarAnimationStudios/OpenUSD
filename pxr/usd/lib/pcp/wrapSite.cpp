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

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

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

void wrapSite()
{    
    class_<PcpSite>
        ("Site", "", no_init)
        .add_property("layerStack", 
                      make_getter(&PcpSite::layerStackIdentifier,
                                  return_value_policy<return_by_value>()),
                      make_setter(&PcpSite::layerStackIdentifier))
        .add_property("path", 
                      make_getter(&PcpSite::path,
                                  return_value_policy<return_by_value>()),
                      make_setter(&PcpSite::path))
        .def("__str__", &_PcpSiteStr)
        ;

    class_<PcpLayerStackSite>
        ("LayerStackSite", "", no_init)
        .add_property("layerStack", 
                      make_getter(&PcpLayerStackSite::layerStack,
                                  return_value_policy<return_by_value>()),
                      make_setter(&PcpLayerStackSite::layerStack))
        .add_property("path", 
                      make_getter(&PcpLayerStackSite::path,
                                  return_value_policy<return_by_value>()),
                      make_setter(&PcpLayerStackSite::path))
        .def("__str__", &_PcpLayerStackSiteStr)
        ;
}

PXR_NAMESPACE_CLOSE_SCOPE
