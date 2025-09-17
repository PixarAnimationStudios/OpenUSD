# OpenExec Tutorial 2: Defining Schema Computations

## Overview

The purpose of this tutorial is to demonstrate how to define OpenExec
computations associated with USD schemas in order to publish computational
behaviors that can be evaluated using the OpenExec engine. Specifically, we will
show how to implement simple computational behaviors as OpenExec
computations. We do this by building on the `ParamsAPI` applied API schema from
the [Generating New Schema
Classes](https://openusd.org/release/tut_generating_new_schema.html)
tutorial. The schema introduces attributes, which these computations will
consume as inputs, to produce computed values.

This tutorial builds on the
[tutorial on Computing Values in OpenExec](tutorial1ComputingValues.md), which
contains details on using OpenExec client APIs to request the computed results
of computations.


## Plugin Metadata

Computations aren't required to be defined in the same plugin library that
defines the schemas they are attached to. Therefore, the plugin metadata for any
library that defines computations must identify the schema(s) for which it
publishes computations.

The following `plugInfo.json` file shows what this looks like in practice.
Here, we declare `UsdSchemaExamplesParamsAPI` as a schema that allows plugin
computations. The existence of this plugin metadata identifies the library that
contains it as the library to load when OpenExec reguires computation
definitions for any prim that has the `ParamsAPI` schema applied.

```
{
    "Plugins": [
        {
            "Info": {
                "Exec" : {
                    "Schemas": {
                        "UsdSchemaExamplesParamsAPI": {
                            "allowsPluginComputations": true
                        }
                    }
                }
            },
            "LibraryPath": "@PLUG_INFO_LIBRARY_PATH@",
            "Name": "execComputationExamples",
            "ResourcePath": "@PLUG_INFO_RESOURCE_PATH@",
            "Root": "@PLUG_INFO_ROOT@",
            "Type": "library"
        }
    ]
}
```

> **Note**  
> Above, we assume the library that contains the computation definitions
> is named `execComputationExamples`.

> **Note**  
> The `allowsPluginComputations` plugin metadatum, when set to `false`, can be
> used to declare that a given plugin *cannot publish computations.* When that
> is the case, any attempt to register plugin computations for that schema
> results in an error, and such computations are ignored.
>
> The `allowsPluginComputations` plugin metadatum can also be omitted, which has
> the same effect as setting it to `true`.


## Computation Registration

The same plugin library that contains the above metadata must contain a cpp file
containing the code that registers the computations for that schema. When
OpenExec requests computations for the schema, it determines which plugin to
load based on the plugin metadata, loads the plugin, and then runs the
registration code.

In the sections below, we present the different components that make up a single
computation registration before bringing it all together into a complete
example at the end.

### Registration macro

Computations are registered using the macro
`EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA`. There can only be one invocation of
this macro for a given schema. The macro takes the schema type name as a
parameter, and the macro must be immediately followed by the body of a
registration function that registers all computations that are associated with
the schema.

```cpp
EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(UsdSchemaExamplesParamsAPI)
{
    . . .
}
```

### Initiating a computation registration

The body of the registration function contains one or more computation
registrations. Each registration starts with a reference to the `self` object
that is defined by the registration macro, followed by a call to a method that
starts off the process, e.g.:

```cpp
    self.PrimComputation(TfToken("computeMomentum"))
```

The computation we register here is a **prim computation**, meaning that this
computation can be found on prims that have the `ParamsAPI` schema applied to
them. The terminology we use is that such prims **provide** this
computation. Note that it is also possible to register **attribute
computations**, computations that are provided by attributes. See the
[Computation Registrations](#group_Exec_ComputationRegistrations) section of the
[Computation Definition Language](#group_Exec_ComputationDefinitionLanguage)
documentation for more information on registering prim and attribute
computations.


### Input parameters

The `PrimComputation` call above returns a builder object that defines methods
used to build up the computation definition. To add **input parameters**, which
specify how the input values are sourced for the computation at evaluation time,
we call the `Inputs` method:

```cpp
        .Inputs(
            AttributeValue<double>(UsdSchemaExamplesTokens->paramsMass),
            AttributeValue<double>(UsdSchemaExamplesTokens->paramsVelocity)
        )
```

The `Inputs` method accepts one or more **input registrations**. Here, we use
the `AttributeValue` input registration to specify that our input values come
from the computed values of attributes. Internally, this input parameter
requests the [builtin computation](#group_Exec_Builtin_Computations)
`computeValue` on the attribute of the specified name.

OpenExec supports a growing variety of input parameters, each of which requests
the result of a computation on some provider object. It is possible to request
input values from computations provided by the prim or attribute that the
computation lives on, or by the owning prim, a sibling property, objects
targeted by relationship targets, etc. See the [Input
Registrations](#group_Exec_InputRegistrations) section of the [Computation
Definition Language](#group_Exec_ComputationDefinitionLanguage) documentation
for more information on the different kinds of input parameters that are
currently supported.

### Callback function

Now that we have specified input parameters for our computation, we need to
provide the code that implements the evaluation-time logic, to produce a
computed value. We do this using a chained call to the `Callback` method:

```cpp
        .Callback<double>(+[](const VdfContext &context) {
            const double mass = context.GetInputValue<double>(
                UsdSchemaExamplesTokens->paramsMass);
            const double velocity = context.GetInputValue<double>(
                UsdSchemaExamplesTokens->paramsVelocity);

            return mass * velocity;
        })
```

The callback function used here is a lambda that uses the unary plus operator to
yield a function pointer. In general, callbacks can be any function pointer with
the signature `ResultType (*)(const VdfContext &)` (or `void (*)(const
VdfContext &)`, in cases where the callback calls VdfContext::SetOutput). See
the documentation on the [Computation Definition
Language](#group_Exec_ComputationDefinitionLanguage) for more information on
registering callback functions.


## Bringing it all together

The following code would appear in a cpp file, in the same library as the
`plugInfo.json` file given above. Here, we also add a second computation, to
demonstrate how multiple computations can be registered for a single schema.

```cpp
#include "pxr/pxr.h"

#include "pxr/extras/usd/examples/usdSchemaExamples/tokens.h"

#include "pxr/exec/exec/registerSchema.h"

PXR_NAMESPACE_USING_DIRECTIVE

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(UsdSchemaExamplesParamsAPI)
{
    // Define a computation that reads the values of the attributes params:mass
    // and params:velocity and computes the momentum.
    self.PrimComputation(TfToken("computeMomentum"))
        .Callback<double>(+[](const VdfContext &context) {
            const double mass = context.GetInputValue<double>(
                UsdSchemaExamplesTokens->paramsMass);
            const double velocity = context.GetInputValue<double>(
                UsdSchemaExamplesTokens->paramsVelocity);

            return mass * velocity;
        })
        .Inputs(
            AttributeValue<double>(UsdSchemaExamplesTokens->paramsMass),
            AttributeValue<double>(UsdSchemaExamplesTokens->paramsVelocity)
        );

    // Define a computation that reads the values of the attributes params:mass
    // and params:volume and computes the density.
    self.PrimComputation(TfToken("computeDensity"))
        .Callback<double>(+[](const VdfContext &context) {
            const double mass = context.GetInputValue<double>(
                UsdSchemaExamplesTokens->paramsMass);
            const double volume = context.GetInputValue<double>(
                UsdSchemaExamplesTokens->paramsVolume);

            return mass == 0.0 ? 0.0 : volume / mass;
        })
        .Inputs(
            AttributeValue<double>(UsdSchemaExamplesTokens->paramsMass),
            AttributeValue<double>(UsdSchemaExamplesTokens->paramsVolume)
        );
}
```

For information on how to use the OpenExec client API to compute values using
these computations, see the related example in the
[tutorial on Computing Values in OpenExec](tutorial1ComputingValues.md).


## Caveats

- Obviously, these are trivial computations, intended to demonstrate the
  mechanics of how computations are registered. In practice, it might not make
  sense to expose computations like this. A common reason to register
  computations is so the results can be cached, and the small amount of work
  these computations do mean that there's no benefit to caching their results.
  However, it might make sense to do so in order to publish these computations
  as part of the computational interface of a schema.

- These callbacks have a fair amount of boilerplate code for simply extracting
  input values from the VdfContext. The reason for this API is that, in general,
  the logic can be more complex. E.g., input values may be optional, meaning
  they may or may not be present when the callback is invoked, so the calllback
  would then have conditional logic (using VdfContext::HasInputValue or
  VdfContext::GetInputValuePtr). Values can also be vectorized, in which case
  the callback would use an iterator (e.g., VdfReadIterator). That said, we plan
  to introduce a convenience wrapper that will automatically extract input
  values from the context, allowing callbacks to be written compactly, e.g.,
  `Callback(+[](double mass, double velocity) { return mass * velocity; })`
