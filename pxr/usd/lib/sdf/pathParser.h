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
#ifndef SDF_PATH_PARSER_H
#define SDF_PATH_PARSER_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/token.h"
#include <string>
#include <vector>

// Opaque buffer type handle.
struct yy_buffer_state;

// Lexical scanner type.
typedef void *yyscan_t;

// Lexical scanner value type.
struct Sdf_PathLexerValue {
    PXR_NS::TfToken token;
    PXR_NS::Sdf_PathNodeConstRefPtr path;
};
#define YYSTYPE Sdf_PathLexerValue

typedef std::vector< std::pair<PXR_NS::TfToken, 
                               PXR_NS::TfToken> > Sdf_PathVariantSelections;

// Lexical scanner context.
struct Sdf_PathParserContext {
    PXR_NS::Sdf_PathNodeConstRefPtr node;
    std::vector<Sdf_PathVariantSelections> variantSelectionStack;
    std::string errStr;
    yyscan_t scanner;
};

// Generated bison symbols.
int pathYyparse(Sdf_PathParserContext *context);
int pathYylex_init(yyscan_t *yyscanner);
int pathYylex_destroy(yyscan_t yyscanner);
yy_buffer_state *pathYy_scan_string(const char* str, yyscan_t yyscanner);
yy_buffer_state *pathYy_scan_bytes(const char* str, size_t len, 
                                   yyscan_t yyscanner);
void pathYy_delete_buffer(yy_buffer_state *b, yyscan_t yyscanner);

PXR_NAMESPACE_OPEN_SCOPE

SDF_API
int SdfPathYyparse(Sdf_PathParserContext *context);
SDF_API
int SdfPathYylex_init(yyscan_t *yyscanner);
SDF_API
int SdfPathYylex_destroy(yyscan_t yyscanner);
SDF_API
yy_buffer_state *SdfPathYy_scan_string(const char* str, yyscan_t yyscanner);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_PATH_PARSER_H
