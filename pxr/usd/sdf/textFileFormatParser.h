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
#ifndef PXR_USD_SDF_TEXT_FILE_FORMAT_PARSER_H
#define PXR_USD_SDF_TEXT_FILE_FORMAT_PARSER_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/layerHints.h"
#include "pxr/usd/sdf/textParserContext.h"
#include "pxr/usd/ar/asset.h"
#include <string>

// Opaque buffer type handle.
struct yy_buffer_state;

// Lexical scanner type.
typedef void *yyscan_t;
typedef size_t yy_size_t;

// Generated flex symbols
yy_buffer_state* textFileFormatYy_scan_buffer(char* base, yy_size_t size, yyscan_t yyscanner);
yy_buffer_state* textFileFormatUtf8Yy_scan_buffer(char* base, yy_size_t size, yyscan_t yyscanner);

// Generated bison symbols.
int textFileFormatYyparse(PXR_NS::Sdf_TextParserContext *context);
int textFileFormatYylex_init(yyscan_t *yyscanner);
int textFileFormatYylex_destroy(yyscan_t yyscanner);
yy_buffer_state *textFileFormatYy_scan_string(const char* str, yyscan_t yyscanner);
yy_buffer_state *textFileFormatYy_scan_bytes(const char* str, size_t len, 
                                   yyscan_t yyscanner);
void textFileFormatYy_delete_buffer(yy_buffer_state *b, yyscan_t yyscanner);
void textFileFormatYyerror(PXR_NS::Sdf_TextParserContext* context, const char* s);
void textFileFormatYyset_extra(PXR_NS::Sdf_TextParserContext *context, 
                             yyscan_t yyscanner);

// Generated bison symbols for UTF8 version of the parser
int textFileFormatUtf8Yyparse(PXR_NS::Sdf_TextParserContext *context);
int textFileFormatUtf8Yylex_init(yyscan_t *yyscanner);
int textFileFormatUtf8Yylex_destroy(yyscan_t yyscanner);
yy_buffer_state* textFileFormatUtf8Yy_scan_string(const char* str, yyscan_t yyscanner);
yy_buffer_state* textFileFormatUtf8Yy_scan_bytes(const char* str, int len, 
                                    yyscan_t yyscanner);
void textFileFormatUtf8Yy_delete_buffer(yy_buffer_state* b, yyscan_t yyscanner);
void textFileFormatUtf8Yyerror(PXR_NS::Sdf_TextParserContext *context, const char *s);
void textFileFormatUtf8Yyset_extra(PXR_NS::Sdf_TextParserContext *context, 
                             yyscan_t yyscanner);

PXR_NAMESPACE_OPEN_SCOPE

// Helper class for generating/managing the buffer used by flex.
//
// This simply reads the given file entirely into memory, padded as flex
// requires, and passes it along. Normally, flex reads data from a given file in
// blocks of 8KB, which leads to O(n^2) behavior when trying to match strings
// that are over this size. Giving flex a pre-filled buffer avoids this
// behavior.
struct Sdf_MemoryFlexBuffer : public boost::noncopyable
{
public:
    Sdf_MemoryFlexBuffer(const std::shared_ptr<ArAsset>& asset,
                         const std::string& name, yyscan_t scanner);
    ~Sdf_MemoryFlexBuffer();

    yy_buffer_state *GetBuffer() { return _flexBuffer; }

private:
    yy_buffer_state *_flexBuffer;

    std::unique_ptr<char[]> _fileBuffer;

    yyscan_t _scanner;
};

/// Parse a text layer into an SdfData
bool 
Sdf_ParseLayer(
    const std::string& fileContext, 
    const std::shared_ptr<PXR_NS::ArAsset>& asset,
    const std::string& magicId,
    const std::string& versionString,
    bool metadataOnly,
    SdfDataRefPtr data,
    SdfLayerHints *hints);

/// Parse a layer text string into an SdfData
bool
Sdf_ParseLayerFromString(
    const std::string & layerString, 
    const std::string & magicId,
    const std::string & versionString,
    SdfDataRefPtr data,
    SdfLayerHints *hints);

PXR_NAMESPACE_CLOSE_SCOPE

#endif