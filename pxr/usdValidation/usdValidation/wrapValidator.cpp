//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

#include "pxr/pxr.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usdValidation/usdValidation/validator.h"
#include "pxr/usdValidation/usdValidation/error.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace
{

    std::string _Repr(const UsdValidationValidator &self)
    {
        return std::string(
            TF_PY_REPR_PREFIX + 
            "ValidationRegistry().GetOrLoadValidatorByName(" +
            TfPyRepr(self.GetMetadata().name.GetString()) + ")");
    }
    
    UsdValidationValidatorMetadata *
    _NewMetadata(
        const TfToken &name,
        const PlugPluginPtr &plugin,
        const TfTokenVector &keywords,
        const TfToken &doc,
        const TfTokenVector &schemaTypes,
        bool isTimeDependent,
        bool isSuite)
    {
        return new UsdValidationValidatorMetadata{
            name, plugin, keywords, doc, schemaTypes, isTimeDependent, 
            isSuite };
    }

    nb::list
    _GetContainedValidators(const UsdValidationValidatorSuite &validatorSuite)
    {
        nb::list result;
        for (const auto *validator : validatorSuite.GetContainedValidators())
        {
            result.append(nb::cast(validator, nb::rv_policy::reference));
        }
        return result;
    }

    nb::list
    _GetFixers(const UsdValidationValidator &validator)
    {
        nb::list result;
        for (const auto *fixer : validator.GetFixers())
        {
            result.append(nb::cast(fixer, nb::rv_policy::reference));
        }
        return result;
    }

    nb::list
    _GetFixersByErrorName(const UsdValidationValidator &validator, 
                         const TfToken &errorName)
    {
        nb::list result;
        for (const auto *fixer : validator.GetFixersByErrorName(errorName))
        {
            result.append(nb::cast(fixer, nb::rv_policy::reference));
        }
        return result;
    }

    nb::list
    _GetFixersByKeywords(const UsdValidationValidator &validator, 
                        const TfTokenVector &keywords)
    {
        nb::list result;
        for (const auto *fixer : validator.GetFixersByKeywords(keywords))
        {
            result.append(nb::cast(fixer, nb::rv_policy::reference));
        }
        return result;
    }

} // anonymous namespace

void wrapUsdValidationValidator(nb::module_ &m)
{
    nb::class_<UsdValidationValidatorMetadata>(m, "ValidatorMetadata")
        .def(nb::new_(&_NewMetadata),
             nb::arg("name") = TfToken(),
             nb::arg("plugin") = PlugPluginPtr(),
             nb::arg("keywords") = TfTokenVector(),
             nb::arg("doc") = TfToken(),
             nb::arg("schemaTypes") = TfTokenVector(),
             nb::arg("isTimeDependent") = false,
             nb::arg("isSuite") = false)
        .def_ro("name", &UsdValidationValidatorMetadata::name)
        .def_ro("plugin", &UsdValidationValidatorMetadata::pluginPtr)
        .def_ro("doc", &UsdValidationValidatorMetadata::doc)
        .def_ro("isTimeDependent", 
                &UsdValidationValidatorMetadata::isTimeDependent)
        .def_ro("isSuite", &UsdValidationValidatorMetadata::isSuite)
        .def("GetKeywords", 
             +[](const UsdValidationValidatorMetadata &self) {
                 return self.keywords;
             })
        .def("GetSchemaTypes",
             +[](const UsdValidationValidatorMetadata &self) {
                 return self.schemaTypes;
             });
    TfPyRegisterStlSequencesFromPython<UsdValidationValidatorMetadata>();

    TfPyRegisterStlSequencesFromPython<UsdValidationError>();
    nb::class_<UsdValidationValidator>(m, "Validator")
        .def("GetMetadata", 
             +[](const UsdValidationValidator &validator) {
                return validator.GetMetadata();
             })
        .def("Validate", 
             +[](const UsdValidationValidator &validator, 
                 const SdfLayerHandle &layer) 
                -> UsdValidationErrorVector {
                return validator.Validate(layer);
             },
             nb::arg("layer"))
        .def("Validate", 
             +[](const UsdValidationValidator &validator, 
                 const UsdStagePtr &stage, 
                 const UsdValidationTimeRange &timeRange) 
                -> UsdValidationErrorVector {
                return validator.Validate(stage, timeRange);
             },
             nb::arg("stage"), 
             nb::arg("timeRange") = UsdValidationTimeRange())
        .def("Validate", 
             +[](const UsdValidationValidator &validator, const UsdPrim &prim,
                 const UsdValidationTimeRange &timeRange)
                -> UsdValidationErrorVector {
                return validator.Validate(prim, timeRange);
             },
             nb::arg("prim"), 
             nb::arg("timeRange") = UsdValidationTimeRange())
        .def("GetFixers", _GetFixers)
        .def("GetFixerByName", 
             +[](const UsdValidationValidator &validator, 
                 const TfToken &name) 
                -> const UsdValidationFixer* {
                return validator.GetFixerByName(name);
             },
             nb::rv_policy::reference,
             nb::arg("name"))
        .def("GetFixersByErrorName", _GetFixersByErrorName,
             nb::arg("errorName"))
        .def("GetFixerByNameAndErrorName",
             +[](const UsdValidationValidator &validator, 
                 const TfToken &name, 
                 const TfToken &errorName) 
                -> const UsdValidationFixer* {
                return validator.GetFixerByNameAndErrorName(
                    name, errorName);
             },
             nb::rv_policy::reference,
             nb::arg("name"), nb::arg("errorName"))
        .def("GetFixersByKeywords", _GetFixersByKeywords,
             nb::arg("keywords"))
        .def("__eq__", 
             +[](const UsdValidationValidator *left, 
                 const UsdValidationValidator *right) {
                 return left == right;
             }, nb::arg().none())
        .def("__repr__", &_Repr);
    TfPyRegisterStlSequencesFromPython<const UsdValidationValidator*>();

    nb::class_<UsdValidationValidatorSuite>(m, "ValidatorSuite")
        .def("GetMetadata", 
             +[](const UsdValidationValidatorSuite &validatorSuite) {
                return validatorSuite.GetMetadata();
             })
        .def("GetContainedValidators", &_GetContainedValidators)
        .def("__eq__", 
             +[](const UsdValidationValidatorSuite *left, 
                 const UsdValidationValidatorSuite *right) {
                 return left == right;
             }, nb::arg().none());
    TfPyRegisterStlSequencesFromPython<const UsdValidationValidatorSuite*>();
}
