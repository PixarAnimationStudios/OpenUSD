//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/vt/valueFromPython.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

static string
_Repr(const SdfReference &self)
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
    // Always use keyword args for custom data (for readability).
    if (!self.GetCustomData().empty()) {
        args += (args.empty() ? "": ", ");
        args += "customData=" + TfPyRepr(self.GetCustomData());
    }

    return TF_PY_REPR_PREFIX + "Reference(" + args + ")";
}

static size_t __hash__(const SdfReference &self)
{
    return TfHash()(self);
}

} // anonymous namespace 

void wrapReference(nanobind::module_& m)
{    
    using This = SdfReference;

    // Register conversion for python list <-> vector<SdfReference>
    TfPyContainerConversions::from_python_sequence<
        SdfReferenceVector,
        TfPyContainerConversions::variable_capacity_policy >();

    // Note: Since we have no proxy for Sdf.Reference we wrap it as an
    //       immutable type to avoid confusion about code like this
    //       prim.referenceList.explicitItems[0].assetPath = '//pixar/test.usda'
    //       This looks like it's updating the assetPath for the prim's
    //       first explicit reference, but would instead modify a temporary
    //       Sdf.Reference object.

    nb::class_<This>(m, "Reference")
        .def(nb::init<const string &,
                      const SdfPath &,
                      const SdfLayerOffset &,
                      const VtDictionary &>(),
             nb::arg("assetPath") = string(),
             nb::arg("primPath") = SdfPath(),
             nb::arg("layerOffset") = SdfLayerOffset(),
             nb::arg("customData") = VtDictionary(0))
        .def(nb::init<const This &>())

        .def_prop_ro("assetPath", &This::GetAssetPath)
        .def_prop_ro("primPath", &This::GetPrimPath)
        .def_prop_ro("layerOffset", &This::GetLayerOffset)
        .def_prop_ro("customData", &This::GetCustomData)

        .def("IsInternal", &This::IsInternal)

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def(nb::self < nb::self)
        .def(nb::self > nb::self)
        .def(nb::self <= nb::self)
        .def(nb::self >= nb::self)

        .def("__repr__", _Repr)
        .def("__hash__", __hash__)
        ;

    VtValueFromPython<SdfReference>();
}
