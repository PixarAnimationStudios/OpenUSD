//
// Copyright 2024 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_USD_USD_VALIDATOR_H
#define PXR_USD_USD_VALIDATOR_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/stage.h"

#include <functional>
#include <variant>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdValidationError;
using UsdValidationErrorVector = std::vector<UsdValidationError>;
class UsdPrim;

/// \class UsdValidatorMetadata
///
/// A structure which describes metadata for a UsdValidator.
///
/// The metadata values are populated from the plugInfo.json associated with a
/// validator's plugin. PlugInfo can provide the following validator metadata:
///
/// - name: A required field. The metadata stores the validator name as a fully
/// qualified name which includes the pluginName as well, separated by ":".
/// - keywords: Keywords associated with this validator. 
/// - docs: Doc string explainaing the purpose of the validator. 
/// - schemaTypes: If the validator is associated with specific schemaTypes.
/// - isSuite: If the validator represents a suite of validators.
///
struct UsdValidatorMetadata
{
    /// "pluginName:testName" corresponding to the entry in plugInfo.json
    /// This is a mandatory field for a ValidatorMetadata.
    TfToken name;
    
    /// list of keywords extracted for this test from the plugInfo.json
    std::vector<TfToken> keywords;

    /// doc string extracted from plugInfo.json
    /// This is a mandatory field for a ValidatorMetadata.
    std::string docs;

    /// list of schemaTypes names this test applies to, extracted from
    /// plugInfo.json
    std::vector<TfToken> schemaTypes;

    /// whether this test represents a test suite or not
    bool isSuite;
}; // UsdValidatorMetadata

// TODO:
// - TimeCode (Range), leaving right now for brevity. Will introduce in
// subsequent iterations.
//
//

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
    const SdfLayerHandle&)>;
/// UsdValidateStageTaskFn: Validation logic operating on a given UsdStage
using UsdValidateStageTaskFn = std::function<UsdValidationErrorVector(
    const UsdStagePtr&)>;
/// UsdValidatePrimTaskFn: Validation logic operating on a given UsdPrim
using UsdValidatePrimTaskFn = std::function<UsdValidationErrorVector(
    const UsdPrim&)>;

/// @}

/// \class UsdValidator
///
/// UsdValidator is a class describing a single test.
///
/// An instance of UsdValidator is created when plugins are loaded and tests are
/// registered and cached in the UsdValidationRegistry.  UsdValidator can
/// consist of any one of the 3 testing tasks: LayerTestingTask,
/// StageTestingTask or PrimTestingTask, which correspond to testing the given
/// SdfLayer, an entire UsdStage or a UsdPrim respectively. UsdValidator
/// instances are immutable and non-copyable.
///
class UsdValidator
{
public:
    /// Instantiate a UsdValidator which has no validation logic implementation.
    /// This is primarily used by UsdValidatorSuite.
    USD_API
    explicit UsdValidator(const UsdValidatorMetadata &metadata);

    UsdValidator(const UsdValidator &other) = delete;
    UsdValidator &operator=(const UsdValidator&) = delete;

    UsdValidator(UsdValidator &&other) noexcept = default;
    UsdValidator& operator=(UsdValidator&&) noexcept = default;

    /// Instantiate a UsdValidator which has its validation logic implemented by
    /// a UsdValidateLayerTaskFn. 
    USD_API
    UsdValidator(const UsdValidatorMetadata &metadata,
                 const UsdValidateLayerTaskFn &validateLayerTaskFn);

    /// Instantiate a UsdValidator which has its validation logic implemented by
    /// a UsdValidateStageTaskFn.
    USD_API
    UsdValidator(const UsdValidatorMetadata &metadata,
                 const UsdValidateStageTaskFn &validateStageTaskFn);

    /// Instantiate a UsdValidator which has its validation logic implemented by
    /// a UsdValidatePrimTaskFn.
    USD_API
    UsdValidator(const UsdValidatorMetadata &metadata,
                 const UsdValidatePrimTaskFn &validatePrimTaskFn);

    /// Return metadata associated with this Validator.
    const UsdValidatorMetadata& GetMetadata() const &
    {
        return _metadata;
    }

    /// Return metadata associated with this validator by-value.
    UsdValidatorMetadata GetMetadata() const &&
    {
        return std::move(_metadata);
    }

    /// Run validation on the given \p layer by executing the contained
    /// validateTaskFn and returns UsdValidationErrorVector.
    ///
    /// If this Validator doesn't provide a UsdValidateLayerTaskFn, then an
    /// empty vector is returned, which signifies no error.
    USD_API
    const UsdValidationErrorVector Validate(const SdfLayerHandle &layer) const;

    /// Run validation on the given \p usdStage by executing the contained
    /// validateTaskFn and returns UsdValidationErrorVector.
    ///
    /// If this Validator doesn't provide a UsdValidateStageTaskFn, then an
    /// empty vector is returned, which signifies no error.
    USD_API
    const UsdValidationErrorVector Validate(const UsdStagePtr &usdStage) const;

    /// Run validation on the given \p usdPrim by executing the contained
    /// validateTaskFn and returns UsdValidationErrorVector.
    ///
    /// If this Validator doesn't provide a UsdValidatePrimTaskFn, then an
    /// empty vector is returned, which signifies no error.
    USD_API
    const UsdValidationErrorVector Validate(const UsdPrim& usdPrim) const;

  private:
    UsdValidatorMetadata _metadata;
    std::variant<UsdValidateLayerTaskFn, 
                 UsdValidateStageTaskFn,
                 UsdValidatePrimTaskFn> _validateTaskFn;

    // Return UsdValidateLayerTaskFn if provided by the validator, else a
    // nullptr.
    const UsdValidateLayerTaskFn* _GetValidateLayerTask() const;

    // Return UsdValidateStageTaskFn if provided by the validator, else a
    // nullptr.
    const UsdValidateStageTaskFn* _GetValidateStageTask() const;

    // Return UsdValidatePrimTaskFn if provided by the validator, else a
    // nullptr.
    const UsdValidatePrimTaskFn* _GetValidatePrimTask() const;

}; // UsdValidator

/// \class UsdValidatorSuite
///
/// UsdValidatorSuite acts like a suite for a collection of tests, which
/// clients can use to bundle all tests relevant to test their concepts.
///
/// If client failed to provide isSuite metadata for a UsdValidatorSuite
/// instance then the validatorSuite will not be registered, and client will
/// appropriately be warned.
class UsdValidatorSuite
{
public:
    /// Instantiate UsdValidatorSuite using \p metadata and a vector of \p
    /// validators.
    USD_API
    UsdValidatorSuite(const UsdValidatorMetadata &metadata,
                      const std::vector<const UsdValidator*>& validators);

    UsdValidatorSuite(UsdValidatorSuite &&other) noexcept = default;

    UsdValidatorSuite& operator=(UsdValidatorSuite&&) noexcept = default;

    /// Returns a vector of const UsdValidator pointers, which make this
    /// UsdValidatorSuite. Note that the validators are guaranteed to be valid,
    /// since their lifetime is managed by the UsdValidationRegistry, which has
    /// a higher scope than individual validators. 
    const std::vector<const UsdValidator*>& GetContainedValidators() const &
    {
        return _containedValidators;
    }

    /// Returns a vector of const UsdValidator pointers, which make this
    /// UsdValidatorSuite. Note that the validators are guaranteed to be valid,
    /// since their lifetime is managed by the UsdValidationRegistry, which has
    /// a higher scope than individual validators. 
    std::vector<const UsdValidator*> GetContainedValidators() const &&
    {
        return _containedValidators;
    }

    /// Return metadata associated with this validator.
    const UsdValidatorMetadata& GetMetadata() const &
    {
        return _metadata;
    }

    /// Return metadata associated with this validator.
    UsdValidatorMetadata GetMetadata() const &&
    {
        return std::move(_metadata);
    }

private:
    UsdValidatorMetadata _metadata;
    std::vector<const UsdValidator*> _containedValidators;
}; // UsdValidatorSuite

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_VALIDATOR_H
