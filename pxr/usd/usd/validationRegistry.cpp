//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/usd/validationRegistry.h"

#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/token.h"

#include <algorithm>
#include <memory>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(UsdValidationRegistry);

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((PluginValidatorsKey, "Validators"))
    ((Keywords, "keywords"))
    ((Doc, "doc"))
    ((SchemaTypes, "schemaTypes"))
    ((IsSuite, "isSuite"))
    ((PluginValidatorNameDelimiter, ":"))
);



UsdValidationRegistry::UsdValidationRegistry()
{
    // Do any plugin processing before subscription starts.
    _PopulateMetadataFromPlugInfo();
    TfSingleton<UsdValidationRegistry>::SetInstanceConstructed(*this);
    TfRegistryManager::GetInstance().SubscribeTo<UsdValidationRegistry>();
}

// Helper to return a TfTokenVector from a json string array
static TfTokenVector
_FillTokenVector(const JsValue &value)
{
    TfTokenVector tokens;
    const auto arr = value.GetArrayOf<std::string>();
    tokens.reserve(arr.size());
    for (const auto &a : arr) {
        tokens.emplace_back(a);
    }
    return tokens;
}

// Helper to return common keywords associated with all validators in plugInfo
static TfTokenVector
_ParseStringArrayValue(const JsObject &object, const TfToken &token,
                       const TfToken &validatorName = TfToken())
{
    const std::string &errorStrTemplate = 
        "Expected array of strings for " + token.GetString() + " metadata";
    const std::string &errorStr = validatorName.IsEmpty() ?
        errorStrTemplate : 
        errorStrTemplate + " for validator " + validatorName.GetString();
    if (const JsValue * const value = TfMapLookupPtr(object, token)) {
        if (!value->IsArrayOf<std::string>()) {
            TF_RUNTIME_ERROR("%s", errorStr.c_str());
            return TfTokenVector{};
        }
        return _FillTokenVector(*value);
    }
    return TfTokenVector{};
}

void 
UsdValidationRegistry::_PopulateMetadataFromPlugInfo()
{
    const PlugPluginPtrVector &plugins =
        PlugRegistry::GetInstance().GetAllPlugins();

    auto _GetKey = [](const JsObject &dict, const std::string &key,
                      JsObject *value) {
        const JsObject::const_iterator i = dict.find(key);
        if (i != dict.end() && i->second.IsObject()) {
            *value = i->second.GetJsObject();
            return true;
        }
        return false;
    };

    for(const PlugPluginPtr &plugin : plugins) {
        JsObject validators;
        const JsObject &plugMetadata = plugin->GetMetadata();

        // No Validators in plugInfo, continue
        if (!_GetKey(plugMetadata, _tokens->PluginValidatorsKey, &validators)) {
            continue;
        }

        const TfTokenVector keywords = 
            _ParseStringArrayValue(validators,
                                   _tokens->Keywords);
        
        const TfToken pluginName = TfToken(plugin->GetName());
        TfTokenVector validatorNames;
        // Parse all validator names and respective plugMetadata
        for (const auto &validatorEntry : validators) {
            // Skip keywords, move to validator entries
            if (validatorEntry.first == _tokens->Keywords) {
                continue;
            }
            const TfToken validatorName(validatorEntry.first);
            JsObject validatorDict;
            if (!_GetKey(validators, validatorName, &validatorDict)) {
                TF_RUNTIME_ERROR("Expected dict for validator '%s'",
                                 validatorName.GetText());
                continue;
            }

            const JsValue * const doc = 
                TfMapLookupPtr(validatorDict, _tokens->Doc);
            if (!doc || !doc->IsString() || doc->GetString().empty()) {
                TF_RUNTIME_ERROR("Missing or invalid or empty doc string "
                                 "metadata for '%s' validator",
                                 validatorName.GetText());
                continue;
            }

            UsdValidatorMetadata metadata;

            metadata.pluginPtr = plugin;
            // prefix pluginName to validatorName to make these plugin
            // validator's metadata.name unique
            metadata.name = TfToken(TfStringJoin(
                std::vector<std::string>{pluginName.GetString(), 
                                         validatorName.GetString()},
                _tokens->PluginValidatorNameDelimiter.GetText()));
            validatorNames.push_back(metadata.name);

            metadata.doc = doc->GetString();

            metadata.schemaTypes = 
                _ParseStringArrayValue(validatorDict, _tokens->SchemaTypes,
                                       metadata.name);
            
            metadata.keywords = keywords;
            const TfTokenVector &localKeywords = 
                _ParseStringArrayValue(validatorDict, _tokens->Keywords,
                                       metadata.name);
            metadata.keywords.reserve(
                metadata.keywords.size() + localKeywords.size());
            metadata.keywords.insert(metadata.keywords.end(), 
                                     localKeywords.begin(), 
                                     localKeywords.end());
            
            if (const JsValue * const isSuite = 
                TfMapLookupPtr(validatorDict, _tokens->IsSuite)) {
                if (!isSuite->IsBool()) {
                    TF_RUNTIME_ERROR("Expected bool for isSuite for validator "
                                     "'%s'", metadata.name.GetText());
                } else {
                    metadata.isSuite = isSuite->GetBool();
                }
            } else {
                metadata.isSuite = false;
            }

            _AddValidatorMetadata(metadata);
        }
        if (!validatorNames.empty()) {
            _pluginNameToValidatorNames.emplace(plugin->GetName(), 
                                                validatorNames);
        }
    }
}

void 
UsdValidationRegistry::RegisterPluginValidator(
    const TfToken &validatorName, const UsdValidateLayerTaskFn &layerTaskFn)
{
    _RegisterPluginValidator<UsdValidateLayerTaskFn>(
            validatorName, layerTaskFn);
}

void
UsdValidationRegistry::RegisterPluginValidator(
    const TfToken &validatorName, const UsdValidateStageTaskFn &stageTaskFn)
{
    _RegisterPluginValidator<UsdValidateStageTaskFn>(
            validatorName, stageTaskFn);
}

void
UsdValidationRegistry::RegisterPluginValidator(
    const TfToken &validatorName, const UsdValidatePrimTaskFn &primTaskFn)
{
    _RegisterPluginValidator<UsdValidatePrimTaskFn>(
            validatorName, primTaskFn);
}

template<typename ValidateTaskFn>
void
UsdValidationRegistry::_RegisterPluginValidator(const TfToken &validatorName, 
                                                const ValidateTaskFn &taskFn)
{
    static_assert(std::is_same_v<ValidateTaskFn, UsdValidateLayerTaskFn> ||
                  std::is_same_v<ValidateTaskFn, UsdValidateStageTaskFn> ||
                  std::is_same_v<ValidateTaskFn, UsdValidatePrimTaskFn>,
                  "template parameter must be UsdValidateLayerTaskFn," 
                  "UsdValidateStageTaskFn, or UsdValidatePrimTaskFn");

    UsdValidatorMetadata metadata;
    if (!GetValidatorMetadata(validatorName, &metadata)) {
        // if this validatorName is from a plugin, which it should be since
        // this API is only for registering validators which are defined
        // in plugInfo, then we should have parsed its metadata already,
        // and if it's not found that means it's not coming from a plugInfo,
        // so bail out.
        TF_CODING_ERROR(
            "Validator metadata missing for '%s', validator registered "
            "using this API must be defined in the plugInfo.json",
            validatorName.GetText());
        return;
    }

    _RegisterValidator(metadata, taskFn, /* addMetadata */ false);
}

void
UsdValidationRegistry::RegisterValidator(
    const UsdValidatorMetadata &metadata, 
    const UsdValidateLayerTaskFn &layerTaskFn)
{
    _RegisterValidator(metadata, layerTaskFn);
}

void
UsdValidationRegistry::RegisterValidator(
    const UsdValidatorMetadata &metadata, 
    const UsdValidateStageTaskFn &stageTaskFn)
{
    _RegisterValidator(metadata, stageTaskFn);
}

void
UsdValidationRegistry::RegisterValidator(
    const UsdValidatorMetadata &metadata, 
    const UsdValidatePrimTaskFn &primTaskFn)
{
    _RegisterValidator(metadata, primTaskFn);
}

template<typename ValidateTaskFn>
void
UsdValidationRegistry::_RegisterValidator(const UsdValidatorMetadata &metadata, 
    const ValidateTaskFn &taskFn, bool addMetadata)
{
    static_assert(std::is_same_v<ValidateTaskFn, UsdValidateLayerTaskFn> ||
                  std::is_same_v<ValidateTaskFn, UsdValidateStageTaskFn> ||
                  std::is_same_v<ValidateTaskFn, UsdValidatePrimTaskFn>,
                  "template parameter must be UsdValidateLayerTaskFn," 
                  "UsdValidateStageTaskFn, or UsdValidatePrimTaskFn");

    static constexpr bool isPrimTaskFn = 
        std::is_same<ValidateTaskFn, UsdValidatePrimTaskFn>::value;
    if (!_CheckMetadata(metadata, isPrimTaskFn)) {
        return;
    }

    {
        // Lock for writing validators.
        std::unique_lock lock(_validatorMutex);
        if (_validators.find(metadata.name) != _validators.end()) {
            TF_CODING_ERROR(
                "Validator '%s' already registered with the "
                "UsdValidationRegistry", metadata.name.GetText());
            return;
        }

        // Note in case validator metadata needs to be added and there is
        // contention only the first validator's (which is being added)
        // metadata will be added.
        if (addMetadata) {
            if (!_AddValidatorMetadata(metadata)) {
                TF_CODING_ERROR(
                    "Metadata already added for a UsdValidatorSuite with the "
                    "same name '%s'.", metadata.name.GetText());
                return;
            }
        }

        std::unique_ptr<UsdValidator> validator =
            std::make_unique<UsdValidator>(metadata, taskFn);
        if (!_validators.emplace(metadata.name, std::move(validator)).second) {
            TF_CODING_ERROR(
                "Validator with name '%s' already exists, failed to register "
                "it again.", metadata.name.GetText());
        }
    }
}

bool
UsdValidationRegistry::HasValidator(const TfToken &validatorName) const
{
    std::shared_lock lock(_validatorMutex);
    return _validators.find(validatorName) != _validators.end();
}

std::vector<const UsdValidator*> 
UsdValidationRegistry::GetOrLoadAllValidators()
{
    const TfTokenVector &validatorNames = [&]() {
            TfTokenVector result;
            std::shared_lock lock(_metadataMutex);
            result.reserve(_validatorNameToMetadata.size());
            for (const auto &entry : _validatorNameToMetadata) {
                if (!entry.second.isSuite) {
                    result.push_back(entry.first);
                }
            }
            return result;
        }();

    return GetOrLoadValidatorsByName(validatorNames);
}

const UsdValidator*
UsdValidationRegistry::GetOrLoadValidatorByName(const TfToken &validatorName)
{
    auto _GetValidator = [&](const TfToken &name) -> const UsdValidator* {
        std::shared_lock lock(_validatorMutex);
        const auto &validatorItr = _validators.find(name);
        if (validatorItr != _validators.end()) {
            return validatorItr->second.get();
        }
        return nullptr;
    };

    if (const UsdValidator *const validator = _GetValidator(validatorName)) {
        return validator;
    }
    
    UsdValidatorMetadata metadata;
    if (!GetValidatorMetadata(validatorName, &metadata)) {
        // No validatorMetadata found corresponding to this validatorName
        return nullptr;
    }

    // If metadata was found but validator was not found / loaded, that implies
    // this validator must be plugin provided.
    TF_VERIFY(metadata.pluginPtr);

    if (metadata.pluginPtr->Load()) {
        // Plugin was loaded, lets look again.
        return _GetValidator(validatorName);
    }

    return nullptr;
}

std::vector<const UsdValidator*>
UsdValidationRegistry::GetOrLoadValidatorsByName(
    const TfTokenVector &validatorNames)
{
    std::vector<const UsdValidator*> validators;
    validators.reserve(validatorNames.size());

    for (const TfToken &validatorName : validatorNames) {
        const UsdValidator *validator = GetOrLoadValidatorByName(validatorName);
        // If validator is nullptr, that means the validatorName was not found
        // in the registry and it failed to register, in which case appropriate
        // coding error would have been reported.
        if (validator) {
            validators.push_back(validator);
        }
    }
    return validators;
}

void
UsdValidationRegistry::RegisterPluginValidatorSuite(
    const TfToken &suiteName, 
    const std::vector<const UsdValidator*> &containedValidators)
{
    UsdValidatorMetadata metadata;
    if (!GetValidatorMetadata(suiteName, &metadata)) {
        // if this suiteName is from a plugin, which it should be since
        // this API is only for registering validators which are defined
        // in plugInfo, then we should have parsed its metadata already,
        // and if it's not found that means it's not coming from a plugInfo,
        // so bail out.
        TF_CODING_ERROR(
            "Validator Suite metadata missing for '%s', validator registered "
            "using this API must be defined in the plugInfo.json",
            suiteName.GetText());
        return;
    }
    _RegisterValidatorSuite(metadata, containedValidators, 
                                   /* addMetadata */ false);
}

void
UsdValidationRegistry::RegisterValidatorSuite(
    const UsdValidatorMetadata &metadata, 
    const std::vector<const UsdValidator*> &containedValidators)
{
    _RegisterValidatorSuite(metadata, containedValidators);
}

void
UsdValidationRegistry::_RegisterValidatorSuite(
    const UsdValidatorMetadata &metadata,
    const std::vector<const UsdValidator*> &containedValidators,
    bool addMetadata)
{
    if (!_CheckMetadata(metadata, /* checkForPrimTask */ false, 
                        /* expectSuite */ true)) {
        return;
    }

    // Make sure containedValidators are conforming if suite has schemaTypes.
    // That is, validators have PrimTaskFn, otherwise, do not register this
    // validator. And contained validators's schemaType is a subset of Suite's
    // schemaTypes metadata.
    if (!metadata.schemaTypes.empty()) {
        for(const UsdValidator *const validator : containedValidators) {
            if (!validator) {
                // Possible clients try to register a validator which is
                // invalid/nullptr
                TF_CODING_ERROR(
                    "Validator Suite '%s' not registered, one of the contained "
                    "validator is invalid.",
                    metadata.name.GetText());
                return;
            }
            if (!validator->_GetValidatePrimTask()) {
                TF_CODING_ERROR(
                    "ValidatorSuite '%s' cannot be registered, as it provides "
                    "schemaTypes, but at least one of its contained validator "
                    "'%s' does not provide a UsdValidatePrimTaskFn", 
                    metadata.name.GetText(), 
                    validator->GetMetadata().name.GetText());
                return;
            }
            // We also need to make sure the contained validator's schemaTypes 
            // is a subset of validatorSuite's schemaTypes
            // NB: The size of the vectors here should be small.
            for (const TfToken& schemaType : 
                validator->GetMetadata().schemaTypes) {
                if (std::find(metadata.schemaTypes.begin(),
                              metadata.schemaTypes.end(), schemaType) ==
                    metadata.schemaTypes.end()) {
                    TF_CODING_ERROR(
                        "schemaType '%s' provided by a contained validator "
                        "'%s' is not in schemaTypes for '%s' validator suite",
                        schemaType.GetText(), 
                        validator->GetMetadata().name.GetText(),
                        metadata.name.GetText());
                    return;
                }
            }
        }
    }

    {
        // Lock for writing validatorSuites 
        std::unique_lock lock(_validatorSuiteMutex);
        if (_validatorSuites.find(metadata.name) != 
            _validatorSuites.end()) {
            TF_CODING_ERROR(
                "ValidatorSuite '%s' already registered with the "
                "UsdValidationRegistry", metadata.name.GetText());
            return;
        }

        // Note in case validator metadata needs to be added and there is
        // contention only the first validator's (which is being added)
        // metadata will be added.
        if (addMetadata) {
            if (!_AddValidatorMetadata(metadata)) {
                TF_CODING_ERROR(
                    "Metadata already added for a UsdValidator with the same "
                    "name '%s'.", metadata.name.GetText());
                return;
            }
        }

        std::unique_ptr<UsdValidatorSuite> validatorSuite =
            std::make_unique<UsdValidatorSuite>(metadata, containedValidators);
        if (!_validatorSuites.emplace(metadata.name, 
                                            std::move(validatorSuite)).second) {
            TF_CODING_ERROR(
                "Suite with name '%s' already exists, failed to register it "
                "again.", metadata.name.GetText());
        }
    }
}

bool
UsdValidationRegistry::HasValidatorSuite(const TfToken &suiteName) const
{
    std::shared_lock lock(_validatorSuiteMutex);
    return _validatorSuites.find(suiteName) != _validatorSuites.end();
}

std::vector<const UsdValidatorSuite*> 
UsdValidationRegistry::GetOrLoadAllValidatorSuites()
{

    const TfTokenVector suiteNames = [&]() {
            TfTokenVector result;
            std::shared_lock lock(_metadataMutex);
            result.reserve(_validatorNameToMetadata.size());
            for (const auto &entry : _validatorNameToMetadata) {
                if (entry.second.isSuite) {
                    result.push_back(entry.first);
                }
            }
            return result;
        }();
    
    std::vector<const UsdValidatorSuite*> suites;
    suites.reserve(suiteNames.size());
    for (const auto& suiteName : suiteNames) {
        const UsdValidatorSuite* suite = 
            GetOrLoadValidatorSuiteByName(suiteName);
        if (suite) {
            suites.push_back(suite);
        }
    }
    return suites;
}

const UsdValidatorSuite*
UsdValidationRegistry::GetOrLoadValidatorSuiteByName(
    const TfToken &suiteName)
{
    auto _GetValidatorSuite = [&](const TfToken &name) 
            -> const UsdValidatorSuite* {
        std::shared_lock lock(_validatorSuiteMutex);
        const auto& validatorSuiteItr = _validatorSuites.find(name);
        if (validatorSuiteItr != _validatorSuites.end()) {
            return validatorSuiteItr->second.get();
        }
        return nullptr;
    };

    if (const UsdValidatorSuite *const suite = _GetValidatorSuite(suiteName)) {
        return suite;
    }

    UsdValidatorMetadata metadata;
    if (!GetValidatorMetadata(suiteName, &metadata)) {
        // No validatorMetadata found corresponding to this suiteName
        return nullptr;
    }

    // If metadata was found but suite was not found / loaded, that implies
    // this suite must be plugin provided.
    TF_VERIFY(metadata.pluginPtr);

    if (metadata.pluginPtr->Load()) {
        return _GetValidatorSuite(suiteName);
        // Plugin was loaded, lets look again.
    }

    return nullptr;
}

std::vector<const UsdValidatorSuite*>
UsdValidationRegistry::GetOrLoadValidatorSuitesByName(
    const TfTokenVector &suiteNames)
{
    std::vector<const UsdValidatorSuite*> suites;
    suites.reserve(suiteNames.size());

    for (const TfToken &suiteName : suiteNames) {
        const UsdValidatorSuite *suite = GetOrLoadValidatorSuiteByName(suiteName);
        // if suite is nullptr, that means suiteName was not found in the
        // registry and it failed to register, in which case appropriate coding
        // error would have been reported.
        if (suite) {
            suites.push_back(suite);
        }
    }
    return suites;
}

bool
UsdValidationRegistry::GetValidatorMetadata(
    const TfToken &name, 
    UsdValidatorMetadata *metadata) const 
{
    std::shared_lock lock(_metadataMutex);
    const auto& validatorNameToMetadataItr = 
        _validatorNameToMetadata.find(name);
    if (validatorNameToMetadataItr == _validatorNameToMetadata.end()) {
        return false;
    }
    *metadata = validatorNameToMetadataItr->second;
    return true;
}

UsdValidatorMetadataVector
UsdValidationRegistry::GetAllValidatorMetadata() const
{
    UsdValidatorMetadataVector result;
    std::shared_lock lock(_metadataMutex);
    result.reserve(_validatorNameToMetadata.size());
    for (const auto &entry : _validatorNameToMetadata) {
        result.push_back(entry.second);
    }
    return result;
}

UsdValidatorMetadataVector
UsdValidationRegistry::GetValidatorMetadataForPlugin(
    const TfToken &pluginName) const
{
    return GetValidatorMetadataForPlugins({pluginName});
}

UsdValidatorMetadataVector
UsdValidationRegistry::GetValidatorMetadataForKeyword(
    const TfToken &keyword) const
{
    return GetValidatorMetadataForKeywords({keyword});
}

UsdValidatorMetadataVector
UsdValidationRegistry::GetValidatorMetadataForSchemaType(
    const TfToken &schemaType) const
{
    return GetValidatorMetadataForSchemaTypes({schemaType});
}

UsdValidatorMetadataVector
UsdValidationRegistry::GetValidatorMetadataForPlugins(
    const TfTokenVector &pluginNames) const
{
    // Since the _pluginNameToValidatorNames is created during registry
    // initialization this method is inherently thread safe, as
    // _pluginNameToValidatorNames is already populated and never updated.
    return _GetValidatorMetadataForToken(_pluginNameToValidatorNames, 
                                         pluginNames);
}

UsdValidatorMetadataVector
UsdValidationRegistry::GetValidatorMetadataForKeywords(
    const TfTokenVector &keywords) const
{
    std::shared_lock lock(_keywordValidatorNamesMutex);
    return _GetValidatorMetadataForToken(_keywordToValidatorNames, keywords);
}

UsdValidatorMetadataVector
UsdValidationRegistry::GetValidatorMetadataForSchemaTypes(
    const TfTokenVector &schemaTypes) const
{
    std::shared_lock lock(_schemaTypeValidatorNamesMutex);
    return _GetValidatorMetadataForToken(_schemaTypeToValidatorNames, 
                                         schemaTypes);
}

UsdValidatorMetadataVector
UsdValidationRegistry::_GetValidatorMetadataForToken(
    const _TokenToValidatorNamesMap &tokenToValidatorNames,
    const TfTokenVector &tokens) const
{
    UsdValidatorMetadataVector result;
    for (const TfToken &token : tokens) {
        const auto &itr = tokenToValidatorNames.find(token);
        if (itr == tokenToValidatorNames.end()) {
            continue;
        }
        std::shared_lock lockMetadata(_metadataMutex);
        for (const TfToken &validatorName : itr->second) {
            // If we have a validatorName in tokenToValidatorNames, we
            // must have a validatorMetadata for this validatorName, because
            // that's how these are added in _AddValidatorMetadata
            const auto &nameToMetadataItr = 
                _validatorNameToMetadata.find(validatorName);
            TF_VERIFY(nameToMetadataItr != _validatorNameToMetadata.end());
            result.push_back(nameToMetadataItr->second);
        }
    }
    return result;
}

bool
UsdValidationRegistry::_AddValidatorMetadata(
    const UsdValidatorMetadata &metadata)
{
    const bool didAddValidatorMetadata = [&]()
        {
            std::unique_lock lock(_metadataMutex);
            return _validatorNameToMetadata.emplace(
                metadata.name, metadata).second;
        }();

    if (didAddValidatorMetadata) {
        _UpdateValidatorNamesMappings(_schemaTypeToValidatorNames,
                                      metadata.name, metadata.schemaTypes,
                                      _schemaTypeValidatorNamesMutex);
        _UpdateValidatorNamesMappings(_keywordToValidatorNames,
                                      metadata.name, metadata.keywords,
                                      _keywordValidatorNamesMutex);
    }
    return didAddValidatorMetadata;
}

/* static */
bool
UsdValidationRegistry::_CheckMetadata(
    const UsdValidatorMetadata &metadata, 
    bool checkForPrimTask, 
    bool expectSuite)
{
    // return false if we are trying to register a validator which is
    // associated with schemaTypes, and testing task is not
    // UsdValidatePrimTaskFn!
    if (!checkForPrimTask && !metadata.schemaTypes.empty()) {
        TF_CODING_ERROR(
            "Invalid metadata for ('%s') validator. Can not provide "
            "schemaTypes metadata when registering a "
            "UsdValidateLayerTaskFn or UsdValidateStageTaskFn validator.", 
            metadata.name.GetText());
        return false;
    }

    // Return false if isSuite metadata is set, but we are dealing with a
    // UsdValidator, similarly returns false if isSuite metadata is not set, but
    // we are dealing with a UsdValidatorSuite.
    if (metadata.isSuite != expectSuite) {
        TF_CODING_ERROR(
            "Invalid metadata for '%s' validator. Incompatible isSuite "
            "metadata set. Expected '%d', but '%d' provided.",
            metadata.name.GetText(), expectSuite, metadata.isSuite);
        return false;
    }
    return true;
}

/* static */
void
UsdValidationRegistry::_UpdateValidatorNamesMappings(
    _TokenToValidatorNamesMap &tokenMap,
    const TfToken &validatorName,
    const TfTokenVector &tokens,
    std::shared_mutex &mutex)
{
    std::unique_lock lock(mutex);
    for (const TfToken &token : tokens) {
        if (tokenMap.find(token) == tokenMap.end()) {
            tokenMap.emplace(token, TfTokenVector{validatorName});
        } else {
            // Since this method is only called from the
            // _PopulateMetadataFromPlugInfo, its guaranteed that
            // validatorNames will be unique in this vector.
            tokenMap[token].push_back(validatorName);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
