//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdValidation/usdValidation/context.h"

#include "pxr/base/tf/type.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/withScopedParallelism.h"
#include "pxr/usd/usd/primFlags.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"
#include "pxr/usdValidation/usdValidation/validator.h"

#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Helper function to collect validators from the given vector of metadata.
// This can result in new plugins being loaded. If the validator metadata
// being processed is a suite, then all validators contained in the suite
// are collected.
static void
_AppendValidatorsFromMetadata(
    const UsdValidationValidatorMetadataVector &metadata,
    std::unordered_set<const UsdValidationValidator *> *outUniqueValidators)
{
    UsdValidationRegistry &validationRegistry
        = UsdValidationRegistry::GetInstance();

    for (const UsdValidationValidatorMetadata &m : metadata) {
        if (!m.isSuite) {
            if (const UsdValidationValidator *const validator
                = validationRegistry.GetOrLoadValidatorByName(m.name)) {
                outUniqueValidators->insert(validator);
            }
        } else {
            if (const UsdValidationValidatorSuite *const suite
                = validationRegistry.GetOrLoadValidatorSuiteByName(m.name)) {
                const std::vector<const UsdValidationValidator *>
                    suiteValidators = suite->GetContainedValidators();
                outUniqueValidators->insert(suiteValidators.begin(),
                                            suiteValidators.end());
            }
        }
    }
}

// Helper function to collect all validators for the given schema types,
// including all ancestor types. This then calls _AppendValidatorsFromMetadata
// to collect validators for the ancestor schema types.
static void
_CollectAncestorTypeValidators(
    const TfTokenVector &schemaTypeNames,
    std::unordered_set<const UsdValidationValidator *> *outUniqueValidators)
{
    std::unordered_set<TfToken, TfToken::HashFunctor> allTypes(
        schemaTypeNames.begin(), schemaTypeNames.end());

    // Collect ancestor type validators for the filtered schema types.
    // This is done before filtering the validators by schema types, so that
    // validators for all ancestor types are collected.
    for (const TfToken &schemaType : schemaTypeNames) {
        const TfType type = UsdSchemaRegistry::GetTypeFromName(schemaType);
        std::vector<TfType> ancestorTypes;
        type.GetAllAncestorTypes(&ancestorTypes);
        for (const TfType &ancestorType : ancestorTypes) {
            allTypes.insert(TfToken(ancestorType.GetTypeName()));
        }
    }

    // Convert the set to a vector.
    TfTokenVector allSchemaTypes(allTypes.begin(), allTypes.end());
    UsdValidationRegistry &validationRegistry
        = UsdValidationRegistry::GetInstance();
    const UsdValidationValidatorMetadataVector ancestorsMetadata
        = validationRegistry.GetValidatorMetadataForSchemaTypes(allSchemaTypes);
    _AppendValidatorsFromMetadata(ancestorsMetadata, outUniqueValidators);
}

static bool
_ShouldRunSchemaTypeValidator(const UsdPrim &prim, const TfToken &schema)
{
    // Check if the prim is of the given schema type.
    const TfType type = TfType::FindByName(schema.GetString());
    if (!type) {
        return false;
    }

    const TfToken schemaTypeName
        = UsdSchemaRegistry::GetInstance().GetSchemaTypeName(type);
    if (prim.IsA(schemaTypeName)) {
        return true;
    }

    // If schema is not an API schema, then it should not be run for the prim.
    if (!UsdSchemaRegistry::GetInstance().IsAppliedAPISchema(type)) {
        return false;
    }

    const TfTokenVector apiSchemas = prim.GetAppliedSchemas();
    // Check if the prim has the given API schema applied.
    return std::find(apiSchemas.begin(), apiSchemas.end(), schemaTypeName)
        != apiSchemas.end();
}

UsdValidationContext::UsdValidationContext(const TfTokenVector &keywords,
                                           bool includeAllAncestors)
{
    UsdValidationRegistry &validationRegistry
        = UsdValidationRegistry::GetInstance();

    const UsdValidationValidatorMetadataVector validatorsMetadata
        = validationRegistry.GetValidatorMetadataForKeywords(keywords);
    _InitializeFromValidatorMetadata(validatorsMetadata, includeAllAncestors);
}

UsdValidationContext::UsdValidationContext(const PlugPluginPtrVector &plugins,
                                           bool includeAllAncestors)
{
    UsdValidationRegistry &validationRegistry
        = UsdValidationRegistry::GetInstance();

    const TfTokenVector pluginNames = [&plugins]() {
        TfTokenVector names;
        names.reserve(plugins.size());
        for (const PlugPluginPtr &plugin : plugins) {
            if (plugin) {
                names.emplace_back(plugin->GetName());
            }
        }
        return names;
    }();

    const UsdValidationValidatorMetadataVector validatorsMetadata
        = validationRegistry.GetValidatorMetadataForPlugins(pluginNames);
    _InitializeFromValidatorMetadata(validatorsMetadata, includeAllAncestors);
}

UsdValidationContext::UsdValidationContext(
    const UsdValidationValidatorMetadataVector &metadata,
    bool includeAllAncestors)
{
    _InitializeFromValidatorMetadata(metadata, includeAllAncestors);
}

void
UsdValidationContext::_InitializeFromValidatorMetadata(
    const UsdValidationValidatorMetadataVector &metadata,
    bool includeAllAncestors)
{
    std::unordered_set<const UsdValidationValidator *> uniqueValidators;
    // collect validators from the metadata
    _AppendValidatorsFromMetadata(metadata, &uniqueValidators);

    if (includeAllAncestors) {
        const TfTokenVector schemaTypesFromMetadata
            = [&metadata]() -> TfTokenVector {
            std::unordered_set<TfToken, TfToken::HashFunctor> uniqueTypes;
            for (const UsdValidationValidatorMetadata &m : metadata) {
                uniqueTypes.insert(m.schemaTypes.begin(), m.schemaTypes.end());
            }
            return { uniqueTypes.begin(), uniqueTypes.end() };
        }();

        // if the collected validators have schematypes metadata, collect all
        // ancestor type validators for those schema types.
        _CollectAncestorTypeValidators(schemaTypesFromMetadata,
                                       &uniqueValidators);
    }
    // distribute the collected validators into different sets based on the type
    // of validation to be performed.
    _DistributeValidators({ uniqueValidators.begin(), uniqueValidators.end() });
}

UsdValidationContext::UsdValidationContext(
    const std::vector<TfType> &schemaTypes)
{
    const TfTokenVector schemaTypeNames = [&schemaTypes]() {
        TfTokenVector names;
        names.reserve(schemaTypes.size());
        for (const TfType &type : schemaTypes) {
            names.emplace_back(type.GetTypeName());
        }
        return names;
    }();

    std::unordered_set<const UsdValidationValidator *> uniqueValidators;
    // collect validators for the given schema types, including all ancestor
    // type validators.
    _CollectAncestorTypeValidators(schemaTypeNames, &uniqueValidators);
    // distribute the collected validators into different sets based on the type
    // of validation to be performed.
    _DistributeValidators({ uniqueValidators.begin(), uniqueValidators.end() });
}

UsdValidationContext::UsdValidationContext(
    const std::vector<const UsdValidationValidator *> &validators)
{
    // distribute the given validators into different sets based on the type of
    // validation to be performed.
    _DistributeValidators(validators);
}

UsdValidationContext::UsdValidationContext(
    const std::vector<const UsdValidationValidatorSuite *> &suites)
{
    std::unordered_set<const UsdValidationValidator *> uniqueValidators;
    for (const UsdValidationValidatorSuite *suite : suites) {
        if (!suite) {
            continue;
        }
        const std::vector<const UsdValidationValidator *> suiteValidators
            = suite->GetContainedValidators();
        uniqueValidators.insert(suiteValidators.begin(), suiteValidators.end());
    }
    _DistributeValidators({ uniqueValidators.begin(), uniqueValidators.end() });
}

void
UsdValidationContext::_DistributeValidators(
    const std::vector<const UsdValidationValidator *> &validators)
{
    size_t layerValidatorCount = 0;
    size_t stageValidatorCount = 0;
    size_t primValidatorCount = 0;

    // Count the number of validators for each type of validation.
    for (const UsdValidationValidator *validator : validators) {
        if (validator->_GetValidateLayerTask()) {
            ++layerValidatorCount;
        } else if (validator->_GetValidateStageTask()) {
            ++stageValidatorCount;
        } else if (validator->_GetValidatePrimTask()) {
            ++primValidatorCount;
        }
    }

    _layerValidators.reserve(layerValidatorCount);
    _stageValidators.reserve(stageValidatorCount);
    _primValidators.reserve(primValidatorCount);

    for (const UsdValidationValidator *const validator : validators) {
        if (validator->_GetValidateLayerTask()) {
            _layerValidators.push_back(validator);
        } else if (validator->_GetValidateStageTask()) {
            _stageValidators.push_back(validator);
        } else if (validator->_GetValidatePrimTask()) {
            if (validator->GetMetadata().schemaTypes.empty()) {
                _primValidators.push_back(validator);
            } else {
                // Distribute schema type validators into a vector of pairs,
                // where the first element is the schema type and the second
                // element is a vector of validators for that schema type.
                for (const TfToken &schemaType :
                     validator->GetMetadata().schemaTypes) {
                    const auto it = std::find_if(
                        _schemaTypeValidators.begin(),
                        _schemaTypeValidators.end(),
                        [&](const _SchemaTypeValidatorPair &pair) {
                            return pair.first == schemaType;
                        });
                    if (it != _schemaTypeValidators.end()) {
                        it->second.push_back(validator);
                    } else {
                        _schemaTypeValidators.emplace_back(
                            schemaType,
                            std::vector<const UsdValidationValidator *> {
                                validator });
                    }
                }
            }
        }
    }
}

UsdValidationErrorVector
UsdValidationContext::Validate(const SdfLayerHandle &layer) const
{
    if (!layer) {
        TF_CODING_ERROR("Invalid layer provided to validate.");
        return {};
    }

    UsdValidationErrorVector errors;
    std::mutex errorsMutex;
    WorkWithScopedDispatcher(
        [this, &layer, &errors, &errorsMutex](WorkDispatcher &dispatcher) {
            _ValidateLayer(dispatcher, layer, &errors, &errorsMutex);
        });
    return errors;
}

UsdValidationErrorVector
UsdValidationContext::Validate(
    const UsdStagePtr &stage,
    const Usd_PrimFlagsPredicate &predicate,
    const UsdValidationTimeRange &timeRange) const
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage provided to validate.");
        return {};
    }

    UsdValidationErrorVector errors;
    std::mutex errorsMutex;
    WorkWithScopedDispatcher([this, &stage, &errors, &errorsMutex,
                              &predicate, &timeRange](
                             WorkDispatcher &dispatcher) {
        _ValidateStage(dispatcher, stage, &errors, &errorsMutex, predicate,
                       timeRange);
    });
    return errors;
}

UsdValidationErrorVector
UsdValidationContext::Validate(const UsdStagePtr &stage,
                               const Usd_PrimFlagsPredicate &predicate) const
{
    return Validate(stage, predicate, UsdValidationTimeRange());
}

UsdValidationErrorVector
UsdValidationContext::Validate(const UsdStagePtr &stage) const
{
    return Validate(stage, UsdTraverseInstanceProxies(UsdPrimDefaultPredicate));
}

UsdValidationErrorVector
UsdValidationContext::Validate(
    const UsdStagePtr &stage,
    const UsdValidationTimeRange &timeRange) const
{
    return Validate(stage, UsdTraverseInstanceProxies(UsdPrimDefaultPredicate),
                    timeRange);
}

UsdValidationErrorVector
UsdValidationContext::Validate(const UsdStagePtr &stage,
                               const Usd_PrimFlagsPredicate &predicate,
                               const std::vector<UsdTimeCode> &timeCodes) const
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage provided to validate.");
        return {};
    }

    UsdValidationErrorVector errors;
    std::mutex errorsMutex;
    WorkWithScopedDispatcher(
        [this, &stage, &errors, &errorsMutex, &predicate, timeCodes](
            WorkDispatcher &dispatcher) {
        _ValidateStage(dispatcher, stage, &errors, &errorsMutex, predicate,
                       timeCodes);
    });

    return errors;
}

UsdValidationErrorVector
UsdValidationContext::Validate(const UsdStagePtr &stage,
                               const std::vector<UsdTimeCode> &timeCodes) const
{
    return Validate(stage, UsdTraverseInstanceProxies(UsdPrimDefaultPredicate),
                    timeCodes);
}

UsdValidationErrorVector
UsdValidationContext::Validate(
    const std::vector<UsdPrim> &prims, 
    const UsdValidationTimeRange &timeRange) const
{
    UsdValidationErrorVector errors;
    std::mutex errorsMutex;
    _RunValidatePrims(prims, &errors, &errorsMutex, timeRange);
    return errors;
}

UsdValidationErrorVector
UsdValidationContext::Validate(
    const UsdPrimRange &prims, 
    const UsdValidationTimeRange &timeRange) const
{
    UsdValidationErrorVector errors;
    std::mutex errorsMutex;
    _RunValidatePrims(prims, &errors, &errorsMutex, timeRange);
    return errors;
}

UsdValidationErrorVector
UsdValidationContext::Validate(
    const std::vector<UsdPrim> &prims, 
    const std::vector<UsdTimeCode> &timeCodes) const
{
    UsdValidationErrorVector errors;
    std::mutex errorsMutex;
    _RunValidatePrims(prims, &errors, &errorsMutex, timeCodes);
    return errors;
}

UsdValidationErrorVector
UsdValidationContext::Validate(
    const UsdPrimRange &prims, const std::vector<UsdTimeCode> &timeCodes) const
{
    UsdValidationErrorVector errors;
    std::mutex errorsMutex;
    _RunValidatePrims(prims, &errors, &errorsMutex, timeCodes);
    return errors;
}

// Helper function to add errors to the output vector guarded by a mutex.
void
_AddErrors(const UsdValidationErrorVector &errors,
           UsdValidationErrorVector *outErrors, std::mutex *errorsMutex)
{
    if (errors.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(*errorsMutex);
    outErrors->insert(outErrors->end(), errors.begin(), errors.end());
}

void
UsdValidationContext::_ValidateLayer(WorkDispatcher &dispatcher,
                                     const SdfLayerHandle &layer,
                                     UsdValidationErrorVector *errors,
                                     std::mutex *errorsMutex) const
{
    // If we reached here via Validate(const SdfLayerHandle&), then the layer
    // must be valid. Else if we reach here via _ValidateStage, then the layer
    // should be valid as well as these are gathered using
    // UsdStage::GetUsedLayers.
    if (!TF_VERIFY(layer)) {
        return;
    }

    for (const UsdValidationValidator *validator : _layerValidators) {
        dispatcher.Run([validator, layer, errors, errorsMutex]() {
            _AddErrors(validator->Validate(layer), errors, errorsMutex);
        });
    }
}

void
UsdValidationContext::_ValidateStage(
    WorkDispatcher &dispatcher, const UsdStagePtr &stage,
    UsdValidationErrorVector *errors, std::mutex *errorsMutex,
    const Usd_PrimFlagsPredicate &predicate,
    const std::variant<UsdValidationTimeRange, 
        std::vector<UsdTimeCode>> &times) const
{
    // If we reached here via Validate(const UsdStagePtr&), then the stage
    // must be valid.
    if (!TF_VERIFY(stage)) {
        return;
    }

    for (const SdfLayerHandle &layer : stage->GetUsedLayers()) {
        _ValidateLayer(dispatcher, layer, errors, errorsMutex);
    }

    auto _RunValidators = [this, &dispatcher, stage, errors, errorsMutex, 
                           predicate](
            const UsdValidationTimeRange &timeRange,
            const _TimeDependencyState timeDependencyState = 
                _TimeDependencyState::All) 
    {
        for (const UsdValidationValidator *validator : _stageValidators) {
            // skip non-time dependent validators if state is DoTimeDependent,
            // or skip time dependent validators if state is DoNonTimeDependent
            if ((timeDependencyState == _TimeDependencyState::DoTimeDependent
                    && !validator->GetMetadata().isTimeDependent) || 
                (timeDependencyState == _TimeDependencyState::DoNonTimeDependent
                    && validator->GetMetadata().isTimeDependent)) {
                continue;
            }
            dispatcher.Run([validator, stage, errors, errorsMutex, timeRange]() {
                _AddErrors(validator->Validate(stage, timeRange), errors,
                           errorsMutex);
            });
        }

        _ValidatePrims(dispatcher, stage->Traverse(predicate), errors, 
                       errorsMutex, timeRange, timeDependencyState);
    };

    // Run stage and prim validators for timeRange
    if (std::holds_alternative<UsdValidationTimeRange>(times)) {
            _RunValidators(std::get<UsdValidationTimeRange>(times));
        return;
    }

    std::vector<UsdTimeCode> timeCodes = 
        std::get<std::vector<UsdTimeCode>>(times);
    if (timeCodes.empty()) {
        return;
    }

    // Run TimeNonDependent validators
    _RunValidators(UsdValidationTimeRange(), 
                   _TimeDependencyState::DoNonTimeDependent);

    for (const UsdTimeCode &timeCode : timeCodes) {
        _RunValidators(UsdValidationTimeRange(timeCode), 
                       _TimeDependencyState::DoTimeDependent);
    }
}

template <typename T>
void
UsdValidationContext::_RunValidatePrims(
    const T &prims, UsdValidationErrorVector *errors, std::mutex *errorsMutex,
    const std::variant<UsdValidationTimeRange, 
        std::vector<UsdTimeCode>> &times) const
{
    // Both PrimRange and vector<> have empty() method. We only use this for
    // these 2 types.
    if (prims.empty()) {
        return;
    }

    WorkWithScopedDispatcher(
        [this, &prims, errors, errorsMutex, times](WorkDispatcher &dispatcher) {
            // timeRange
            if (std::holds_alternative<UsdValidationTimeRange>(times)) {
                _ValidatePrims(dispatcher, prims, errors, errorsMutex, 
                               std::get<UsdValidationTimeRange>(times));
                return;
            }

            // Explicit timeCodes
            std::vector<UsdTimeCode> timeCodes = 
                std::get<std::vector<UsdTimeCode>>(times);
            if (timeCodes.empty()) {
                return;
            }

            // Run only non-time dependent validators for all prims once
            _ValidatePrims(dispatcher, prims, errors, errorsMutex, 
                           UsdValidationTimeRange(),
                           _TimeDependencyState::DoNonTimeDependent);
            // Run only time dependent validators for each timeCode in timeCodes
            for (const UsdTimeCode &timeCode : timeCodes) {
                _ValidatePrims(dispatcher, prims, errors, errorsMutex, 
                               UsdValidationTimeRange(timeCode),
                               _TimeDependencyState::DoTimeDependent);
            }
        });
}

template <typename T>
void
UsdValidationContext::_ValidatePrims(
    WorkDispatcher &dispatcher, const T &prims, 
    UsdValidationErrorVector *errors, std::mutex *errorsMutex,
    UsdValidationTimeRange timeRange, 
    const _TimeDependencyState timeDependencyState) const
{
    for (const UsdValidationValidator *validator : _primValidators) {
        // skip non-time dependent validators if state is DoTimeDependent, or
        // skip time dependent validators if state is DoNonTimeDependent
        if ((timeDependencyState == _TimeDependencyState::DoTimeDependent
                && !validator->GetMetadata().isTimeDependent) ||
            (timeDependencyState == _TimeDependencyState::DoNonTimeDependent
                && validator->GetMetadata().isTimeDependent)) {
            continue;
        }
        for (const UsdPrim &prim : prims) {
            if (!prim) {
                TF_CODING_ERROR("Invalid prim found in the vector of prims to "
                                "validate.");
                return;
            }
            dispatcher.Run([validator, prim, errors, errorsMutex, 
                           timeRange]() {
                _AddErrors(validator->Validate(prim, timeRange), 
                           errors, errorsMutex);
            });
        }
    }

    for (const _SchemaTypeValidatorPair &pair : _schemaTypeValidators) {
        for (const UsdPrim &prim : prims) {
            if (!prim) {
                TF_CODING_ERROR("Invalid prim found in the vector of prims to "
                                "validate.");
                return;
            }
            if (_ShouldRunSchemaTypeValidator(prim, pair.first)) {
                for (const UsdValidationValidator *validator : pair.second) {
                    // skip non-time dependent validators if state is 
                    // DoTimeDependent, or
                    // skip time dependent validators if state is
                    // DoNonTimeDependent
                    if ((timeDependencyState == 
                            _TimeDependencyState::DoTimeDependent && 
                                !validator->GetMetadata().isTimeDependent) ||
                        (timeDependencyState ==
                            _TimeDependencyState::DoNonTimeDependent &&
                                validator->GetMetadata().isTimeDependent)) {
                        continue;
                    }
                    dispatcher.Run(
                        [validator, prim, errors, errorsMutex, timeRange]() {
                            _AddErrors(validator->Validate(prim, timeRange), 
                                       errors, errorsMutex);
                    });
                }
            }
        }
    }
}

// Explicit instantiations for the required types: UsdPrimRange and vector of
// Prims
template void UsdValidationContext::_ValidatePrims<UsdPrimRange>(
    WorkDispatcher &dispatcher, const UsdPrimRange &prims,
    UsdValidationErrorVector *errors, std::mutex *errorsMutex,
    UsdValidationTimeRange timeRange, 
    const _TimeDependencyState timeDependencyState) const;

template void UsdValidationContext::_ValidatePrims<std::vector<UsdPrim>>(
    WorkDispatcher &dispatcher, const std::vector<UsdPrim> &prims,
    UsdValidationErrorVector *errors, std::mutex *errorsMutex,
    UsdValidationTimeRange timeRange, 
    const _TimeDependencyState timeDependencyState) const;

PXR_NAMESPACE_CLOSE_SCOPE
