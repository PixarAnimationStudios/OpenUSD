//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderNodeQuery.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

static TfTokenVector
GetKeys(const SdrShaderNodeQueryResult& result) {
    return result.GetKeys();
}

static std::vector<std::vector<VtValue>>
GetValues(const SdrShaderNodeQueryResult& result) {
    return result.GetValues();
}

/// Without this wrapper to call `ptr(node)`, Boost Python will attempt to
/// copy the SdrShaderNodeConstPtr argument by-value as a SdrShaderNode.
/// The same by-value copy attempt happens if we declare the `customFn`
/// argument as type `SdrShaderNodeQuery::FilterFn` (same as
/// `std::function<bool (SdrShaderNodeConstPtr)>`) directly. Declaring this
/// argument as a std::function allows TfPyFunctionFromPython to handle
/// function lifetimes and error translation.
SdrShaderNodeQuery&
CustomFilterWrapper(
    SdrShaderNodeQuery& query,
    nb::callable customFn)
{
    SdrShaderNodeQuery::FilterFn fn = [customFn](SdrShaderNodeConstPtr node) {
        TfPyLock lock;
        try {
            nb::object res = customFn(nb::cast(node, nb::rv_policy::reference));
            bool out = false;
            if (!nb::try_cast<bool>(res, out)) {
                PyErr_SetString(PyExc_TypeError, "Expected bool return value");
                TfPyConvertPythonExceptionToTfErrors();
                return false;
            }
            return out;
        } catch (nb::python_error &e) {
            e.restore();
            TfPyConvertPythonExceptionToTfErrors();
            return false;
        }
    };
    return query.CustomFilter(fn);
}

} // anonymous namespace

void wrapShaderNodeQuery(nanobind::module_ &m)
{
    nb::class_<SdrShaderNodeQuery>(m, "ShaderNodeQuery")
        .def(nb::init<>())
        .def("SelectDistinct", (SdrShaderNodeQuery&
                (SdrShaderNodeQuery::*) (const TfToken&))
             &SdrShaderNodeQuery::SelectDistinct, nb::rv_policy::reference)
        .def("SelectDistinct", (SdrShaderNodeQuery&
                (SdrShaderNodeQuery::*) (const std::vector<TfToken>&))
             &SdrShaderNodeQuery::SelectDistinct, nb::rv_policy::reference)
        .def("NodeValueIs", (SdrShaderNodeQuery&
                (SdrShaderNodeQuery::*) (const TfToken&, const VtValue&))
             &SdrShaderNodeQuery::NodeValueIs, nb::rv_policy::reference)
        .def("NodeValueIsIn", &SdrShaderNodeQuery::NodeValueIsIn,
             nb::rv_policy::reference)
        .def("NodeHasValueFor", &SdrShaderNodeQuery::NodeHasValueFor,
             nb::rv_policy::reference)
        .def("NodeValueIsNot", (SdrShaderNodeQuery&
                (SdrShaderNodeQuery::*) (const TfToken&, const VtValue&))
             &SdrShaderNodeQuery::NodeValueIsNot, nb::rv_policy::reference)
        .def("NodeValueIsNotIn", &SdrShaderNodeQuery::NodeValueIsNotIn,
             nb::rv_policy::reference)
        .def("NodeHasNoValueFor", &SdrShaderNodeQuery::NodeHasNoValueFor,
             nb::rv_policy::reference)
        .def("CustomFilter", &CustomFilterWrapper, nb::rv_policy::reference)
        .def("Run", &SdrShaderNodeQuery::Run);

    nb::class_<SdrShaderNodeQueryResult>(m, "ShaderNodeQueryResult")
        .def("GetKeys", GetKeys)
        .def("GetValues", GetValues)
        .def("GetStringifiedValues",
             &SdrShaderNodeQueryResult::GetStringifiedValues)
        .def("GetShaderNodesByValues",
            [](const SdrShaderNodeQueryResult& r) {
                nb::list l;
                for (const auto& nodes : r.GetShaderNodesByValues()) {
                    nb::list inner;
                    for (const auto& node : nodes) {
                        inner.append(nb::cast(node, nb::rv_policy::reference));
                    }
                    l.append(inner);
                }
                return l;
            })
        .def("GetAllShaderNodes",
            [](const SdrShaderNodeQueryResult& r) {
                nb::list l;
                for (const auto& node : r.GetAllShaderNodes()) {
                    l.append(nb::cast(node, nb::rv_policy::reference));
                }
                return l;
            });
}
