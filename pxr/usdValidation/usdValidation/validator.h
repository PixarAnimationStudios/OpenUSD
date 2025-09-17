//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_VALIDATION_USD_VALIDATION_VALIDATOR_H
#define PXR_USD_VALIDATION_USD_VALIDATION_VALIDATOR_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/gf/interval.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usdValidation/usdValidation/api.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"

#include <functional>
#include <string>
#include <variant>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdValidationError;
using UsdValidationErrorVector = std::vector<UsdValidationError>;
class UsdPrim;

/// \class UsdValidationValidatorMetadata
///
/// A structure which describes metadata for a UsdValidationValidator.
///
/// The metadata values are populated from the plugInfo.json associated with a
/// validator's plugin. PlugInfo can provide the following validator metadata:
///
/// - name: A required field. This metadatum stores the validator name. For
/// validators defined in a plugin, the name must be a fully qualified name
/// which includes the pluginName as well, separated by ":". This ensures,
/// plugin provided validator names are guaranteed to be unique.
/// - pluginPtr: Pointer to the plugin where a plugin based validator is
/// defined. nullptr for a non-plugin based validator.
/// - keywords: Keywords associated with this validator.
/// - doc: Doc string explaining the purpose of the validator.
/// - schemaTypes: If the validator is associated with specific schemaTypes.
/// - isTimeDependent: If the validator is testing rules which are time
///   dependent. Note that only PrimValidators and StageValidators can be time 
///   dependent, as these are the ones which can evaluate time dependent 
///   properties. Note that its still recommended to use PrimValidators for
///   time dependent properties. Default is false.
/// - isSuite: If the validator represents a suite of validators.
///
struct UsdValidationValidatorMetadata
{
    /// Name of the validator.
    ///
    /// For plugin provided validators, this is prefixed with the pluginName,
    /// like "pluginName:testName" in order to uniquely identify these plugin
    /// provided validators.
    ///
    /// This is a mandatory field for a ValidatorMetadata.
    TfToken name;

    /// Pointer to the plugin to which a plugin based validator belongs.
    ///
    /// For non-plugin based validator, pluginPtr is nullptr.
    PlugPluginPtr pluginPtr;

    /// list of keywords extracted for this test from the plugInfo.json
    TfTokenVector keywords;

    /// doc string extracted from plugInfo.json
    /// This is a mandatory field for a ValidatorMetadata.
    std::string doc;

    /// list of schemaTypes names this test applies to, extracted from
    /// plugInfo.json
    TfTokenVector schemaTypes;

    /// whether this test is time dependent or not.
    /// Only PrimValidators and StageValidators can be time dependent.
    /// By default this is false.
    bool isTimeDependent;

    /// whether this test represents a test suite or not
    bool isSuite;
}; // UsdValidationValidatorMetadata

using UsdValidationValidatorMetadataVector
    = std::vector<UsdValidationValidatorMetadata>;

/// \defgroup UsdValidateTaskFn_group Validating Task Functions
///
/// UsdValidateLayerTaskFn, UsdValidateStageTaskFn and UsdValidatePrimTaskFn
/// represent the callbacks associated with each validator's validation logic.
///
/// Clients must provide implementation for these in their respective plugin
/// registration code.
/// @{

/// UsdValidateLayerTaskFn: Validation logic operating on a given SdfLayerHandle
using UsdValidateLayerTaskFn = std::function<UsdValidationErrorVector(
    const SdfLayerHandle &)>;
/// UsdValidateStageTaskFn: Validation logic operating on a given UsdStage
using UsdValidateStageTaskFn = std::function<UsdValidationErrorVector(
    const UsdStagePtr &, const UsdValidationTimeRange)>;
/// UsdValidatePrimTaskFn: Validation logic operating on a given UsdPrim
using UsdValidatePrimTaskFn = std::function<UsdValidationErrorVector(
    const UsdPrim &, const UsdValidationTimeRange)>;

/// @}

/// \class UsdValidationValidator
///
/// UsdValidationValidator is a class describing a single test.
///
/// An instance of UsdValidationValidator is created when plugins are loaded and
/// tests are registered and cached in the UsdValidationRegistry.
/// UsdValidationValidator can consist of any one of the 3 testing tasks:
/// LayerTestingTask, StageTestingTask or PrimTestingTask, which correspond to
/// testing the given SdfLayer, an entire UsdStage or a UsdPrim respectively.
///
/// UsdValidationValidator instances are immutable and non-copyable. Note that
/// all validators which are registered with the UsdValidationRegistry are
/// immortal.
///
/// \sa UsdValidationRegistry
class UsdValidationValidator
{
public:
    /// Instantiate a UsdValidationValidator which has no validation logic
    /// implementation.
    ///
    /// This is primarily used by UsdValidationValidatorSuite.
    USDVALIDATION_API
    explicit UsdValidationValidator(
        const UsdValidationValidatorMetadata &metadata);

    UsdValidationValidator(const UsdValidationValidator &other) = delete;
    UsdValidationValidator &operator=(const UsdValidationValidator &) = delete;

    UsdValidationValidator(UsdValidationValidator &&other) noexcept = default;
    UsdValidationValidator &operator=(UsdValidationValidator &&) noexcept
        = default;

    /// Instantiate a UsdValidationValidator which has its validation logic
    /// implemented by a UsdValidateLayerTaskFn.
    USDVALIDATION_API
    UsdValidationValidator(const UsdValidationValidatorMetadata &metadata,
                           const UsdValidateLayerTaskFn &validateLayerTaskFn);

    /// Instantiate a UsdValidationValidator which has its validation logic
    /// implemented by a UsdValidateStageTaskFn.
    USDVALIDATION_API
    UsdValidationValidator(const UsdValidationValidatorMetadata &metadata,
                           const UsdValidateStageTaskFn &validateStageTaskFn);

    /// Instantiate a UsdValidationValidator which has its validation logic
    /// implemented by a UsdValidatePrimTaskFn.
    USDVALIDATION_API
    UsdValidationValidator(const UsdValidationValidatorMetadata &metadata,
                           const UsdValidatePrimTaskFn &validatePrimTaskFn);

    /// Return metadata associated with this Validator.
    const UsdValidationValidatorMetadata &GetMetadata() const &
    {
        return _metadata;
    }

    /// Return metadata associated with this validator by-value.
    UsdValidationValidatorMetadata GetMetadata() &&
    {
        return std::move(_metadata);
    }

    /// Run validation on the given \p layer by executing the contained
    /// validateTaskFn and returns UsdValidationErrorVector.
    ///
    /// If this Validator doesn't provide a UsdValidateLayerTaskFn, then an
    /// empty vector is returned, which signifies no error.
    USDVALIDATION_API
    UsdValidationErrorVector Validate(const SdfLayerHandle &layer) const;

    /// Run validation on the given \p usdStage by executing the contained
    /// validateTaskFn and returns UsdValidationErrorVector.
    ///
    /// \p timeRange is used to evaluate the prims and their properties in 
    /// the stage at a specific time or interval. If no \p timeRange is 
    /// provided, then full time interval is used by validation callback's 
    /// implementation.
    ///
    /// If this Validator doesn't provide a UsdValidateStageTaskFn, then an
    /// empty vector is returned, which signifies no error.
    USDVALIDATION_API
    UsdValidationErrorVector Validate(
        const UsdStagePtr &usdStage,
        const UsdValidationTimeRange &timeRange = {}) const;

    /// Run validation on the given \p usdPrim by executing the contained
    /// validateTaskFn and returns UsdValidationErrorVector.
    ///
    /// \p timeRange is used to evaluate the prims and their properties in 
    /// the stage at a specific time or interval. If no \p timeRange is 
    /// provided, then full time interval is used by validation callback's 
    /// implementation.
    ///
    /// If this Validator doesn't provide a UsdValidatePrimTaskFn, then an
    /// empty vector is returned, which signifies no error.
    USDVALIDATION_API
    UsdValidationErrorVector Validate(
        const UsdPrim &usdPrim, 
        const UsdValidationTimeRange &timeRange = {}) const;

private:
    // To make sure registry can query task types on a validator.
    // Registry needs access to _GetValidatorPrimTasks, in order to determine if
    // the contained validators in a suite, which provides schemaTypes metadata
    // are compliant.
    friend class UsdValidationRegistry;
    // In order to distribute validators into different sets based on the type
    // of validation to be performed, ValidationContext needs to access
    // _GetValidateLayerTask, _GetValidateStageTask and _GetValidatePrimTask.
    friend class UsdValidationContext;

    UsdValidationValidatorMetadata _metadata;
    std::variant<UsdValidateLayerTaskFn, UsdValidateStageTaskFn,
                 UsdValidatePrimTaskFn>
        _validateTaskFn;

    // Return UsdValidateLayerTaskFn if provided by the validator, else a
    // nullptr.
    const UsdValidateLayerTaskFn *_GetValidateLayerTask() const;

    // Return UsdValidateStageTaskFn if provided by the validator, else a
    // nullptr.
    const UsdValidateStageTaskFn *_GetValidateStageTask() const;

    // Return UsdValidatePrimTaskFn if provided by the validator, else a
    // nullptr.
    const UsdValidatePrimTaskFn *_GetValidatePrimTask() const;

}; // UsdValidationValidator

/// \class UsdValidationValidatorSuite
///
/// UsdValidationValidatorSuite acts like a suite for a collection of tests,
/// which clients can use to bundle all tests relevant to test their concepts.
///
/// If client failed to provide isSuite metadata for a
/// UsdValidationValidatorSuite instance then the validatorSuite will not be
/// registered, and client will appropriately be warned.
///
/// UsdValidationValidatorSuite instances are immutable and non-copyable. Note
/// that all validator suites which are registered with the
/// UsdValidationRegistry are immortal.
///
/// isTimeDependent metadata is a no-op for a UsdValidationValidatorSuite.
///
/// \sa UsdValidationRegistry
class UsdValidationValidatorSuite
{
public:
    /// Instantiate UsdValidationValidatorSuite using \p metadata and a vector
    /// of \p validators.
    USDVALIDATION_API
    UsdValidationValidatorSuite(
        const UsdValidationValidatorMetadata &metadata,
        const std::vector<const UsdValidationValidator *> &validators);

    UsdValidationValidatorSuite(UsdValidationValidatorSuite &&other) noexcept
        = default;

    UsdValidationValidatorSuite &
    operator=(UsdValidationValidatorSuite &&) noexcept
        = default;

    /// Returns a vector of const UsdValidationValidator pointers, which make
    /// this UsdValidationValidatorSuite. Note that the validators are
    /// guaranteed to be valid, since their lifetime is managed by the
    /// UsdValidationRegistry, which has a higher scope than individual
    /// validators.
    const std::vector<const UsdValidationValidator *> &
    GetContainedValidators() const &
    {
        return _containedValidators;
    }

    /// Returns a vector of const UsdValidationValidator pointers, which make
    /// this UsdValidationValidatorSuite. Note that the validators are
    /// guaranteed to be valid, since their lifetime is managed by the
    /// UsdValidationRegistry, which has a higher scope than individual
    /// validators.
    std::vector<const UsdValidationValidator *> GetContainedValidators() &&
    {
        return std::move(_containedValidators);
    }

    /// Return metadata associated with this validator.
    const UsdValidationValidatorMetadata &GetMetadata() const &
    {
        return _metadata;
    }

    /// Return metadata associated with this validator.
    UsdValidationValidatorMetadata GetMetadata() &&
    {
        return std::move(_metadata);
    }

private:
    UsdValidationValidatorMetadata _metadata;
    std::vector<const UsdValidationValidator *> _containedValidators;
}; // UsdValidationValidatorSuite

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_VALIDATION_USD_VALIDATION_VALIDATOR_H
