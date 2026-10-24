//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/regressionPreventer.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/tf/pyEnum.h"

PXR_NAMESPACE_USING_DIRECTIVE

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

#define MEMBER(cls, name) def_ro(#name, &cls::name)

void wrapRegressionPreventer(nanobind::module_& m)
{
    namespace nb = nanobind;
    using This = TsRegressionPreventer;
    using SetResult = This::SetResult;

    // First the class object, so we can create a scope for it...
    nb::class_<This> classObj(m, "RegressionPreventer");

    // ...then the nested type wrappings, which require the scope...

    TfPyWrapEnum<This::InteractiveMode>{classObj};

    nb::class_<SetResult>(classObj, "SetResult")
        .MEMBER(SetResult, adjusted)
        .MEMBER(SetResult, havePreSegment)
        .MEMBER(SetResult, preActiveAdjusted)
        .MEMBER(SetResult, preActiveAdjustedWidth)
        .MEMBER(SetResult, preOppositeAdjusted)
        .MEMBER(SetResult, preOppositeAdjustedWidth)
        .MEMBER(SetResult, havePostSegment)
        .MEMBER(SetResult, postActiveAdjusted)
        .MEMBER(SetResult, postActiveAdjustedWidth)
        .MEMBER(SetResult, postOppositeAdjusted)
        .MEMBER(SetResult, postOppositeAdjustedWidth)
        .def("GetDebugDescription",
            &SetResult::GetDebugDescription,
            nb::arg("precision") = 6)
        ;

    // ...then the defs, which must occur after the nested type wrappings.
    classObj

        .def("__init__", [](This* self, TsSpline* spline,
                            TsTime activeKnotTime, bool limit) {
            new (self) This(spline, activeKnotTime, limit);
        }, nb::arg("spline"),
           nb::arg("activeKnotTime"),
           nb::arg("limit") = true)

        .def("__init__", [](This* self, TsSpline* spline,
                            TsTime activeKnotTime,
                            This::InteractiveMode mode, bool limit) {
            new (self) This(spline, activeKnotTime, mode, limit);
        }, nb::arg("spline"),
           nb::arg("activeKnotTime"),
           nb::arg("mode"),
           nb::arg("limit") = true)

        .def("Set", [](This& self, const TsKnot& proposedActiveKnot) -> nb::object {
            SetResult result;
            if (self.Set(proposedActiveKnot, &result)) {
                return nb::cast(result);
            }
            return nb::none();
        }, nb::arg("proposedActiveKnot"))

        ;
}
