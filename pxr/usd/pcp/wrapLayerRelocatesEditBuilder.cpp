//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
