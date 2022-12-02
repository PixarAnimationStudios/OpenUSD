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

#include "pxr/pxr.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/trace/trace.h"
#include "pxr/usd/sdf/textFileFormatParser.h"
#include "pxr/usd/sdf/textParserContext.h"


#ifdef SDF_PARSER_DEBUG_MODE
extern int textFileFormatYydebug;
extern int textFileFormatUtf8Yydebug;
#else
static int textFileFormatYydebug;
static int textFileFormatUtf8Yydebug;
#endif // SDF_PARSER_DEBUG_MODE

PXR_NAMESPACE_USING_DIRECTIVE

static void _ReportParseError(Sdf_TextParserContext *context, 
                              const std::string &text)
{
    if (!context->values.IsRecordingString()) {
        if (UseUTF8Identifiers())
        {
            textFileFormatUtf8Yyerror(context, text.c_str());
        }
        else
        {
            textFileFormatYyerror(context, text.c_str());
        }
    }
}

Sdf_MemoryFlexBuffer::Sdf_MemoryFlexBuffer(
    const std::shared_ptr<ArAsset>& asset,
    const std::string& name, yyscan_t scanner)
    : _flexBuffer(nullptr)
    , _scanner(scanner)
{
    // flex requires 2 bytes of null padding at the end of any buffers it is
    // given.  We'll allocate a buffer with 2 padding bytes, then read the
    // entire file in.
    static const size_t paddingBytesRequired = 2;

    size_t size = asset->GetSize();
    std::unique_ptr<char[]> buffer(new char[size + paddingBytesRequired]);

    if (asset->Read(buffer.get(), size, 0) != size) {
        TF_RUNTIME_ERROR("Failed to read asset contents @%s@: "
                         "an error occurred while reading",
                         name.c_str());
        return;
    }

    // Set null padding.
    memset(buffer.get() + size, '\0', paddingBytesRequired);
    _fileBuffer = std::move(buffer);
    if (UseUTF8Identifiers())
    {
        _flexBuffer = textFileFormatUtf8Yy_scan_buffer(
            _fileBuffer.get(), size + paddingBytesRequired, _scanner);
    }
    else
    {
        _flexBuffer = textFileFormatYy_scan_buffer(
            _fileBuffer.get(), size + paddingBytesRequired, _scanner);
    }
}

Sdf_MemoryFlexBuffer::~Sdf_MemoryFlexBuffer()
{
    if (_flexBuffer)
    {
        if (UseUTF8Identifiers())
        {
            textFileFormatUtf8Yy_delete_buffer(_flexBuffer, _scanner);
        }
        else
        {
            textFileFormatYy_delete_buffer(_flexBuffer, _scanner);
        }
    }
}

namespace {
struct _DebugContext {
    explicit _DebugContext(bool state=true) : _old(textFileFormatYydebug) { textFileFormatYydebug = state; }
    ~_DebugContext() { textFileFormatYydebug = _old; }
private:
    bool _old;
};
struct _DebugContextUtf8 {
    explicit _DebugContextUtf8(bool state=true) : _old(textFileFormatUtf8Yydebug) { textFileFormatUtf8Yydebug = state; }
    ~_DebugContextUtf8() { textFileFormatUtf8Yydebug = _old; }
private:
    bool _old;
};
}

/// Parse a text layer into an SdfData
bool 
Sdf_ParseLayer(
    const std::string& fileContext, 
    const std::shared_ptr<ArAsset>& asset,
    const std::string& magicId,
    const std::string& versionString,
    bool metadataOnly,
    SdfDataRefPtr data,
    SdfLayerHints *hints)
{
    TfAutoMallocTag2 tag("Sdf", "Sdf_ParseLayer");

    TRACE_FUNCTION();

    // Turn on debugging, if enabled.
    if (UseUTF8Identifiers())
    {
        _DebugContextUtf8 debugCtx;
    }
    else
    {
        _DebugContext debugCtx;
    }

    // Configure for input file.
    Sdf_TextParserContext context;

    context.data = data;
    context.fileContext = fileContext;
    context.magicIdentifierToken = magicId;
    context.versionString = versionString;
    context.metadataOnly = metadataOnly;
    context.values.errorReporter =
        std::bind(_ReportParseError, &context, std::placeholders::_1);


    if (UseUTF8Identifiers())
    {
        // Initialize the scanner, allowing it to be reentrant.
        textFileFormatUtf8Yylex_init(&context.scanner);
        textFileFormatUtf8Yyset_extra(&context, context.scanner);

        int status = -1;
        {
            Sdf_MemoryFlexBuffer input(asset, fileContext, context.scanner);
            yy_buffer_state *buf = input.GetBuffer();

            // Continue parsing if we have a valid input buffer. If there 
            // is no buffer, the appropriate error will have already been emitted.
            if (buf) {
                try {
                    TRACE_SCOPE("textFileFormatYyParse");
                    status = textFileFormatUtf8Yyparse(&context);
                    *hints = context.layerHints;
                } catch (boost::bad_get const &) {
                    TF_CODING_ERROR("Bad boost:get<T>() in layer parser.");
                    textFileFormatUtf8Yyerror(&context, TfStringPrintf("Internal layer parser error.").c_str());
                }
            }
        }

        // Note that the destructor for 'input' calls
        // textFileFormatYy_delete_buffer(), which requires a valid scanner
        // object. So we need 'input' to go out of scope before we can destroy the
        // scanner.
        textFileFormatUtf8Yylex_destroy(context.scanner);

        return status == 0;
    }
    else
    {
        // Initialize the scanner, allowing it to be reentrant.
        textFileFormatYylex_init(&context.scanner);
        textFileFormatYyset_extra(&context, context.scanner);

        int status = -1;
        {
            Sdf_MemoryFlexBuffer input(asset, fileContext, context.scanner);
            yy_buffer_state *buf = input.GetBuffer();

            // Continue parsing if we have a valid input buffer. If there 
            // is no buffer, the appropriate error will have already been emitted.
            if (buf) {
                try {
                    TRACE_SCOPE("textFileFormatYyParse");
                    status = textFileFormatYyparse(&context);
                    *hints = context.layerHints;
                } catch (boost::bad_get const &) {
                    TF_CODING_ERROR("Bad boost:get<T>() in layer parser.");
                    textFileFormatYyerror(&context, TfStringPrintf("Internal layer parser error.").c_str());
                }
            }
        }

        // Note that the destructor for 'input' calls
        // textFileFormatYy_delete_buffer(), which requires a valid scanner
        // object. So we need 'input' to go out of scope before we can destroy the
        // scanner.
        textFileFormatYylex_destroy(context.scanner);

        return status == 0;
    }
}

/// Parse a layer text string into an SdfData
bool
Sdf_ParseLayerFromString(
    const std::string & layerString, 
    const std::string & magicId,
    const std::string & versionString,
    SdfDataRefPtr data,
    SdfLayerHints *hints)
{
    TfAutoMallocTag2 tag("Sdf", "Sdf_ParseLayerFromString");

    TRACE_FUNCTION();

    // Configure for input string.
    Sdf_TextParserContext context;

    context.data = data;
    context.magicIdentifierToken = magicId;
    context.versionString = versionString;
    context.values.errorReporter =
        std::bind(_ReportParseError, &context, std::placeholders::_1);

    if (UseUTF8Identifiers())
    {
        // Initialize the scanner, allowing it to be reentrant.
        textFileFormatUtf8Yylex_init(&context.scanner);
        textFileFormatUtf8Yyset_extra(&context, context.scanner);

        // Run parser.
        yy_buffer_state *buf = textFileFormatUtf8Yy_scan_string(
            layerString.c_str(), context.scanner);
        int status = -1;
        try {
            TRACE_SCOPE("textFileFormatYyParse");
            status = textFileFormatUtf8Yyparse(&context);
            *hints = context.layerHints;
        } catch (boost::bad_get const &) {
            TF_CODING_ERROR("Bad boost:get<T>() in layer parser.");
            textFileFormatUtf8Yyerror(&context, TfStringPrintf("Internal layer parser error.").c_str());
        }

        // Clean up.
        textFileFormatUtf8Yy_delete_buffer(buf, context.scanner);
        textFileFormatUtf8Yylex_destroy(context.scanner);

        return status == 0;
    }
    else
    {
        // Initialize the scanner, allowing it to be reentrant.
        textFileFormatYylex_init(&context.scanner);
        textFileFormatYyset_extra(&context, context.scanner);

        // Run parser.
        yy_buffer_state *buf = textFileFormatYy_scan_string(
            layerString.c_str(), context.scanner);
        int status = -1;
        try {
            TRACE_SCOPE("textFileFormatYyParse");
            status = textFileFormatYyparse(&context);
            *hints = context.layerHints;
        } catch (boost::bad_get const &) {
            TF_CODING_ERROR("Bad boost:get<T>() in layer parser.");
            textFileFormatYyerror(&context, TfStringPrintf("Internal layer parser error.").c_str());
        }

        // Clean up.
        textFileFormatYy_delete_buffer(buf, context.scanner);
        textFileFormatYylex_destroy(context.scanner);

        return status == 0;
    }
}