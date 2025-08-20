# OpenExec Overview

OpenExec supports efficient evaluation of computational behaviors encoded in USD
scenes. In brief, this works as follows: Schemas publish *computations*, atoms
of computational behavior that take inputs from other computations, and from
values authored in the scene, to produce output values. OpenExec ingests a
UsdStage, along with registered computations to produce an internal
representation of the instantiated computations in the scene. Clients can then
request sets of computation outputs to be computed. In addition to evaluation
that results in the requested values, OpenExec supports caching of computed
values and invalidation of caches in response to scene changes.

This document describes the [concepts](#section_Concepts) on which OpenExec is
built, with links to detailed documentation for the APIs that implement these
concepts, and provides [tutorials](#section_Tutorials) that demonstrate the
details of how OpenExec features are used in practice.


## OpenExec Concepts {#section_Concepts}

### Computations

**Computations** are the basic unit of computational work in OpenExec. A
computation definition is a template for producing data flow nodes that take
inputs from other computations and produce an output value.

Each computation is associated with a scene object that hosts the computation,
referred to as the **computation provider**. The provider object, either a
UsdPrim or a UsdAttribute, anchors the computation in a USD scene, providing a
reference point for the computation to use for drawing input values. For
example, an attribute computation may take input from the authored value of the
attribute that provides the computation.

#### Plugin Computations

Computations that are associated with schemas are called **plugin
computations**; OpenExec will load the plugin that defines computations for a
given schema the first time any computation is requested for a prim that uses
that schema. Plugin computations are registered using an embedded [computation
definition language](#group_Exec_ComputationDefinitionLanguage), which allows
schema authors to register the callback that implements the computation, along
with a specification of the input values required by the computation.

#### Builtin Computations

OpenExec provides a fixed set of **builtin computations**, computations that are
always provided by a given type of provider object. For example, attributes
provide a `computeValue` builtin computation that yields the value of the
attribute at the current evaluation time.

See the [builtin computation documentation](#group_Exec_Builtin_Computations)
for a desciption of the builtin computations that are available.


## Tutorials {#section_Tutorials}

- [Tutorial 1: Computing Values](tutorial1ComputingValues.md)
- [Tutorial 2: Defining Schema Computations](tutorial2DefiningComputations.md)


## Advanced Topics {#section_AdvancedTopics}

**Coming Soon:**
- Caching, invalidation, and invalidation notification
- Exec architecture/phases of execution: compilation, scheduling, evaluation
- Dispatched computations
