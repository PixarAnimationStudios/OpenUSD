//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/pyEditContext.h"

#include <boost/python.hpp>

#include <memory>

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

struct Usd_PyEditContextAccess {
    static void __enter__(UsdPyEditContext &self) {
        self._editContext = self._editTarget.IsValid() ?
            std::make_shared<UsdEditContext>(self._stage, self._editTarget) :
            std::make_shared<UsdEditContext>(self._stage);
    }

    static void __exit__(UsdPyEditContext &self, object, object, object) {
        self._editContext.reset();
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdEditContext()
{
    class_<UsdPyEditContext>(
        "EditContext", init<UsdStagePtr, optional<UsdEditTarget> >(
            (arg("stage"), arg("editTarget")=UsdEditTarget())))
        .def("__enter__", &Usd_PyEditContextAccess::__enter__, return_self<>())
        .def("__exit__", &Usd_PyEditContextAccess::__exit__)
        ;
}
