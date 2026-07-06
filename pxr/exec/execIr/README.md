# ExecIr: Invertible Rigs Schemas {#page_ExecIr_readme}

The execIr library is built on top of [execUsd](../execUsd/README.md).

> [!warning]
> The schemas and APIs defined in this library are currently very limited in
> functionality, subject to change, and not yet ready for production use.

The execIr library defines schemas that are used to build **invertible rigs**,
rigs that pose model skeletons and that support
[inversion](#section_ExecIr_Inversion), which is required for certain animation
authoring workflows.

# Schemas

Schemas provided by this library include:

- ExecIrController, an abstract base schema for **controllers**, prims that have
  input and output attributes and define forward computations that compute
  output values from input values. Controllers are invertible in that they also
  define inverse computations that compute inputs from outputs.

- ExecIrFkController, a controller that computes a simple forward kinematic
  (FK) transform from scalar input attributes.

- ExecIrSwitchController, a controller that takes multiple input values and
  passes through one of them to its output. The value of the `switch` attribute
  determines which input value is selected. Multiple switch controllers can be
  ganged together to switch among sub-rigs, one of which provides the model pose
  at a given time.

- ExecIrJointScope, used to form the hierarchy of prims that represents the
  animator interface for a model. Joint scopes host scalar **avars**, attributes
  onto which animation is authored, and matrix-valued attributes, whose computed
  values come across attribute connections from the transforms computed by
  controllers.

# Invertible Rigs

An invertible rig built from execIr schemas consists of a hierarchy of joint
scope prims that serves as the animation interface and a network of controllers
whose input attributes receive values via connections to the joint scope
avars. The controller network generally contains multiple sub-networks, each of
which represents a sub-rig that implements a particular posing scheme. Switch
controllers are used to select the sub-rig that is active at any given time,
with their `switch` attributes receiving values via attribute connections to
joint scope avars, whose value can be animated.

## Inversion {#section_ExecIr_Inversion}

When computing in the forward direction, an invertible rig takes avar values as
input and produces transform values that collectively represent the posed
configuration of the model skeleton. Such a rig is *invertible* in the sense
that the network can also take a desired set of transform values and compute the
input values that produce that pose. (Assuming that it's possible for the
desired pose to be achieved; when that's not possible, controller inverses
produce a "best try" solution.) Inversion is critical for certain animation
authoring workflows when a model can be posed by multiple sub-rigs, switched by
switch controllers.

One such authoring workflow, **switch compensation**, comes into play when the
animator wants to change the rig that controls posing during the course of a
continuous animation. E.g., the animator might want the first segment of an
animation to be interpolated using FK avars (to produce smooth rotation arcs),
but want the next segment to be interpolated using IK avars (to produce linear
end-effector motion). If the animator simply animates a change in the value of
the switch attribute, the resulting model pose will, in general, change
discontinuously. To instead produce continuous motion as the switch attribute
value changes, inversion can be invoked to compute the input avar values
necessary to maintain the pose at the time of the switch.

Another animation workflow that relies on inversion is when the animator wants
to use one rig to interactively pose a model while crafting a pose, but use a
different rig to interpolate the resulting animation. One example is "IK for
FK," where the animator manipulates the end effector position directly (using IK
avars), but authors to FK avars so that they will be used for interpolation.
The IK rig is used to compute posed spaces based on the end effector position,
and then inversion is invoked, with the computed space values given as desired
values, to solve for the values of the input avars for the FK rig.

> [!note]
> Though inverse kinematics is integral to many of the canonical use cases for
> invertible rigs, the execIr library does not currently include any IK solver
> schemas.

## Invoking Inversion in OpenExec

Computing forward poses using OpenExec API is reasonably straightforward: We
simply compute the value of the `posed:space` attribute on each joint
scope. Because of attribute connection dataflow, that will transitively compute
the value of the controller output attribute that `posed:space` is connected
to. And controller output attributes have expressions that produce the results
of the controller's forward computation, given the input values as authored in
the scene.

To compute in the inverse direction, we need some way to specify the desired
skeleton pose. We _could_ do this by authoring values onto attributes in the
scene, but these values are not intended to be a lasting part of the scene; they
are transient data, only needed for the purpose of performing a computation as
part of an authoring operation. And relying on authoring as a way to communicate
those values has significant disadvantages, including possible accumulation of
scene cruft, incorrect results due to authored values that we fail to clean up,
and exec network invalidation that could cause downstream caches to be
unnecessarily invalidated. Therefore, OpenExec provides special API that allows
us to specify **overrides**, values that are transiently injected directly into
the exec network for the purpose of a single evaluation.

The function used for this is ExecUsdSystem::ComputeWithOverrides(), which takes
an ExecUsdRequest containing value keys that indicate the computations we want
to evaluate and a vector of ExecUsdValueOverride%s that specify the value keys
we want to override, along with the values to assign to those keys. For the
request, the providers we use are the input avars for which we want to compute
values and a special
[computeDesiredValue](#ExecIrComputationsType::computeDesiredValue) computation
that indicates that we want to invoke inversion. For the overrides, we supply
values using the `posed:space` attributes as providers and the
[explicitDesiredValue](#ExecIrComputationsType::explicitDesiredValue)
computation, which indicates that we want to override the desired values for
these attributes.
