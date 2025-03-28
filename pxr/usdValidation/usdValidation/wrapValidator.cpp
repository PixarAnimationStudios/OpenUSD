//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usdValidation/usdValidation/validator.h"
#include "pxr/usdValidation/usdValidation/error.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/make_constructor.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/object.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

    list
    _GetContainedValidators(const UsdValidationValidatorSuite &validatorSuite)
    {
        list result;
        for (const auto *validator : validatorSuite.GetContainedValidators())
        {
            result.append(pointer_wrapper(validator));
        }
        return result;
    }

} // anonymous namespace

void wrapUsdValidationValidator()
{
    class_<UsdValidationValidatorMetadata>("ValidatorMetadata", no_init)
        .def("__init__", make_constructor(&_NewMetadata, default_call_policies(),
                                          (arg("name") = TfToken(),
                                           arg("plugin") = PlugPluginPtr(),
                                           arg("keywords") = TfTokenVector(),
                                           arg("doc") = TfToken(),
                                           arg("schemaTypes") = TfTokenVector(),
                                           arg("isTimeDependent") = false,
                                           arg("isSuite") = false)))
        .add_property("name", make_getter(
            &UsdValidationValidatorMetadata::name, 
            return_value_policy<return_by_value>()))
        .add_property("plugin", make_getter(
            &UsdValidationValidatorMetadata::pluginPtr, 
            return_value_policy<return_by_value>()))
        .def_readonly("doc", &UsdValidationValidatorMetadata::doc)
        .def_readonly("isTimeDependent", 
                      &UsdValidationValidatorMetadata::isTimeDependent)
        .def_readonly("isSuite", &UsdValidationValidatorMetadata::isSuite)
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
    class_<UsdValidationValidator, noncopyable>("Validator", no_init)
        .def("GetMetadata", 
             +[](const UsdValidationValidator &validator) {
                return validator.GetMetadata();
             },
             return_value_policy<return_by_value>())
        .def("Validate", 
             +[](const UsdValidationValidator &validator, 
                 const SdfLayerHandle &layer) 
                -> UsdValidationErrorVector {
                return validator.Validate(layer);
             },
             return_value_policy<TfPySequenceToList>(),
             (arg("layer")))
        .def("Validate", 
             +[](const UsdValidationValidator &validator, 
                 const UsdStagePtr &stage, 
                 const UsdValidationTimeRange &timeRange) 
                -> UsdValidationErrorVector {
                return validator.Validate(stage, timeRange);
             },
             return_value_policy<TfPySequenceToList>(),
             (arg("stage"), 
              arg("timeRange") = UsdValidationTimeRange()))
        .def("Validate", 
             +[](const UsdValidationValidator &validator, const UsdPrim &prim,
                 const UsdValidationTimeRange &timeRange)
                -> UsdValidationErrorVector {
                return validator.Validate(prim, timeRange);
             },
             return_value_policy<TfPySequenceToList>(),
             (arg("prim"), 
              arg("timeRange") = UsdValidationTimeRange()))
        .def("__eq__", 
             +[](const UsdValidationValidator *left, 
                 const UsdValidationValidator *right) {
                 return left == right;
             })
        .def("__repr__", &_Repr);
        TfPyRegisterStlSequencesFromPython<const UsdValidationValidator*>();

    class_<UsdValidationValidatorSuite, noncopyable>("ValidatorSuite", no_init)
        .def("GetMetadata", 
             +[](const UsdValidationValidatorSuite &validatorSuite) {
                return validatorSuite.GetMetadata();
             },
             return_value_policy<return_by_value>())
        .def("GetContainedValidators", &_GetContainedValidators)
        .def("__eq__", 
             +[](const UsdValidationValidatorSuite *left, 
                 const UsdValidationValidatorSuite *right) {
                 return left == right;
             });
        TfPyRegisterStlSequencesFromPython<const UsdValidationValidatorSuite*>();
}
