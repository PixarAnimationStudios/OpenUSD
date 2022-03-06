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
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
#include "pxr/usd/ndr/filesystemDiscovery.h"

#include <boost/python.hpp>

PXR_NAMESPACE_USING_DIRECTIVE
using namespace TfPyContainerConversions;

namespace pxrUsdNdrWrapFilesystemDiscovery {

static _NdrFilesystemDiscoveryPluginRefPtr New()
{
    return TfCreateRefPtr(
        new _NdrFilesystemDiscoveryPlugin()
    );
}

static
_NdrFilesystemDiscoveryPluginRefPtr
NewWithFilter(_NdrFilesystemDiscoveryPlugin::Filter filter)
{
    return TfCreateRefPtr(
        new _NdrFilesystemDiscoveryPlugin(std::move(filter))
    );
}

// This is testing discovery from Python.  We need a discovery context
// but Python can't normally create one.  We implement a dummy context
// for just that purpose.
class _Context : public NdrDiscoveryPluginContext {
public:
    ~_Context() override = default;

    TfToken GetSourceType(const TfToken& discoveryType) const override
    {
        return discoveryType;
    }

    static TfRefPtr<_Context> New()
    {
        return TfCreateRefPtr(new _Context);
    }
};

void wrapFilesystemDiscoveryContext()
{
    typedef _Context This;
    typedef TfWeakPtr<_Context> ThisPtr;

    boost::python::class_<This, ThisPtr, boost::python::bases<NdrDiscoveryPluginContext>, boost::noncopyable>(
        "Context", boost::python::no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(This::New))
        ;
}

}

void wrapFilesystemDiscovery()
{
    typedef _NdrFilesystemDiscoveryPlugin This;
    typedef _NdrFilesystemDiscoveryPluginPtr ThisPtr;

    boost::python::return_value_policy<boost::python::copy_const_reference> copyRefPolicy;
    from_python_sequence<std::vector<ThisPtr>, variable_capacity_policy>();

    TfPyFunctionFromPython<bool(NdrNodeDiscoveryResult&)>();

    boost::python::scope s =
    boost::python::class_<This, ThisPtr, boost::python::bases<NdrDiscoveryPlugin>, boost::noncopyable>(
        "_FilesystemDiscoveryPlugin", boost::python::no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(pxrUsdNdrWrapFilesystemDiscovery::New))
        .def(TfMakePyConstructor(pxrUsdNdrWrapFilesystemDiscovery::NewWithFilter))
        .def("DiscoverNodes", &This::DiscoverNodes,
            boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetSearchURIs", &This::GetSearchURIs, copyRefPolicy)
        ;

    pxrUsdNdrWrapFilesystemDiscovery::wrapFilesystemDiscoveryContext();
}
