# Validation

The OpenUSD Validation framework provides a system to validate assets, 
verifying core rules, schema rules, and client-provided rules via plugins,
to ensure assets are robust and interchangeable between different USD workflows.

[UsdValidationValidators](@ref UsdValidationValidator) are used to run 
validation tests. A single UsdValidationValidator instance represents a single
validation test that can result in zero or more named validation errors when 
run. For example a "usdValidation:CompositionErrorTest" validator might test for 
multiple types of composition errors, returning errors for any composition 
issues it encounters.

Each Validator instance has metadata represented by 
UsdValidationValidatorMetadata. Validator metadata includes:

* name: The validator name. For validators defined in a plugin, the name must be 
in the format "pluginName:validatorName", e.g. 
"usdGeomValidators:StageMetadataChecker"
* pluginPtr: Pointer to the plugin where a plugin based validator is defined, 
used to dynamically load plugin validators as needed. If the validator was
explicitly created (see ["Explicit Validators"](#explicit_validators) 
below), this will be null. Note that this metadata is implicitly handled by
validator registration and does not need to be provided by custom validator
developers.
* keywords: Keywords associated with this validator.
* doc: Doc string explaining the purpose of the validator.
* schemaTypes: The schema types the validator is associated with, if any.
* isTimeDependent: If the validator is testing rules which are time dependent. 
* isSuite: If the validator represents a suite of validators.

Validation instances can be used to run validation tests, but more commonly a
set of validators will be used, represented by a UsdValidationContext. 
A UsdValidationContext can be created from a vector of UsdValidators, which 
can be created manually, or obtained via metadata query methods on 
UsdValidationRegistry. UsdValidationContext also provides convenience 
constructors that determine which validators to use based on metadata filters, 
such as a list of keywords. Some constructors allow for including validators
for ancestor schema types for any found validators associated with schema
types. For example, when using a constructor that includes validators that have
certain keywords, validators for the UsdGeomSphere context get included. If the
constructor was called with `includeAllAncestors` set to true (the default),
the context would additionally include validators for schemas that UsdGeomSphere
inherits from, such as UsdGeomBoundable and UsdGeomImageable.

UsdValidationRegistry is the central registry that manages all registered
validators. The registry is used to obtain validator instances via validator 
metadata (name, keywords, schemaTypes). The registry also provides access to 
registered [UsdValidationValidatorSuites](@ref UsdValidationValidatorSuite) 
which represent predefined sets of validators (that can be used to create a 
UsdValidationContext). Finally, the registry can be used to register custom
validators (see ["Creating Custom Validators"](#creating_custom_validators)
below).

Validator instances (as well as the UsdValidationRegistry singleton) are
immutable, non-copyable, and (if the validator is registered in the
registry) immortal throughout a given USD session.

When validation tests are run (see 
["Running Validator Tests"](#running_validator_tests) below), any 
errors are captured in [UsdValidationErrors](@ref UsdValidationError). Errors
contain the following information:

* Name: A name the validator writer provided for the error, e.g. 
"MissingDefaultPrim".
* Identifier: An identifier used to distinguish similar-named errors from 
different validators. For a plugin validator the identifier should be of the
format "pluginName:validatorName.errorName". For an explicit validator, the
format is "validatorName.errorName". 
* Error type: A UsdValidationErrorType that indicates the severity of the error
(None, Error, Warn, Info).
* Error sites: One or more sites where the error was reported. An error could be 
reported in a SdfLayer (in layer metadata, for example), or a UsdStage (in stage 
metadata, for example) or a prim within a stage, or a property of a prim.
* Message: The error message provided by the validator writer that contains
more detailed information about the error.
* Access to the UsdValidationValidator that created the error, via 
UsdValidationError::GetValidator().
* Convenience access to any [UsdValidationFixers](@ref UsdValidationFixer) 
associated with the validator for this error, via 
UsdValidationError::GetFixers() methods.
* Error metadata: A VtValue set by the UsdValidationValidator issuing this 
error, that represents additional metadata passed to the client, or used by an 
associated UsdValidationFixer. For example, a validator test might test if an
attribute value meets a certain criteria, and the error metadata can be used to
pass along a value that meets requirements to a fixer, which will update the 
value accordingly.

A UsdValidationFixer represents a fix that can be applied to fix specific 
validation errors. A fixer is associated with a specific validator, and can be 
associated with a specific error name, or can be generic to any error associated 
with the corresponding validator. A fixer contains a name (unique among all
fixers associated with a specific validator), description, keywords used
to filter/group fixers (e.g. by department, show, etc.), and implementations
of FixerImplFn and FixerCanApplyFn functions to apply a fix and verify if a
fix can be applied respectively.

## Running Validator Tests  {#running_validator_tests}

Validation tests can be run on a stage, layer, or prim, by using the various
`Validate()` methods on UsdValidationValidator or UsdValidationContext. When
validating using a UsdValidationContext, multiple UsdValidationValidator
tests will be run in parallel. 

Validation tests can potentially initiate stage traversal, and it's the caller's
responsibility to maintain the lifetime of the stage/layer/prims that are being 
validated during the lifetime of the validation tests. UsdValidationContext
provides `Validate()` methods for validating stages that can take a 
Usd_PrimFlagsPredicate to control stage traversal.

Validation tests that test time-dependent values will by default be run against
the GfInterval::GetFullInterval() (-inf to inf) time interval. There are 
`Validate()` methods that can take a specific time interval to run against,
and will run tests on all timeCodes in the given time interval.

When validation tests have finished running, any validation errors will be 
returned as [UsdValidationErrors](@ref UsdValidationError). See details above
for information contained in a UsdValidationError. If the error provides 
associated [UsdValidationFixers](@ref UsdValidationFixer), it is the
responsibility of the caller to fix errors using the fixer's 
[CanApplyFix()](@ref UsdValidationFixer::CanApplyFix()) and 
[ApplyFix()](@ref UsdValidationFixer::ApplyFix()) methods on the client provided
UsdEditTarget. Validation tests will not automatically call any fixers.

## Creating Custom Validators  {#creating_custom_validators}

Custom validators can be created either via the OpenUSD plugin infrastructure,
which results in lazy loading of the validators, or via explicitly creating
and registering validators via UsdValidationValidator and UsdValidationRegistry
APIs.

A custom validator must implement a validator task function 
([UsdValidateLayerTaskFn](@ref UsdValidateLayerTaskFn()), 
[UsdValidateStageTaskFn](@ref UsdValidateStageTaskFn()), or 
[UsdValidatePrimTaskFn](@ref UsdValidatePrimTaskFn()))
which gets passed to the registry during registration, and called when the
validator's test is run. Validators should implement the task function at the
appropriate or desired granularity level. For example, if the validation logic 
can be succinctly defined to be applied to a prim, the UsdValidatePrimTaskFn
function should be implemented, rather than implementing a 
UsdValidateStageTaskFn or UsdValidateLayerTaskFn that has to traverse the 
stage/layer to validate the prim.

### Plugin Validators

For custom validators created in a plugin, the plugin's `plugInfo.json` will
contain the custom validator metadata. For example, a `plugInfo.json` for a 
plugin that has a "Validator1" validator, and a "ValidatorSuite1" validator
suite, might look something like the following.

```
{
    "Plugins": [
        {
            "Info": {
                "Validators": {
                    "keywords" : ["commonKeyword"],
                    "Validator1": {
                        "doc": "Validator that has test for imageable Gprims.", 
                        "schemaTypes": [
                            "UsdGeomImageable"
                        ],
                        "keywords": [
                            "UsdGeomImageable",
                            "keyword1"
                        ]                        
                    }
                    "ValidatorSuite1": {
                        "doc": "Suite of validators",
                        "keywords": ["suite"],
                        "isSuite": true
                    }                          
                }
            }, 
            "LibraryPath": "@PLUG_INFO_LIBRARY_PATH@", 
            "Name": "newValidatorPlugin", 
            "ResourcePath": "@PLUG_INFO_RESOURCE_PATH@", 
            "Root": "@PLUG_INFO_ROOT@", 
            "Type": "library"
        }
    ]
}
```

Note how the validator metadata is set in the `plugInfo.json`, along with an
extra "keywords" entry for keywords that are added to all validators 
defined in the plugin.

The plugin code to implement and register the validator might look something
like the following.

```cpp
TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const TfToken validatorName("newValidatorPlugin:Validator1");

    // Create our validator UsdValidateStageTaskFn here
    // (you could also use a static function defined elsewhere)
    const UsdValidateStageTaskFn stageTaskFn =
        [](const UsdStagePtr &usdStage,
           const UsdValidationTimeRange &timeRange) {
            UsdValidationErrorVector errors;

            // ...Validator test logic here, accessing usdStage as needed,
            //    creating errors as needed...

            return errors;
        };

    registry.RegisterPluginValidator(validatorName, stageTaskFn);

    // Register the validator suite to include the validator we just registered
    // (in practice, suites will most likely contain more than one validator).

    const TfToken suiteName("newValidatorPlugin:ValidatorSuite1");
    const std::vector<const UsdValidationValidator *> containedValidators
            = registry.GetOrLoadValidatorsByName({ validatorName });
    registry.RegisterPluginValidatorSuite(suiteName, containedValidators);      
}
```

Note that UsdValidationError instances are typically created by the validation 
task functions.

### Explicit Validators  {#explicit_validators}

For custom validators created explicitly, create a 
UsdValidationValidatorMetadata with the desired validator metadata along with
the validator test implementation, and use 
UsdValidationRegistry::RegisterValidator(). The following example creates a
UsdValidateStageTaskFn and UsdValidationValidatorMetadata to register an
explicit validator.

```cpp
const UsdValidateStageTaskFn explicitStageTaskFn =
    [](const UsdStagePtr &usdStage,
       const UsdValidationTimeRange &timeRange) {
        UsdValidationErrorVector errors;

        // ...Validator test logic here, accessing usdStage as needed,
        //    creating errors as needed...

        return errors;
    };
const UsdValidationValidatorMetadata explicitValidatorMetadata = {
    TfToken("ExplicitValidator"),

    // ...other metadata fields...

};

registry.RegisterValidator(explicitValidatorMetadata, explicitStageTaskFn);
```

### Adding Fixers

UsdValidationRegistry::RegisterPluginValidator() and 
UsdValidationRegistry::RegisterValidator() can optionally take a vector of
[UsdValidationFixers](@ref UsdValidationFixer). Each fixer will specify a name, 
description, FixerCanApplyFn and FixerImplFn functions, a list of keywords, and 
the error name the fixer can fix. 

Validator tests can result in multiple errors, and multiple fixers may be
associated with some of these errors. UsdValidationFixer::CanApplyFix() will 
utilize all of this information to determine if a fixer can be applied. 

The following example shows a utility function that creates a new fixer,
adds it to a vector of fixers, and returns the vector.

```cpp
const std::vector<UsdValidationFixer>
_ValidatorFixers() {
    std::vector<UsdValidationFixer> fixers;

    FixerCanApplyFn fixerCanApplyFn =
        [](const UsdValidationError &error, const UsdEditTarget &editTarget,
           const UsdTimeCode &/*timeCode*/) -> bool {
            
            // ...fixer logic here...

            return true;
        };

    FixerImplFn fixerImplFn =
        [](const UsdValidationError &error, const UsdEditTarget &editTarget,
           const UsdTimeCode &/*timeCode*/) -> bool {

            // ...can apply fixer logic here...

            return true;
        };

    fixers.emplace_back(
        TfToken("Example Fixer"),
        "An example fixer.",
        fixerImplFn, fixerCanApplyFn, TfTokenVector{},
        TfToken("ErrorNameAssociatedWithFixer"));

    return fixers;
}
```

Pass in the vector of fixers when the validator is registered. For example, the
registration code to register a plugin validator with fixers, using the 
previously shown utility function, might look something like the following.

```cpp
registry.RegisterPluginValidator(validatorName, stageTaskFn, _ValidatorFixers());
```

Note that UsdValidationRegistry does not manage fixers directly, and these are 
held by their respective UsdValidationValidator(s).

## Additional Examples

The code for `usdchecker` (in pxr/usdValidation/bin/usdchecker) has been 
updated to use validators and provides additional examples for using the 
validation framework.

See the various schema validators in /pxr/usdValidation for more example 
validator plugins.

