//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https:/</openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usdValidation/usdValidation/validator.h"
#include "pxr/usdValidation/usdValidation/context.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapUsdValidationContext(nb::module_ &m)
{
    TfPyRegisterStlSequencesFromPython<UsdValidationError>();
    nb::class_<UsdValidationContext>(m, "ValidationContext")
        .def(nb::init<const TfTokenVector &, bool>(),
             nb::arg("keywords"), nb::arg("includeAllAncestors") = true)
        .def(nb::init<const PlugPluginPtrVector &, bool>(),
             nb::arg("plugins"), nb::arg("includeAllAncestors") = true)
        .def(nb::init<const UsdValidationValidatorMetadataVector &, bool>(),
             nb::arg("metadata"), nb::arg("includeAllAncestors") = true)
        .def(nb::init<const std::vector<TfType> &>(), nb::arg("schemaTypes"))
        .def(nb::init<const std::vector<const UsdValidationValidator*> &>(),
             nb::arg("validators"))
        .def(nb::init<const std::vector<const UsdValidationValidatorSuite*> &>(),
             nb::arg("suites"))
        .def("Validate", 
             +[](const UsdValidationContext &ctx, const SdfLayerHandle &layer) 
                -> UsdValidationErrorVector {
                return ctx.Validate(layer);
             },
             nb::arg("layer"))
        .def("Validate", 
             +[](const UsdValidationContext &ctx, const UsdStagePtr &stage) 
                -> UsdValidationErrorVector {
                return ctx.Validate(stage);
             },
             nb::arg("stage"))
        .def("Validate",
             +[](const UsdValidationContext &ctx, const UsdStagePtr &stage, 
                const Usd_PrimFlagsPredicate &predicate) 
                -> UsdValidationErrorVector {
                return ctx.Validate(stage, predicate);
             },
             nb::arg("stage"), nb::arg("predicate"))
        .def("Validate",
             +[](const UsdValidationContext &ctx, const UsdStagePtr &stage, 
                 const Usd_PrimFlagsPredicate &predicate,
                 const UsdValidationTimeRange &timeRange)
                -> UsdValidationErrorVector {
                return ctx.Validate(stage, predicate, timeRange);
             },
             nb::arg("stage"), nb::arg("predicate"), nb::arg("timeRange"))
        .def("Validate", 
             +[](const UsdValidationContext &ctx, const UsdStagePtr &stage,
                 const UsdValidationTimeRange &timeRange) 
                -> UsdValidationErrorVector {
                return ctx.Validate(stage, timeRange);
             },
             nb::arg("stage"), nb::arg("timeRange"))
        .def("Validate",
             +[](const UsdValidationContext &ctx, const UsdStagePtr &stage,
                 const Usd_PrimFlagsPredicate &predicate,
                 const std::vector<UsdTimeCode> &timeCodes)
                -> UsdValidationErrorVector {
                return ctx.Validate(stage, predicate, timeCodes);
            },
            nb::arg("stage"), nb::arg("predicate"), nb::arg("timeCodes"))
        .def("Validate",
             +[](const UsdValidationContext &ctx, const UsdStagePtr &stage,
                 const std::vector<UsdTimeCode> &timeCodes)
                -> UsdValidationErrorVector {
                return ctx.Validate(stage, timeCodes);
            },
            nb::arg("stage"), nb::arg("timeCodes"))
        .def("Validate", 
             +[](const UsdValidationContext &ctx, 
                 const std::vector<UsdPrim> &prims,
                 const UsdValidationTimeRange &timeRange)
                -> UsdValidationErrorVector {
                return ctx.Validate(prims, timeRange);
             },
             nb::arg("prims"), 
             nb::arg("timeRange") = UsdValidationTimeRange())
        .def("Validate", 
             +[](const UsdValidationContext &ctx, const UsdPrimRange &prims,
                 const UsdValidationTimeRange &timeRange)
                -> UsdValidationErrorVector {
                return ctx.Validate(prims, timeRange);
             },
             nb::arg("prims"), 
             nb::arg("timeCode") = UsdValidationTimeRange())
        .def("Validate",
             +[](const UsdValidationContext &ctx,
                 const std::vector<UsdPrim> &prims,
                 const std::vector<UsdTimeCode> &timeCodes)
                -> UsdValidationErrorVector {
                return ctx.Validate(prims, timeCodes);
             },
             nb::arg("prims"), nb::arg("timeCodes"))
        .def("Validate",
             +[](const UsdValidationContext &ctx, const UsdPrimRange &prims,
                 const std::vector<UsdTimeCode> &timeCodes)
                -> UsdValidationErrorVector {
                return ctx.Validate(prims, timeCodes);
             },
             nb::arg("prims"), nb::arg("timeCodes"));
}
