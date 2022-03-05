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
/// \file wrapLayerStackIdentifier.cpp

#include "pxr/pxr.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/python.hpp>


PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static
std::string
_Repr(const PcpLayerStackIdentifier& x)
{
    return TfStringPrintf("%sLayerStackIdentifier(%s, %s, %s)",
                          TF_PY_REPR_PREFIX.c_str(),
                          TfPyRepr(x.rootLayer).c_str(),
                          TfPyRepr(x.sessionLayer).c_str(),
                          TfPyRepr(x.pathResolverContext).c_str());
}

} // anonymous namespace 

void wrapLayerStackIdentifier()
{
    typedef PcpLayerStackIdentifier This;

    boost::python::class_<This>("LayerStackIdentifier")
        .def(boost::python::init<>())
        .def(boost::python::init<
             const SdfLayerHandle &,
             const SdfLayerHandle &,
             const ArResolverContext & >
             ((boost::python::args("rootLayer"),
               boost::python::args("sessionLayer") = SdfLayerHandle(),
               boost::python::args("pathResolverContext") = ArResolverContext())))

        .add_property("sessionLayer", 
                      boost::python::make_getter(&This::sessionLayer, 
                                  boost::python::return_value_policy<boost::python::return_by_value>()))
        .add_property("rootLayer", 
                      boost::python::make_getter(&This::rootLayer, 
                                  boost::python::return_value_policy<boost::python::return_by_value>()))
        .add_property("pathResolverContext", 
                      boost::python::make_getter(&This::pathResolverContext, 
                                  boost::python::return_value_policy<boost::python::return_by_value>()))

        .def("__repr__", &_Repr)
        .def("__hash__", &This::GetHash)
        .def(!boost::python::self)
        .def(boost::python::self == boost::python::self)
        .def(boost::python::self != boost::python::self)
        .def(boost::python::self <  boost::python::self)
        .def(boost::python::self <= boost::python::self)
        .def(boost::python::self >  boost::python::self)
        .def(boost::python::self >= boost::python::self)
        ;
}
