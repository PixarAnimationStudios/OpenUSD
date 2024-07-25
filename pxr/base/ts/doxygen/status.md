
# USD Anim Project Status {#page_ts_status}

**USD Anim is IN DEVELOPMENT.**  It is not yet ready for general use.

# Mostly Complete

The following are believed close to final:

- The Spline / KnotMap / Knot API.
- USD serialization formats (usda, usdc).
- Bezier evaluation.
- Anti-regression.

That said, these aspects are **still subect to change**.

# Still to Come

## UNIMPLEMENTED API

### Hermite Evaluation

Currently Ts only supports Bezier curves.  It will be expanded to support
Hermites as well, matching Maya behavior.  Hermites have fewer degrees of
freedom than Beziers - tangent lengths are fixed - and, in return, they are
faster to evaluate, requiring only a cubic evaluation rather than an iterative
parametric solve.

### Evaluation Variations

- `Sample()`: fast curve approximation for drawing.
- A simple evaluation cache class.
- Find the time regions where two splines differ.

### Spline Editing

- `Split()`: add knots without affecting the curve shape.
- `RemoveKnot()` with `affectedIntervalOut`.
- `ClearRedundantKnots()`.
- Simplify and resample.

### Looping

Ts supports both _inner loops_ and _extrapolating loops_.  These are currently
handled correctly in Bezier evaluation.

There will also be support for _baking_, in which copies of the knots are
returned with duplicates made to represent loops.  This is often convenient for
interactive spline editing.

Loops are not yet correctly supported by the anti-regression system.

### Queries

- `GetValueRange()`
- `IsLinear()`
- `IsC*Continuous()`
- `IsSegment{Flat,Monotonic}()`
- `IsKnotRedundant()`

## ADDITIONAL FEATURES

### Automatic Tangents

Ts will support one of Maya's automatic tangent algorithms, allowing Bezier
knots to assume "nice" tangents based only on their values.  The specific
algorithm has yet to be chosen.

### Reduction

Ts will not require clients to implement every feature.  A simple client, for
example, may permit series authoring, but support only Bezier curves.  We want
all clients to be able to read all series, so we will allow clients to transform
Ts series into other Ts series that use only a specified set of features.  Most
such conversions will be exact.  Examples include the emulation of Hermites,
looping, and dual-valued knots.

## USD INTEGRATION

### Attribute Value Resolution

This will be the main purpose of USD Anim: to have a USD attribute's value be
determined by a Ts spline or series.  This work will be done last, after
everything else is finished.

In addition to evaluating exactly at a given time, `UsdAttribute::Get()` will be
able to evaluate at a _pre-time_, a "limit from the left".  This will allow
introspection of discontinuous value functions, including from held
interpolation and dual-valued knots.

### usdview

Basic visualization of splines will be added to `usdview`.

### Scalar xformOps

Currently, USD's `xformOp`s support scalar-valued rotations, but for translation
and scaling, only vector values (three scalars, one for each spatial axis).  We
will expand the `xformOp` schema to allow scalar values for translations and
scales also.  This will make USD Anim more useful in its first release: all
basic transforms will be animatable with splines.  (Ts will not support splines
of vectors, only linearly interpolating `TsLerpSeries` of them.)

## TESTS AND DOCUMENTATION

There will be extensive additional:

- Correctness tests.
- Performance tests.
- Documentation.


