# OpenExec

OpenExec supports efficient computation of values that are derived from data
encoded in USD scenes. Computational behaviors can be published for USD
schemas. These computations are ingested by OpenExec, along with a composed
scene, to yield structures that OpenExec uses to compute and cache values, with
support for invalidating cached values in response to scene changes.

The [execUsd](execUsd/README.md) library is the primary entry point for
OpenExec.
