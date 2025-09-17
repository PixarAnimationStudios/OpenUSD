//
// Copyright 2025 Pixar
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
#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/discoveryPlugin.h"
#include "pxr/usd/sdr/filesystemDiscovery.h"

#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;
using namespace TfPyContainerConversions;

namespace {

static _SdrFilesystemDiscoveryPluginRefPtr New()
{
    return TfCreateRefPtr(
        new _SdrFilesystemDiscoveryPlugin()
    );
}

static
_SdrFilesystemDiscoveryPluginRefPtr
NewWithFilter(_SdrFilesystemDiscoveryPlugin::Filter filter)
{
    return TfCreateRefPtr(
        new _SdrFilesystemDiscoveryPlugin(std::move(filter))
    );
}

// This is testing discovery from Python.  We need a discovery context
// but Python can't normally create one.  We implement a dummy context
// for just that purpose.
class _SdrContext : public SdrDiscoveryPluginContext {
public:
    ~_SdrContext() override = default;

    TfToken GetSourceType(const TfToken& discoveryType) const override
    {
        return discoveryType;
    }

    static TfRefPtr<_SdrContext> New()
    {
        return TfCreateRefPtr(new _SdrContext);
    }
};

void wrapFilesystemDiscoveryContext()
{
    typedef _SdrContext This;
    typedef TfWeakPtr<_SdrContext> ThisPtr;

    class_<This, ThisPtr, bases<SdrDiscoveryPluginContext>, noncopyable>(
        "Context", no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(This::New))
        ;
}

}

void wrapFilesystemDiscovery()
{
    typedef _SdrFilesystemDiscoveryPlugin This;
    typedef _SdrFilesystemDiscoveryPluginPtr ThisPtr;

    return_value_policy<copy_const_reference> copyRefPolicy;
    from_python_sequence<std::vector<ThisPtr>, variable_capacity_policy>();

    TfPyFunctionFromPython<bool(SdrShaderNodeDiscoveryResult&)>();

    scope s =
    class_<This, ThisPtr, bases<SdrDiscoveryPlugin>, noncopyable>(
        "_FilesystemDiscoveryPlugin", no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(New))
        .def(TfMakePyConstructor(NewWithFilter))
        .def("DiscoverShaderNodes", &This::DiscoverShaderNodes,
            return_value_policy<TfPySequenceToList>())
        .def("GetSearchURIs", &This::GetSearchURIs, copyRefPolicy)
        ;

    wrapFilesystemDiscoveryContext();
}
