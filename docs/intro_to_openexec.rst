.. include:: rolesAndUtils.rst

.. _intro_to_openexec:

========================
Introduction to OpenExec
========================

.. contents:: :local:

.. _openexec_background:

**********
Background
**********

Many schemas in OpenUSD provide methods to compute values from authored scene 
description. Some of these methods (e.g., 
:usdcpp:`UsdGeomBoundable::ComputeExtent`) require bespoke APIs for caching and
invalidation. Other schemas, like those in the UsdSkel schema domain, involve 
more computation than others, and their computational implementations can span 
several layers (e.g., imaging) of the OpenUSD framework. Overall, the 
computational capabilities of the core schemas often leave much to be desired, 
especially with respect to the domain of rigging and posing of characters, as 
well as constraint targets. 

Additionally, expensive computation often necessitates caching of resulting 
values (e.g., with :usdcpp:`UsdGeomXformCache`), which in turn requires 
consideration of cache invalidation in response to input value changes. In 
addition to driving cache invalidation, tracking input dependencies (for 
example, being able to query whether a given computed result depends on time) 
is critical for debuggability.

Lastly, there is a desire to author and combine building blocks for solving 
computational challenges within a scene. This is especially important for, but 
not limited to, the domain of character rigging.

.. _openexec_introduction:

********************
Introducing OpenExec
********************

OpenExec provides a general-purpose framework for expressing and evaluating 
computational behaviors in an Open USD scene. This framework is built on top of 
OpenUSD and introduces new first-class concepts, including custom and built-in 
named computations associated with OpenUSD scene objects. The framework includes 
a fast, multi-threaded evaluation engine, and data management concepts for 
automatically caching and invalidating computed values.

Behind the scenes, OpenExec builds and maintains a dataflow network in the form 
of a directed acyclic graph, with computational tasks encoded as nodes, and with 
data flowing between computations encoded as edges. This 
data structure, which we call the *execution network* (or *exec network* for 
short) implicitly encodes data dependencies, which fosters optimal result reuse 
(e.g., where multiple computations rely on the same intermediate result), 
and simplifies the invalidation process as well.

This framework already exists in Presto, Pixar's in-house digital content 
creation (DCC) application, where it is called the Presto Execution System. We 
use this system to support character rigging, direct manipulation, transform 
hierarchy computation, attribute inheritance chains, dataflow connections, 
bounding volume calculation, constraint target computation, aspects of 
validation, and imaging behaviors, to name a few.

OpenExec represents the open-sourcing of this existing system, as well as a 
redesign of how it interfaces with scene description.

.. _openexec_example:

********************
Illustrative Example
********************

Consider an interactive viewer displaying a car, where clicking on a car door 
should open said door. In OpenUSD, we might have a :usdcpp:`UsdGeomXform` with a 
nested mesh, which represents the car door's coordinate frame along with its 
geometry. 

.. code-block:: usda

    def Xform "CarDoorFrame" {
        def Mesh "CarDoor" {
        }
    }

In addition, we might create an applied schema called :mono:`CarDoorRotator` 
that publishes an :mono:`openness` attribute for which 0 represents fully 
closed and 100 represents fully opened, as well as an API 
:mono:`computeTransformFromOpenness`, which converts the value of 
:mono:`openness` to a rotation transform.

Working only with OpenUSD, we must take care of several things. First, for 
values to stay properly updated, we must monitor changes to the :mono:`openness` 
attribute, and upon notification, pass the attribute value to the API, and 
author the resulting transform to the :mono:`CarDoorFrame` prim. Depending on 
how complicated the computation is, we might also want to cache the outputs 
corresponding to certain inputs. 

Alternatively, we could register and implement this API as a computation in 
OpenExec. Doing so encodes the computation, its input dependencies, and any 
dependent outputs as an exec network, which handles for free the 
responsibilities we worried about above. A change to the :mono:`openness` 
attribute now automatically causes invalidation of any dependent computations. 
Also, results are automatically cached as a byproduct of execution. 

Finally, OpenExec's evaluation engine is heavily multithreaded, so we can 
expect that multiple nodes in an exec network get executed in parallel. The 
scene in this toy example is extremely simple, but one can imagine a more 
complicated scene with many more prims, properties, schemas, and 
interdependencies. Here, the advantages quickly become clear. Evaluation of 
exec networks of hundreds of thousands or even millions of nodes scales quite 
smoothly. OpenExec's performance advantages along with the responsibilities it 
takes off of our plate all become quite significant with anything but the most 
trivial scenes.

.. _openexec_is_not:

********************
What OpenExec Is Not
********************

While OpenExec is a general purpose computation framework with the ability to 
compute values of arbitrary and custom data types, computations cannot modify 
the topology of the :usdcpp:`UsdStage`. For example, computations cannot add or 
remove prims, sub-trees of prims, or properties on prims, nor can they author 
new values to scene description. OpenExec always observes the composed 
:usdcpp:`UsdStage`.

Also, OpenExec on its own is not a rigging system. Pixar's rigging system in 
Presto comprises:

* The Presto Execution System
* A rigging object model (OM) built on top of the execution system
* A library of deformers and rigging primitives built on-top of the rigging OM

As-is, OpenExec provides the equivalent of item (1) to OpenUSD, that is, just 
the execution framework. In the future, OpenUSD may provide many of the pieces 
that make up item (2), however, this would be considered a separate component 
built on top of OpenExec.

Finally, OpenExec is a value-driven computation system, as opposed to being 
event-driven. When an input dependency changes, computed values receive 
invalidation, and subsequently reevaluate when the client next requests results. 
There is no event architecture included with OpenExec that triggers 
recomputation by, say, clicking on an object in the scene. 

Rather, scene authoring is the only "event" that can induce recomputation. This 
means that, while there is overlap between OpenExec and application-level 
interactive behaviors, OpenExec itself is not an interactive behavior system. 
In the example above, the developer would be responsible for implementing the 
click handler and hit test to figure out that the car door had been clicked, 
and perhaps author a new value to the openness attribute. Once the openness 
attribute is authored to, however, OpenExec is notified and can proceed to 
invalidate any results that are now out-of-date, and reevaluate the results of 
any dependent computations. 

.. _openexec_new_concepts:

************
New concepts
************

OpenExec introduces new, first-class concepts to USD:

* Computations provided by USD scene objects (e.g., prims and properties)
* Static, declarative input parameters to computations
* Custom computation callbacks
* Execution behavior registration as an extension to USD schema registration
* Client API for requesting computed values

.. _openexec_computations:

Computations
============

With OpenExec, any USD scene object can publish an arbitrary number of named 
computations that can be executed efficiently by the underlying execution 
system. At runtime, when a computation associated with a scene object is being 
invoked, we say that such a scene object is the *provider* of the invoked 
computation.

Every computation takes zero or more input parameters (typically at least one), 
and has associated with it a C++ computation callback responsible for reading 
input values, performing the computational work, and then outputting values 
(typically one).

Computations are published from two sources.

Built-in Computations
---------------------

OpenExec ships with several computations built into its infrastructure. These 
are computations that every USD object automatically publishes by default 
(e.g., :mono:`computePath`, which returns the scene graph path of the object), 
or that every USD object of a specific type publishes by default (e.g., 
:mono:`computeValue`, which returns the resolved value of a 
:usdcpp:`UsdAttribute` object).

Plugin Computations
-------------------

Schema registration has been extended with the ability to register custom 
computations that perform arbitrary computational work implemented as a C++ 
callback (e.g., a computation :mono:`computeExtent` registered on 
:usdcpp:`UsdGeomBoundable` doing the work equivalent to 
:usdcpp:`UsdGeomBoundable::ComputeExtent() <UsdGeomBoundable::ComputeExtent>`). 
OpenUSD will eventually ship with its codeful schemas implemented as OpenExec 
plugin computations.

.. _openexec_computation_input_parameters:

Computation Input Parameters
============================

Input parameters are scene description aware specifications of input data 
sources for computations. They allow computations to ingest anything from 
resolved attribute values to results of other computations. Moreover, such 
attributes and computations may live on any prim or property in the scene. In 
order to find these values, therefore, the execution system may need, for 
example, to traverse relationships, or to search up or down the transform 
hierarchy. As part of its computation definition language, OpenExec provides 
different input parameter types with which to specify how a computation should 
source its inputs.

As mentioned above, input parameters can source values from the outputs of 
other computations, and sourced computations need not be published on the same 
scene object as the consuming computation. In order to resolve the input 
parameter to source computations, input parameters generally encode three pieces 
of information:

**Computation name**: The name of the source computation as built-in, or 
registered on the computation provider explained above.

**Computation result type**: Denotes the data type of the input value as 
expected by the consuming computation, and it must match the data type of the 
output value returned from the source computation. OpenExec dataflow values are 
strongly typed. 

**Computation provider resolution**: Determines the scene object the system is 
going to use in order to look up the computation by name. The OpenExec 
extensions to OpenUSD schema registration (see below) will provide many 
different kinds of inputs with unique provider resolution behaviors (e.g., an 
attribute input that resolves to a named :usdcpp:`UsdAttribute` on a prim, or a 
relationship-targeted input that resolves to zero or more scene object(s) 
targeted by a specified :usdcpp:`UsdRelationship`). 

Every input parameter provides zero or more input values to the computation 
callback. For example, a relationship-targeted input, as mentioned above, may 
not resolve to any source computations if the specified 
:usdcpp:`UsdRelationship` does not have any authored targets, or if the 
targeted objects do not publish a computation with the specified name or result 
type. Conversely, this same input parameter may resolve multiple source 
computations (and thus provide multiple input values to the computation 
callback) in the case that there are multiple authored targets.

It should be noted that once input parameters are registered, input dependencies 
are established, and these dependencies will be static with respect to 
evaluation. That is, during evaluation, the evaluation engine will always make 
sure the dependencies for a computation are satisfied -- even if the callback 
does not end up consuming the resulting value.

.. _openexec_computation_callbacks:

Computation Callbacks
=====================

Computation callbacks are the C++ functions, or function objects, that perform 
the computational work of a computation. Callbacks access computation input 
values via the named tokens that were associated with them during computation 
registration. The following is an example of a computation callback:

.. code-block:: cpp

    // Free-standing, static callback function
    static void MyCallback(const VdfContext &ctx)
    {
        // Reads input 'myInputName' with single value, or first value.
        const int inputValue = ctx.GetInputValue<int>(tokens->myInputName);
        // Sets output value.
        ctx.SetOutput(inputValue + 1);
    }

OpenExec treats callbacks as black boxes, but requires them to be stateless and 
side-effect free. That is, callbacks are expected to return the same output when 
invoked with the same input, and are further expected not to make modifications 
to the global state. This is an important limitation that enables OpenExec's 
powerful and efficient data management and caching mechanisms, as well as basic 
thread-safety guarantees that every callback must satisfy.

.. _openexec_computation_registration:

Computation Registration
========================

OpenExec extends OpenUSD's schema registration facilities and provides the 
ability to register computations on schemas. The new execution behavior 
registration populates an execution registry (analogous to the OpenUSD schema 
registry), which in turn generates computation definitions (analogous to OpenUSD 
prim definitions) that serve as blueprints that instruct the framework how to 
instantiate and execute these computations. 

OpenExec registration introduces a domain-specific language built from C++ 
meta-programming constructs, not unlike :mono:`pxr_boost::python` or the 
:mono:`pybind11` Python wrapping language. Execution behaviors can currently 
only be registered in C++. 

.. code-block:: cpp

    // Callback referenced by the registration
    static GfMatrix4d 
    _ComputeLocalToWorldTransform(const VdfContext &ctx) 
    { 
      // Callback implementation. Uses the VdfContext API to read values
      // from the tokens->transform and tokens->computeLocalToWorldTransform
      // input parameters.
      ...
    }

    // Execution registration for UsdGeomXformable
    EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(UsdGeomXformable)
    {
        // The prim publishes a computation called computeLocalToWorldTransform
        self.PrimComputation(tokens->computeLocalToWorldTransform)
            .Callback<GfMatrix4d>(&_ComputeLocalToWorldTransform)
            .Inputs(
                // The first input to the computation: The value of the transform
                // attribute on the same prim.
                AttributeValue<GfMatrix4d>(tokens->transform),
                // The second input: The value of the computeLocalToWorldTransform
                // computation on the nearest namespace ancestor of this prim
                // that provides this computation.
                NamespaceAncestor<GfMatrix4d>(tokens->computeLocalToWorldTransform)
            );
    }

When a client requests a computation, the system automatically loads the plugin 
that defines the registration code, and runs it to generate the relevant 
computation definitions.

See `Computation Definition Language <api/group__group___exec___computation_definition_language.html>`__
for more information about OpenExec's computation definition language.

.. _openexec_client_api:

Client API
==========

Besides execution registration, clients interface with OpenExec by making 
requests for computed values. To do so, clients are first required to construct 
an execution system object instance, which maintains the execution state tied 
to a specific :usdcpp:`UsdStage`. 

.. code-block:: cpp

    // Construct an instance of the exec system state object. This
    // object contains the primary OpenExec client API.
    ExecUsdSystem exec(stage);

The :usdcpp:`ExecUsdSystem` instance maintains a shared reference to the 
:usdcpp:`UsdStage`, though the object will typically live alongside the stage 
and not outlive it. Multiple instances referring to the same USD stage may be 
constructed, although caches are not shared across instances. It is typical for 
one system instance to live alongside one :usdcpp:`UsdStage`.

Values computed by OpenExec will be automatically and aggressively cached. 
Cached values will be invalidated in response to changes on the 
:usdcpp:`UsdStage`.

Requesting Values
-----------------

In order for OpenExec to provide computed values, it must perform a lot of 
preparation work. Fortunately, the runtime (and to some extent memory) cost of 
this work amortizes significantly over batched requests for values. Meanwhile, 
requesting one value key at a time is pessimal for OpenExec performance, and so 
performance-sensitive clients should attempt to request all required value keys 
in one batch, before making decisions based on the computed values.

OpenExec's lowest level API for requesting values is the 
:usdcpp:`ExecUsdRequest`, which was designed to encourage this pattern and 
incur the lowest possible performance overhead. 
:usdcpp:`ExecUsdRequests <ExecUsdRequest>` are built out of a set of 
:usdcpp:`ExecUsdValueKeys <ExecUsdValueKey>`, which serve as references to 
requested results of named computations invoked via their scene object 
providers. The following code demonstrates building and executing a request:

.. code-block:: cpp

    // Builds a request for values. Reminder, value keys refer to computations.
    ExecUsdRequest request = exec.BuildRequest({
    ExecUsdValueKey(primA, tokens->fooComputation),
    ExecUsdValueKey(primB, tokens->barComputation),
    ...
    });

    // Computes all the requested values.
    ExecUsdCacheView cacheView = exec.Compute(request);

Once a request has been computed, resulting values can be extracted given the 
index of the value key in the request.

.. code-block:: cpp

    VtValue fooComputatationValue = cacheView.Get(0);
    VtValue barComputationValue = cacheView.Get(1);
    VtValue ... = cacheView.Get(i);

Receiving Notification About Invalidation
-----------------------------------------

OpenExec also provides a mechanism for receiving invalidation notification when 
input dependencies on previously computed values changes, causing their 
previously computed values to go invalid. This mechanism takes the form of a 
callback function object that can optionally be provided when building an 
:usdcpp:`ExecUsdRequest`. Upon invocation by the system, this callback is passed 
a set containing the indices of value keys with invalidated values. The callback 
also provides a time range over which the value keys have been invalidated.

.. code-block:: cpp

    // Builds a request with an invalidation callback.
    ExecRequest request = exec.BuildRequest(
    { /* value keys in the request */ },
    [](const ExecRequestIndexSet &indexSet, const EfTimeInterval &timeRange){
        // Invalidation callback code.
    });

.. _openexec_conclusion:

**********
Conclusion
**********

We hope you have enjoyed this overview of OpenExec. To dive deeper, you can:

* Download the code from our `GitHub repository <https://github.com/PixarAnimationStudios/OpenUSD>`__ 
  and work through the examples and tutorials
* Read the `OpenExec white paper <https://github.com/FlorianZ/OpenUSD-proposals-openexec/tree/main/proposals/openexec#readme>`__
* Explore the 
  `API reference documentation <api/md_pxr_exec_exec_usd_docs_overview.html>`__

Thanks for reading!