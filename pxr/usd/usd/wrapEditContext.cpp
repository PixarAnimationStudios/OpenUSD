//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/pyEditContext.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>

#include <memory>

namespace nb = nanobind;

PXR_NAMESPACE_OPEN_SCOPE

struct Usd_PyEditContextAccess {
    static nb::object __enter__(nb::object selfObj) {
        UsdPyEditContext &self = nb::cast<UsdPyEditContext &>(selfObj);
        self._editContext = self._editTarget.IsValid() ?
            std::make_shared<UsdEditContext>(self._stage, self._editTarget) :
            std::make_shared<UsdEditContext>(self._stage);
        return selfObj;
    }

    static void __exit__(UsdPyEditContext &self, nb::args) {
        self._editContext.reset();
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdEditContext(nb::module_ &m)
{
    nb::class_<UsdPyEditContext>(m, "EditContext")
        .def(nb::init<const UsdStagePtr &, const UsdEditTarget &>(),
             nb::arg("stage"), nb::arg("editTarget")=UsdEditTarget())
        .def("__enter__", &Usd_PyEditContextAccess::__enter__)
        .def("__exit__", &Usd_PyEditContextAccess::__exit__)
        ;
}
