# OpenExec Tutorial 1: Computing Values

## Overview

The purpose of this tutorial is to demonstrate how to use OpenExec APIs to
request values to be computed.

For this tutorial, we will make use of the `computeLocalToWorldTransform`
computations provided by UsdGeomXformable prims. The result of this computation
on a given Xform is a 4x4 matrix that transforms points local to that Xform into
points in world-space. The result of this computation depends on two values:
  1. The authored value of the `transform` attribute on the given Xform.
  2. The computed result of `computeLocalToWorldTransform` recursively invoked
     on the Xform's parent Xform.

> **Note:**  
> The API that is presented here is the lowest-level API, designed for
> performance-intensive clients, such as an imaging system. OpenExec will
> eventually include convenience API, layered on top of the API shown here, for
> use cases that don't require maximum performance and for use cases that adhere
> to certain patterns that allow for more convenient API while still maintaining
> maxiumum performance.

## Create a UsdStage

The first step is to create a UsdStage from a scene that contains
UsdGeomXformable prims, such as the scene described by this usda file:

```
    #usda 1.0

    def Xform "Root" (
        kind = "component"
    )
    {
        uniform token[] xformOpOrder = [ "xformOp:transform" ]
        matrix4d xformOps:transform = (
            (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (1, 0, 0, 1) )

        def Xform "A1"
        {
            uniform token[] xformOpOrder = [ "xformOp:transform" ]
            matrix4d xformOps:transform = (
                (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 2, 0, 1) )
        }

        def Xform "A2"
        {
            uniform token[] xformOpOrder = [ "xformOp:transform" ]
            matrix4d xformOps:transform = (
                (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 3, 1) )
        }
    }
```

Assuming this layer is in a file named `xformPrims.usda` we can open the layer
on a UsdStage as follows:

```cpp
    UsdStageRefPtr stage = UsdStage::Open("xformPrims.usda");
```

## Create an ExecUsdSystem

In order to compute values from a UsdStage, we need to create an ExecUsdSystem
object from the stage:

```cpp
    ExecUsdSystem execSystem(stage);
```

The ExecUsdSystem maintains the internal state needed to compute values from a
given UsdStage. In particular, the system object holds onto the compiled data
flow **network** that is used for evaluation. Note that we can make requests for
different sets of values originating from the same stage using the same system
object and the execution processes benefit from the shared state, amortizing
costs (including the cost of compiling the network) across different value
requests.

## Build an ExecUsdRequest

A set of values to be computed is specified by building an ExecUsdRequest. Each
requested value is identified by an ExecUsdValueKey, which contains a
**provider**--a UsdObject that provides a computation--and a TfToken that gives
the name of the requested computation.

To build a request containing a collection of value keys, we call
ExecUsdSystem::BuildRequest:

```cpp
    std::vector<ExecUsdValueKey> valueKeys {
        {stage->GetPrimAtPath(SdfPath("/Root/A1")),
         ExecGeomXformableTokens->computeLocalToWorldTransform},
        {stage->GetPrimAtPath(SdfPath("/Root/A2")),
         ExecGeomXformableTokens->computeLocalToWorldTransform},
    };

    const ExecUsdRequest request = execSystem.BuildRequest(std::move(valueKeys));
```

A request maintains state that is required to effiently compute the particular
set of requested values that it represents. In particular, it holds onto a
**schedule** that is used to accelerate evaluation, by amortizing the cost of
creating the schedule across multiple rounds of computation using the same
request.

## Prepare the request

Preparing the request, by calling ExecUsdSystem::PrepareRequest, does two
things:
1. It ensures that the network held by the system is compiled for the
   request. I.e., it makes sure that all data flow nodes, and connections that
   flow data between nodes, that are required to compute the requested values
   are present in the network and that their structure is up-to-date with
   respect to the current authored state of the UsdStage.
2. It ensures that the request's schedule is up-to-date, and will re-schedule
   (including scheduling for the first time) if necessary.

```cpp
    execSystem.PrepareRequest(request);
```

Note that explicitly preparing the request is optional; if a client calls
`Compute` without first calling `PrepareRequest`, the call to `Compute` will
prepare the request before computing. However, it is often desirable for client
code to have explicit control over *when* the request is prepared, because of
the cost of doing so. Specifically, compiling and scheduling tend to be more
expensive than evaluation, so it often makes sense to ensure that these happen
first, before multiple rounds of computation.

## Compute values

To compute the set of requested values, we simply call ExecUsdSystem::Compute:

```cpp
    ExecUsdCacheView cache = execSystem.Compute(request);
```

The computed results are now ready, and can be acessed via the returned
ExecUsdCacheView object.

## Extract computed values

To access the computed values, we call ExecUsdCacheView::Get to extract them,
using indices that correspond to the order of the value keys in the vector used
to build the request:

```cpp
    VtValue value;
    value = cache.Get(0);
    const GfMatrix4d a1LocalToWorld = value.Get<GfMatrix4d>();
    value = cache.Get(1);
    const GfMatrix4d a2LocalToWorld = value.Get<GfMatrix4d>();
```

## Putting it all together

Bringing this all together into a single block of example code:

```cpp
    #include "pxr/base/gf/matrix4d.h"
    #include "pxr/base/vt/value.h"
    #include "pxr/exec/execGeom/tokens.h"
    #include "pxr/exec/execUsd/request.h"
    #include "pxr/exec/execUsd/system.h"
    #include "pxr/exec/execUsd/valueKey.h"
    #include "pxr/usd/sdf/path.h"
    #include "pxr/usd/usd/stage.h"

    #include <utility>
    #include <vector>

    void Example()
    {
        // Open the layer that contains our scene on a UsdStage.
        const UsdStageRefPtr stage = UsdStage::Open("xformPrims.usda");

        // Create an ExecUsdSystem, which we will use to evaluate computations
        // on the stage.
        ExecUsdSystem execSystem(stage);

        // Create a vector of value keys that indicate which computed values we
        // are requesting for evaluation.
        std::vector<ExecUsdValueKey> valueKeys {
            {stage->GetPrimAtPath(SdfPath("/Root/A1")),
             ExecGeomXformableTokens->computeLocalToWorldTransform},
            {stage->GetPrimAtPath(SdfPath("/Root/A2")),
             ExecGeomXformableTokens->computeLocalToWorldTransform},
        };

        // Build the request.
        const ExecUsdRequest request =
            execSystem.BuildRequest(std::move(valueKeys));

        // Prepare the request, ensuring the data flow graph is compiled and the
        // schedule is created.
        execSystem.PrepareRequest(request);

        // Evaluate the data flow graph according to the schedule, to yield the
        // requested computed values.
        ExecUsdCacheView cache = execSystem.Compute(request);

        // Extract the values.
        VtValue value;
        value = cache.Get(0);
        const GfMatrix4d a1LocalToWorld = value.Get<GfMatrix4d>();
        value = cache.Get(1);
        const GfMatrix4d a2LocalToWorld = value.Get<GfMatrix4d>();

        // The resulting matrices are the concatenation of the transforms
        // authored on A1 and Root and A2 and Root, respectively. Here, we
        // extract the translations from the resulting matrices, demonstrating
        // that we end up with the expected net translations necessary to
        // translate points in each local space into world space.
        TF_AXIOM(GfIsClose(
            a1LocalToWorld.ExtractTranslation(), GfVec3d(1, 2, 0), 1e-6));
        TF_AXIOM(GfIsClose(
            a2LocalToWorld.ExtractTranslation(), GfVec3d(1, 0, 3), 1e-6));
    }
```
