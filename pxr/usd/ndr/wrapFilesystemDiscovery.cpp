//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;
using namespace TfPyContainerConversions;

namespace {

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

    class_<This, ThisPtr, bases<NdrDiscoveryPluginContext>, noncopyable>(
        "Context", no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(This::New))
        ;
}

}

void wrapFilesystemDiscovery()
{
    typedef _NdrFilesystemDiscoveryPlugin This;
    typedef _NdrFilesystemDiscoveryPluginPtr ThisPtr;

    return_value_policy<copy_const_reference> copyRefPolicy;
    from_python_sequence<std::vector<ThisPtr>, variable_capacity_policy>();

    TfPyFunctionFromPython<bool(NdrNodeDiscoveryResult&)>();

    scope s =
    class_<This, ThisPtr, bases<NdrDiscoveryPlugin>, noncopyable>(
        "_FilesystemDiscoveryPlugin", no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(New))
        .def(TfMakePyConstructor(NewWithFilter))
        .def("DiscoverNodes", &This::DiscoverNodes,
            return_value_policy<TfPySequenceToList>())
        .def("GetSearchURIs", &This::GetSearchURIs, copyRefPolicy)
        ;

    wrapFilesystemDiscoveryContext();
}
