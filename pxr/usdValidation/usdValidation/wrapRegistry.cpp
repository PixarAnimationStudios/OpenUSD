//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/fixer.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/validator.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/external/boost/python/call.hpp"
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
    UsdValidationValidatorMetadata metadata;
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

// ---------------------------------------------------------------------------
// Python callable → C++ task function wrappers
//
// UsdValidationContext runs validators on a work queue, so task functions can
// be invoked from C++ worker threads that do not hold the Python GIL.  The 
// wrapper template below handles this safely:
//
//   1. The Python callable is stored in a TfPyObjWrapper rather than a raw
//      pxr_boost::python::object.  TfPyObjWrapper holds the callable via a
//      shared_ptr and can be copied or destroyed from any thread without
//      acquiring the GIL.
//
//   2. Wrapper lambda acquires the GIL (via TfPyLock) before touching
//      any Python objects, then releases it on exit.
//
//   3. If the Python callable raises an exception, error_already_set is
//      caught, the exception is forwarded as a Tf error (so it surfaces
//      through the normal USD diagnostic machinery), and PyErr_Clear() is
//      called to clean up CPython's per-thread exception state.  Without the
//      explicit clear, the stale exception would corrupt subsequent Python
//      API calls on that thread.

static UsdValidationErrorVector
_ExtractErrorsFromPyResult(object result)
{
    // Caller must hold the GIL.  We coerce the return value to a
    // pxr_boost::python::list explicitly so that any non-list iterable
    // (e.g. a generator) is also accepted.
    UsdValidationErrorVector errors;
    list resultList(result);
    for (ssize_t i = 0, n = len(resultList); i < n; ++i) {
        errors.push_back(extract<UsdValidationError>(resultList[i]));
    }
    return errors;
}

template <typename TaskFn>
struct _WrapTaskFnHelper;

template <typename... Args>
struct _WrapTaskFnHelper<std::function<UsdValidationErrorVector(Args...)>>
{
    static std::function<UsdValidationErrorVector(Args...)> WrapPyTaskFn(
        object pyFn)
    {
        TfPyObjWrapper wrapper(pyFn);
        return [wrapper](Args&&... args) -> UsdValidationErrorVector {
            TfPyLock lock;
            try {
                object result = wrapper.Get()(object(
                    std::forward<Args>(args))...);
                return _ExtractErrorsFromPyResult(result);
            }
            catch (error_already_set const &) {
                TfPyConvertPythonExceptionToTfErrors();
                PyErr_Clear();
                return {};
            }
        };
    }
};

static UsdValidateLayerTaskFn
_WrapLayerTaskFn(object pyFn)
{
    return _WrapTaskFnHelper<UsdValidateLayerTaskFn>::WrapPyTaskFn(pyFn);
}

static UsdValidateStageTaskFn
_WrapStageTaskFn(object pyFn)
{
    return _WrapTaskFnHelper<UsdValidateStageTaskFn>::WrapPyTaskFn(pyFn);
}

static UsdValidatePrimTaskFn
_WrapPrimTaskFn(object pyFn)
{
    return _WrapTaskFnHelper<UsdValidatePrimTaskFn>::WrapPyTaskFn(pyFn);
}

// The three _Register* functions below are thin shims that wrap the Python
// callable and forward to the appropriate RegisterValidator() overload.
// They are bound as Python methods on ValidationRegistry below.

static void
_RegisterLayerValidator(UsdValidationRegistry &registry,
                        const UsdValidationValidatorMetadata &metadata,
                        object pyFn,
                        const std::vector<UsdValidationFixer> &fixers)
{
    if (!PyCallable_Check(pyFn.ptr())) {
        TfPyThrowTypeError("layerTaskFn must be callable");
    }

    registry.RegisterValidator(
        metadata, _WrapLayerTaskFn(pyFn), fixers);
}

static void
_RegisterStageValidator(UsdValidationRegistry &registry,
                        const UsdValidationValidatorMetadata &metadata,
                        object pyFn,
                        const std::vector<UsdValidationFixer> &fixers)
{
    if (!PyCallable_Check(pyFn.ptr())) {
        TfPyThrowTypeError("stageTaskFn must be callable");
    }

    registry.RegisterValidator(
        metadata, _WrapStageTaskFn(pyFn), fixers);
}

static void
_RegisterPrimValidator(UsdValidationRegistry &registry,
                       const UsdValidationValidatorMetadata &metadata,
                       object pyFn,
                       const std::vector<UsdValidationFixer> &fixers)
{
    if (!PyCallable_Check(pyFn.ptr())) {
        TfPyThrowTypeError("primTaskFn must be callable");
    }

    registry.RegisterValidator(
        metadata, _WrapPrimTaskFn(pyFn), fixers);
}

static void
_RegisterValidatorSuite(UsdValidationRegistry &registry,
                        const UsdValidationValidatorMetadata &metadata,
                        list validators)
{
    // Extract raw pointers from the Python Validator objects.  The registry
    // owns all validators and they are immortal for the process lifetime, so
    // raw-pointer storage is safe here.
    std::vector<const UsdValidationValidator *> containedValidators;
    for (ssize_t i = 0, n = len(validators); i < n; ++i) {
        containedValidators.push_back(
            extract<const UsdValidationValidator *>(
                validators[i]));
    }
    registry.RegisterValidatorSuite(metadata, containedValidators);
}

// ---------------------------------------------------------------------------
// Plugin validator registration
//
// RegisterPluginValidator differs from RegisterValidator in that it takes
// only a TfToken name -- the metadata is already populated from the
// plugin's plugInfo.json during registry initialization.  This is the
// standard registration path for validators defined in plugins.

static void
_RegisterPluginLayerValidator(UsdValidationRegistry &registry,
                              const TfToken &validatorName,
                              object pyFn,
                              const std::vector<UsdValidationFixer> &fixers)
{
    if (!PyCallable_Check(pyFn.ptr())) {
        TfPyThrowTypeError("layerTaskFn must be callable");
    }

    registry.RegisterPluginValidator(
        validatorName, _WrapLayerTaskFn(pyFn), fixers);
}

static void
_RegisterPluginStageValidator(UsdValidationRegistry &registry,
                              const TfToken &validatorName,
                              object pyFn,
                              const std::vector<UsdValidationFixer> &fixers)
{
    if (!PyCallable_Check(pyFn.ptr())) {
        TfPyThrowTypeError("stageTaskFn must be callable");
    }

    registry.RegisterPluginValidator(
        validatorName, _WrapStageTaskFn(pyFn), fixers);
}

static void
_RegisterPluginPrimValidator(UsdValidationRegistry &registry,
                             const TfToken &validatorName,
                             object pyFn,
                             const std::vector<UsdValidationFixer> &fixers)
{
    if (!PyCallable_Check(pyFn.ptr())) {
        TfPyThrowTypeError("primTaskFn must be callable");
    }

    registry.RegisterPluginValidator(
        validatorName, _WrapPrimTaskFn(pyFn), fixers);
}

static void
_RegisterPluginValidatorSuite(UsdValidationRegistry &registry,
                              const TfToken &validatorSuiteName,
                              list validators)
{
    std::vector<const UsdValidationValidator *> containedValidators;
    for (ssize_t i = 0, n = len(validators); i < n; ++i) {
        containedValidators.push_back(
            extract<const UsdValidationValidator *>(
                validators[i]));
    }
    registry.RegisterPluginValidatorSuite(
        validatorSuiteName, containedValidators);
}

} // anonymous namespace

void wrapUsdValidationRegistry()
{
    class_<UsdValidationRegistry, noncopyable>("ValidationRegistry",
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
             return_value_policy<TfPySequenceToList>(), (args("schemaTypes")))
        // Explicit registration -- caller provides full metadata.
        // Use when registering validators at runtime without a plugin.
        // An optional list of ValidationFixer objects can be provided to
        // associate fixers with the validator.
        .def("RegisterLayerValidator", &_RegisterLayerValidator,
             (args("metadata", "layerTaskFn"),
              arg("fixers") = list()))
        .def("RegisterStageValidator", &_RegisterStageValidator,
             (args("metadata", "stageTaskFn"),
              arg("fixers") = list()))
        .def("RegisterPrimValidator", &_RegisterPrimValidator,
             (args("metadata", "primTaskFn"),
              arg("fixers") = list()))
        .def("RegisterValidatorSuite", &_RegisterValidatorSuite,
             (args("metadata", "validators")))
        // Plugin registration -- metadata comes from plugInfo.json.
        // Use when implementing a validator declared in a plugin.
        // An optional list of ValidationFixer objects can be provided to
        // associate fixers with the validator.
        .def("RegisterPluginLayerValidator",
             &_RegisterPluginLayerValidator,
             (args("validatorName", "layerTaskFn"),
              arg("fixers") = list()))
        .def("RegisterPluginStageValidator",
             &_RegisterPluginStageValidator,
             (args("validatorName", "stageTaskFn"),
              arg("fixers") = list()))
        .def("RegisterPluginPrimValidator",
             &_RegisterPluginPrimValidator,
             (args("validatorName", "primTaskFn"),
              arg("fixers") = list()))
        .def("RegisterPluginValidatorSuite",
             &_RegisterPluginValidatorSuite,
             (args("validatorSuiteName", "validators")));
}
