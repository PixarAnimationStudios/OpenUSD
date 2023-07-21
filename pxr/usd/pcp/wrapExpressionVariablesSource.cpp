//
// Copyright 2023 Pixar
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

#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/expressionVariablesSource.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_value_policy.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void
wrapExpressionVariablesSource()
{
    using This = PcpExpressionVariablesSource;

    class_<This>("ExpressionVariablesSource")
        .def(init<>())
        .def(init<
                const PcpLayerStackIdentifier&,
                const PcpLayerStackIdentifier&>(
                (args("layerStackId"), args("rootLayerStackId"))))

        .def(self == self)
        .def(self != self)

        .def("IsRootLayerStack", &This::IsRootLayerStack)
        .def("GetLayerStackIdentifier", 
            +[](const This& s) { 
                const PcpLayerStackIdentifier* id = s.GetLayerStackIdentifier();
                return id ? object(*id) : object();
            })
        .def("ResolveLayerStackIdentifier",
            (const PcpLayerStackIdentifier& (This::*)(
                const PcpLayerStackIdentifier&) const)
                &This::ResolveLayerStackIdentifier,
            return_value_policy<return_by_value>())

        .def("ResolveLayerStackIdentifier",
            (const PcpLayerStackIdentifier& (This::*)(const PcpCache&) const)
                &This::ResolveLayerStackIdentifier,
            return_value_policy<return_by_value>())

        .def("__repr__",
            +[](const PcpExpressionVariablesSource &s) {
                return TfStringPrintf(
                    "%sExpressionVariablesSource(%s)",
                    TF_PY_REPR_PREFIX.c_str(),
                    s.IsRootLayerStack() ? 
                    "" : TfPyRepr(*s.GetLayerStackIdentifier()).c_str());
            })
        ;
}
