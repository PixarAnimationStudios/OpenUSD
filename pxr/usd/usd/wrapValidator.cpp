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
        return std::string(
            TF_PY_REPR_PREFIX + 
            "ValidationRegistry().GetOrLoadValidatorByName(" +
            TfPyRepr(self.GetMetadata().name.GetString()) + ")");
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
        return new UsdValidatorMetadata{
            name, plugin, keywords, doc, schemaTypes, isSuite };
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
            &UsdValidatorMetadata::pluginPtr, 
            return_value_policy<return_by_value>()))
        .def_readonly("doc", &UsdValidatorMetadata::doc)
        .def_readonly("isSuite", &UsdValidatorMetadata::isSuite)
        .def("GetKeywords", 
             +[](const UsdValidatorMetadata &self) {
                 return self.keywords;
             })
        .def("GetSchemaTypes",
             +[](const UsdValidatorMetadata &self) {
                 return self.schemaTypes;
             });

    TfPyRegisterStlSequencesFromPython<UsdValidationError>();
    class_<UsdValidator, boost::noncopyable>("Validator", no_init)
        .def("GetMetadata", 
             +[](const UsdValidator &validator) {
                return validator.GetMetadata();
             },
             return_value_policy<return_by_value>())
        .def("Validate", 
             +[](const UsdValidator& validator, const SdfLayerHandle& layer) 
                -> UsdValidationErrorVector {
                return validator.Validate(layer);
             },
             return_value_policy<TfPySequenceToList>(),
             (arg("layer")))
        .def("Validate", 
             +[](const UsdValidator& validator, const UsdStagePtr& stage) 
                -> UsdValidationErrorVector {
                return validator.Validate(stage);
             },
             return_value_policy<TfPySequenceToList>(),
             (arg("stage")))
        .def("Validate", 
             +[](const UsdValidator& validator, const UsdPrim& prim) 
                -> UsdValidationErrorVector {
                return validator.Validate(prim);
             },
             return_value_policy<TfPySequenceToList>(),
             (arg("prim")))
        .def("__eq__", 
             +[](const UsdValidator *left, const UsdValidator *right) {
                 return left == right;
             })
        .def("__repr__", &_Repr);

    class_<UsdValidatorSuite, boost::noncopyable>("ValidatorSuite", no_init)
        .def("GetMetadata", 
             +[](const UsdValidatorSuite &validatorSuite) {
                return validatorSuite.GetMetadata();
             },
             return_value_policy<return_by_value>())
        .def("GetContainedValidators", &_GetContainedValidators)
        .def("__eq__", 
             +[](const UsdValidatorSuite *left, const UsdValidatorSuite *right) {
                 return left == right;
             });
}
