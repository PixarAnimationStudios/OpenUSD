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
