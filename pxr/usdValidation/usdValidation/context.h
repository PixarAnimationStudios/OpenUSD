//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_VALIDATION_USD_VALIDATION_CONTEXT_H
#define PXR_USD_VALIDATION_USD_VALIDATION_CONTEXT_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usdValidation/usdValidation/api.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"
#include "pxr/usdValidation/usdValidation/validator.h"

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

class TfType;
class WorkDispatcher;
class Usd_PrimFlagsPredicate;

/// \class UsdValidationContext
///
/// UsdValidationContext provides an interface for managing and running
/// validators on USD layers, stages, or prims.
///
/// The UsdValidationContext can be constructed using various methods to select
/// validators by keywords, schema types, plugins, or pre-selected sets of
/// validatorMetadata or validators.
///
/// Pre-selected set of UsdValidationValidatorMetadata or
/// UsdValidationValidators can be gathered using various UsdValidationRegistry
/// APIs. For example, a client can construct a validation context by providing
/// validator metadata for all usdGeom plugin validators but excluding all
/// validators metadata belonging to a specific schemaType like UsdGeomPoints.
///
/// When schema type validators are provided, an appropriate hierarchy of schema
/// validators are included in the selected list of validators. For example,
/// if UsdGeomSphere schema type is provided, validators for UsdGeomGprim,
/// UsdGeomBoundable, UsdGeomXformable, and UsdGeomImageable are also included.
///
/// Clients can also provide \p includeAllAncestors flag (defaults to true) to
/// also select validators from all ancestor TfType for any selected schemaType
/// validator when initializing a UsdValidationContext using \p keywords and
/// of the provided schemaType(s) validators.\p metadata constructors.
///
/// Once a context is created and the list of validators to be run is populated,
/// clients can simply run the validators on a layer, stage or a set of prims.
/// All validators with UsdValidateLayerTaskFn, UsdValidateStageTaskFn or
/// UsdValidatePrimTaskFn will be run in a parallel and UsdValidationError will
/// be collected for each validator.
///
/// Note that initializing a UsdValidationContext can result in loading new
/// plugins, if the validators are not already loaded.
///
/// UsdValidationContext::Validate() call could initiate a stage traversal, and
/// appropriately call various validation tasks on the validators.
///
/// UsdValidationContext does not hold any state about the validation errors
/// collected during validation. The errors are returned as a vector of
/// UsdValidationError when Validate() is called.
///
/// \sa UsdValidationRegistry
/// \sa UsdValidationValidator
/// \sa UsdValidationError
///
class UsdValidationContext
{
public:
    /// Create a UsdValidationContext by collecting validators using the
    /// specified keywords.
    ///
    /// \param keywords
    /// A vector of keywords to select validators from. All validators having
    /// these keywords will get loaded and included in the selected group of
    /// validators to be run for validation. It will also collect validators
    /// from a UsdValidationValidatorSuite if the suite also contains the
    /// specified keywords.
    ///
    /// \param includeAllAncestors
    /// An optional parameter to include all validators from ancestor TfTypes
    /// for any schema type validators found, default is `true`.
    USDVALIDATION_API
    explicit UsdValidationContext(const TfTokenVector &keywords,
                                  bool includeAllAncestors = true);

    /// Create a UsdValidationContext by collecting validators using the
    /// specified vector of plugins.
    ///
    /// \param plugins
    /// A vector of plugins to select validators from. All validators belonging
    /// to the specified plugins will get loaded and included in the selected
    /// group of validators to be run for validation. It will also collect
    /// validators from a UsdValidationValidatorSuite if the suite belongs to
    /// the specified plugins
    ///
    /// \param includeAllAncestors
    /// An optional parameter to include all validators from ancestor TfTypes
    /// for any schema type validators found, default is `true`.
    USDVALIDATION_API
    explicit UsdValidationContext(const PlugPluginPtrVector &plugins,
                                  bool includeAllAncestors = true);

    /// Create a UsdValidationContext by collecting validators using the
    /// specified vector of validator metadata.
    ///
    /// \param metadata
    /// A vector of validator metadata. All validators corresponding to the
    /// metadata will get loaded and included in the selected group of
    /// validators to be run for validation. It will also collect validators
    /// from a UsdValidationValidatorSuite if a metadata has isSuite set to
    /// true.
    ///
    /// \param includeAllAncestors
    /// An optional parameter to include all validators from ancestor TfTypes
    /// for any schema type validators found, default is `true`.
    USDVALIDATION_API
    explicit UsdValidationContext(
        const UsdValidationValidatorMetadataVector &metadata,
        bool includeAllAncestors = true);

    /// Create a UsdValidationContext by collecting validators using the
    /// specified schema types.
    ///
    /// \param schemaTypes
    /// A vector of schema types to select validators from. All validators
    /// corresponding to the provided schemaTypes are included in the selected
    /// group of validators to be run for validation.
    ///
    /// Note that all validators corresponding to the ancestor TfTypes for the
    /// provided schemaTypes are included in the selected group of validators.
    USDVALIDATION_API
    explicit UsdValidationContext(const std::vector<TfType> &schemaTypes);

    /// Create a UsdValidationContext by collecting validators using the
    /// specified vector of validators.
    ///
    /// \param validators
    /// A vector of explicit validators.
    USDVALIDATION_API
    explicit UsdValidationContext(
        const std::vector<const UsdValidationValidator *> &validators);

    /// Create a UsdValidationContext by collecting validators from the
    /// specified vector of validator suites.
    ///
    /// \param suites
    /// A vector of validator suites.
    USDVALIDATION_API
    explicit UsdValidationContext(
        const std::vector<const UsdValidationValidatorSuite *> &suites);

    /// Run validation on the given valid \p layer by executing the selected
    /// validators for this UsdValidationContext; Returns a vector of errors
    /// collected during validation.
    ///
    /// Only layer validators in the selected group of validators will be run on
    /// the given layer.
    ///
    /// All the validators run in parallel. Any resulting errors are collected
    /// in the returned vector.
    ///
    /// Note that it's the responsibility of the caller to maintain the lifetime
    /// of the layer during the lifetime of the this validation context.
    /// UsdValidationErrorSites in the returned vector will reference the layer
    /// and hence are valid only as long as the layer is valid.
    ///
    /// A coding error is issued if the layer being validated is not valid.
    USDVALIDATION_API
    UsdValidationErrorVector Validate(const SdfLayerHandle &layer) const;

    /// Run validation on the given valid \p stage by executing the selected
    /// validators for this UsdValidationContext; Returns a vector of errors
    ///
    /// Any Layer validators in the selected group of validators will be run on
    /// the layers reachable from the stage. In addition to that any Stage
    /// validators will also be run on the given stage. The stage will also be
    /// traversed to run prim and schema type validators on all the prims in the
    /// stage. \p predicate will be used to traverse the prims to be validated.
    ///
    /// PrimValidators and StageValidators will be passed the provided 
    /// \p timeRange.
    ///
    /// All the validators run in parallel. Any resulting errors are collected
    /// in the returned vector.
    ///
    /// Note that it's the responsibility of the caller to maintain the lifetime
    /// of the stage during the lifetime of the this validation context.
    /// UsdValidationErrorSites in the returned vector will reference the stage
    /// and hence are valid only as long as the stage is valid.
    ///
    /// A coding error is issued if the stage being validated is not valid.
    USDVALIDATION_API
    UsdValidationErrorVector Validate(
        const UsdStagePtr &stage, 
        const Usd_PrimFlagsPredicate &predicate, 
        const UsdValidationTimeRange &timeRange) const;

    /// Run validation on the given valid \p stage by executing the selected
    /// validators for this UsdValidationContext; Returns a vector of errors
    ///
    /// StageValidators and PrimValidators will also be passed the default
    /// UsdValidationTimeRange, which uses GfInterval::GetFullInterval() 
    /// as the time interval.
    ///
    /// \sa UsdValidationContext::Validate(const UsdStagePtr &stage,
    /// const Usd_PrimFlagsPredicate &predicate) const
    USDVALIDATION_API
    UsdValidationErrorVector Validate(
        const UsdStagePtr &stage, 
        const Usd_PrimFlagsPredicate &predicate) const;

    /// Run validation on the given valid \p stage by executing the selected
    /// validators for this UsdValidationContext; Returns a vector of errors
    ///
    /// \ref UsdTraverseInstanceProxies "Instance Proxy predicate" is used to
    /// traverse the prims to be validated in this overload.
    ///
    /// StageValidators and PrimValidators will also be passed the default
    /// UsdValidationTimeRange, which uses GfInterval::GetFullInterval() 
    /// as the time interval.
    ///
    /// \sa UsdValidationContext::Validate(const UsdStagePtr &stage,
    /// const Usd_PrimFlagsPredicate &predicate) const
    USDVALIDATION_API
    UsdValidationErrorVector Validate(const UsdStagePtr &stage) const;

    /// Run validation on the given valid \p stage by executing the selected
    /// validators for this UsdValidationContext; Returns a vector of errors
    ///
    /// PrimValidators and StageValidators will be passed the provided 
    /// \p timeRange.
    ///
    /// \ref UsdTraverseInstanceProxies "Instance Proxy predicate" is used to
    /// traverse the prims to be validated in this overload.
    ///
    /// \sa UsdValidationContext::Validate(const UsdStagePtr &stage,
    /// const Usd_PrimFlagsPredicate &predicate) const
    USDVALIDATION_API
    UsdValidationErrorVector Validate(
        const UsdStagePtr &stage, 
        const UsdValidationTimeRange &timeRange) const;

    /// Run validation on the given valid \p stage by executing the selected
    /// validators for this UsdValidationContext; Returns a vector of errors
    ///
    /// PrimValidators and StageValidators will be run on all \p timeCodes.
    /// If \p timeCodes is empty, then no validation will be performed.
    ///
    /// Note that non-time dependent validators will be run only once and time 
    /// dependent validators will be run at all the provided timeCodes.
    ///
    /// \sa UsdValidationContext::Validate(const UsdStagePtr &stage,
    /// const Usd_PrimFlagsPredicate &predicate) const
    USDVALIDATION_API
    UsdValidationErrorVector Validate(
        const UsdStagePtr &stage, 
        const Usd_PrimFlagsPredicate &predicate,
        const std::vector<UsdTimeCode> &timeCodes) const;

    /// Run validation on the given valid \p stage by executing the selected
    /// validators for this UsdValidationContext; Returns a vector of errors
    ///
    /// PrimValidators and StageValidators will be run on all \p timeCodes.
    /// If \p timeCodes is empty, then no validation will be performed.
    ///
    /// Note that non-time dependent validators will be run only once and time 
    /// dependent validators will be run at all the provided timeCodes.
    ///
    /// \ref UsdTraverseInstanceProxies "Instance Proxy predicate" is used to
    /// traverse the prims to be validated in this overload.
    ///
    /// \sa UsdValidationContext::Validate(const UsdStagePtr &stage,
    /// const Usd_PrimFlagsPredicate &predicate) const
    USDVALIDATION_API
    UsdValidationErrorVector Validate(
        const UsdStagePtr &stage, 
        const std::vector<UsdTimeCode> &timeCodes) const;

    /// Run validation on the given valid \p prims by executing the selected
    /// validators for this UsdValidationContext; Returns a vector of errors
    /// collected during validation.
    ///
    /// Prim and Schema type validators will be run on the given prims using
    /// UsdValidationTimeRange(), if \p timeRange is not provided.
    ///
    /// All the validators run in parallel. Any resulting errors are collected
    /// in the returned vector.
    ///
    /// Note that it's the responsibility of the caller to maintain the lifetime
    /// of the stage that the prims belong to, during the lifetime of the
    /// this validation context.
    ///
    /// A coding error is issued if any of the prims being validated are
    /// invalid.
    USDVALIDATION_API
    UsdValidationErrorVector Validate(
        const std::vector<UsdPrim> &prims, 
        const UsdValidationTimeRange &timeRange = {}) const;

    /// Run validation on the given valid \p prims by executing the selected
    /// validators for this UsdValidationContext; Returns a vector of errors
    /// collected during validation.
    ///
    /// Prim and Schema type validators will be run on the given prims using
    /// UsdValidationTimeRange(), if \p timeRange is not provided.
    ///
    /// Note that it's the responsibility of the caller to maintain the lifetime
    /// of the stage that the prims belong to, during the lifetime of the
    /// this validation context.
    ///
    /// All the validators run in parallel. Any resulting errors are collected
    /// in the returned vector.
    ///
    /// A coding error is issued if any of the prims being validated are
    /// invalid.
    USDVALIDATION_API
    UsdValidationErrorVector Validate(
        const UsdPrimRange &prims, 
        const UsdValidationTimeRange &timeRange = {}) const;

    /// Run validation on the given valid \p prims by executing the selected
    /// validators for this UsdValidationContext; Returns a vector of errors
    /// collected during validation.
    ///
    /// Prim and Schema type validators will be run on all \p timeCodes.
    /// If \p timeCodes is empty, then no validation will be performed.
    ///
    /// Note that non-time dependent validators will be run only once and time 
    /// dependent validators will be run at all the provided timeCodes.
    /// 
    /// \sa UsdValidationContext::Validate(const std::vector<UsdPrim> &prims,
    /// UsdValidationTimeRange timeRange) const
    USDVALIDATION_API
    UsdValidationErrorVector Validate(
        const std::vector<UsdPrim> &prims, 
        const std::vector<UsdTimeCode> &timeCodes) const;

    /// Run validation on the given valid \p prims by executing the selected
    /// validators for this UsdValidationContext; Returns a vector of errors
    /// collected during validation.
    ///
    /// Prim and Schema type validators will be run on all \p timeCodes.
    /// If \p timeCodes is empty, then no validation will be performed.
    ///
    /// Note that non-time dependent validators will be run only once and time 
    /// dependent validators will be run at all the provided timeCodes.
    /// 
    /// \sa UsdValidationContext::Validate(const UsdPrimRange &prims,
    /// UsdValidationTimeRange timeRange) const
    USDVALIDATION_API
    UsdValidationErrorVector Validate(
        const UsdPrimRange &prims, 
        const std::vector<UsdTimeCode> &timeCodes) const;

private:
    // helper to initialize UsdValidationContext, given a vector of metadata
    // and a flag to include all ancestors.
    void _InitializeFromValidatorMetadata(
        const UsdValidationValidatorMetadataVector &metadata,
        bool includeAllAncestors);

    // Distribute the validators into different groups based on the type of
    // validation to be performed. This method distributes the selected
    // validators into _layerValidators, _stageValidators, _primValidators and
    // _schemaTypeValidators.
    void _DistributeValidators(
        const std::vector<const UsdValidationValidator *> &validators);

    // Helper enum to specify the state of time dependency for validation.
    // This is used to determine if context need to run just time dependent, 
    // just non-time dependent or all validators.
    enum class _TimeDependencyState {
        DoTimeDependent,
        DoNonTimeDependent,
        All
    };

    // Private helper functions to validate layers, stages and prims.
    void _ValidateLayer(WorkDispatcher &dispatcher, const SdfLayerHandle &layer,
                        UsdValidationErrorVector *errors,
                        std::mutex *errorsMutex) const;

    // Helper function to validate a stage.
    //    - Layer Validators for all used layers in the stage
    //    - Stage Validators for the stage
    //    - Prim Validators for all prims in the stage, using the predicate
    //      to traverse the prims on the stage.
    //
    // Depending on whether an explicit timeRange is provided or a vector
    // of timeCodes are provided, this function will do the following:
    //  - if timeRange is provided, it uses that to validate the stage and
    //  its prims using the predicate.
    //  - if timeCodes are provided:
    //      - Run all non-time dependent validators once.
    //      - Run all time dependent validators for all timeCodes.
    void _ValidateStage(WorkDispatcher &dispatcher,
                        const UsdStagePtr &stage, 
                        UsdValidationErrorVector *errors,
                        std::mutex *errorsMutex,
                        const Usd_PrimFlagsPredicate &predicate,
                        const std::variant<UsdValidationTimeRange,
                            std::vector<UsdTimeCode>> &times) const;

    // Helper function to validate prims. Generalized for UsdPrimRange and
    // vector of UsdPrims.
    template <typename T>
    void _ValidatePrims(
        WorkDispatcher &dispatcher, const T &prims, 
        UsdValidationErrorVector *errors, std::mutex *errorsMutex,
        UsdValidationTimeRange timeRange,
        _TimeDependencyState timeDependencyState = 
            _TimeDependencyState::All) const;

    // Helper function to run _ValidatePrims within a scoped dispatcher.
    // Templated to handle both PrimRange and vector of UsdPrims.
    // Depending on whether an explicit timeRange is provided or a vector
    // of timeCodes are provided, this function will do the following:
    // - if timeRange is provided, it uses that to validate the prims.
    // - if timeCodes are provided:
    //    - Run all non-time dependent validators once.
    //    - Run all time dependent validators for all timeCodes.
    template <typename T>
    void _RunValidatePrims(
        const T &prims, UsdValidationErrorVector *errors, 
        std::mutex *errorsMutex, 
        const std::variant<UsdValidationTimeRange, 
            std::vector<UsdTimeCode>> &times) const;

    // Validators catering to a specific schemaType
    using _SchemaTypeValidatorPair
        = std::pair<TfToken, std::vector<const UsdValidationValidator *>>;
    using _SchemaTypeValidatorPairVector
        = std::vector<_SchemaTypeValidatorPair>;

    // Vectors of selected sets of validators, which will be run for this
    // UsdValidationContext. Validation tasks will be enqueued for each of these
    // validators on a given layer / stage or prims (traversed or explicitly
    // specified).
    std::vector<const UsdValidationValidator *> _layerValidators;
    std::vector<const UsdValidationValidator *> _stageValidators;
    std::vector<const UsdValidationValidator *> _primValidators;

    // validators here will be used to validate prims based on their schema
    // types, such that:
    // - For every typed schemaType found in here, prim being validated will be
    //   checked if it satisfies the IsA<schemaType> and validation task will be
    //   enqueued.
    // - For every applied schemaType found in here, prim's appliedAPISchemas
    //   will be checked and if found, validation task will be enqueued for the
    //   prim.
    _SchemaTypeValidatorPairVector _schemaTypeValidators;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_VALIDATION_USD_VALIDATION_CONTEXT_H
