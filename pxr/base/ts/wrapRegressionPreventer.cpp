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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/make_constructor.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;


static TsRegressionPreventer* _WrapConstructor1(
    TsSpline *spline,
    TsTime activeKnotTime,
    bool limit)
{
    return new TsRegressionPreventer(spline, activeKnotTime, limit);
}

static TsRegressionPreventer* _WrapConstructor2(
    TsSpline *spline,
    TsTime activeKnotTime,
    TsRegressionPreventer::InteractiveMode mode,
    bool limit)
{
    return new TsRegressionPreventer(spline, activeKnotTime, mode, limit);
}

static object _WrapSet(
    TsRegressionPreventer &self,
    const TsKnot &proposedActiveKnot)
{
    TsRegressionPreventer::SetResult result;
    const bool ok = self.Set(proposedActiveKnot, &result);

    if (ok)
    {
        return object(result);
    }
    else
    {
        return object();
    }
}


#define MEMBER(cls, name) def_readonly(#name, &cls::name)

void wrapRegressionPreventer()
{
    using This = TsRegressionPreventer;
    using SetResult = This::SetResult;

    // First the class object, so we can create a scope for it...
    class_<This> classObj("RegressionPreventer", no_init);
    scope classScope(classObj);

    // ...then the nested type wrappings, which require the scope...

    TfPyWrapEnum<This::InteractiveMode>();

    class_<SetResult>("SetResult", no_init)
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
            (arg("precision") = 6))
        ;

    // ...then the defs, which must occur after the nested type wrappings.
    classObj

        .def("__init__", make_constructor(
                &_WrapConstructor1,
                default_call_policies(),
                (arg("spline"),
                    arg("activeKnotTime"),
                    arg("limit") = true)))

        .def("__init__", make_constructor(
                &_WrapConstructor2,
                default_call_policies(),
                (arg("spline"),
                    arg("activeKnotTime"),
                    arg("mode"),
                    arg("limit") = true)))

        .def("Set", &_WrapSet,
            (arg("proposedActiveKnot")))

        ;
}
