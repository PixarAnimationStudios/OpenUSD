//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/layerRelocatesEditBuilder.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"

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

Pcp_LayerRelocatesEditBuilderRelocateResult
_RemoveRelocate(PcpLayerRelocatesEditBuilder &self, 
    const SdfPath &source)
{
    std::string whyNot;
    bool result = self.RemoveRelocate(source, &whyNot);
    return Pcp_LayerRelocatesEditBuilderRelocateResult(result, whyNot);
}

} // anonymous namespace 

void wrapLayerRelocatesEditBuilder(nanobind::module_ &m)
{
    namespace nb = nanobind;

    Pcp_LayerRelocatesEditBuilderRelocateResult
        ::WrapNb<Pcp_LayerRelocatesEditBuilderRelocateResult>(
            m, "_LayerRelocatesEditBuilderRelocateResult", "whyNot");

    nb::class_<PcpLayerRelocatesEditBuilder>(m, "LayerRelocatesEditBuilder")
        .def(nb::init<const PcpLayerStackPtr &>())
        .def(nb::init<const PcpLayerStackPtr &, const SdfLayerHandle &>())
        .def("Relocate", &_Relocate)
        .def("RemoveRelocate", &_RemoveRelocate)
        .def("GetEditedRelocatesMap",
             &PcpLayerRelocatesEditBuilder::GetEditedRelocatesMap,
             nb::rv_policy::copy)
        .def("GetEdits",
             &PcpLayerRelocatesEditBuilder::GetEdits,
             nb::rv_policy::copy)
        ;

}
