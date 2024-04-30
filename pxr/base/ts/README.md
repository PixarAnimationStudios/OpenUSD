
# TS LIBRARY DEVELOPMENT STATUS

The Ts library is under development.  This is not a final version.  The code
here was extracted from Pixar's Presto software, and will be modified
extensively before public release.

This document lists all of the major changes that are expected.  These changes
will make Ts (and satellite code in other libraries) match the description in
the [USD Anim Proposal](https://github.com/PixarAnimationStudios/OpenUSD-proposals/blob/main/proposals/spline-animation/README.md).


# TESTS

There are two families of tests: those that begin with `tsTest`, and everything
else.  The `tsTest` system is new, and most or all tests will eventually be
migrated to it.  `tsTest` provides a test framework that includes features like
graphical output, and it allows multiple evaluation backends for comparison.
See the file `tsTest.dox` for details.

There will be extensive coverage of spline evaluation and the Ts API.  Tests
will be written as parts of the library reach their intended state.


# API REORGANIZATION

## Series Classes

Ts stands for "time series".  A series is a generalization of a spline.  Splines
are the most flexible type of series; there are other types of series that lack
some of the features of splines.  Series types will be chosen according to the
value type that they hold.  Value type categories, and their associated series
types, are detailed in the USD Anim proposal.  In addition to splines, there
will be `TsLerpSeries`, `TsQuatSeries`, and `TsHeldSeries`.  There will also be
a `TsSeries` container that can hold any of these, and a common
`TsSeriesInterface` implemented by all of them.

Currently there is only `TsSpline`, with varying behavior depending on value
type.  `TsSpline` wil be split out into the series classes, and the type
registry will change substantially or be eliminated.

## Spline API

Many methods of `TsSpline`, `TsKeyFrame`, etc will change.  Some changes will be
to accomodate new or changed features; some will be for clarity or convenience.

## Nomenclature

The object-model vocabulary will change in several ways.  Some examples:
"keyframes" will become "knots"; "left" and "right" will become "pre" and
"post".

## Dual Tangents

Tangents are currently specified only in Presto form (slope and length).  It
will also become possible to specify tangents in Maya form (height and length).


# EVALUATION BEHAVIOR

## Maya Evaluator

Ts currently uses its own Bezier evaluator.  In the future, it will switch to a
Maya-identical evaluator.  The two give very similar results, but exact matches
with Maya will be a plus.

## Quaternion Easing

Currently quaternions are only interpolated linearly.  We will add an "eased"
option that comes from Maya.

## No Half-Beziers

Currently, Ts only allows tangents on Bezier-typed knots.  When there is a
Bezier knot followed by a held knot, a Bezier knot followed by a linear knot, or
a linear knot followed by a Bezier knot, we end up with a "half-Bezier" segment,
one of whose tangents cannot be specified, and is given an implied automatic
value.

In the new Ts object model, "knot type" will give way to "next segment
interpolation method".  Any knot that follows a Bezier segment will have an
in-tangent, regardless of the interpolation method of the following segment.
Depending on the surrounding segment methods, knots may have no tangents, only
in-tangents, only out-tangents, or both.

## New Time-Regressive Behavior

When very long tangents cause Bezier segments to form "recurves" or
"crossovers", that results in a non-function from time to value (multiple values
per time).  Presto and Maya both force strict functional behavior in these
situations, but they do it differently, and neither behavior is particularly
easy to predict or control.  For crossovers, we will likely create a cusp at the
crossover point.  Details are TBD for recurves.  We don't expect these to be
common cases, but we want our behavior to be unsurprising when they occur.


# NEW FEATURES

## Hermites

Currently Ts only supports Bezier curves.  It will be expanded to support
Hermites as well, matching Maya behavior.  Hermites have fewer degrees of
freedom than Beziers - tangent lengths are fixed - and, in return, they are
faster to evaluate, requiring only a cubic evaluation rather than an iterative
parametric solve.

## Extrapolating Loops

Currently Ts supports only one form of looping: "inner" looping, where a
specific portion of a series is repeated.  Ts will also support Maya's
extrapolating loops, where the portion of the series outside all knots is
evaluated using repeats of the series shape.  See the USD Anim proposal for
details.

## New Extrapolation Modes

We will add the extrapolation modes "sloped", which is similar to "linear" but
allows explicit specification of the slope; and "none", which causes a series
not to have a value at all outside of its knot range.

## Automatic Tangents

Ts will support one of Maya's automatic tangent algorithms, allowing Bezier
knots to assume "nice" tangents based only on their values.  The specific
algorithm has yet to be chosen.

## Reduction

Ts will not require clients to implement every feature.  A simple client, for
example, may permit series authoring, but support only Bezier curves.  We want
all clients to be able to read all series, so we will allow clients to transform
Ts series into other Ts series that use only a specified set of features.  Most
such conversions will be exact.  Examples include the emulation of Hermites,
looping, and dual-valued knots.

## Knot Metadata

Ts will support arbitrary, structured metadata on any knot, similar to the
metadata on USD properties.


# USD INTEGRATION

## Serialization

Splines and series will serialize to and from `usda` and `usdc`.

## Attribute Value Resolution

This will be the main purpose of USD Anim: to have a USD attribute's value be
determined by a Ts spline or series.


# TUNING

## Storage Reorganization

The current in-memory representation of splines will need to change in order to
accommodate new features, but we may also change it in order to ensure efficient
memory access.  One example might be side-allocation of less commonly used
fields like metadata.

## Performance Tests and Optimization

We will write tests that profile the most common Ts operations, and optimize the
implementation accordingly.


# AUXILIARY WORK

## usdview

Basic visualization of splines will be added to `usdview`.

## Scalar xformOps

Currently, USD's `xformOp`s support scalar-valued rotations, but for translation
and scaling, only vector values (three scalars, one for each spatial axis).  We
will expand the `xformOp` schema to allow scalar values for translations and
scales also.  This will make USD Anim more useful in its first release: all
basic transforms will be animatable with splines.  (Ts will not support splines
of vectors, only linearly interpolating `TsLerpSeries` of them.)

## Documentation

Ts will be extensively documented.


# INVISIBLE WORK

At Pixar, there will be much work to keep Presto running on top of Ts, even as
Ts changes from the form it had in Presto (it has already departed somewhat).
This will result in development periods during which little obvious change is
occurring in the open-source code.  It may also result in minor Ts changes that
don't serve any obvious external purpose.
