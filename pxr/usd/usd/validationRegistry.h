//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_VALIDATION_REGISTRY_H
#define PXR_USD_USD_VALIDATION_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/validator.h"
#include "pxr/base/tf/singleton.h"

#include <shared_mutex>
#include <memory>
#include <unordered_map>

/// \file

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdValidationRegistry
///
/// UsdValidationRegistry manages and provides access to UsdValidator /
/// UsdValidatorSuite for USD Validation.
///
/// UsdValidationRegistry is a singleton class, which serves as a central
/// registry to hold / own all validators and validatorSuites by their names.
/// UsdValidationRegistry is also immortal and its singleton instance is never
/// destroyed. This is to ensure that all validators and suites registered with
/// the registry are available throughout the lifetime of the application.
///
/// Both Core USD and client-provided validators are registered with the
/// registry. Validators can be registered and retrieved dynamically, supporting
/// complex validation scenarios across different modules or plugins.
///
/// Clients of USD can register validators either via plugin infrastructure,
/// which results in lazy loading of the validators, or explicitly register
/// validators in their code via appropriate APIs.
///
/// As discussed in UsdValidator, validators are associated with
/// UsdValidateLayerTaskFn, UsdValidateStageTaskFn or UsdValidatePrimTaskFn,
/// which govern how a layer, stage or a prim needs to be validated.
/// UsdValidator / UsdValidatorSuite also have metadata, which can either be
/// provided in the plugInfo.json when registering the validators via plugin
/// mechanism, or by providing metadata field when registering validators.
///
/// Example of registering a validator named "StageMetadataValidator" with 
/// doc metadata using plufInfo.json:
///
/// \code
/// {
///     "Plugins": [
///     {
///         "Info": {
///             "Name": "usd"
///             "LibraryPath": "@PLUG_INFO_LIBRARY_PATH",
///             ....
///             ....
///             ....
///             "Validators": {
///                 "keywords" : ["UsdCoreValidators"],
///                 ...
///                 "StageMetadataValidator": {
///                     "doc": "Validates stage metadata."
///                 },
///                 ...
///                 ...
///                 ...
///             }
///         }
///     } ]
/// }
/// \endcode
///
/// The above example can then be registered in the plugin:
///
/// ```cpp
/// TF_REGISTRY_FUNCTION(UsdValidationRegistry)
/// {
///     UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();
///     const TfToken validatorName("usd:StageMetadataValidator");
///     const UsdValidateStageTaskFn stageTaskFn = 
///         [](const UsdStagePtr &usdStage) {
///             UsdValidationErrorVector errors;
///             if (!usdStage->GetDefaultPrim()) {
///                 errors.emplace_back(UsdValidationErrorType::Error,
///                     {UsdValidationErrorSite(usdStage, SdfPath("/"))}, 
///                     "Stage has missing or invalid defaultPrim.");
///             }
///             if (!usdStage->HasAuthoredMetadata(
///                     UsdGeomTokens->metersPerUnit)) {
///                 errors.emplace_back(UsdValidationErrorType::Error,
///                     {UsdValidationErrorSite(usdStage, SdfPath("/"))},
///                     "Stage does not specify its linear scale in "
///                     "metersPerUnit.");
///             }
///             if (!usdStage->HasAuthoredMetadata(
///                     UsdGeomTokens->upAxis)) {
///                 errors.emplace_back(UsdValidationErrorType::Error,
///                     {UsdValidationErrorSite(usdStage, SdfPath("/"))},
///                     "Stage does not specify an upAxis.");
///             }
///             return errors;
///         };
///     registry.RegisterPluginValidator(validatorName, stageTaskFn);
/// }
/// ```
///
/// Clients can also register validators by explicitly providing
/// UsdValidatorMetadata, instead of relying on plugInfo.json for the same.
/// Though its recommended to use appropriate APIs when validator metadata is
/// being provided in the plugInfo.json.
///
/// Example of validator registration by explicitly providing metadata, when its
/// not available in the plugInfo.json:
///
/// ```cpp
/// {
///     UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();
///     const UsdValidatorMetadata &metadata = GetMetadataToBeRegistered();
///     const UsdValidateLayerTaskFn &layerTask = GetLayerTaskForValidator();
///     registry.RegisterValidator(metadata, layerTask);
/// }
/// ```
///
/// Usage:
///
/// As shown above, UsdValidator or UsdValidatorSuite can be registered using
/// specific metadata or names, and retrieved by their name. The registry also
/// provides functionality to check the existence of a validator / suite, load
/// validators / suites dynamically if they are not in the registry.
///
/// Clients can also retrieve metadata for validators associated with a
/// specific plugin, keywords or schemaTypes, this can help clients filter out
/// relevant validators they need to validate their context / scene.
///
/// Note that this class is designed to be thread-safe:
/// Querying of validator metadata, registering new validator (hence mutating
/// the registry) or retrieving previously registered validator are designed to
/// be thread-safe.
///
/// \sa UsdValidator
/// \sa UsdValidatorSuite
class UsdValidationRegistry
{
    UsdValidationRegistry(const UsdValidationRegistry&) = delete;
    UsdValidationRegistry& operator=(const UsdValidationRegistry&) = delete;
public:
    USD_API
    static UsdValidationRegistry& GetInstance() {
        return TfSingleton<UsdValidationRegistry>::GetInstance();
    }

    /// Register UsdValidator defined in a plugin using \p validatorName and 
    /// \p layerTaskFn with the UsdValidationRegistry.
    ///
    /// Here \p validatorName should include the name of the plugin the
    /// validator belongs to, delimited by ":".
    ///
    /// Note calling RegisterPluginValidator with a validatorName which is
    /// already registered will result in a coding error. HasValidator can be
    /// used to determine if a validator is already registered and associated
    /// with validatorName.
    ///
    /// Also note any other failure to register a validator results in a coding 
    /// error.
    ///
    /// \sa HasValidator
    USD_API
    void RegisterPluginValidator(const TfToken &validatorName,
                           const UsdValidateLayerTaskFn &layerTaskFn);

    /// Register UsdValidator defined in a plugin using \p validatorName and 
    /// \p stageTaskFn with the UsdValidationRegistry.
    ///
    /// Here \p validatorName should include the name of the plugin the
    /// validator belongs to, delimited by ":".
    ///
    /// Note calling RegisterPluginValidator with a validatorName which is
    /// already registered will result in a coding error. HasValidator can be
    /// used to determine if a validator is already registered and associated
    /// with validatorName.
    ///
    /// Also note any other failure to register a validator results in a coding 
    /// error.
    ///
    /// \sa HasValidator
    USD_API
    void RegisterPluginValidator(const TfToken &validatorName,
                           const UsdValidateStageTaskFn &stageTaskFn);

    /// Register UsdValidator defined in a plugin using \p validatorName and 
    /// \p primTaskFn with the UsdValidationRegistry.
    ///
    /// Here \p validatorName should include the name of the plugin the
    /// validator belongs to, delimited by ":".
    ///
    /// Note calling RegisterPluginValidator with a validatorName which is
    /// already registered will result in a coding error. HasValidator can be
    /// used to determine if a validator is already registered and associated
    /// with validatorName.
    ///
    /// Also note any other failure to register a validator results in a coding 
    /// error.
    ///
    /// \sa HasValidator
    USD_API
    void RegisterPluginValidator(const TfToken &validatorName,
                           const UsdValidatePrimTaskFn &primTaskFn);

    /// Register UsdValidator using \p metadata and \p layerTaskFn
    /// with the UsdValidationRegistry.
    ///
    /// Clients can explicitly provide validator metadata, which is then used to 
    /// register a validator and associate it with name metadata. The metadata
    /// here is not specified in a plugInfo.
    ///
    /// Note calling RegisterValidator with a validator name which is already
    /// registered will result in a coding error. HasValidator can be used to
    /// determine if a validator is already registered and associated with
    /// validatorName.
    ///
    /// Also note any other failure to register a validator results in a coding 
    /// error.
    ///
    /// \sa HasValidator
    USD_API
    void RegisterValidator(const UsdValidatorMetadata &metadata,
                           const UsdValidateLayerTaskFn &layerTaskFn);

    /// Register UsdValidator using \p metadata and \p stageTaskFn
    /// with the UsdValidationRegistry.
    ///
    /// Clients can explicitly provide validator metadata, which is then used to 
    /// register a validator and associate it with name metadata. The metadata
    /// here is not specified in a plugInfo.
    ///
    /// Note calling RegisterValidator with a validator name which is already
    /// registered will result in a coding error. HasValidator can be used to
    /// determine if a validator is already registered and associated with
    /// validatorName.
    ///
    /// Also note any other failure to register a validator results in a coding 
    /// error.
    ///
    /// \sa HasValidator
    USD_API
    void RegisterValidator(const UsdValidatorMetadata &metadata,
                           const UsdValidateStageTaskFn &stageTaskFn);

    /// Register UsdValidator using \p metadata and \p primTaskFn
    /// with the UsdValidationRegistry.
    ///
    /// Clients can explicitly provide validator metadata, which is then used to 
    /// register a validator and associate it with name metadata. The metadata
    /// here is not specified in a plugInfo.
    ///
    /// Note calling RegisterValidator with a validator name which is already
    /// registered will result in a coding error. HasValidator can be used to
    /// determine if a validator is already registered and associated with
    /// validatorName.
    ///
    /// Also note any other failure to register a validator results in a coding 
    /// error.
    ///
    /// \sa HasValidator
    USD_API
    void RegisterValidator(const UsdValidatorMetadata &metadata,
                           const UsdValidatePrimTaskFn &primTaskFn);

    /// Register UsdValidatorSuite defined in a plugin using
    /// \p validatorSuiteName and \p containedValidators with the
    /// UsdValidationRegistry.
    ///
    /// Here \p validatorSuiteName should include the name of the plugin the
    /// validator belongs to, delimited by ":".
    ///
    /// Note UsdValidatorMetadata::isSuite must be set to true in the plugInfo,
    /// else the validatorSuite will not be registered.
    ///
    /// Note calling RegisterPluginValidatorSuite with a validatorSuiteName
    /// which is already registered will result in a coding error.
    /// HasValidatorSuite can be used to determine if a validator is already
    /// registered and associated with validatorName.
    ///
    /// Also note any other failure to register a validator results in a coding 
    /// error.
    ///
    /// \sa HasValidatorSuite
    USD_API
    void RegisterPluginValidatorSuite(const TfToken &validatorSuiteName, 
                                      const std::vector<const UsdValidator*>& 
                                          containedValidators);

    /// Register UsdValidatorSuite using \p metadata and
    /// \p containedValidators with the UsdValidationRegistry.
    ///
    /// Clients can explicitly provide validator metadata, which is then used to 
    /// register a suite and associate it with name metadata. The metadata
    /// here is not specified in a plugInfo.
    /// 
    /// Note UsdValidatorMetadata::isSuite must be set to true in the plugInfo,
    /// else the validatorSuite will not be registered.
    ///
    /// Note calling RegisterPluginValidatorSuite with a validatorSuiteName
    /// which is already registered will result in a coding error.
    /// HasValidatorSuite can be used to determine if a validator is already
    /// registered and associated with validatorName.
    ///
    /// Also note any other failure to register a validator results in a coding 
    /// error.
    ///
    /// \sa HasValidatorSuite
    USD_API
    void RegisterValidatorSuite(const UsdValidatorMetadata &metadata,
                                const std::vector<const UsdValidator*>&
                                    containedValidators);

    /// Return true if a UsdValidator is registered with the name \p
    /// validatorName; false otherwise.
    USD_API
    bool HasValidator(const TfToken &validatorName) const;

    /// Return true if a UsdValidatorSuite is registered with the name \p
    /// validatorSuiteName; false otherwise.
    USD_API
    bool HasValidatorSuite(const TfToken &suiteName) const;

    /// Returns a vector of const pointer to UsdValidator corresponding to all
    /// validators registered in the UsdValidationRegistry.
    ///
    /// If a validator is not found in the registry, this method will load
    /// appropriate plugins, if the validator is made available via a plugin.
    ///
    /// Note that this call will load in many plugins which provide a
    /// UsdValidator, if not already loaded. Also note that returned validators
    /// will only include validators defined in plugins or any explicitly
    /// registered validators before this call.
    USD_API
    std::vector<const UsdValidator*> GetOrLoadAllValidators();

    /// Returns a const pointer to UsdValidator if \p validatorName is found in
    /// the registry.
    ///
    /// If a validator is not found in the registry, this method will load
    /// appropriate plugins, if the validator is made available via a plugin.
    ///
    /// Returns a nullptr if no validator is found.
    USD_API
    const UsdValidator* GetOrLoadValidatorByName(const TfToken &validatorName);

    /// Returns a vector of const pointer to UsdValidator corresponding to 
    /// \p validatorNames found in the registry.
    ///
    /// If a validator is not found in the registry, this method will load
    /// appropriate plugins, if the validator is made available via a plugin.
    ///
    /// Size of returned vector might be less than the size of the input
    /// validatorNames, in case of missing validators.
    USD_API
    std::vector<const UsdValidator*> 
    GetOrLoadValidatorsByName(const TfTokenVector &validatorNames);

    /// Returns a vector of const pointer to UsdValidatorSuite corresponding to 
    /// all validator suites registered in the UsdValidationRegistry.
    ///
    /// If a suite is not found in the registry, this method will load
    /// appropriate plugins, if the suite is made available via a plugin.
    ///
    /// Note that this call might load in many plugins which provide a
    /// UsdValidatorSuite, if not already loaded. Also note that returned suites 
    /// will only include suites defined in plugins or any explicitly registered
    /// suites before this call.
    USD_API
    std::vector<const UsdValidatorSuite*> GetOrLoadAllValidatorSuites();

    /// Returns a const pointer to UsdValidatorSuite if \p suiteName
    /// is found in the registry.
    ///
    /// If a suite is not found in the registry, this method will load
    /// appropriate plugins, if the suite is made available via a plugin.
    ///
    /// Returns a nullptr if no validator is found.
    USD_API
    const UsdValidatorSuite* GetOrLoadValidatorSuiteByName(
        const TfToken &suiteName);

    /// Returns a vector of const pointer to UsdValidatorSuite corresponding to
    /// \p suiteNames found in the registry.
    ///
    /// If a suite is not found in the registry, this method will load
    /// appropriate plugins, if the suite is made available via a plugin.
    ///
    /// Size of returned vector might be less than the size of the input
    /// suiteNames, in case of missing validators.
    USD_API
    std::vector<const UsdValidatorSuite*> 
    GetOrLoadValidatorSuitesByName(const TfTokenVector &suiteNames);

    /// Returns true if metadata is found in the _validatorNameToMetadata for
    /// a validator/suite name, false otherwise.
    ///
    /// \p metadata parameter is used as an out parameter here.
    USD_API
    bool GetValidatorMetadata(const TfToken &name, 
                              UsdValidatorMetadata *metadata) const;

    /// Return vector of all UsdValidatorMetadata known to the registry 
    USD_API
    UsdValidatorMetadataVector GetAllValidatorMetadata() const;

    /// Returns vector of UsdValidatorMetadata associated with the Validators
    /// which belong to the \p pluginName.
    ///
    /// This API can be used to curate a vector of validator metadata, that 
    /// clients may want to load and use in their validation context.
    ///
    /// Note that this method does not result in any plugins to be loaded.
    USD_API
    UsdValidatorMetadataVector GetValidatorMetadataForPlugin(
        const TfToken &pluginName) const;

    /// Returns vector of UsdValidatorMetadata associated with the Validators 
    /// which has the \p keyword.
    ///
    /// This API can be used to curate a vector of validator metadata, that 
    /// clients may want to load and use in their validation context.
    ///
    /// Note that this method does not result in any plugins to be loaded.
    USD_API
    UsdValidatorMetadataVector GetValidatorMetadataForKeyword(
        const TfToken &keyword) const;

    /// Returns vector of UsdValidatorMetadata associated with the Validators 
    /// which has the \p schemaType.
    ///
    /// This API can be used to curate a vector of validator metadata, that 
    /// clients may want to load and use in their validation context.
    ///
    /// Note that this method does not result in any plugins to be loaded.
    USD_API
    UsdValidatorMetadataVector GetValidatorMetadataForSchemaType(
        const TfToken &schemaType) const;

    /// Returns vector of UsdValidatorMetadata associated with the Validators 
    /// which belong to the \p pluginNames.
    ///
    /// The returned vector is a union of all UsdValidatorMetadata associated
    /// with the plugins.
    ///
    /// This API can be used to curate a vector of validator metadata, that 
    /// clients may want to load and use in their validation context.
    ///
    /// Note that this method does not result in any plugins to be loaded.
    USD_API
    UsdValidatorMetadataVector GetValidatorMetadataForPlugins(
        const TfTokenVector &pluginNames) const;

    /// Returns vector of UsdValidatorMetadata associated with the Validators 
    /// which has at least one of the \p keywords.
    ///
    /// The returned vector is a union of all UsdValidatorMetadata associated
    /// with the keywords.
    ///
    /// This API can be used to curate a vector of validator metadata, that 
    /// clients may want to load and use in their validation context.
    ///
    /// Note that this method does not result in any plugins to be loaded.
    USD_API
    UsdValidatorMetadataVector GetValidatorMetadataForKeywords(
        const TfTokenVector &keywords) const;

    /// Returns vector of UsdValidatorMetadata associated with the Validators 
    /// which has at least one of the \p schameTypes.
    ///
    /// The returned vector is a union of all UsdValidatorMetadata associated
    /// with the schemaTypes.
    ///
    /// This API can be used to curate a vector of validator metadata, that 
    /// clients may want to load and use in their validation context.
    ///
    /// Note that this method does not result in any plugins to be loaded.
    USD_API
    UsdValidatorMetadataVector GetValidatorMetadataForSchemaTypes(
        const TfTokenVector &schemaTypes) const;

private:
    friend class TfSingleton<UsdValidationRegistry>;

    UsdValidationRegistry();

    // Initialize _validatorNameToMetadata, _keywordToValidatorNames and
    // _schemaTypeToValidatorNames by parsing all plugInfo.json, find all 
    // Validators.
    void _PopulateMetadataFromPlugInfo();

    // Templated method to register validator, called by appropriate
    // RegisterValidator methods, providing UsdValidateLayerTaskFn,
    // UsdValidateStageTaskFn or UsdValidatePrimTaskFn.
    template<typename ValidateTaskFn>
    void _RegisterPluginValidator(const TfToken &validatorName, 
        const ValidateTaskFn &taskFn);

    // Overloaded templated _RegisterValidator, where metadata is explicitly
    // provided. 
    template<typename ValidateTaskFn>
    void _RegisterValidator(const UsdValidatorMetadata &metadata, 
        const ValidateTaskFn &taskFn, bool addMetadata = true);

    void _RegisterValidatorSuite(const UsdValidatorMetadata &metadata,
        const std::vector<const UsdValidator*>& containedValidators, 
        bool addMetadata = true);

    // makes sure metadata provided is legal
    // checkForPrimTask parameter is used to determine if schemaTypes metadata
    // is provided and if the task being registered for the validator is
    // UsdValidatePrimTaskFn.
    // expectSuite parameter is used to determine if the isSuite metadata is
    // appropriately set (for UsdValidatorSuite) or not (for UsdValidator).
    static
    bool _CheckMetadata(const UsdValidatorMetadata &metadata,
            bool checkForPrimTask, bool expectSuite = false);

    // Add validator metadata to _validatorNameToMetadata, also updates
    // _schemaTypeToValidatorNames and _keywordToValidatorNames, for easy access
    // to what validators are linked to specific schemaTypes or keywords.
    // _mutex must be acquired before calling this method.
    bool _AddValidatorMetadata(const UsdValidatorMetadata &metadata);
    
    using _ValidatorNameToValidatorMap = 
        std::unordered_map<TfToken, std::unique_ptr<UsdValidator>, 
            TfToken::HashFunctor>;
    using _ValidatorSuiteNameToValidatorSuiteMap = 
        std::unordered_map<TfToken, std::unique_ptr<UsdValidatorSuite>,
            TfToken::HashFunctor>;
    using _ValidatorNameToMetadataMap =
        std::unordered_map<TfToken, UsdValidatorMetadata, TfToken::HashFunctor>;
    using _TokenToValidatorNamesMap =
        std::unordered_map<TfToken, TfTokenVector, TfToken::HashFunctor>;

    // Helper to query 
    UsdValidatorMetadataVector _GetValidatorMetadataForToken(
        const _TokenToValidatorNamesMap &tokenToValidatorNames,
        const TfTokenVector &tokens) const;

    // Helper to populate _keywordToValidatorNames and
    // _schemaTypeToValidatorNames
    // _mutex must be acquired before calling this method.
    static
    void _UpdateValidatorNamesMappings(_TokenToValidatorNamesMap &tokenMap, 
        const TfToken &validatorName, const TfTokenVector &tokens);

    // Main datastructure which holds validatorName to 
    // std::unique_ptr<UsdValidator>
    _ValidatorNameToValidatorMap _validators;
    // Main datastructure which holds suiteName to 
    // std::unique_ptr<UsdValidatorSuite>
    _ValidatorSuiteNameToValidatorSuiteMap _validatorSuites;

    // ValidatorName to ValidatorMetadata map
    _ValidatorNameToMetadataMap _validatorNameToMetadata;

    // Following 3 are helper data structures to easy lookup for Validators,
    // when queried for keywords, schemaType or pluginName.

    // This map stores the mapping from keyword to validator names. It may get
    // updated as validators can be registered dynamically outside of the plugin
    // infrastructure.
    _TokenToValidatorNamesMap _keywordToValidatorNames;

    // This map stores the mapping from schemaTypes to validator names. It may
    // get updated as validators can be registered dynamically outside of the
    // plugin infrastructure.
    _TokenToValidatorNamesMap _schemaTypeToValidatorNames;

    // This map stores the mapping from plugin names to validator names.
    // It is populated during the initialization of UsdValidationRegistry
    // and remains constant thereafter.
    _TokenToValidatorNamesMap _pluginNameToValidatorNames;

    // Mutex to protect access to all data members.
    mutable std::shared_mutex _mutex;
};

// Specialize and delete the DeleteInstance function to prevent destruction.
// This will prevent the singleton instance for UsdValidationRegistry from
// being destroyed and hence making it immortal.
template <>
void TfSingleton<UsdValidationRegistry>::DeleteInstance() = delete;

USD_API_TEMPLATE_CLASS(TfSingleton<UsdValidationRegistry>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_VALIDATION_REGISTRY_H
