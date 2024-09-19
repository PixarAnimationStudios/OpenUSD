//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validator.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/object.hpp"
#include "pxr/external/boost/python/raw_function.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace
{

list _GetOrLoadAllValidators(UsdValidationRegistry &validationRegistry)
{
    list result;
    for (const auto *validator : validationRegistry.GetOrLoadAllValidators())
    {
        result.append(pointer_wrapper(validator));
    }
    return result;
}

list _GetOrLoadValidatorsByName(UsdValidationRegistry &validationRegistry,
                                const TfTokenVector &validatorNames)
{
    list result;
    for (const auto *validator :
         validationRegistry.GetOrLoadValidatorsByName(validatorNames))
    {
        result.append(pointer_wrapper(validator));
    }
    return result;
}

list _GetOrLoadAllValidatorSuites(UsdValidationRegistry &validationRegistry)
{
    list result;
    for (const auto *validator :
         validationRegistry.GetOrLoadAllValidatorSuites())
    {
        result.append(pointer_wrapper(validator));
    }
    return result;
}

list _GetOrLoadValidatorSuitesByName(UsdValidationRegistry &validationRegistry,
                                     const TfTokenVector &suiteNames)
{
    list result;
    for (const auto *validatorSuite :
         validationRegistry.GetOrLoadValidatorSuitesByName(suiteNames))
    {
        result.append(pointer_wrapper(validatorSuite));
    }
    return result;
}

object _GetValidatorMetadata(const UsdValidationRegistry &validationRegistry,
                             const TfToken &name)
{
    UsdValidatorMetadata metadata;
    bool success = validationRegistry.GetValidatorMetadata(name, &metadata);
    if (success)
    {
        return object(metadata);
    }

    return object();
}

UsdValidationRegistry &_GetRegistrySingleton(object const & /* classObj */)
{
    return UsdValidationRegistry::GetInstance();
}

object _DummyInit(tuple const & /* args */, dict const & /* kw */)
{
    return object();
}

} // anonymous namespace

void wrapUsdValidationRegistry()
{
    class_<UsdValidationRegistry, boost::noncopyable>("ValidationRegistry",
                                                      no_init)
        .def("__new__", &_GetRegistrySingleton,
             return_value_policy<reference_existing_object>())
        .staticmethod("__new__")
        .def("__init__", raw_function(_DummyInit))
        .def("HasValidator", &UsdValidationRegistry::HasValidator,
             (args("validatorName")))
        .def("HasValidatorSuite", &UsdValidationRegistry::HasValidatorSuite,
             (args("suiteName")))
        .def("GetOrLoadAllValidators", &_GetOrLoadAllValidators)
        .def("GetOrLoadValidatorByName",
             &UsdValidationRegistry::GetOrLoadValidatorByName,
             return_value_policy<reference_existing_object>(),
             (args("validatorName")))
        .def("GetOrLoadValidatorsByName", &_GetOrLoadValidatorsByName,
             (args("validatorNames")))
        .def("GetOrLoadAllValidatorSuites", &_GetOrLoadAllValidatorSuites)
        .def("GetOrLoadValidatorSuiteByName",
             &UsdValidationRegistry::GetOrLoadValidatorSuiteByName,
             return_value_policy<reference_existing_object>(),
             (args("suiteName")))
        .def("GetOrLoadValidatorSuitesByName", &_GetOrLoadValidatorSuitesByName,
             (args("suiteNames")))
        .def("GetValidatorMetadata", &_GetValidatorMetadata, (args("name")))
        .def("GetAllValidatorMetadata",
             &UsdValidationRegistry::GetAllValidatorMetadata,
             return_value_policy<TfPySequenceToList>())
        .def("GetValidatorMetadataForPlugin",
             &UsdValidationRegistry::GetValidatorMetadataForPlugin,
             return_value_policy<TfPySequenceToList>(), (args("pluginName")))
        .def("GetValidatorMetadataForKeyword",
             &UsdValidationRegistry::GetValidatorMetadataForKeyword,
             return_value_policy<TfPySequenceToList>(), (args("keyword")))
        .def("GetValidatorMetadataForSchemaType",
             &UsdValidationRegistry::GetValidatorMetadataForSchemaType,
             return_value_policy<TfPySequenceToList>(), (args("schemaType")))
        .def("GetValidatorMetadataForPlugins",
             &UsdValidationRegistry::GetValidatorMetadataForPlugins,
             return_value_policy<TfPySequenceToList>(), (args("pluginNames")))
        .def("GetValidatorMetadataForKeywords",
             &UsdValidationRegistry::GetValidatorMetadataForKeywords,
             return_value_policy<TfPySequenceToList>(), (args("keywords")))
        .def("GetValidatorMetadataForSchemaTypes",
             &UsdValidationRegistry::GetValidatorMetadataForSchemaTypes,
             return_value_policy<TfPySequenceToList>(), (args("schemaTypes")));
}
