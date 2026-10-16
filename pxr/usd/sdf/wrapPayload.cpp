//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/vt/valueFromPython.h"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static string
_Repr(const SdfPayload &self)
{
    string args;
    bool useKeywordArgs = false;

    if (!self.GetAssetPath().empty()) {
        args += TfPyRepr(self.GetAssetPath());
    } else {
        useKeywordArgs = true;
    }
    if (!self.GetPrimPath().IsEmpty()) {
        args += (args.empty() ? "": ", ");
        args += (useKeywordArgs ? "primPath=" : "") +
            TfPyRepr(self.GetPrimPath());
    } else {
        useKeywordArgs = true;
    }
    if (!self.GetLayerOffset().IsIdentity()) {
        args += (args.empty() ? "": ", ");
        args += (useKeywordArgs ? "layerOffset=" : "") +
            TfPyRepr(self.GetLayerOffset());
    } else {
        useKeywordArgs = true;
    }

    return TF_PY_REPR_PREFIX + "Payload(" + args + ")";
}

static size_t __hash__(const SdfPayload &self)
{
    return TfHash()(self);
}

} // anonymous namespace 

void wrapPayload(nanobind::module_ &m)
{    
    namespace nb = nanobind;
    typedef SdfPayload This;

    nb::class_<This>( m, "Payload" )
        .def(nb::init<const string &,
                      const SdfPath &,
                      const SdfLayerOffset &>(),
             nb::arg("assetPath") = string(),
             nb::arg("primPath") = SdfPath(),
             nb::arg("layerOffset") = SdfLayerOffset())
        .def(nb::init<const This &>())

        .def_prop_ro("assetPath", &This::GetAssetPath, nb::rv_policy::copy)

        .def_prop_ro("primPath", &This::GetPrimPath, nb::rv_policy::copy)

        .def_prop_ro("layerOffset", &This::GetLayerOffset, nb::rv_policy::copy)

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def(nb::self < nb::self)
        .def(nb::self > nb::self)
        .def(nb::self <= nb::self)
        .def(nb::self >= nb::self)

        .def("__repr__", _Repr)
        .def("__hash__", __hash__)

        ;

    VtValueFromPython<SdfPayload>();

    // Register conversion for python list <-> vector<SdfPayload>
    TfPyContainerConversions::from_python_sequence<
        SdfPayloadVector,
        TfPyContainerConversions::variable_capacity_policy >();
}
