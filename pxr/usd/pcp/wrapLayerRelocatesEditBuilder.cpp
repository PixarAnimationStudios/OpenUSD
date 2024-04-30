//
// Copyright 2024 Pixar
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
#include "pxr/usd/pcp/layerRelocatesEditBuilder.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

struct Pcp_LayerRelocatesEditBuilderRelocateResult :
    public TfPyAnnotatedBoolResult<std::string>
{
    Pcp_LayerRelocatesEditBuilderRelocateResult(bool val, const std::string &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, std::move(msg)) {}
};

Pcp_LayerRelocatesEditBuilderRelocateResult
_Relocate(PcpLayerRelocatesEditBuilder &self, 
    const SdfPath &source, const SdfPath &target)
{
    std::string whyNot;
    bool result = self.Relocate(source, target, &whyNot);
    return Pcp_LayerRelocatesEditBuilderRelocateResult(result, whyNot);
}

} // anonymous namespace 

void wrapLayerRelocatesEditBuilder()
{
    Pcp_LayerRelocatesEditBuilderRelocateResult
        ::Wrap<Pcp_LayerRelocatesEditBuilderRelocateResult>(
            "_LayerRelocatesEditBuilderRelocateResult", "whyNot");

    class_<PcpLayerRelocatesEditBuilder, boost::noncopyable>(
        "LayerRelocatesEditBuilder", no_init)
        .def(init<const PcpLayerStackPtr &>())
        .def(init<const PcpLayerStackPtr &, const SdfLayerHandle &>())
        .def("Relocate", &_Relocate)
        .def("GetEditedRelocatesMap",
             &PcpLayerRelocatesEditBuilder::GetEditedRelocatesMap,
             return_value_policy<return_by_value>())
        .def("GetEdits",
             &PcpLayerRelocatesEditBuilder::GetEdits,
             return_value_policy<TfPySequenceToList>())
        ;

    to_python_converter<PcpLayerRelocatesEditBuilder::LayerRelocatesEdit, 
        TfPyContainerConversions::to_tuple<
            PcpLayerRelocatesEditBuilder::LayerRelocatesEdit>>();
}
