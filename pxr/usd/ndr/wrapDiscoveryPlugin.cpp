//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
