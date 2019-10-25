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
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/usd/ndr/discoveryPlugin.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static void wrapDiscoveryPluginContext()
{
    typedef NdrDiscoveryPluginContext This;
    typedef TfWeakPtr<NdrDiscoveryPluginContext> ThisPtr;

    class_<This, ThisPtr, boost::noncopyable>("DiscoveryPluginContext", no_init)
        .def(TfPyWeakPtr())
        .def("GetSourceType", pure_virtual(&This::GetSourceType))
        ;
}

void wrapDiscoveryPlugin()
{
    typedef NdrDiscoveryPlugin This;
    typedef NdrDiscoveryPluginPtr ThisPtr;

    return_value_policy<copy_const_reference> copyRefPolicy;

    class_<This, ThisPtr, boost::noncopyable>("DiscoveryPlugin", no_init)
        .def(TfPyWeakPtr())
        .def("DiscoverNodes", pure_virtual(&This::DiscoverNodes))
        .def("GetSearchURIs", pure_virtual(&This::GetSearchURIs), copyRefPolicy)
        ;

    wrapDiscoveryPluginContext();
}
