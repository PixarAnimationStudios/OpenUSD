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

%code requires {
struct Sdf_LayerIdentifierParserContext;
}

%{
#include "pxr/usd/sdf/layer.h"
#include <string>

#define YYSTYPE std::string

struct yy_buffer_state;
typedef void *yyscan_t;

//--------------------------------------------------------------------
// Context object for storing parser results
//--------------------------------------------------------------------
struct Sdf_LayerIdentifierParserContext
{
    std::string layerPath;
    SdfLayer::FileFormatArguments args;

    std::string error;
    yyscan_t scanner;
};

//--------------------------------------------------------------------
// Extern declarations to scanner data and functions
//--------------------------------------------------------------------

// Generated bison symbols.
int layerIdentifierYyparse(Sdf_LayerIdentifierParserContext *);
int layerIdentifierYylex(YYSTYPE *, yyscan_t);
int layerIdentifierYylex_init(yyscan_t *);
int layerIdentifierYylex_destroy(yyscan_t);
yy_buffer_state *layerIdentifierYy_scan_string(const char*, yyscan_t);
void layerIdentifierYy_delete_buffer(yy_buffer_state *, yyscan_t);

static void layerIdentifierYyerror(
    Sdf_LayerIdentifierParserContext *, const char *);

#define yyscanner context->scanner
%}

// Make this re-entrant
%define api.pure
%lex-param { yyscan_t yyscanner }
%parse-param { Sdf_LayerIdentifierParserContext *context }

%token TOK_LAYER_PATH
%token TOK_IDENTIFIER
%token TOK_VALUE

%%

identifier:
    layer_path
    | layer_path '?' arguments
    ;

layer_path:
    TOK_LAYER_PATH {
        context->layerPath = $1;
    }

arguments:
    argument
    | argument '&' arguments
    ;

argument:
    TOK_IDENTIFIER '=' TOK_VALUE {
        context->args[$1] = $3;
    }
    | TOK_IDENTIFIER '=' TOK_IDENTIFIER {
        context->args[$1] = $3;
    }
    | TOK_IDENTIFIER '=' {
        context->args[$1] = std::string();
    }
    ;

%%

static void 
layerIdentifierYyerror(
    Sdf_LayerIdentifierParserContext *context, const char *msg)
{
    context->error = msg;
}

bool
Sdf_ParseLayerIdentifier(
    const std::string& argumentString,
    std::string* layerPath,
    SdfLayer::FileFormatArguments* args)
{
    Sdf_LayerIdentifierParserContext context;

    // Initialize the scanner, allowing it to be reentrant.
    layerIdentifierYylex_init(&context.scanner);

    // Run parser.
    yy_buffer_state *buf = layerIdentifierYy_scan_string(
        argumentString.c_str(), context.scanner);

    bool success = false;
    if (layerIdentifierYyparse(&context) == 0) {
        success = true;
        layerPath->swap(context.layerPath);
        args->swap(context.args);
    }
    else {
        success = false;
        TF_CODING_ERROR(context.error);
    }

    // Clean up.
    layerIdentifierYy_delete_buffer(buf, context.scanner);
    layerIdentifierYylex_destroy(context.scanner);

    return success;
}
