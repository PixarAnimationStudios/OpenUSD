//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/dynamicFileFormatDependencyData.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_USING_DIRECTIVE

void 
wrapDynamicFileFormatDependencyData(nanobind::module_ &m)
{
    namespace nb = nanobind;
    using This = PcpDynamicFileFormatDependencyData;
    nb::class_<This>(m, "DynamicFileFormatDependencyData")
        .def("GetRelevantFieldNames", [](const This &self) {
            return TfNbSequenceToList(self.GetRelevantFieldNames());
        })
        .def("GetRelevantAttributeNames", [](const This &self) {
            return TfNbSequenceToList(self.GetRelevantAttributeNames());
        })
        .def("CanFieldChangeAffectFileFormatArguments", 
             &This::CanFieldChangeAffectFileFormatArguments)
        .def("CanAttributeDefaultValueChangeAffectFileFormatArguments", 
             &This::CanAttributeDefaultValueChangeAffectFileFormatArguments)
        .def("IsEmpty", &This::IsEmpty)
        ;
}
