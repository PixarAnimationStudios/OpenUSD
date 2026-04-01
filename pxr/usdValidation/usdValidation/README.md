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

Validator instances can be used to run validation tests, but more commonly a
set of validators will be used, represented by a UsdValidationContext. 
A UsdValidationContext can be created from a vector of UsdValidators, which 
can be created manually, or obtained via metadata query methods on 
UsdValidationRegistry. UsdValidationContext also provides convenience 
constructors that determine which validators to use based on metadata filters, 
such as a list of keywords. 

Some constructors allow for including validators for ancestor schema types, for
any found validators associated with schema types. For example, when using a
UsdValidationContext constructor with the keywords parameter, if
`includeAllAncestors` is set to true (the default), and a validator is found 
for, say, the UsdGeomSphere schema type, any validators associated with ancestor 
schema types of UsdGeomSphere (such as UsdGeomGprim, UsdGeomImageable, etc.) 
will also be included in the context.

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
* Error Data: A VtValue set by the UsdValidationValidator issuing this 
error, that represents additional data passed to the client, or used by an 
associated UsdValidationFixer. For example, a validator test might test if an
attribute value meets a certain criteria, and the error data can be used to
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
appropriate granularity level. For example, if the validation logic 
can be succinctly defined to be applied to a prim, implement 
UsdValidatePrimTaskFn rather than UsdValidateStageTaskFn or 
UsdValidateLayerTaskFn. Using too broad a granularity can impact performance 
&mdash; for instance, a validator task that only needs to operate at the prim 
level but is implemented as a stage task may incur unnecessary stage traversal 
each time it runs.

Similarly, when a prim-level validator only applies to prims of a specific 
schema type, ensure `schemaTypes` is set appropriately. This allows the 
validation framework to skip non-matching prims and avoid unnecessary 
validator invocations.

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
                    },
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

### Choosing a Registration Path  {#choosing_registration_path}

The decision comes down to how you want other code to discover your
validator:

| | Explicit | Plugin |
|---|---|---|
| Metadata source | Caller provides `ValidatorMetadata` | `plugInfo.json` |
| Discoverability | Only after registration code has run | Metadata visible at startup; validator lazily loaded |
| Lazy loading | No; must register each session | Yes; validator loaded when first queried by name |
| Use when | Prototyping, one-off scripts, tests, runtime-generated rules | Shipping validators in a distributed plugin |

If other code needs to query your validator's metadata (keywords,
schema types) before the validator is loaded, use plugin registration.
If the validator is created dynamically or is only used by the code
that creates it, explicit registration is simpler.

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

## Creating Custom Validators in Python  {#python_validators}

Custom validators can be implemented in Python using either of the two
registration paths described in
["Creating Custom Validators"](#creating_custom_validators):

* **Plugin registration** (`RegisterPluginLayerValidator`,
  `RegisterPluginStageValidator`, `RegisterPluginPrimValidator`) — 
  the caller provides only the validator name as a `TfToken`.
  Metadata comes from the plugin's `plugInfo.json` and is parsed
  automatically during registry initialization.

* **Explicit registration** (`RegisterLayerValidator`,
  `RegisterStageValidator`, `RegisterPrimValidator`) —  the caller
  provides full `ValidatorMetadata`.  No plugin infrastructure is
  required; the validator is available immediately after registration.

### Performance Considerations  {#python_validator_performance}

When a `ValidationContext` runs validators, all validator tasks — C++ and
Python — are dispatched into the same shared TBB worker thread pool.  Python
task functions must acquire the Python GIL on each invocation.  A TBB worker 
thread executing one of these Python validator tasks is blocked waiting for the 
GIL. This has two consequences:

* **Python validators do not benefit from parallelism among themselves.**
  Even with task parallelism available via UsdValidationContext, only one 
  Python validator task runs at a time; the rest are blocked waiting for the 
  GIL.

* **Python validators can starve C++ validators.**  If enough Python
  validator tasks are scheduled simultaneously to occupy all TBB worker
  threads, C++ validator tasks that are ready to run will sit in the queue
  with no available workers until a GIL-blocked thread finishes and is freed.

For performance-sensitive validation pipelines, prefer C++ implementations
for validators.  Python validators are best suited for prototyping, tooling, or
validators that run rarely and on small scenes.

### Task Function Signatures

Each registration method accepts a Python callable with a specific
signature, matching the corresponding C++ task function type:

| Method | Callable signature |
|---|---|
| `RegisterLayerValidator` / `RegisterPluginLayerValidator` | `(layer: Sdf.Layer) -> list[ValidationError]` |
| `RegisterStageValidator` / `RegisterPluginStageValidator` | `(stage: Usd.Stage, timeRange: UsdValidation.TimeRange) -> list[ValidationError]` |
| `RegisterPrimValidator` / `RegisterPluginPrimValidator` | `(prim: Usd.Prim, timeRange: UsdValidation.TimeRange) -> list[ValidationError]` |

The callable must return a list (or any iterable) of
`UsdValidation.ValidationError` objects. Return an empty list when the
validation passes.

### Explicit Registration Examples

#### Layer Validator

```python
from pxr import Sdf, UsdValidation

registry = UsdValidation.ValidationRegistry()

metadata = UsdValidation.ValidatorMetadata(
    name="myPackage:RequiresDefaultPrim",
    doc="Warn when a layer has no default prim set.",
    keywords=["myPackage"],
)

def _CheckDefaultPrim(layer):
    if not layer.defaultPrim:
        return [
            UsdValidation.ValidationError(
                "MissingDefaultPrim",
                UsdValidation.ValidationErrorType.Warn,
                [UsdValidation.ValidationErrorSite(
                    layer, Sdf.Path.absoluteRootPath)],
                f"Layer '{layer.identifier}' has no defaultPrim.",
            )
        ]
    return []

registry.RegisterLayerValidator(metadata, _CheckDefaultPrim)
```

#### Stage Validator

```python
from pxr import Sdf, Usd, UsdGeom, UsdValidation

registry = UsdValidation.ValidationRegistry()

metadata = UsdValidation.ValidatorMetadata(
    name="myPackage:RequiresUpAxis",
    doc="Error when a stage has no upAxis metadata.",
    keywords=["myPackage"],
)

def _CheckUpAxis(stage, timeRange):
    if not stage.HasAuthoredMetadata(UsdGeom.Tokens.upAxis):
        return [
            UsdValidation.ValidationError(
                "MissingUpAxis",
                UsdValidation.ValidationErrorType.Error,
                [UsdValidation.ValidationErrorSite(
                    stage, Sdf.Path.absoluteRootPath)],
                "Stage is missing upAxis metadata.",
            )
        ]
    return []

registry.RegisterStageValidator(metadata, _CheckUpAxis)
```

#### Prim Validator

```python
from pxr import Sdf, Usd, UsdValidation

registry = UsdValidation.ValidationRegistry()

metadata = UsdValidation.ValidatorMetadata(
    name="myPackage:NoPrimsMissingKind",
    doc="Warn when a prim has no kind set.",
    keywords=["myPackage"],
)

def _CheckKind(prim, timeRange):
    if prim.IsPseudoRoot():   # skip pseudo-root
        return []
    model = Usd.ModelAPI(prim)
    if not model.GetKind():
        return [
            UsdValidation.ValidationError(
                "MissingKind",
                UsdValidation.ValidationErrorType.Warn,
                [UsdValidation.ValidationErrorSite(
                    prim.GetStage(), prim.GetPath())],
                f"Prim '{prim.GetPath()}' has no kind.",
            )
        ]
    return []

registry.RegisterPrimValidator(metadata, _CheckKind)
```

### Plugin Registration Example

When a validator is declared in `plugInfo.json`, only the name is
needed at registration time; all other metadata is already known to
the registry.

Given a `plugInfo.json` that declares:

```json
{
    "Plugins": [{
        "Info": {
            "Validators": {
                "CheckUpAxis": {
                    "doc": "Error when upAxis is missing.",
                    "keywords": ["stageMetadata"]
                }
            }
        },
        "Name": "myPlugin",
        "Type": "library",
        ...
    }]
}
```

The Python implementation registers the task function by name:

```python
from pxr import Sdf, UsdGeom, UsdValidation

registry = UsdValidation.ValidationRegistry()

def _CheckUpAxis(stage, timeRange):
    if not stage.HasAuthoredMetadata(UsdGeom.Tokens.upAxis):
        return [
            UsdValidation.ValidationError(
                "MissingUpAxis",
                UsdValidation.ValidationErrorType.Error,
                [UsdValidation.ValidationErrorSite(
                    stage, Sdf.Path.absoluteRootPath)],
                "Stage is missing upAxis metadata.",
            )
        ]
    return []

# Name must match "pluginName:validatorName" from plugInfo.json.
registry.RegisterPluginStageValidator(
    "myPlugin:CheckUpAxis", _CheckUpAxis
)
```

Plugin validator suites work the same way:

```python
registry.RegisterPluginValidatorSuite(
    "myPlugin:MySuite",
    [registry.GetOrLoadValidatorByName("myPlugin:CheckUpAxis")]
)
```

### How Python Plugin Validators Are Triggered  {#python_plugin_triggering}

For C++ plugins the Plug system loads the shared library and the 
`TF_REGISTRY_FUNCTION(UsdValidationRegistry)` macro ensures the registration 
code runs automatically at load time.

Python plugins work the same way, but will rely on module-level registration 
code in `__init__.py` instead of `TF_REGISTRY_FUNCTION`.

The lazy-load flow for a Python plugin:

1. **Startup**: `ValidationRegistry` parses `plugInfo.json` for all
   discovered plugins. Validator metadata (name, doc, keywords,
   schemaTypes) is available immediately, before any code is loaded.

2. **Query**: Client code calls
   `registry.GetOrLoadValidatorByName("myPlugin:CheckUpAxis")`.
   The registry finds metadata for this name and sees it belongs to a plugin 
   that has not been loaded yet. The same load is triggered when validators are 
   accessed via UsdValidationContext.

3. **Load**: The registry calls `plugin->Load()`. For a Python-type
   plugin, the Plug system executes
   `import <module_name>` (where `<module_name>` matches the `"Name"`
   field in `plugInfo.json`).

4. **Register**: The module's `__init__.py` runs at import time.
   Its top-level code calls `RegisterPluginStageValidator` (or
   the layer/prim variant) to register task functions with the
   registry.

5. **Return**: The registry now has a fully registered validator and
   returns it to the caller.

#### Python Plugin Directory Structure

The module directory name must match the `"Name"` field in
`plugInfo.json`, and the `plugInfo.json` lives inside the module
directory alongside `__init__.py`:

```
myPlugin/
    __init__.py       # Registration code runs at import time
    plugInfo.json     # "Type": "python", "Name": "myPlugin"
```

#### Example plugInfo.json (Python scenario)

```json
{
    "Plugins": [{
        "Type": "python",
        "Name": "myPlugin",
        "Info": {
            "Validators": {
                "CheckUpAxis": {
                    "doc": "Error when upAxis is missing.",
                    "keywords": ["stageMetadata"]
                }
            }
        }
    }]
}
```

#### Example __init__.py

```python
from pxr import Sdf, UsdGeom, UsdValidation

_PLUGIN_NAME = "myPlugin"

def _CheckUpAxis(stage, timeRange):
    if not stage.HasAuthoredMetadata(UsdGeom.Tokens.upAxis):
        return [
            UsdValidation.ValidationError(
                "MissingUpAxis",
                UsdValidation.ValidationErrorType.Error,
                [UsdValidation.ValidationErrorSite(
                    stage, Sdf.Path.absoluteRootPath)],
                "Stage is missing upAxis metadata.",
            )
        ]
    return []

# Registration at import time — equivalent to TF_REGISTRY_FUNCTION
_registry = UsdValidation.ValidationRegistry()
_registry.RegisterPluginStageValidator(
    _PLUGIN_NAME + ":CheckUpAxis", _CheckUpAxis)
```

The plugin directory must be discoverable by `Plug.Registry` (either
on `PXR_PLUGINPATH_NAME` or registered via
`Plug.Registry().RegisterPlugins()`). The parent directory of the
module must be on `sys.path` so the `import` succeeds.

### Running a Python Validator

Retrieve the registered validator by name and call `Validate()`, or
pass it to a `ValidationContext` to run it alongside other validators.

```python
# Direct invocation
validator = registry.GetOrLoadValidatorByName(
    "myPackage:RequiresUpAxis"
)
stage = Usd.Stage.Open("asset.usda")
errors = validator.Validate(stage)
for error in errors:
    print(error.GetErrorAsString())

# Via ValidationContext (runs all provided validators in parallel)
context = UsdValidation.ValidationContext([validator])
errors = context.Validate(stage)
```

### Grouping Validators Into a Suite

```python
stage_validator = registry.GetOrLoadValidatorByName(
    "myPackage:RequiresUpAxis"
)
prim_validator = registry.GetOrLoadValidatorByName(
    "myPackage:NoPrimsMissingKind"
)

suite_metadata = UsdValidation.ValidatorMetadata(
    name="myPackage:BaselineChecks",
    doc="Suite of baseline asset checks.",
    keywords=["myPackage"],
    isSuite=True,
)
registry.RegisterValidatorSuite(
    suite_metadata, [stage_validator, prim_validator]
)
```

### Adding Fixers in Python

Fixers can be created in Python and passed to any registration method
via the optional `fixers` parameter. Each fixer requires two callables:

| Callable | Signature | Purpose |
|---|---|---|
| `fixerImplFn` | `(error: ValidationError, editTarget: Usd.EditTarget, timeCode: Usd.TimeCode) -> bool` | Apply the fix; return `True` on success |
| `canApplyFn` | `(error: ValidationError, editTarget: Usd.EditTarget, timeCode: Usd.TimeCode) -> bool` | Return whether the fix is applicable |

```python
from pxr import Sdf, Usd, UsdValidation

registry = UsdValidation.ValidationRegistry()

# Define fixer callables.
def _CanFixMissingDoc(error, editTarget, timeCode):
    layer = editTarget.GetLayer()
    prim_spec = layer.GetPrimAtPath(error.GetSites()[0].GetPrim().GetPath())
    return prim_spec is not None and not prim_spec.documentation

def _FixMissingDoc(error, editTarget, timeCode):
    layer = editTarget.GetLayer()
    prim_spec = layer.GetPrimAtPath(error.GetSites()[0].GetPrim().GetPath())
    if prim_spec is None:
        return False
    prim_spec.documentation = "TODO: add documentation"
    return True

# Create the fixer.
fixer = UsdValidation.ValidationFixer(
    name="AddPlaceholderDoc",
    description="Add a placeholder documentation string.",
    fixerImplFn=_FixMissingDoc,
    canApplyFn=_CanFixMissingDoc,
    errorName="MissingDocumentation",       # optional; omit to match any error
    keywords=["pipeline"],                  # optional
)

# Register a validator with the fixer attached.
metadata = UsdValidation.ValidatorMetadata(
    name="myPackage:RequiresDocumentation",
    doc="Warn when a prim has no documentation.",
    keywords=["myPackage"],
)

def _CheckDocumentation(prim, timeRange):
    if prim.IsPseudoRoot():
        return []
    if not prim.GetDocumentation():
        return [
            UsdValidation.ValidationError(
                "MissingDocumentation",
                UsdValidation.ValidationErrorType.Warn,
                [UsdValidation.ValidationErrorSite(
                    prim.GetStage(), prim.GetPath())],
                f"Prim '{prim.GetPath()}' has no documentation.",
            )
        ]
    return []

registry.RegisterPrimValidator(metadata, _CheckDocumentation, fixers=[fixer])
```

After validation, retrieve and apply fixers:

```python
validator = registry.GetOrLoadValidatorByName(
    "myPackage:RequiresDocumentation"
)

# Fixers are accessible from the validator or from individual errors.
fixers = validator.GetFixers()
fixer = validator.GetFixerByName("AddPlaceholderDoc")

# After running validation:
stage = Usd.Stage.Open("asset.usda")
prim = stage.GetPrimAtPath("/MyPrim")
errors = validator.Validate(prim)

for error in errors:
    for f in error.GetFixersByErrorName():
        editTarget = Usd.EditTarget(stage.GetRootLayer())
        if f.CanApplyFix(error, editTarget):
            f.ApplyFix(error, editTarget)
```

Note that `ApplyFix` calls `editTarget.GetLayer()->Save()` internally
on success. The target layer must be file-backed (not anonymous).

### Notes

* The `ValidationRegistry` is a singleton; validators registered in
  one module are visible to all other modules in the same process.
* Validator names must be unique across the registry. Re-registering
  an existing name will fail silently; use `HasValidator()` to check
  before registering if needed.
* Python exceptions raised inside a task function are converted to Tf
  errors and the validator returns an empty error list for that
  invocation. Add explicit error handling inside the callable if you
  need richer diagnostics.
* Explicitly registered validators have no associated plugin and are
  not lazily loaded. They must be registered each session before they
  can be used.
* Plugin-registered validators get their metadata from `plugInfo.json`.
  The metadata is discoverable at startup even before the Python task
  function is registered, enabling tools to enumerate available
  validators without loading every plugin.

## Additional Examples

The code for `usdchecker` (in pxr/usdValidation/bin/usdchecker) has been 
updated to use validators and provides additional examples for using the 
validation framework.

See the various schema validators in /pxr/usdValidation for more example 
validator plugins.

