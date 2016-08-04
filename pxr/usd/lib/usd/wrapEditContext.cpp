//
// Copyright 2016 Pixar
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

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/python.hpp>

#include "pxr/usd/usd/editContext.h"

using namespace boost::python;

struct Usd_PyEditContextAccess {
    static void __enter__(UsdPyEditContext &self) {
        self._editContext = self._editTarget.IsValid() ?
            boost::make_shared<UsdEditContext>(self._stage, self._editTarget) :
            boost::make_shared<UsdEditContext>(self._stage);
    }

    static void __exit__(UsdPyEditContext &self, object, object, object) {
        self._editContext.reset();
    }
};

void wrapUsdEditContext()
{
    class_<UsdPyEditContext>(
        "EditContext", init<UsdStagePtr, optional<UsdEditTarget> >(
            (arg("stage"), arg("editTarget")=UsdEditTarget())))
        .def("__enter__", &Usd_PyEditContextAccess::__enter__, return_self<>())
        .def("__exit__", &Usd_PyEditContextAccess::__exit__)
        ;
}
