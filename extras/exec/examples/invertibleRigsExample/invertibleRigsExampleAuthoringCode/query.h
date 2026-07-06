//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_EXEC_EXAMPLES_INVERTIBLE_RIGS_EXAMPLE_QUERY_H
#define PXR_EXTRAS_EXEC_EXAMPLES_INVERTIBLE_RIGS_EXAMPLE_QUERY_H

#include "pxr/pxr.h"

#include "api.h"

#include "pxr/usd/usd/attribute.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Class that implements query functionality needed to demonstrate an
/// invertible rig with switch compensation.
///
class InvertibleRigsExample_Query {
public:
    EXEC_INVERTIBLERIGSEXAMPLE_API
    InvertibleRigsExample_Query(const UsdStageRefPtr &stage);

    /// Finds all switch avars that are sources for switch attributes named
    /// \p attributeName.
    ///
    /// Traverses the stage to find all attributes with the given name that have
    /// IrRole metdata identifying them as switch attributes. From these
    /// attributes, traverses connections to find the unique set of switch avars
    /// that are dataflow sources for the found attributes.
    ///
    EXEC_INVERTIBLERIGSEXAMPLE_API
    UsdAttributeVector
    FindSwitchAvars(const TfToken &attributeName) const;

private:
    UsdStageRefPtr _stage;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
