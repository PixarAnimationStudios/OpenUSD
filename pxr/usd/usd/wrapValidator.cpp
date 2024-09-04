//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/validator.h"
#include "pxr/usd/usd/validationError.h"

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

    std::string _Repr(const UsdValidator &self)
    {
        return TfStringPrintf(
            "%sValidationRegistry().GetOrLoadValidatorByName('%s')",
            TF_PY_REPR_PREFIX.c_str(), self.GetMetadata().name.GetText());
    }
    
    UsdValidatorMetadata *
    _NewMetadata(
        const TfToken &name,
        const PlugPluginPtr &plugin,
        const TfTokenVector &keywords,
        const TfToken &doc,
        const TfTokenVector &schemaTypes,
        bool isSuite)
    {
        return new UsdValidatorMetadata{name, plugin, keywords, doc, schemaTypes, isSuite};
    }

    const UsdValidatorMetadata &
    _GetValidatorMetadata(const UsdValidator &validator)
    {
        return validator.GetMetadata();
    }

    const UsdValidatorMetadata &
    _GetValidatorSuiteMetadata(const UsdValidatorSuite &validatorSuite)
    {
        return validatorSuite.GetMetadata();
    }

    list
    _GetContainedValidators(const UsdValidatorSuite &validatorSuite)
    {
        list result;
        for (const auto *validator : validatorSuite.GetContainedValidators())
        {
            result.append(pointer_wrapper(validator));
        }
        return result;
    }

    bool _ValidatorEqual(const UsdValidator *left, const UsdValidator *right)
    {
        return left == right;
    }

    bool _ValidatorSuiteEqual(const UsdValidatorSuite *left, const UsdValidatorSuite *right)
    {
        return left == right;
    }

} // anonymous namespace

void wrapUsdValidator()
{
    class_<UsdValidatorMetadata>("ValidatorMetadata", no_init)
        .def("__init__", make_constructor(&_NewMetadata, default_call_policies(),
                                          (arg("name") = TfToken(),
                                           arg("plugin") = PlugPluginPtr(),
                                           arg("keywords") = TfTokenVector(),
                                           arg("doc") = TfToken(),
                                           arg("schemaTypes") = TfTokenVector(),
                                           arg("isSuite") = false)))
        .add_property("name", make_getter(
            &UsdValidatorMetadata::name, return_value_policy<return_by_value>()))
        .add_property("plugin", make_getter(
            &UsdValidatorMetadata::pluginPtr, return_value_policy<return_by_value>()))
        .add_property("keywords", make_getter(
            &UsdValidatorMetadata::keywords, return_value_policy<TfPySequenceToList>()))
        .def_readonly("doc", &UsdValidatorMetadata::doc)
        .add_property("schemaTypes", make_getter(
            &UsdValidatorMetadata::schemaTypes, return_value_policy<TfPySequenceToList>()))
        .def_readonly("isSuite", &UsdValidatorMetadata::isSuite);

    TfPyRegisterStlSequencesFromPython<UsdValidationError>();
    UsdValidationErrorVector (UsdValidator::*_ValidateLayer)(
        const SdfLayerHandle &layer) const = &UsdValidator::Validate;
    UsdValidationErrorVector (UsdValidator::*_ValidateStage)(
        const UsdStagePtr &stage) const = &UsdValidator::Validate;
    UsdValidationErrorVector (UsdValidator::*_ValidatePrim)(const UsdPrim &prim)
        const = &UsdValidator::Validate;
    class_<UsdValidator, boost::noncopyable>("Validator", no_init)
        .def("GetMetadata", &_GetValidatorMetadata,
             return_value_policy<return_by_value>())
        .def("ValidateLayer", _ValidateLayer,
             return_value_policy<TfPySequenceToList>())
        .def("ValidatePrim", _ValidatePrim,
             return_value_policy<TfPySequenceToList>())
        .def("ValidateStage", _ValidateStage,
             return_value_policy<TfPySequenceToList>())
        .def("__eq__", &_ValidatorEqual)
        .def("__repr__", &_Repr);

    class_<UsdValidatorSuite, boost::noncopyable>("ValidatorSuite", no_init)
        .def("GetMetadata", &_GetValidatorSuiteMetadata,
             return_value_policy<return_by_value>())
        .def("GetContainedValidators", &_GetContainedValidators)
        .def("__eq__", &_ValidatorSuiteEqual);
}
