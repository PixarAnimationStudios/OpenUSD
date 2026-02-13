//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderNodeQuery.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/return_arg.hpp"
#include "pxr/external/boost/python/copy_const_reference.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
    std::function<bool (object)> customFn)
{
    SdrShaderNodeQuery::FilterFn fn = [customFn](SdrShaderNodeConstPtr node) {
        TfPyLock lock;
        return customFn(object(ptr(node)));
    };
    return query.CustomFilter(fn);
}

} // anonymous namespace

void wrapShaderNodeQuery()
{
    TfPyFunctionFromPython<bool (object)>();

    class_<SdrShaderNodeQuery>("ShaderNodeQuery")
        .def("SelectDistinct", (SdrShaderNodeQuery&
                (SdrShaderNodeQuery::*) (const TfToken&))
             &SdrShaderNodeQuery::SelectDistinct, return_self<>{})
        .def("SelectDistinct", (SdrShaderNodeQuery&
                (SdrShaderNodeQuery::*) (const std::vector<TfToken>&))
             &SdrShaderNodeQuery::SelectDistinct, return_self<>{})
        .def("NodeValueIs", (SdrShaderNodeQuery&
                (SdrShaderNodeQuery::*) (const TfToken&, const VtValue&))
             &SdrShaderNodeQuery::NodeValueIs, return_self<>{})
        .def("NodeValueIsIn", &SdrShaderNodeQuery::NodeValueIsIn,
             return_self<>{})
        .def("NodeHasValueFor", &SdrShaderNodeQuery::NodeHasValueFor,
             return_self<>{})
        .def("NodeValueIsNot", (SdrShaderNodeQuery&
                (SdrShaderNodeQuery::*) (const TfToken&, const VtValue&))
             &SdrShaderNodeQuery::NodeValueIsNot, return_self<>{})
        .def("NodeValueIsNotIn", &SdrShaderNodeQuery::NodeValueIsNotIn,
             return_self<>{})
        .def("NodeHasNoValueFor", &SdrShaderNodeQuery::NodeHasNoValueFor,
             return_self<>{})
        .def("CustomFilter", &CustomFilterWrapper, return_self<>{})
        .def("Run", &SdrShaderNodeQuery::Run);

    class_<SdrShaderNodeQueryResult>("ShaderNodeQueryResult", no_init)
        .def("GetKeys", GetKeys)
        .def("GetValues", GetValues, return_value_policy<TfPySequenceToList>())
        .def("GetStringifiedValues",
             &SdrShaderNodeQueryResult::GetStringifiedValues,
             return_value_policy<TfPySequenceToList>())
        .def("GetShaderNodesByValues",
            +[](const SdrShaderNodeQueryResult& r) {
                list l;
                for (const auto& nodes : r.GetShaderNodesByValues()) {
                    list inner;
                    for (const auto& node : nodes) {
                        inner.append(ptr(node));
                    }
                    l.append(inner);
                }
                return l;
            })
        .def("GetAllShaderNodes",
            +[](const SdrShaderNodeQueryResult& r) {
                list l;
                for (const auto& node :r.GetAllShaderNodes()) {
                    l.append(ptr(node));
                }
                return l;
            });
}
