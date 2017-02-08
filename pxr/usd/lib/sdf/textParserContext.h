//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef SDF_TEXTPARSERCONTEXT_H
#define SDF_TEXTPARSERCONTEXT_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/parserValueContext.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/vt/dictionary.h"

#include "pxr/base/tf/token.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Lexical scanner type.
typedef void *yyscan_t;

// This class contains the global state while parsing a menva file.
// It contains the data structures that we use to create the scene description
// from the file.

class Sdf_TextParserContext {
public:
    // Constructor.
    Sdf_TextParserContext();

    // Destructor.
    ~Sdf_TextParserContext() {
    }

    std::string magicIdentifierToken;
    std::string versionString;
    std::string fileContext;

    // State for layer refs, in general
    std::string layerRefPath;
    SdfLayerOffset layerRefOffset;

    // State for sublayers
    std::vector<std::string> subLayerPaths;

    // State for sublayer offsets
    std::vector<SdfLayerOffset> subLayerOffsets;

    // The connection target for the mapper currently being specified
    SdfPath mapperTarget;
    std::string mapperParamName;
    std::vector<TfToken> mapperArgsNameVector;

    // String list currently being built
    std::vector<TfToken> nameVector;

    SdfTimeSampleMap timeSamples;
    double timeSampleTime;

    SdfPath savedPath;

    // Whether the current relationship target being parsed is allowed to
    // have data like markers or relational attributes.
    bool relParsingAllowTargetData;
    // relationship target paths that will be saved in a list op
    // (use a boost::optional to track whether we have seen an opinion at all.)
    boost::optional<SdfPathVector> relParsingTargetPaths;
    // relationship target paths that will be appended to the relatioship's
    // list of target children.
    SdfPathVector relParsingNewTargetChildren;

    // helpers for connection path parsing
    SdfPathVector connParsingTargetPaths;
    bool connParsingAllowConnectionData;

    // Relationship target or attribute connection marker
    std::string marker;

    // helpers for inherit path parsing
    SdfPathVector inheritParsingTargetPaths;

    // helpers for specializes path parsing
    SdfPathVector specializesParsingTargetPaths;

    // helpers for reference parsing
    SdfReferenceVector referenceParsingRefs;

    // helper for relocates parsing
    SdfRelocatesMap relocatesParsingMap;

    // helpers for generic metadata
    TfToken genericMetadataKey;
    SdfListOpType listOpType;

    // The value parser context
    Sdf_ParserValueContext values;

    // Last parsed value
    VtValue currentValue;

    // Vector of dictionaries used to parse nested dictionaries.  
    // The first element in the vector contains the last parsed dictionary.
    std::vector<VtDictionary> currentDictionaries;

    bool seenError;

    bool custom;
    SdfSpecifier specifier;
    SdfDataRefPtr data;
    SdfPath path;
    TfToken typeName;
    VtValue variability;
    VtValue assoc;

    // Should we only read metadata from the file?
    bool metadataOnly;

    // Stack for the child names of all the prims currently being parsed
    // For instance if we're currently parsing /A/B then this vector
    // will contain three elements:
    //    names of the root prims
    //    names of A's children
    //    names of B's children.
    std::vector<std::vector<TfToken> > nameChildrenStack;

    // Stack for the property names of all the objects currently being parsed
    std::vector<std::vector<TfToken> > propertiesStack;

    // Stack of names of variant sets  being built.
    std::vector<std::string> currentVariantSetNames;

    // Stack of names of variants for the variant sets being built
    std::vector<std::vector<std::string> > currentVariantNames;

    unsigned int menvaLineNo;

    // Used by flex for reentrant parsing
    yyscan_t scanner;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_TEXTPARSERCONTEXT_H
