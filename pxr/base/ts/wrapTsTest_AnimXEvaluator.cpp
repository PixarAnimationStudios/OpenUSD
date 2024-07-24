//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_AnimXEvaluator.h"

#include "pxr/base/tf/pyEnum.h"

#include <boost/python.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;

using This = TsTest_AnimXEvaluator;


static This*
_ConstructEvaluator(
    const This::AutoTanType autoTanType)
{
    return new This(autoTanType);
}


void wrapTsTest_AnimXEvaluator()
{
    // First the class object, so we can create a scope for it...
    class_<This, bases<TsTest_Evaluator>>
        classObj("TsTest_AnimXEvaluator", no_init);

    // ...then the nested type wrappings, which require the scope...
    TfPyWrapEnum<This::AutoTanType>();

    // ...then the defs, which must occur after the nested type wrappings.
    classObj

        .def("__init__",
            make_constructor(
                &_ConstructEvaluator, default_call_policies(),
                (arg("autoTanType") = This::AutoTanAuto)))

        // Wrapping for Eval is inherited from TsTest_Evaluator.
        ;
}
