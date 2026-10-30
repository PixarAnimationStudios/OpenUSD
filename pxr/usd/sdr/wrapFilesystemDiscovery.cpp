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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/discoveryPlugin.h"
#include "pxr/usd/sdr/filesystemDiscovery.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace TfPyContainerConversions;
namespace nb = nanobind;

namespace {

static _SdrFilesystemDiscoveryPluginRefPtr New()
{
    return TfCreateRefPtr(
        new _SdrFilesystemDiscoveryPlugin()
    );
}

static
_SdrFilesystemDiscoveryPluginRefPtr
NewWithFilter(nb::callable filter)
{
    return TfCreateRefPtr(
        new _SdrFilesystemDiscoveryPlugin(
            [filter](SdrShaderNodeDiscoveryResult& result) -> bool {
                TfPyLock lock;
                try {
                    return nb::cast<bool>(
                        filter(nb::cast(&result, nb::rv_policy::reference)));
                } catch (nb::python_error &e) {
                    e.restore();
                    TfPyConvertPythonExceptionToTfErrors();
                    return false;
                }
            })
    );
}

// This is testing discovery from Python.  We need a discovery context
// but Python can't normally create one.  We implement a dummy context
// for just that purpose.
class _SdrContext : public SdrDiscoveryPluginContext {
public:
    ~_SdrContext() override = default;

    TfToken GetShadingSystem(const TfToken& discoveryType) const override
    {
        return discoveryType;
    }

    static TfRefPtr<_SdrContext> New()
    {
        return TfCreateRefPtr(new _SdrContext);
    }
};

void wrapFilesystemDiscoveryContext(nb::handle scope)
{
    typedef _SdrContext This;

    nb::class_<This, SdrDiscoveryPluginContext> cls(
        scope, "Context",
        nb::is_weak_referenceable(), nb::dynamic_attr());
    TfNbRefAndWeakPtr(cls);
    cls.def(TfNbMakePyConstructor(This::New))
        ;
}

}

void wrapFilesystemDiscovery(nanobind::module_ &m)
{
    typedef _SdrFilesystemDiscoveryPlugin This;
    typedef _SdrFilesystemDiscoveryPluginPtr ThisPtr;

    from_python_sequence<std::vector<ThisPtr>, variable_capacity_policy>();

    TfPyFunctionFromPython<bool(SdrShaderNodeDiscoveryResult&)>();

    nb::class_<This, SdrDiscoveryPlugin> cls(
        m, "_FilesystemDiscoveryPlugin",
        nb::is_weak_referenceable(), nb::dynamic_attr());
    TfNbRefAndWeakPtr(cls);
    cls.def(TfNbMakePyConstructor(New))
        .def(TfNbMakePyConstructor(NewWithFilter))
        .def("DiscoverShaderNodes", &This::DiscoverShaderNodes)
        .def("GetSearchURIs", &This::GetSearchURIs)
        ;

    wrapFilesystemDiscoveryContext(cls);
}
