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

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace
{

nb::list _GetOrLoadAllValidators(UsdValidationRegistry &validationRegistry)
{
    nb::list result;
    for (const auto *validator : validationRegistry.GetOrLoadAllValidators())
    {
        result.append(nb::cast(validator, nb::rv_policy::reference));
    }
    return result;
}

nb::list _GetOrLoadValidatorsByName(UsdValidationRegistry &validationRegistry,
                                    const TfTokenVector &validatorNames)
{
    nb::list result;
    for (const auto *validator :
         validationRegistry.GetOrLoadValidatorsByName(validatorNames))
    {
        result.append(nb::cast(validator, nb::rv_policy::reference));
    }
    return result;
}

nb::list _GetOrLoadAllValidatorSuites(UsdValidationRegistry &validationRegistry)
{
    nb::list result;
    for (const auto *validator :
         validationRegistry.GetOrLoadAllValidatorSuites())
    {
        result.append(nb::cast(validator, nb::rv_policy::reference));
    }
    return result;
}

nb::list _GetOrLoadValidatorSuitesByName(UsdValidationRegistry &validationRegistry,
                                         const TfTokenVector &suiteNames)
{
    nb::list result;
    for (const auto *validatorSuite :
         validationRegistry.GetOrLoadValidatorSuitesByName(suiteNames))
    {
        result.append(nb::cast(validatorSuite, nb::rv_policy::reference));
    }
    return result;
}

nb::object _GetValidatorMetadata(const UsdValidationRegistry &validationRegistry,
                                 const TfToken &name)
{
    UsdValidationValidatorMetadata metadata;
    bool success = validationRegistry.GetValidatorMetadata(name, &metadata);
    if (success)
    {
        return nb::cast(metadata);
    }

    return nb::none();
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
_ExtractErrorsFromPyResult(nb::object result)
{
    // Caller must hold the GIL.  We coerce the return value to a
    //pxr_boost::python::list explicitly so that any non-list iterable
    // (e.g. a generator) is also accepted.
    UsdValidationErrorVector errors;
    nb::list resultList(result);
    for (size_t i = 0, n = nb::len(resultList); i < n; ++i) {
        errors.push_back(nb::cast<UsdValidationError>(resultList[i]));
    }
    return errors;
}

template <typename TaskFn>
struct _WrapTaskFnHelper;

template <typename... Args>
struct _WrapTaskFnHelper<std::function<UsdValidationErrorVector(Args...)>>
{
    static std::function<UsdValidationErrorVector(Args...)> WrapPyTaskFn(
        nb::object pyFn)
    {
        TfPyObjWrapper wrapper(pyFn);
        return [wrapper](Args&&... args) -> UsdValidationErrorVector {
            TfPyLock lock;
            try {
                nb::object fn = wrapper.Get();
                nb::object result = fn(nb::cast(
                    std::forward<Args>(args))...);
                return _ExtractErrorsFromPyResult(result);
            }
            catch (nb::python_error &e) {
                e.restore();
                TfPyConvertPythonExceptionToTfErrors();
                PyErr_Clear();
                return {};
            }
        };
    }
};

static UsdValidateLayerTaskFn
_WrapLayerTaskFn(nb::object pyFn)
{
    return _WrapTaskFnHelper<UsdValidateLayerTaskFn>::WrapPyTaskFn(pyFn);
}

static UsdValidateStageTaskFn
_WrapStageTaskFn(nb::object pyFn)
{
    return _WrapTaskFnHelper<UsdValidateStageTaskFn>::WrapPyTaskFn(pyFn);
}

static UsdValidatePrimTaskFn
_WrapPrimTaskFn(nb::object pyFn)
{
    return _WrapTaskFnHelper<UsdValidatePrimTaskFn>::WrapPyTaskFn(pyFn);
}

// The three _Register* functions below are thin shims that wrap the Python
// callable and forward to the appropriate RegisterValidator() overload.
// They are bound as Python methods on ValidationRegistry below.

static void
_RegisterLayerValidator(UsdValidationRegistry &registry,
                        const UsdValidationValidatorMetadata &metadata,
                        nb::object pyFn,
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
                        nb::object pyFn,
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
                       nb::object pyFn,
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
                        nb::list validators)
{
    // Extract raw pointers from the Python Validator objects.  The registry
    // owns all validators and they are immortal for the process lifetime, so
    // raw-pointer storage is safe here.
    std::vector<const UsdValidationValidator *> containedValidators;
    for (size_t i = 0, n = nb::len(validators); i < n; ++i) {
        containedValidators.push_back(
            nb::cast<const UsdValidationValidator *>(
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
                              nb::object pyFn,
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
                              nb::object pyFn,
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
                             nb::object pyFn,
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
                              nb::list validators)
{
    std::vector<const UsdValidationValidator *> containedValidators;
    for (size_t i = 0, n = nb::len(validators); i < n; ++i) {
        containedValidators.push_back(
            nb::cast<const UsdValidationValidator *>(
                validators[i]));
    }
    registry.RegisterPluginValidatorSuite(
        validatorSuiteName, containedValidators);
}

} // anonymous namespace

void wrapUsdValidationRegistry(nb::module_ &m)
{
    nb::class_<UsdValidationRegistry>(m, "ValidationRegistry")
        .def_static("__new__",
             [](nb::handle, nb::args, nb::kwargs) -> UsdValidationRegistry & {
                 return UsdValidationRegistry::GetInstance();
             },
             nb::rv_policy::reference)
        .def("__init__", [](nb::handle, nb::args, nb::kwargs) {})
        .def("HasValidator", &UsdValidationRegistry::HasValidator,
             nb::arg("validatorName"))
        .def("HasValidatorSuite", &UsdValidationRegistry::HasValidatorSuite,
             nb::arg("suiteName"))
        .def("GetOrLoadAllValidators", &_GetOrLoadAllValidators)
        .def("GetOrLoadValidatorByName",
             &UsdValidationRegistry::GetOrLoadValidatorByName,
             nb::rv_policy::reference,
             nb::arg("validatorName"))
        .def("GetOrLoadValidatorsByName", &_GetOrLoadValidatorsByName,
             nb::arg("validatorNames"))
        .def("GetOrLoadAllValidatorSuites", &_GetOrLoadAllValidatorSuites)
        .def("GetOrLoadValidatorSuiteByName",
             &UsdValidationRegistry::GetOrLoadValidatorSuiteByName,
             nb::rv_policy::reference,
             nb::arg("suiteName"))
        .def("GetOrLoadValidatorSuitesByName", &_GetOrLoadValidatorSuitesByName,
             nb::arg("suiteNames"))
        .def("GetValidatorMetadata", &_GetValidatorMetadata, nb::arg("name"))
        .def("GetAllValidatorMetadata",
             &UsdValidationRegistry::GetAllValidatorMetadata)
        .def("GetValidatorMetadataForPlugin",
             &UsdValidationRegistry::GetValidatorMetadataForPlugin,
             nb::arg("pluginName"))
        .def("GetValidatorMetadataForKeyword",
             &UsdValidationRegistry::GetValidatorMetadataForKeyword,
             nb::arg("keyword"))
        .def("GetValidatorMetadataForSchemaType",
             &UsdValidationRegistry::GetValidatorMetadataForSchemaType,
             nb::arg("schemaType"))
        .def("GetValidatorMetadataForPlugins",
             &UsdValidationRegistry::GetValidatorMetadataForPlugins,
             nb::arg("pluginNames"))
        .def("GetValidatorMetadataForKeywords",
             &UsdValidationRegistry::GetValidatorMetadataForKeywords,
             nb::arg("keywords"))
        .def("GetValidatorMetadataForSchemaTypes",
             &UsdValidationRegistry::GetValidatorMetadataForSchemaTypes,
             nb::arg("schemaTypes"))
        // Explicit registration -- caller provides full metadata.
        // Use when registering validators at runtime without a plugin.
        // An optional list of ValidationFixer objects can be provided to
        // associate fixers with the validator.
        .def("RegisterLayerValidator", &_RegisterLayerValidator,
             nb::arg("metadata"), nb::arg("layerTaskFn"),
             nb::arg("fixers") = std::vector<UsdValidationFixer>())
        .def("RegisterStageValidator", &_RegisterStageValidator,
             nb::arg("metadata"), nb::arg("stageTaskFn"),
             nb::arg("fixers") = std::vector<UsdValidationFixer>())
        .def("RegisterPrimValidator", &_RegisterPrimValidator,
             nb::arg("metadata"), nb::arg("primTaskFn"),
             nb::arg("fixers") = std::vector<UsdValidationFixer>())
        .def("RegisterValidatorSuite", &_RegisterValidatorSuite,
             nb::arg("metadata"), nb::arg("validators"))
        // Plugin registration -- metadata comes from plugInfo.json.
        // Use when implementing a validator declared in a plugin.
        // An optional list of ValidationFixer objects can be provided to
        // associate fixers with the validator.
        .def("RegisterPluginLayerValidator",
             &_RegisterPluginLayerValidator,
             nb::arg("validatorName"), nb::arg("layerTaskFn"),
             nb::arg("fixers") = std::vector<UsdValidationFixer>())
        .def("RegisterPluginStageValidator",
             &_RegisterPluginStageValidator,
             nb::arg("validatorName"), nb::arg("stageTaskFn"),
             nb::arg("fixers") = std::vector<UsdValidationFixer>())
        .def("RegisterPluginPrimValidator",
             &_RegisterPluginPrimValidator,
             nb::arg("validatorName"), nb::arg("primTaskFn"),
             nb::arg("fixers") = std::vector<UsdValidationFixer>())
        .def("RegisterPluginValidatorSuite",
             &_RegisterPluginValidatorSuite,
             nb::arg("validatorSuiteName"), nb::arg("validators"));
}
