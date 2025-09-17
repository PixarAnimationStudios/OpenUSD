//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_TOKENS_H
#define PXR_USD_SDF_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

// Miscellaneous Tokens
#define SDF_TOKENS                                      \
    ((AnyTypeToken, "__AnyType__"))

TF_DECLARE_PUBLIC_TOKENS(SdfTokens, SDF_API, SDF_TOKENS);

#define SDF_PATH_ABSOLUTE_INDICATOR_CHAR '/'
#define SDF_PATH_ABSOLUTE_INDICATOR_STR "/"
#define SDF_PATH_RELATIVE_ROOT_CHAR '.'
#define SDF_PATH_RELATIVE_ROOT_STR "."
#define SDF_PATH_CHILD_DELIMITER_CHAR '/'
#define SDF_PATH_CHILD_DELIMITER_STR "/"
#define SDF_PATH_NS_DELIMITER_CHAR ':'
#define SDF_PATH_NS_DELIMITER_STR ":"
#define SDF_PATH_RELATIONSHIP_TARGET_START_CHAR '['
#define SDF_PATH_RELATIONSHIP_TARGET_START_STR "["
#define SDF_PATH_RELATIONSHIP_TARGET_END_CHAR ']'
#define SDF_PATH_RELATIONSHIP_TARGET_END_STR "]"
#define SDF_PATH_PROPERTY_DELIMITER_CHAR '.'
#define SDF_PATH_PROPERTY_DELIMITER_STR "."

#define SDF_PATH_TOKENS                                                 \
    ((absoluteIndicator, SDF_PATH_ABSOLUTE_INDICATOR_STR))              \
    ((relativeRoot, SDF_PATH_RELATIVE_ROOT_STR))                        \
    ((childDelimiter, SDF_PATH_CHILD_DELIMITER_STR))                    \
    ((propertyDelimiter, SDF_PATH_PROPERTY_DELIMITER_STR))              \
    ((relationshipTargetStart, SDF_PATH_RELATIONSHIP_TARGET_START_STR)) \
    ((relationshipTargetEnd, SDF_PATH_RELATIONSHIP_TARGET_END_STR))     \
    ((parentPathElement, ".."))                                         \
    ((mapperIndicator, "mapper"))                                       \
    ((expressionIndicator, "expression"))                               \
    ((mapperArgDelimiter, "."))                                         \
    ((namespaceDelimiter, SDF_PATH_NS_DELIMITER_STR))                   \
    ((empty, ""))

TF_DECLARE_PUBLIC_TOKENS(SdfPathTokens, SDF_API, SDF_PATH_TOKENS);

#define SDF_METADATA_DISPLAYGROUP_TOKENS              \
    ((core, ""))                                      \
    ((internal, "Internal"))                          \
    ((dmanip, "Direct Manip"))                        \
    ((pipeline, "Pipeline"))                          \
    ((symmetry, "Symmetry"))                          \
    ((ui, "User Interface"))

TF_DECLARE_PUBLIC_TOKENS(SdfMetadataDisplayGroupTokens,
                         SDF_API,
                         SDF_METADATA_DISPLAYGROUP_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_TOKENS_H
