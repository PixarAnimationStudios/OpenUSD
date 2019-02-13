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

/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         textFileFormatYyparse
#define yylex           textFileFormatYylex
#define yyerror         textFileFormatYyerror
#define yylval          textFileFormatYylval
#define yychar          textFileFormatYychar
#define yydebug         textFileFormatYydebug
#define yynerrs         textFileFormatYynerrs


/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 25 "pxr/usd/sdf/textFileFormat.yy"


#include "pxr/pxr.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/sdf/allowed.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/fileIO_Common.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/textParserContext.h"
#include "pxr/usd/sdf/parserValueContext.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/arch/errno.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/mallocTag.h"

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <functional>
#include <sstream>
#include <string>
#include <vector>

// See this page for info as to why this is here.  Especially note the last
// paragraph.  http://www.delorie.com/gnu/docs/bison/bison_91.html
#define YYINITDEPTH 1500

PXR_NAMESPACE_USING_DIRECTIVE

using Sdf_ParserHelpers::Value;
using boost::get;

//--------------------------------------------------------------------
// Helper macros/functions for handling errors
//--------------------------------------------------------------------

#define ABORT_IF_ERROR(seenError) if (seenError) YYABORT
#define Err(context, ...)                                        \
    textFileFormatYyerror(context, TfStringPrintf(__VA_ARGS__).c_str())

#define ERROR_IF_NOT_ALLOWED(context, allowed)                   \
    {                                                            \
        const SdfAllowed allow = allowed;                        \
        if (!allow) {                                            \
            Err(context, "%s", allow.GetWhyNot().c_str());       \
        }                                                        \
    }

#define ERROR_AND_RETURN_IF_NOT_ALLOWED(context, allowed)        \
    {                                                            \
        const SdfAllowed allow = allowed;                        \
        if (!allow) {                                            \
            Err(context, "%s", allow.GetWhyNot().c_str());       \
            return;                                              \
        }                                                        \
    }

//--------------------------------------------------------------------
// Extern declarations to scanner data and functions
//--------------------------------------------------------------------

#define YYSTYPE Sdf_ParserHelpers::Value

// Opaque buffer type handle.
struct yy_buffer_state;

// Generated bison symbols.
void textFileFormatYyerror(Sdf_TextParserContext *context, const char *s);

extern int textFileFormatYylex(YYSTYPE *yylval_param, yyscan_t yyscanner);
extern char *textFileFormatYyget_text(yyscan_t yyscanner);
extern size_t textFileFormatYyget_leng(yyscan_t yyscanner);
extern int textFileFormatYylex_init(yyscan_t *yyscanner);
extern int textFileFormatYylex_destroy(yyscan_t yyscanner);
extern void textFileFormatYyset_extra(Sdf_TextParserContext *context, 
                             yyscan_t yyscanner);
extern yy_buffer_state *textFileFormatYy_scan_buffer(char *yy_str, size_t size, 
                                            yyscan_t yyscanner);
extern yy_buffer_state *textFileFormatYy_scan_string(const char *yy_str, 
                                            yyscan_t yyscanner);
extern yy_buffer_state *textFileFormatYy_scan_bytes(const char *yy_str, size_t numBytes,
                                           yyscan_t yyscanner);
extern void textFileFormatYy_delete_buffer(yy_buffer_state *b, yyscan_t yyscanner);

#define yyscanner context->scanner

//--------------------------------------------------------------------
// Helpers
//--------------------------------------------------------------------

static bool
_SetupValue(const std::string& typeName, Sdf_TextParserContext *context)
{
    return context->values.SetupFactory(typeName);
}

template <class T>
static bool
_HasDuplicates(const std::vector<T> &v)
{
    std::set<T> s;
    TF_FOR_ALL(i, v) {
        if (!s.insert(*i).second) {
            return true;
        }
    }
    return false;
}

namespace
{
template <class T> 
const std::vector<T>& _ToItemVector(const std::vector<T>& v)
{
    return v;
}
template <class T>
std::vector<T> _ToItemVector(const VtArray<T>& v)
{
    return std::vector<T>(v.begin(), v.end());
}
}

// Set a single ListOp vector in the list op for the current
// path and specified key.
template <class T>
static void
_SetListOpItems(const TfToken &key, SdfListOpType type,
                const T &itemList, Sdf_TextParserContext *context)
{
    typedef SdfListOp<typename T::value_type> ListOpType;
    typedef typename ListOpType::ItemVector ItemVector;

    const ItemVector& items = _ToItemVector(itemList);

    if (_HasDuplicates(items)) {
        Err(context, "Duplicate items exist for field '%s' at '%s'",
            key.GetText(), context->path.GetText());
    }

    const SdfAbstractDataSpecId specId(&context->path);

    ListOpType op = context->data->GetAs<ListOpType>(specId, key);
    op.SetItems(items, type);

    context->data->Set(specId, key, VtValue::Take(op));
}

// Append a single item to the vector for the current path and specified key.
template <class T>
static void
_AppendVectorItem(const TfToken& key, const T& item,
                  Sdf_TextParserContext *context)
{
    const SdfAbstractDataSpecId specId(&context->path);

    std::vector<T> vec = context->data->GetAs<std::vector<T> >(specId, key);
    vec.push_back(item);

    context->data->Set(specId, key, VtValue(vec));
}

template <class T>
inline static void
_SetField(const SdfPath& path, const TfToken& key, const T& item,
          Sdf_TextParserContext *context)
{
    context->data->Set(SdfAbstractDataSpecId(&path), key, VtValue(item));
}

inline static bool
_HasField(const SdfPath& path, const TfToken& key, VtValue* value, 
          Sdf_TextParserContext *context)
{
    return context->data->Has(SdfAbstractDataSpecId(&path), key, value);
}

inline static bool
_HasSpec(const SdfPath& path, Sdf_TextParserContext *context)
{
    return context->data->HasSpec(SdfAbstractDataSpecId(&path));
}

inline static void
_CreateSpec(const SdfPath& path, SdfSpecType specType, 
            Sdf_TextParserContext *context)
{
    context->data->CreateSpec(SdfAbstractDataSpecId(&path), specType);
}

static void
_MatchMagicIdentifier(const Value& arg1, Sdf_TextParserContext *context)
{
    const std::string cookie = TfStringTrimRight(arg1.Get<std::string>());
    const std::string expected = "#" + context->magicIdentifierToken + " ";
    if (TfStringStartsWith(cookie, expected)) {
        if (!context->versionString.empty() &&
            !TfStringEndsWith(cookie, context->versionString)) {
            TF_WARN("File '%s' is not the latest %s version (found '%s', "
                "expected '%s'). The file may parse correctly and yield "
                "incorrect results.",
                context->fileContext.c_str(),
                context->magicIdentifierToken.c_str(),
                cookie.substr(expected.length()).c_str(),
                context->versionString.c_str());
        }
    }
    else {
        Err(context, "Magic Cookie '%s'. Expected prefix of '%s'",
            TfStringTrim(cookie).c_str(),
            expected.c_str());
    }
}

static SdfPermission
_GetPermissionFromString(const std::string & str,
                         Sdf_TextParserContext *context)
{
    if (str == "public") {
        return SdfPermissionPublic;
    } else if (str == "private") {
        return SdfPermissionPrivate;
    } else {
        Err(context, "'%s' is not a valid permission constant", str.c_str());
        return SdfPermissionPublic;
    }
}

static TfEnum
_GetDisplayUnitFromString(const std::string & name,
                          Sdf_TextParserContext *context)
{
    const TfEnum &unit = SdfGetUnitFromName(name);
    if (unit == TfEnum()) {
        Err(context,  "'%s' is not a valid display unit", name.c_str());
    }
    return unit;
}

static void
_ValueAppendAtomic(const Value& arg1, Sdf_TextParserContext *context)
{
    context->values.AppendValue(arg1);
}

static void
_ValueSetAtomic(Sdf_TextParserContext *context)
{
    if (!context->values.IsRecordingString()) {
        if (context->values.valueIsShaped) {
            Err(context, "Type name has [] for non-shaped value!\n");
            return;
        }
    }

    std::string errStr;
    context->currentValue = context->values.ProduceValue(&errStr);
    if (context->currentValue.IsEmpty()) {
        Err(context, "Error parsing simple value: %s", errStr.c_str());
        return;
    }
}

static void
_PrimSetInheritListItems(SdfListOpType opType, Sdf_TextParserContext *context) 
{
    if (context->inheritParsingTargetPaths.empty() &&
        opType != SdfListOpTypeExplicit) {
        Err(context, 
            "Setting inherit paths to None (or empty list) is only allowed "
            "when setting explicit inherit paths, not for list editing");
        return;
    }

    TF_FOR_ALL(path, context->inheritParsingTargetPaths) {
        ERROR_AND_RETURN_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidInheritPath(*path));
    }

    _SetListOpItems(SdfFieldKeys->InheritPaths, opType, 
                    context->inheritParsingTargetPaths, context);
}

static void
_InheritAppendPath(Sdf_TextParserContext *context)
{
    // Expand paths relative to the containing prim.
    //
    // This strips any variant selections from the containing prim
    // path before expanding the relative path, which is what we
    // want.  Inherit paths are not allowed to be variants.
    SdfPath absPath = 
        context->savedPath.MakeAbsolutePath(context->path.GetPrimPath());

    context->inheritParsingTargetPaths.push_back(absPath);
}

static void
_PrimSetSpecializesListItems(SdfListOpType opType, Sdf_TextParserContext *context) 
{
    if (context->specializesParsingTargetPaths.empty() &&
        opType != SdfListOpTypeExplicit) {
        Err(context, 
            "Setting specializes paths to None (or empty list) is only allowed "
            "when setting explicit specializes paths, not for list editing");
        return;
    }

    TF_FOR_ALL(path, context->specializesParsingTargetPaths) {
        ERROR_AND_RETURN_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidSpecializesPath(*path));
    }

    _SetListOpItems(SdfFieldKeys->Specializes, opType, 
                    context->specializesParsingTargetPaths, context);
}

static void
_SpecializesAppendPath(Sdf_TextParserContext *context)
{
    // Expand paths relative to the containing prim.
    //
    // This strips any variant selections from the containing prim
    // path before expanding the relative path, which is what we
    // want.  Specializes paths are not allowed to be variants.
    SdfPath absPath = 
        context->savedPath.MakeAbsolutePath(context->path.GetPrimPath());

    context->specializesParsingTargetPaths.push_back(absPath);
}

static void
_PrimSetReferenceListItems(SdfListOpType opType, Sdf_TextParserContext *context) 
{
    if (context->referenceParsingRefs.empty() &&
        opType != SdfListOpTypeExplicit) {
        Err(context, 
            "Setting references to None (or an empty list) is only allowed "
            "when setting explicit references, not for list editing");
        return;
    }

    TF_FOR_ALL(ref, context->referenceParsingRefs) {
        ERROR_AND_RETURN_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidReference(*ref));
    }

    _SetListOpItems(SdfFieldKeys->References, opType, 
                    context->referenceParsingRefs, context);
}

static void
_PrimSetPayloadListItems(SdfListOpType opType, Sdf_TextParserContext *context) 
{
    if (context->payloadParsingRefs.empty() &&
        opType != SdfListOpTypeExplicit) {
        Err(context, 
            "Setting payload to None (or an empty list) is only allowed "
            "when setting explicit payloads, not for list editing");
        return;
    }

    TF_FOR_ALL(ref, context->payloadParsingRefs) {
        ERROR_AND_RETURN_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidPayload(*ref));
    }

    _SetListOpItems(SdfFieldKeys->Payload, opType, 
                    context->payloadParsingRefs, context);
}

static void
_PrimSetVariantSetNamesListItems(SdfListOpType opType, 
                                 Sdf_TextParserContext *context)
{
    std::vector<std::string> names;
    names.reserve(context->nameVector.size());
    TF_FOR_ALL(name, context->nameVector) {
        ERROR_AND_RETURN_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidVariantIdentifier(*name));
        names.push_back(name->GetText());
    }

    _SetListOpItems(SdfFieldKeys->VariantSetNames, opType, names, context);

    // If the op type is added or explicit, create the variant sets
    if (opType == SdfListOpTypeAdded || opType == SdfListOpTypeExplicit) {
        TF_FOR_ALL(i, context->nameVector) {
            _CreateSpec(
                context->path.AppendVariantSelection(*i,""),
                SdfSpecTypeVariantSet, context);
        }

        _SetField(
            context->path, SdfChildrenKeys->VariantSetChildren, 
            context->nameVector, context);
    }

}

static void
_RelationshipInitTarget(const SdfPath& targetPath,
                        Sdf_TextParserContext *context)
{
    SdfPath path = context->path.AppendTarget(targetPath);

    if (!_HasSpec(path, context)) {
        // Create relationship target spec by setting the appropriate 
        // object type flag.
        _CreateSpec(path, SdfSpecTypeRelationshipTarget, context);

        // Add the target path to the owning relationship's list of target 
        // children.
        context->relParsingNewTargetChildren.push_back(targetPath);
    }
}

static void
_RelationshipSetTargetsList(SdfListOpType opType, 
                            Sdf_TextParserContext *context)
{
    if (!context->relParsingTargetPaths) {
        // No target paths were encountered.
        return;
    }

    if (context->relParsingTargetPaths->empty() &&
        opType != SdfListOpTypeExplicit) {
        Err(context, 
            "Setting relationship targets to None (or empty list) is only "
            "allowed when setting explicit targets, not for list editing");
        return;
    }

    TF_FOR_ALL(path, *context->relParsingTargetPaths) {
        ERROR_AND_RETURN_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidRelationshipTargetPath(*path));
    }

    if (opType == SdfListOpTypeAdded || 
        opType == SdfListOpTypeExplicit) {

        // Initialize relationship target specs for each target path that
        // is added in this layer.
        TF_FOR_ALL(pathIter, *context->relParsingTargetPaths) {
            _RelationshipInitTarget(*pathIter, context);
        }
    }

    _SetListOpItems(SdfFieldKeys->TargetPaths, opType, 
                    *context->relParsingTargetPaths, context);
}

static void
_PrimSetVariantSelection(Sdf_TextParserContext *context)
{
    SdfVariantSelectionMap refVars;

    // The previous parser implementation allowed multiple variant selection
    // dictionaries in prim metadata to be merged, so we do the same here.
    VtValue oldVars;
    if (_HasField(
            context->path, SdfFieldKeys->VariantSelection, &oldVars, context)) {
        refVars = oldVars.Get<SdfVariantSelectionMap>();
    }

    TF_FOR_ALL(it, context->currentDictionaries[0]) {
        if (!it->second.IsHolding<std::string>()) {
            Err(context, "variant name must be a string");
            return;
        } else {
            const std::string variantName = it->second.Get<std::string>();
            ERROR_AND_RETURN_IF_NOT_ALLOWED(
                context, 
                SdfSchema::IsValidVariantIdentifier(variantName));

            refVars[it->first] = variantName;
        }
    }

    _SetField(context->path, SdfFieldKeys->VariantSelection, refVars, context);
    context->currentDictionaries[0].clear();
}

static void
_RelocatesAdd(const Value& arg1, const Value& arg2, 
              Sdf_TextParserContext *context)
{
    const std::string& srcStr    = arg1.Get<std::string>();
    const std::string& targetStr = arg2.Get<std::string>();

    SdfPath srcPath(srcStr);
    SdfPath targetPath(targetStr);

    if (!srcPath.IsPrimPath()) {
        Err(context, "'%s' is not a valid prim path",
            srcStr.c_str());
        return;
    }
    if (!targetPath.IsPrimPath()) {
        Err(context, "'%s' is not a valid prim path",
            targetStr.c_str());
        return;
    }

    // The relocates map is expected to only hold absolute paths. The
    // SdRelocatesMapProxy ensures that all paths are made absolute when
    // editing, but since we're bypassing that proxy and setting the map
    // directly into the underlying SdfData, we need to explicitly absolutize
    // paths here.
    const SdfPath srcAbsPath = srcPath.MakeAbsolutePath(context->path);
    const SdfPath targetAbsPath = targetPath.MakeAbsolutePath(context->path);

    context->relocatesParsingMap.insert(std::make_pair(srcAbsPath, 
                                                        targetAbsPath));
}

static void
_AttributeSetConnectionTargetsList(SdfListOpType opType, 
                                   Sdf_TextParserContext *context)
{
    if (context->connParsingTargetPaths.empty() &&
        opType != SdfListOpTypeExplicit) {
        Err(context, "Setting connection paths to None (or an empty list) "
            "is only allowed when setting explicit connection paths, "
            "not for list editing");
        return;
    }

    TF_FOR_ALL(path, context->connParsingTargetPaths) {
        ERROR_AND_RETURN_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidAttributeConnectionPath(*path));
    }

    if (opType == SdfListOpTypeAdded || 
        opType == SdfListOpTypeExplicit) {

        TF_FOR_ALL(pathIter, context->connParsingTargetPaths) {
            SdfPath path = context->path.AppendTarget(*pathIter);
            if (!_HasSpec(path, context)) {
                _CreateSpec(path, SdfSpecTypeConnection, context);
            }
        }

        _SetField(
            context->path, SdfChildrenKeys->ConnectionChildren,
            context->connParsingTargetPaths, context);
    }

    _SetListOpItems(SdfFieldKeys->ConnectionPaths, opType, 
                    context->connParsingTargetPaths, context);
}

static void
_AttributeAppendConnectionPath(Sdf_TextParserContext *context)
{
    // Expand paths relative to the containing prim.
    //
    // This strips any variant selections from the containing prim
    // path before expanding the relative path, which is what we
    // want.  Connection paths never point into the variant namespace.
    SdfPath absPath = 
        context->savedPath.MakeAbsolutePath(context->path.GetPrimPath());

    // XXX Workaround for bug 68132:
    // Prior to the fix to bug 67916, FilterGenVariantBase was authoring
    // invalid connection paths containing variant selections (which
    // Sd was failing to report as erroneous).  As a result, there's
    // a fair number of assets out there with these broken forms of
    // connection paths.  As a migration measure, we discard those
    // variant selections here.
    if (absPath.ContainsPrimVariantSelection()) {
        TF_WARN("Connection path <%s> (in file @%s@, line %i) has a variant "
                "selection, but variant selections are not meaningful in "
                "connection paths.  Stripping the variant selection and "
                "using <%s> instead.  Resaving the file will fix this issue.",
                absPath.GetText(),
                context->fileContext.c_str(),
                context->menvaLineNo,
                absPath.StripAllVariantSelections().GetText());
        absPath = absPath.StripAllVariantSelections();
    }

    context->connParsingTargetPaths.push_back(absPath);
}

static void
_PrimInitAttribute(const Value &arg1, Sdf_TextParserContext *context)  
{
    TfToken name(arg1.Get<std::string>());
    if (!SdfPath::IsValidNamespacedIdentifier(name)) {
        Err(context, "'%s' is not a valid attribute name", name.GetText());
    }

    if (context->path.IsTargetPath())
        context->path = context->path.AppendRelationalAttribute(name);
    else
        context->path = context->path.AppendProperty(name);

    // If we haven't seen this attribute before, then set the object type
    // and add it to the parent's list of properties. Otherwise both have
    // already been done, so we don't need to do anything.
    if (!_HasSpec(context->path, context)) {
        context->propertiesStack.back().push_back(name);
        _CreateSpec(context->path, SdfSpecTypeAttribute, context);
        _SetField(context->path, SdfFieldKeys->Custom, false, context);
    }

    if(context->custom)
        _SetField(context->path, SdfFieldKeys->Custom, true, context);

    // If the type was previously set, check that it matches. Otherwise set it.
    const TfToken newType(context->values.valueTypeName);

    VtValue oldTypeValue;
    if (_HasField(
            context->path, SdfFieldKeys->TypeName, &oldTypeValue, context)) {
        const TfToken& oldType = oldTypeValue.Get<TfToken>();

        if (newType != oldType) {
            Err(context,
                "attribute '%s' already has type '%s', cannot change to '%s'",
                context->path.GetName().c_str(),
                oldType.GetText(),
                newType.GetText());
        }
    }
    else {
        _SetField(context->path, SdfFieldKeys->TypeName, newType, context);
    }

    // If the variability was previously set, check that it matches. Otherwise
    // set it.  If the 'variability' VtValue is empty, that indicates varying
    // variability.
    SdfVariability variability = context->variability.IsEmpty() ? 
        SdfVariabilityVarying : context->variability.Get<SdfVariability>();
    VtValue oldVariability;
    if (_HasField(
            context->path, SdfFieldKeys->Variability, &oldVariability, 
            context)) {
        if (variability != oldVariability.Get<SdfVariability>()) {
            Err(context, 
                "attribute '%s' already has variability '%s', "
                "cannot change to '%s'",
                context->path.GetName().c_str(),
                TfEnum::GetName(oldVariability.Get<SdfVariability>()).c_str(),
                TfEnum::GetName(variability).c_str() );
        }
    } else {
        _SetField(
            context->path, SdfFieldKeys->Variability, variability, context);
    }
}

static void
_DictionaryBegin(Sdf_TextParserContext *context)
{
    context->currentDictionaries.push_back(VtDictionary());

    // Whenever we parse a value for an unregistered generic metadata field, 
    // the parser value context records the string representation only, because
    // we don't have enough type information to generate a C++ value. However,
    // dictionaries are a special case because we have all the type information
    // we need to generate C++ values. So, override the previous setting.
    if (context->values.IsRecordingString()) {
        context->values.StopRecordingString();
    }
}

static void
_DictionaryEnd(Sdf_TextParserContext *context)
{
    context->currentDictionaries.pop_back();
}

static void
_DictionaryInsertValue(const Value& arg1, Sdf_TextParserContext *context)
{
    size_t n = context->currentDictionaries.size();
    context->currentDictionaries[n-2][arg1.Get<std::string>()] = 
        context->currentValue;
}

static void
_DictionaryInsertDictionary(const Value& arg1,
                            Sdf_TextParserContext *context)
{
    size_t n = context->currentDictionaries.size();
    // Insert the parsed dictionary into the parent dictionary.
    context->currentDictionaries[n-2][arg1.Get<std::string>()].Swap(
        context->currentDictionaries[n-1]);
    // Clear out the last dictionary (there can be more dictionaries on the
    // same nesting level).
    context->currentDictionaries[n-1].clear();
}

static void
_DictionaryInitScalarFactory(const Value& arg1,
                             Sdf_TextParserContext *context)
{
    const std::string& typeName = arg1.Get<std::string>();
    if (!_SetupValue(typeName, context)) {
        Err(context, "Unrecognized value typename '%s' for dictionary", 
            typeName.c_str());
    }
}

static void
_DictionaryInitShapedFactory(const Value& arg1,
                             Sdf_TextParserContext *context)
{
    const std::string typeName = arg1.Get<std::string>() + "[]";
    if (!_SetupValue(typeName, context)) {
        Err(context, "Unrecognized value typename '%s' for dictionary", 
            typeName.c_str());
    }
}

static void
_ValueSetTuple(Sdf_TextParserContext *context)
{
    if (!context->values.IsRecordingString()) {
        if (context->values.valueIsShaped) {
            Err(context, "Type name has [] for non-shaped value.\n");
            return;
        }
    }

    std::string errStr;
    context->currentValue = context->values.ProduceValue(&errStr);
    if (context->currentValue == VtValue()) {
        Err(context, "Error parsing tuple value: %s", errStr.c_str());
        return;
    }
}

static void
_ValueSetList(Sdf_TextParserContext *context)
{
    if (!context->values.IsRecordingString()) {
        if (!context->values.valueIsShaped) {
            Err(context, "Type name missing [] for shaped value.");
            return;
        }
    }

    std::string errStr;
    context->currentValue = context->values.ProduceValue(&errStr);
    if (context->currentValue == VtValue()) {
        Err(context, "Error parsing shaped value: %s", errStr.c_str());
        return;
    }
}

static void
_ValueSetShaped(Sdf_TextParserContext *context)
{
    if (!context->values.IsRecordingString()) {
        if (!context->values.valueIsShaped) {
            Err(context, "Type name missing [] for shaped value.");
            return;
        }
    }

    std::string errStr;
    context->currentValue = context->values.ProduceValue(&errStr);
    if (context->currentValue == VtValue()) {
        // The factory method ProduceValue() uses for shaped types
        // only returns empty VtArrays, not empty VtValues, so this
        // is impossible to hit currently.
        // CODE_COVERAGE_OFF
        Err(context, "Error parsing shaped value: %s", errStr.c_str());
        // CODE_COVERAGE_OFF_GCOV_BUG
        // The following line actually shows as executed (a ridiculous 
        // number of times) even though the line above shwos as 
        // not executed
        return;
        // CODE_COVERAGE_ON_GCOV_BUG
        // CODE_COVERAGE_ON
    }
}

static void _ValueSetCurrentToSdfPath(const Value& arg1,
                                     Sdf_TextParserContext *context)
{
    // make current Value an SdfPath of the given argument...
    std::string s = arg1.Get<std::string>();
    // If path is empty, use default c'tor to construct empty path.
    // XXX: 08/04/08 Would be nice if SdfPath would allow 
    // SdfPath("") without throwing a warning.
    context->currentValue = s.empty() ? SdfPath() : SdfPath(s);
}

static void
_PrimInitRelationship(const Value& arg1,
                      Sdf_TextParserContext *context)
{
    TfToken name( arg1.Get<std::string>() );
    if (!SdfPath::IsValidNamespacedIdentifier(name)) {
        Err(context, "'%s' is not a valid relationship name", name.GetText());
        return;
    }

    context->path = context->path.AppendProperty(name);

    if (!_HasSpec(context->path, context)) {
        context->propertiesStack.back().push_back(name);
        _CreateSpec(context->path, SdfSpecTypeRelationship, context);
    }

    _SetField(
        context->path, SdfFieldKeys->Variability, 
        context->variability, context);

    if (context->custom) {
        _SetField(context->path, SdfFieldKeys->Custom, context->custom, context);
    }

    context->relParsingAllowTargetData = false;
    context->relParsingTargetPaths.reset();
    context->relParsingNewTargetChildren.clear();
}

static void
_PrimEndRelationship(Sdf_TextParserContext *context)
{
    if (!context->relParsingNewTargetChildren.empty()) {
        std::vector<SdfPath> children = 
            context->data->GetAs<std::vector<SdfPath> >(
                SdfAbstractDataSpecId(&context->path), 
                SdfChildrenKeys->RelationshipTargetChildren);

        children.insert(children.end(), 
                        context->relParsingNewTargetChildren.begin(),
                        context->relParsingNewTargetChildren.end());

        _SetField(
            context->path, SdfChildrenKeys->RelationshipTargetChildren, 
            children, context);
    }

    context->path = context->path.GetParentPath();
}

static void
_RelationshipAppendTargetPath(const Value& arg1,
                              Sdf_TextParserContext *context)
{
    // Add a new target to the current relationship
    const std::string& pathStr = arg1.Get<std::string>();
    SdfPath path(pathStr);

    if (!path.IsAbsolutePath()) {
        // Expand paths relative to the containing prim.
        //
        // This strips any variant selections from the containing prim
        // path before expanding the relative path, which is what we
        // want.  Target paths never point into the variant namespace.
        path = path.MakeAbsolutePath(context->path.GetPrimPath());
    }

    if (!context->relParsingTargetPaths) {
        // This is the first target we've seen for this relationship.
        // Start tracking them in a vector.
        context->relParsingTargetPaths = SdfPathVector();
    }
    context->relParsingTargetPaths->push_back(path);
}

static void
_PathSetPrim(const Value& arg1, Sdf_TextParserContext *context)
{
    const std::string& pathStr = arg1.Get<std::string>();
    context->savedPath = SdfPath(pathStr);
    if (!context->savedPath.IsPrimPath()) {
        Err(context, "'%s' is not a valid prim path", pathStr.c_str());
    }
}

static void
_PathSetProperty(const Value& arg1, Sdf_TextParserContext *context)
{
    const std::string& pathStr = arg1.Get<std::string>();
    context->savedPath = SdfPath(pathStr);
    if (!context->savedPath.IsPropertyPath()) {
        Err(context, "'%s' is not a valid property path", pathStr.c_str());
    }
}

static void
_PathSetPrimOrPropertyScenePath(const Value& arg1,
                                Sdf_TextParserContext *context)
{
    const std::string& pathStr = arg1.Get<std::string>();
    context->savedPath = SdfPath(pathStr);
    // Valid paths are prim or property paths that do not contain variant
    // selections.
    SdfPath const &path = context->savedPath;
    bool pathValid = (path.IsPrimPath() || path.IsPropertyPath()) &&
        !path.ContainsPrimVariantSelection();
    if (!pathValid) {
        Err(context, "'%s' is not a valid prim or property scene path",
            pathStr.c_str());
    }
}

template <class ListOpType>
static bool
_SetItemsIfListOp(const TfType& type, Sdf_TextParserContext *context)
{
    if (!type.IsA<ListOpType>()) {
        return false;
    }

    typedef VtArray<typename ListOpType::value_type> ArrayType;

    if (!TF_VERIFY(context->currentValue.IsHolding<ArrayType>() ||
                   context->currentValue.IsEmpty())) {
        return true;
    }

    ArrayType vtArray;
    if (context->currentValue.IsHolding<ArrayType>()) {
        vtArray = context->currentValue.UncheckedGet<ArrayType>();
    }

    _SetListOpItems(
        context->genericMetadataKey, context->listOpType, vtArray, context);
    return true;
}

static void
_SetGenericMetadataListOpItems(const TfType& fieldType, 
                               Sdf_TextParserContext *context)
{
    // Chain together attempts to set list op items using 'or' to bail
    // out as soon as we successfully write out the list op we're holding.
    _SetItemsIfListOp<SdfIntListOp>(fieldType, context)    ||
    _SetItemsIfListOp<SdfInt64ListOp>(fieldType, context)  ||
    _SetItemsIfListOp<SdfUIntListOp>(fieldType, context)   ||
    _SetItemsIfListOp<SdfUInt64ListOp>(fieldType, context) ||
    _SetItemsIfListOp<SdfStringListOp>(fieldType, context) ||
    _SetItemsIfListOp<SdfTokenListOp>(fieldType, context);
}

template <class ListOpType>
static bool
_IsListOpType(const TfType& type, TfType* itemArrayType = nullptr)
{
    if (type.IsA<ListOpType>()) {
        if (itemArrayType) {
            typedef VtArray<typename ListOpType::value_type> ArrayType;
            *itemArrayType = TfType::Find<ArrayType>();
        }
        return true;
    }
    return false;
}

static bool
_IsGenericMetadataListOpType(const TfType& type,
                             TfType* itemArrayType = nullptr)
{
    return _IsListOpType<SdfIntListOp>(type, itemArrayType)    ||
           _IsListOpType<SdfInt64ListOp>(type, itemArrayType)  || 
           _IsListOpType<SdfUIntListOp>(type, itemArrayType)   ||
           _IsListOpType<SdfUInt64ListOp>(type, itemArrayType) ||
           _IsListOpType<SdfStringListOp>(type, itemArrayType) ||
           _IsListOpType<SdfTokenListOp>(type, itemArrayType);
}

static void
_GenericMetadataStart(const Value &name, SdfSpecType specType,
                      Sdf_TextParserContext *context)
{
    context->genericMetadataKey = TfToken(name.Get<std::string>());
    context->listOpType = SdfListOpTypeExplicit;

    const SdfSchema& schema = SdfSchema::GetInstance();
    const SdfSchema::SpecDefinition &specDef = 
        *schema.GetSpecDefinition(specType);
    if (specDef.IsMetadataField(context->genericMetadataKey)) {
        // Prepare to parse a known field
        const SdfSchema::FieldDefinition &fieldDef = 
            *schema.GetFieldDefinition(context->genericMetadataKey);
        const TfType fieldType = fieldDef.GetFallbackValue().GetType();

        // For list op-valued metadata fields, set up the parser as if
        // we were parsing an array of the list op's underlying type.
        // In _GenericMetadataEnd, we'll produce this list and set it
        // into the appropriate place in the list op.
        TfType itemArrayType;
        if (_IsGenericMetadataListOpType(fieldType, &itemArrayType)) {
            _SetupValue(schema.FindType(itemArrayType).
                            GetAsToken().GetString(), context);
        }
        else {
            _SetupValue(schema.FindType(fieldDef.GetFallbackValue()).
		            GetAsToken().GetString(), context);
        }
    } else {
        // Prepare to parse only the string representation of this metadata
        // value, since it's an unregistered field.
        context->values.StartRecordingString();
    }
}

static void
_GenericMetadataEnd(SdfSpecType specType, Sdf_TextParserContext *context)
{
    const SdfSchema& schema = SdfSchema::GetInstance();
    const SdfSchema::SpecDefinition &specDef = 
        *schema.GetSpecDefinition(specType);
    if (specDef.IsMetadataField(context->genericMetadataKey)) {
        // Validate known fields before storing them
        const SdfSchema::FieldDefinition &fieldDef = 
            *schema.GetFieldDefinition(context->genericMetadataKey);
        const TfType fieldType = fieldDef.GetFallbackValue().GetType();

        if (_IsGenericMetadataListOpType(fieldType)) {
            if (!fieldDef.IsValidListValue(context->currentValue)) {
                Err(context, "invalid value for field \"%s\"", 
                    context->genericMetadataKey.GetText());
            }
            else {
                _SetGenericMetadataListOpItems(fieldType, context);
            }
        }
        else {
            if (!fieldDef.IsValidValue(context->currentValue) ||
                context->currentValue.IsEmpty()) {
                Err(context, "invalid value for field \"%s\"", 
                    context->genericMetadataKey.GetText());
            }
            else {
                _SetField(
                    context->path, context->genericMetadataKey, 
                    context->currentValue, context);
            }
        }
    } else if (specDef.IsValidField(context->genericMetadataKey)) {
        // Prevent the user from overwriting fields that aren't metadata
        Err(context, "\"%s\" is registered as a non-metadata field", 
            context->genericMetadataKey.GetText());
    } else {
        // Stuff unknown fields into a SdfUnregisteredValue so they can pass
        // through loading and saving unmodified
        VtValue value;
        if (context->currentValue.IsHolding<VtDictionary>()) {
            // If we parsed a dictionary, store it's actual value. Dictionaries
            // can be parsed fully because they contain type information.
            value = 
                SdfUnregisteredValue(context->currentValue.Get<VtDictionary>());
        } else {
            // Otherwise, we parsed a simple value or a shaped list of simple
            // values. We want to store the parsed string, but we need to
            // determine whether to unpack it into an SdfUnregisteredListOp
            // or to just store the string directly.
            auto getOldValue = [context]() {
                VtValue v;
                if (_HasField(context->path, context->genericMetadataKey,
                              &v, context)
                    && TF_VERIFY(v.IsHolding<SdfUnregisteredValue>())) {
                    v = v.UncheckedGet<SdfUnregisteredValue>().GetValue();
                }
                else {
                    v = VtValue();
                }
                return v;
            };

            auto getRecordedStringAsUnregisteredValue = [context]() {
                std::string s = context->values.GetRecordedString();
                if (s == "None") { 
                    return std::vector<SdfUnregisteredValue>(); 
                }

                // Put the entire string representation of this list into
                // a single SdfUnregisteredValue, but strip off the enclosing
                // brackets so that we don't write out two sets of brackets
                // when serializing out the list op.
                if (!s.empty() && s.front() == '[') { s.erase(0, 1); }
                if (!s.empty() && s.back() == ']') { s.pop_back(); }
                return std::vector<SdfUnregisteredValue>(
                    { SdfUnregisteredValue(s) });
            };

            VtValue oldValue = getOldValue();
            if (context->listOpType == SdfListOpTypeExplicit) {
                // In this case, we can't determine whether the we've parsed
                // an explicit list op statement or a simple value.
                // We just store the recorded string directly, as that's the
                // simplest thing to do.
                value = 
                    SdfUnregisteredValue(context->values.GetRecordedString());
            }
            else if (oldValue.IsEmpty()
                     || oldValue.IsHolding<SdfUnregisteredValueListOp>()) {
                // In this case, we've parsed a list op statement so unpack
                // it into a list op unless we've already parsed something
                // for this field that *isn't* a list op.
                SdfUnregisteredValueListOp listOp = 
                    oldValue.GetWithDefault<SdfUnregisteredValueListOp>();
                listOp.SetItems(getRecordedStringAsUnregisteredValue(), 
                                context->listOpType);
                value = SdfUnregisteredValue(listOp);
            }
            else {
                // If we've parsed a list op statement but have a non-list op
                // stored in this field, leave that value in place and ignore
                // the new value. We should only encounter this case if someone
                // hand-edited the layer in an unexpected or invalid way, so
                // just keeping the first value we find should be OK.
            }
        }

        if (!value.IsEmpty()) {
            _SetField(context->path, context->genericMetadataKey, 
                      value, context);
        }
    }

    context->values.Clear();
    context->currentValue = VtValue();
}

//--------------------------------------------------------------------
// The following are used to configure bison
//--------------------------------------------------------------------

// Use this to enable generation of parser trace code.
// Useful when debugging, but costly.  To enable/disable, (un)comment:
//#define SDF_PARSER_DEBUG_MODE

#ifdef SDF_PARSER_DEBUG_MODE
#define YYDEBUG 1
#endif // SDF_PARSER_DEBUG_MODE



/* Line 189 of yacc.c  */
#line 1246 "pxr/usd/sdf/textFileFormat.tab.cpp"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOK_NL = 258,
     TOK_MAGIC = 259,
     TOK_SYNTAX_ERROR = 260,
     TOK_ASSETREF = 261,
     TOK_PATHREF = 262,
     TOK_IDENTIFIER = 263,
     TOK_CXX_NAMESPACED_IDENTIFIER = 264,
     TOK_NAMESPACED_IDENTIFIER = 265,
     TOK_NUMBER = 266,
     TOK_STRING = 267,
     TOK_ABSTRACT = 268,
     TOK_ADD = 269,
     TOK_APPEND = 270,
     TOK_ATTRIBUTES = 271,
     TOK_CLASS = 272,
     TOK_CONFIG = 273,
     TOK_CONNECT = 274,
     TOK_CUSTOM = 275,
     TOK_CUSTOMDATA = 276,
     TOK_DEF = 277,
     TOK_DEFAULT = 278,
     TOK_DELETE = 279,
     TOK_DICTIONARY = 280,
     TOK_DISPLAYUNIT = 281,
     TOK_DOC = 282,
     TOK_INHERITS = 283,
     TOK_KIND = 284,
     TOK_MAPPER = 285,
     TOK_NAMECHILDREN = 286,
     TOK_NONE = 287,
     TOK_OFFSET = 288,
     TOK_OVER = 289,
     TOK_PERMISSION = 290,
     TOK_PAYLOAD = 291,
     TOK_PREFIX_SUBSTITUTIONS = 292,
     TOK_SUFFIX_SUBSTITUTIONS = 293,
     TOK_PREPEND = 294,
     TOK_PROPERTIES = 295,
     TOK_REFERENCES = 296,
     TOK_RELOCATES = 297,
     TOK_REL = 298,
     TOK_RENAMES = 299,
     TOK_REORDER = 300,
     TOK_ROOTPRIMS = 301,
     TOK_SCALE = 302,
     TOK_SPECIALIZES = 303,
     TOK_SUBLAYERS = 304,
     TOK_SYMMETRYARGUMENTS = 305,
     TOK_SYMMETRYFUNCTION = 306,
     TOK_TIME_SAMPLES = 307,
     TOK_UNIFORM = 308,
     TOK_VARIANTS = 309,
     TOK_VARIANTSET = 310,
     TOK_VARIANTSETS = 311,
     TOK_VARYING = 312
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 1345 "pxr/usd/sdf/textFileFormat.tab.cpp"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1155

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  70
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  238
/* YYNRULES -- Number of rules.  */
#define YYNRULES  508
/* YYNRULES -- Number of states.  */
#define YYNSTATES  929

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   312

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      58,    59,     2,     2,    69,     2,    63,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    66,    68,
       2,    60,     2,     2,    67,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    61,     2,    62,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    64,     2,    65,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    17,
      19,    21,    23,    25,    27,    29,    31,    33,    35,    37,
      39,    41,    43,    45,    47,    49,    51,    53,    55,    57,
      59,    61,    63,    65,    67,    69,    71,    73,    75,    77,
      79,    81,    83,    85,    87,    89,    91,    93,    95,    97,
     101,   102,   106,   108,   114,   116,   120,   122,   126,   128,
     130,   131,   136,   137,   143,   144,   150,   151,   157,   158,
     164,   165,   171,   175,   179,   183,   189,   191,   195,   198,
     200,   201,   206,   208,   212,   216,   220,   222,   226,   227,
     231,   232,   237,   238,   242,   243,   248,   249,   253,   254,
     259,   264,   266,   270,   271,   278,   280,   286,   288,   292,
     294,   298,   300,   302,   304,   306,   307,   312,   313,   319,
     320,   326,   327,   333,   334,   340,   341,   347,   351,   355,
     359,   360,   365,   366,   372,   373,   379,   380,   386,   387,
     393,   394,   400,   401,   406,   407,   413,   414,   420,   421,
     427,   428,   434,   435,   441,   442,   447,   448,   454,   455,
     461,   462,   468,   469,   475,   476,   482,   483,   488,   489,
     495,   496,   502,   503,   509,   510,   516,   517,   523,   527,
     531,   535,   540,   545,   550,   555,   560,   564,   567,   571,
     575,   577,   579,   583,   589,   591,   595,   599,   600,   604,
     605,   609,   615,   617,   621,   623,   625,   627,   631,   637,
     639,   643,   647,   648,   652,   653,   657,   663,   665,   669,
     671,   675,   677,   679,   683,   689,   691,   695,   697,   699,
     701,   705,   711,   713,   717,   719,   724,   725,   728,   730,
     734,   738,   740,   746,   748,   752,   754,   756,   759,   761,
     764,   767,   770,   773,   776,   779,   780,   790,   792,   795,
     796,   804,   809,   814,   816,   818,   820,   822,   824,   826,
     830,   832,   835,   836,   837,   844,   845,   846,   854,   855,
     863,   864,   873,   874,   883,   884,   893,   894,   903,   904,
     913,   914,   925,   926,   934,   936,   938,   940,   942,   944,
     945,   950,   951,   955,   961,   963,   967,   968,   974,   975,
     979,   985,   987,   991,   995,   997,   999,  1003,  1009,  1011,
    1015,  1017,  1018,  1023,  1024,  1030,  1031,  1034,  1036,  1040,
    1041,  1046,  1050,  1051,  1055,  1061,  1063,  1067,  1069,  1071,
    1073,  1075,  1076,  1081,  1082,  1088,  1089,  1095,  1096,  1102,
    1103,  1109,  1110,  1116,  1120,  1124,  1128,  1132,  1135,  1136,
    1139,  1141,  1143,  1144,  1150,  1151,  1154,  1156,  1160,  1165,
    1170,  1172,  1174,  1176,  1178,  1180,  1184,  1185,  1191,  1192,
    1195,  1197,  1201,  1205,  1207,  1209,  1211,  1213,  1215,  1217,
    1219,  1221,  1224,  1226,  1228,  1230,  1232,  1234,  1235,  1240,
    1244,  1246,  1250,  1252,  1254,  1256,  1257,  1262,  1266,  1268,
    1272,  1274,  1276,  1278,  1281,  1285,  1288,  1289,  1297,  1304,
    1305,  1311,  1312,  1318,  1319,  1325,  1326,  1332,  1333,  1339,
    1340,  1346,  1347,  1355,  1357,  1359,  1360,  1364,  1370,  1372,
    1376,  1378,  1380,  1382,  1384,  1385,  1390,  1391,  1397,  1398,
    1404,  1405,  1411,  1412,  1418,  1419,  1425,  1429,  1433,  1437,
    1440,  1441,  1444,  1446,  1448,  1452,  1458,  1460,  1464,  1467,
    1469,  1473,  1474,  1476,  1477,  1483,  1484,  1487,  1489,  1493,
    1495,  1497,  1502,  1503,  1505,  1507,  1509,  1511,  1513,  1515,
    1517,  1519,  1521,  1523,  1525,  1527,  1529,  1531,  1533,  1534,
    1536,  1539,  1541,  1543,  1545,  1548,  1549,  1551,  1553
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      71,     0,    -1,    74,    -1,    13,    -1,    14,    -1,    15,
      -1,    16,    -1,    17,    -1,    18,    -1,    19,    -1,    20,
      -1,    21,    -1,    22,    -1,    23,    -1,    24,    -1,    25,
      -1,    26,    -1,    27,    -1,    28,    -1,    29,    -1,    30,
      -1,    31,    -1,    32,    -1,    33,    -1,    34,    -1,    36,
      -1,    35,    -1,    37,    -1,    38,    -1,    39,    -1,    40,
      -1,    41,    -1,    42,    -1,    43,    -1,    44,    -1,    45,
      -1,    46,    -1,    47,    -1,    48,    -1,    49,    -1,    50,
      -1,    51,    -1,    52,    -1,    53,    -1,    54,    -1,    55,
      -1,    56,    -1,    57,    -1,    76,    -1,    76,    94,   306,
      -1,    -1,     4,    75,    73,    -1,   306,    -1,   306,    58,
      77,    59,   306,    -1,   306,    -1,   306,    78,   302,    -1,
      80,    -1,    78,   303,    80,    -1,   300,    -1,    12,    -1,
      -1,    79,    81,    60,   246,    -1,    -1,    24,   300,    82,
      60,   245,    -1,    -1,    14,   300,    83,    60,   245,    -1,
      -1,    39,   300,    84,    60,   245,    -1,    -1,    15,   300,
      85,    60,   245,    -1,    -1,    45,   300,    86,    60,   245,
      -1,    27,    60,    12,    -1,    49,    60,    87,    -1,    61,
     306,    62,    -1,    61,   306,    88,   304,    62,    -1,    89,
      -1,    88,   305,    89,    -1,    90,    91,    -1,     6,    -1,
      -1,    58,    92,   302,    59,    -1,    93,    -1,    92,   303,
      93,    -1,    33,    60,    11,    -1,    47,    60,    11,    -1,
      95,    -1,    94,   307,    95,    -1,    -1,    22,    96,   103,
      -1,    -1,    22,   102,    97,   103,    -1,    -1,    17,    98,
     103,    -1,    -1,    17,   102,    99,   103,    -1,    -1,    34,
     100,   103,    -1,    -1,    34,   102,   101,   103,    -1,    45,
      46,    60,   164,    -1,   300,    -1,   102,    63,   300,    -1,
      -1,    12,   104,   105,    64,   167,    65,    -1,   306,    -1,
     306,    58,   106,    59,   306,    -1,   306,    -1,   306,   107,
     302,    -1,   109,    -1,   107,   303,   109,    -1,   300,    -1,
      21,    -1,    50,    -1,    12,    -1,    -1,   108,   110,    60,
     246,    -1,    -1,    24,   300,   111,    60,   245,    -1,    -1,
      14,   300,   112,    60,   245,    -1,    -1,    39,   300,   113,
      60,   245,    -1,    -1,    15,   300,   114,    60,   245,    -1,
      -1,    45,   300,   115,    60,   245,    -1,    27,    60,    12,
      -1,    29,    60,    12,    -1,    35,    60,   300,    -1,    -1,
      36,   116,    60,   140,    -1,    -1,    24,    36,   117,    60,
     140,    -1,    -1,    14,    36,   118,    60,   140,    -1,    -1,
      39,    36,   119,    60,   140,    -1,    -1,    15,    36,   120,
      60,   140,    -1,    -1,    45,    36,   121,    60,   140,    -1,
      -1,    28,   122,    60,   154,    -1,    -1,    24,    28,   123,
      60,   154,    -1,    -1,    14,    28,   124,    60,   154,    -1,
      -1,    39,    28,   125,    60,   154,    -1,    -1,    15,    28,
     126,    60,   154,    -1,    -1,    45,    28,   127,    60,   154,
      -1,    -1,    48,   128,    60,   157,    -1,    -1,    24,    48,
     129,    60,   157,    -1,    -1,    14,    48,   130,    60,   157,
      -1,    -1,    39,    48,   131,    60,   157,    -1,    -1,    15,
      48,   132,    60,   157,    -1,    -1,    45,    48,   133,    60,
     157,    -1,    -1,    41,   134,    60,   147,    -1,    -1,    24,
      41,   135,    60,   147,    -1,    -1,    14,    41,   136,    60,
     147,    -1,    -1,    39,    41,   137,    60,   147,    -1,    -1,
      15,    41,   138,    60,   147,    -1,    -1,    45,    41,   139,
      60,   147,    -1,    42,    60,   160,    -1,    54,    60,   231,
      -1,    56,    60,   164,    -1,    24,    56,    60,   164,    -1,
      14,    56,    60,   164,    -1,    39,    56,    60,   164,    -1,
      15,    56,    60,   164,    -1,    45,    56,    60,   164,    -1,
      51,    60,   300,    -1,    51,    60,    -1,    37,    60,   240,
      -1,    38,    60,   240,    -1,    32,    -1,   142,    -1,    61,
     306,    62,    -1,    61,   306,   141,   304,    62,    -1,   142,
      -1,   141,   305,   142,    -1,    90,   293,   144,    -1,    -1,
       7,   143,   144,    -1,    -1,    58,   306,    59,    -1,    58,
     306,   145,   302,    59,    -1,   146,    -1,   145,   303,   146,
      -1,    93,    -1,    32,    -1,   149,    -1,    61,   306,    62,
      -1,    61,   306,   148,   304,    62,    -1,   149,    -1,   148,
     305,   149,    -1,    90,   293,   151,    -1,    -1,     7,   150,
     151,    -1,    -1,    58,   306,    59,    -1,    58,   306,   152,
     302,    59,    -1,   153,    -1,   152,   303,   153,    -1,    93,
      -1,    21,    60,   231,    -1,    32,    -1,   156,    -1,    61,
     306,    62,    -1,    61,   306,   155,   304,    62,    -1,   156,
      -1,   155,   305,   156,    -1,   294,    -1,    32,    -1,   159,
      -1,    61,   306,    62,    -1,    61,   306,   158,   304,    62,
      -1,   159,    -1,   158,   305,   159,    -1,   294,    -1,    64,
     306,   161,    65,    -1,    -1,   162,   304,    -1,   163,    -1,
     162,   305,   163,    -1,     7,    66,     7,    -1,   166,    -1,
      61,   306,   165,   304,    62,    -1,   166,    -1,   165,   305,
     166,    -1,    12,    -1,   306,    -1,   306,   168,    -1,   169,
      -1,   168,   169,    -1,   177,   303,    -1,   175,   303,    -1,
     176,   303,    -1,    95,   307,    -1,   170,   307,    -1,    -1,
      55,    12,   171,    60,   306,    64,   306,   172,    65,    -1,
     173,    -1,   172,   173,    -1,    -1,    12,   174,   105,    64,
     167,    65,   306,    -1,    45,    31,    60,   164,    -1,    45,
      40,    60,   164,    -1,   199,    -1,   263,    -1,    53,    -1,
      18,    -1,   178,    -1,   300,    -1,   300,    61,    62,    -1,
     180,    -1,   179,   180,    -1,    -1,    -1,   181,   299,   183,
     229,   184,   219,    -1,    -1,    -1,    20,   181,   299,   186,
     229,   187,   219,    -1,    -1,   181,   299,    63,    19,    60,
     189,   209,    -1,    -1,    14,   181,   299,    63,    19,    60,
     190,   209,    -1,    -1,    39,   181,   299,    63,    19,    60,
     191,   209,    -1,    -1,    15,   181,   299,    63,    19,    60,
     192,   209,    -1,    -1,    24,   181,   299,    63,    19,    60,
     193,   209,    -1,    -1,    45,   181,   299,    63,    19,    60,
     194,   209,    -1,    -1,   181,   299,    63,    30,    61,   295,
      62,    60,   196,   200,    -1,    -1,   181,   299,    63,    52,
      60,   198,   213,    -1,   185,    -1,   182,    -1,   188,    -1,
     195,    -1,   197,    -1,    -1,   298,   201,   206,   202,    -1,
      -1,    64,   306,    65,    -1,    64,   306,   203,   302,    65,
      -1,   204,    -1,   203,   303,   204,    -1,    -1,   180,   298,
     205,    60,   247,    -1,    -1,    58,   306,    59,    -1,    58,
     306,   207,   302,    59,    -1,   208,    -1,   207,   303,   208,
      -1,    50,    60,   231,    -1,    32,    -1,   211,    -1,    61,
     306,    62,    -1,    61,   306,   210,   304,    62,    -1,   211,
      -1,   210,   305,   211,    -1,   296,    -1,    -1,   295,   212,
      67,   297,    -1,    -1,    64,   214,   306,   215,    65,    -1,
      -1,   216,   304,    -1,   217,    -1,   216,   305,   217,    -1,
      -1,   301,    66,   218,   247,    -1,   301,    66,    32,    -1,
      -1,    58,   306,    59,    -1,    58,   306,   220,   302,    59,
      -1,   222,    -1,   220,   303,   222,    -1,   300,    -1,    21,
      -1,    50,    -1,    12,    -1,    -1,   221,   223,    60,   246,
      -1,    -1,    24,   300,   224,    60,   245,    -1,    -1,    14,
     300,   225,    60,   245,    -1,    -1,    39,   300,   226,    60,
     245,    -1,    -1,    15,   300,   227,    60,   245,    -1,    -1,
      45,   300,   228,    60,   245,    -1,    27,    60,    12,    -1,
      35,    60,   300,    -1,    26,    60,   300,    -1,    51,    60,
     300,    -1,    51,    60,    -1,    -1,    60,   230,    -1,   247,
      -1,    32,    -1,    -1,    64,   232,   306,   233,    65,    -1,
      -1,   234,   302,    -1,   235,    -1,   234,   303,   235,    -1,
     237,   236,    60,   247,    -1,    25,   236,    60,   231,    -1,
      12,    -1,   298,    -1,   238,    -1,   239,    -1,   300,    -1,
     300,    61,    62,    -1,    -1,    64,   241,   306,   242,    65,
      -1,    -1,   243,   304,    -1,   244,    -1,   243,   305,   244,
      -1,    12,    66,    12,    -1,    32,    -1,   249,    -1,   231,
      -1,   247,    -1,    32,    -1,   248,    -1,   254,    -1,   249,
      -1,    61,    62,    -1,     7,    -1,    11,    -1,    12,    -1,
     300,    -1,     6,    -1,    -1,    61,   250,   251,    62,    -1,
     306,   252,   304,    -1,   253,    -1,   252,   305,   253,    -1,
     248,    -1,   249,    -1,   254,    -1,    -1,    58,   255,   256,
      59,    -1,   306,   257,   304,    -1,   258,    -1,   257,   305,
     258,    -1,   248,    -1,   254,    -1,    43,    -1,    20,    43,
      -1,    20,    57,    43,    -1,    57,    43,    -1,    -1,   259,
     299,    63,    52,    60,   261,   213,    -1,   259,   299,    63,
      23,    60,     7,    -1,    -1,   259,   299,   264,   281,   271,
      -1,    -1,    24,   259,   299,   265,   281,    -1,    -1,    14,
     259,   299,   266,   281,    -1,    -1,    39,   259,   299,   267,
     281,    -1,    -1,    15,   259,   299,   268,   281,    -1,    -1,
      45,   259,   299,   269,   281,    -1,    -1,   259,   299,    61,
       7,    62,   270,   287,    -1,   260,    -1,   262,    -1,    -1,
      58,   306,    59,    -1,    58,   306,   272,   302,    59,    -1,
     274,    -1,   272,   303,   274,    -1,   300,    -1,    21,    -1,
      50,    -1,    12,    -1,    -1,   273,   275,    60,   246,    -1,
      -1,    24,   300,   276,    60,   245,    -1,    -1,    14,   300,
     277,    60,   245,    -1,    -1,    39,   300,   278,    60,   245,
      -1,    -1,    15,   300,   279,    60,   245,    -1,    -1,    45,
     300,   280,    60,   245,    -1,    27,    60,    12,    -1,    35,
      60,   300,    -1,    51,    60,   300,    -1,    51,    60,    -1,
      -1,    60,   282,    -1,   284,    -1,    32,    -1,    61,   306,
      62,    -1,    61,   306,   283,   304,    62,    -1,   284,    -1,
     283,   305,   284,    -1,   285,   286,    -1,     7,    -1,     7,
      67,   297,    -1,    -1,   287,    -1,    -1,    64,   288,   306,
     289,    65,    -1,    -1,   290,   302,    -1,   291,    -1,   290,
     303,   291,    -1,   199,    -1,   292,    -1,    45,    16,    60,
     164,    -1,    -1,   294,    -1,     7,    -1,     7,    -1,     7,
      -1,   294,    -1,   298,    -1,   300,    -1,    72,    -1,     8,
      -1,    10,    -1,    72,    -1,     8,    -1,     9,    -1,    11,
      -1,     8,    -1,    -1,   303,    -1,    68,   306,    -1,   307,
      -1,   306,    -1,   305,    -1,    69,   306,    -1,    -1,   307,
      -1,     3,    -1,   307,     3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1265,  1265,  1268,  1269,  1270,  1271,  1272,  1273,  1274,
    1275,  1276,  1277,  1278,  1279,  1280,  1281,  1282,  1283,  1284,
    1285,  1286,  1287,  1288,  1289,  1290,  1291,  1292,  1293,  1294,
    1295,  1296,  1297,  1298,  1299,  1300,  1301,  1302,  1303,  1304,
    1305,  1306,  1307,  1308,  1309,  1310,  1311,  1312,  1320,  1321,
    1332,  1332,  1344,  1345,  1357,  1358,  1362,  1363,  1367,  1371,
    1376,  1376,  1385,  1385,  1391,  1391,  1397,  1397,  1403,  1403,
    1409,  1409,  1417,  1424,  1428,  1429,  1443,  1444,  1448,  1456,
    1463,  1465,  1469,  1470,  1474,  1478,  1485,  1486,  1494,  1494,
    1498,  1498,  1502,  1502,  1506,  1506,  1510,  1510,  1514,  1514,
    1518,  1528,  1529,  1536,  1536,  1596,  1597,  1601,  1602,  1606,
    1607,  1611,  1612,  1613,  1617,  1622,  1622,  1631,  1631,  1637,
    1637,  1643,  1643,  1649,  1649,  1655,  1655,  1663,  1670,  1677,
    1684,  1684,  1691,  1691,  1698,  1698,  1705,  1705,  1712,  1712,
    1719,  1719,  1727,  1727,  1732,  1732,  1737,  1737,  1742,  1742,
    1747,  1747,  1752,  1752,  1758,  1758,  1763,  1763,  1768,  1768,
    1773,  1773,  1778,  1778,  1783,  1783,  1789,  1789,  1796,  1796,
    1803,  1803,  1810,  1810,  1817,  1817,  1824,  1824,  1833,  1841,
    1845,  1849,  1853,  1857,  1861,  1865,  1871,  1876,  1883,  1891,
    1900,  1901,  1902,  1903,  1907,  1908,  1912,  1924,  1924,  1947,
    1949,  1950,  1954,  1955,  1959,  1963,  1964,  1965,  1966,  1970,
    1971,  1975,  1988,  1988,  2012,  2014,  2015,  2019,  2020,  2024,
    2025,  2029,  2030,  2031,  2032,  2036,  2037,  2041,  2047,  2048,
    2049,  2050,  2054,  2055,  2059,  2065,  2068,  2070,  2074,  2075,
    2079,  2085,  2086,  2090,  2091,  2095,  2103,  2104,  2108,  2109,
    2113,  2114,  2115,  2116,  2117,  2121,  2121,  2155,  2156,  2160,
    2160,  2203,  2212,  2225,  2226,  2234,  2237,  2243,  2249,  2252,
    2258,  2262,  2268,  2275,  2268,  2286,  2294,  2286,  2305,  2305,
    2313,  2313,  2321,  2321,  2329,  2329,  2337,  2337,  2345,  2345,
    2356,  2356,  2380,  2380,  2392,  2393,  2394,  2395,  2396,  2405,
    2405,  2422,  2424,  2425,  2434,  2435,  2439,  2439,  2454,  2456,
    2457,  2461,  2462,  2466,  2475,  2476,  2477,  2478,  2482,  2483,
    2487,  2490,  2490,  2516,  2516,  2521,  2523,  2527,  2528,  2532,
    2532,  2539,  2551,  2553,  2554,  2558,  2559,  2563,  2564,  2565,
    2569,  2574,  2574,  2583,  2583,  2589,  2589,  2595,  2595,  2601,
    2601,  2607,  2607,  2615,  2622,  2629,  2637,  2642,  2649,  2651,
    2655,  2660,  2672,  2672,  2680,  2682,  2686,  2687,  2691,  2694,
    2702,  2703,  2707,  2708,  2712,  2718,  2728,  2728,  2736,  2738,
    2742,  2743,  2747,  2760,  2766,  2776,  2780,  2781,  2794,  2797,
    2800,  2803,  2814,  2820,  2823,  2826,  2831,  2844,  2844,  2853,
    2857,  2858,  2862,  2863,  2864,  2872,  2872,  2879,  2883,  2884,
    2888,  2889,  2897,  2901,  2905,  2909,  2916,  2916,  2928,  2943,
    2943,  2953,  2953,  2961,  2961,  2969,  2969,  2977,  2977,  2986,
    2986,  2994,  2994,  3008,  3009,  3012,  3014,  3015,  3019,  3020,
    3024,  3025,  3026,  3030,  3035,  3035,  3044,  3044,  3050,  3050,
    3056,  3056,  3062,  3062,  3068,  3068,  3076,  3083,  3091,  3096,
    3103,  3105,  3109,  3110,  3113,  3116,  3120,  3121,  3125,  3129,
    3132,  3156,  3158,  3162,  3162,  3188,  3190,  3194,  3195,  3200,
    3202,  3206,  3219,  3222,  3226,  3232,  3238,  3244,  3247,  3258,
    3259,  3265,  3266,  3267,  3272,  3273,  3278,  3279,  3282,  3284,
    3288,  3289,  3293,  3294,  3298,  3301,  3303,  3307,  3308
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TOK_NL", "TOK_MAGIC",
  "TOK_SYNTAX_ERROR", "TOK_ASSETREF", "TOK_PATHREF", "TOK_IDENTIFIER",
  "TOK_CXX_NAMESPACED_IDENTIFIER", "TOK_NAMESPACED_IDENTIFIER",
  "TOK_NUMBER", "TOK_STRING", "TOK_ABSTRACT", "TOK_ADD", "TOK_APPEND",
  "TOK_ATTRIBUTES", "TOK_CLASS", "TOK_CONFIG", "TOK_CONNECT", "TOK_CUSTOM",
  "TOK_CUSTOMDATA", "TOK_DEF", "TOK_DEFAULT", "TOK_DELETE",
  "TOK_DICTIONARY", "TOK_DISPLAYUNIT", "TOK_DOC", "TOK_INHERITS",
  "TOK_KIND", "TOK_MAPPER", "TOK_NAMECHILDREN", "TOK_NONE", "TOK_OFFSET",
  "TOK_OVER", "TOK_PERMISSION", "TOK_PAYLOAD", "TOK_PREFIX_SUBSTITUTIONS",
  "TOK_SUFFIX_SUBSTITUTIONS", "TOK_PREPEND", "TOK_PROPERTIES",
  "TOK_REFERENCES", "TOK_RELOCATES", "TOK_REL", "TOK_RENAMES",
  "TOK_REORDER", "TOK_ROOTPRIMS", "TOK_SCALE", "TOK_SPECIALIZES",
  "TOK_SUBLAYERS", "TOK_SYMMETRYARGUMENTS", "TOK_SYMMETRYFUNCTION",
  "TOK_TIME_SAMPLES", "TOK_UNIFORM", "TOK_VARIANTS", "TOK_VARIANTSET",
  "TOK_VARIANTSETS", "TOK_VARYING", "'('", "')'", "'='", "'['", "']'",
  "'.'", "'{'", "'}'", "':'", "'@'", "';'", "','", "$accept", "menva_file",
  "keyword", "layer_metadata_form", "layer", "$@1", "layer_metadata_opt",
  "layer_metadata_list_opt", "layer_metadata_list", "layer_metadata_key",
  "layer_metadata", "$@2", "$@3", "$@4", "$@5", "$@6", "$@7",
  "sublayer_list", "sublayer_list_int", "sublayer_stmt", "layer_ref",
  "layer_offset_opt", "layer_offset_int", "layer_offset_stmt", "prim_list",
  "prim_stmt", "$@8", "$@9", "$@10", "$@11", "$@12", "$@13",
  "prim_type_name", "prim_stmt_int", "$@14", "prim_metadata_opt",
  "prim_metadata_list_opt", "prim_metadata_list", "prim_metadata_key",
  "prim_metadata", "$@15", "$@16", "$@17", "$@18", "$@19", "$@20", "$@21",
  "$@22", "$@23", "$@24", "$@25", "$@26", "$@27", "$@28", "$@29", "$@30",
  "$@31", "$@32", "$@33", "$@34", "$@35", "$@36", "$@37", "$@38", "$@39",
  "$@40", "$@41", "$@42", "$@43", "$@44", "payload_list",
  "payload_list_int", "payload_list_item", "$@45", "payload_params_opt",
  "payload_params_int", "payload_params_item", "reference_list",
  "reference_list_int", "reference_list_item", "$@46",
  "reference_params_opt", "reference_params_int", "reference_params_item",
  "inherit_list", "inherit_list_int", "inherit_list_item",
  "specializes_list", "specializes_list_int", "specializes_list_item",
  "relocates_map", "relocates_stmt_list_opt", "relocates_stmt_list",
  "relocates_stmt", "name_list", "name_list_int", "name_list_item",
  "prim_contents_list_opt", "prim_contents_list",
  "prim_contents_list_item", "variantset_stmt", "$@47", "variant_list",
  "variant_stmt", "$@48", "prim_child_order_stmt",
  "prim_property_order_stmt", "prim_property", "prim_attr_variability",
  "prim_attr_qualifiers", "prim_attr_type", "prim_attribute_full_type",
  "prim_attribute_default", "$@49", "$@50", "prim_attribute_fallback",
  "$@51", "$@52", "prim_attribute_connect", "$@53", "$@54", "$@55", "$@56",
  "$@57", "$@58", "prim_attribute_mapper", "$@59",
  "prim_attribute_time_samples", "$@60", "prim_attribute",
  "attribute_mapper_rhs", "$@61", "attribute_mapper_params_opt",
  "attribute_mapper_params_list", "attribute_mapper_param", "$@62",
  "attribute_mapper_metadata_opt", "attribute_mapper_metadata_list",
  "attribute_mapper_metadata", "connect_rhs", "connect_list",
  "connect_item", "$@63", "time_samples_rhs", "$@64", "time_sample_list",
  "time_sample_list_int", "time_sample", "$@65",
  "attribute_metadata_list_opt", "attribute_metadata_list",
  "attribute_metadata_key", "attribute_metadata", "$@66", "$@67", "$@68",
  "$@69", "$@70", "$@71", "attribute_assignment_opt", "attribute_value",
  "typed_dictionary", "$@72", "typed_dictionary_list_opt",
  "typed_dictionary_list", "typed_dictionary_element", "dictionary_key",
  "dictionary_value_type", "dictionary_value_scalar_type",
  "dictionary_value_shaped_type", "string_dictionary", "$@73",
  "string_dictionary_list_opt", "string_dictionary_list",
  "string_dictionary_element", "metadata_listop_list", "metadata_value",
  "typed_value", "typed_value_atomic", "typed_value_list", "$@74",
  "typed_value_list_int", "typed_value_list_items",
  "typed_value_list_item", "typed_value_tuple", "$@75",
  "typed_value_tuple_int", "typed_value_tuple_items",
  "typed_value_tuple_item", "prim_relationship_type",
  "prim_relationship_time_samples", "$@76", "prim_relationship_default",
  "prim_relationship", "$@77", "$@78", "$@79", "$@80", "$@81", "$@82",
  "$@83", "relationship_metadata_list_opt", "relationship_metadata_list",
  "relationship_metadata_key", "relationship_metadata", "$@84", "$@85",
  "$@86", "$@87", "$@88", "$@89", "relationship_assignment_opt",
  "relationship_rhs", "relationship_target_list", "relationship_target",
  "relationship_target_and_opt_marker", "relational_attributes_opt",
  "relational_attributes", "$@90", "relational_attributes_list_opt",
  "relational_attributes_list", "relational_attributes_list_item",
  "relational_attributes_order_stmt", "prim_path_opt", "prim_path",
  "property_path", "prim_or_property_scene_path", "marker", "name",
  "namespaced_name", "identifier", "extended_number", "stmtsep_opt",
  "stmtsep", "listsep_opt", "listsep", "newlines_opt", "newlines", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,    40,    41,
      61,    91,    93,    46,   123,   125,    58,    64,    59,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,    70,    71,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    73,    73,
      75,    74,    76,    76,    77,    77,    78,    78,    79,    80,
      81,    80,    82,    80,    83,    80,    84,    80,    85,    80,
      86,    80,    80,    80,    87,    87,    88,    88,    89,    90,
      91,    91,    92,    92,    93,    93,    94,    94,    96,    95,
      97,    95,    98,    95,    99,    95,   100,    95,   101,    95,
      95,   102,   102,   104,   103,   105,   105,   106,   106,   107,
     107,   108,   108,   108,   109,   110,   109,   111,   109,   112,
     109,   113,   109,   114,   109,   115,   109,   109,   109,   109,
     116,   109,   117,   109,   118,   109,   119,   109,   120,   109,
     121,   109,   122,   109,   123,   109,   124,   109,   125,   109,
     126,   109,   127,   109,   128,   109,   129,   109,   130,   109,
     131,   109,   132,   109,   133,   109,   134,   109,   135,   109,
     136,   109,   137,   109,   138,   109,   139,   109,   109,   109,
     109,   109,   109,   109,   109,   109,   109,   109,   109,   109,
     140,   140,   140,   140,   141,   141,   142,   143,   142,   144,
     144,   144,   145,   145,   146,   147,   147,   147,   147,   148,
     148,   149,   150,   149,   151,   151,   151,   152,   152,   153,
     153,   154,   154,   154,   154,   155,   155,   156,   157,   157,
     157,   157,   158,   158,   159,   160,   161,   161,   162,   162,
     163,   164,   164,   165,   165,   166,   167,   167,   168,   168,
     169,   169,   169,   169,   169,   171,   170,   172,   172,   174,
     173,   175,   176,   177,   177,   178,   178,   179,   180,   180,
     181,   181,   183,   184,   182,   186,   187,   185,   189,   188,
     190,   188,   191,   188,   192,   188,   193,   188,   194,   188,
     196,   195,   198,   197,   199,   199,   199,   199,   199,   201,
     200,   202,   202,   202,   203,   203,   205,   204,   206,   206,
     206,   207,   207,   208,   209,   209,   209,   209,   210,   210,
     211,   212,   211,   214,   213,   215,   215,   216,   216,   218,
     217,   217,   219,   219,   219,   220,   220,   221,   221,   221,
     222,   223,   222,   224,   222,   225,   222,   226,   222,   227,
     222,   228,   222,   222,   222,   222,   222,   222,   229,   229,
     230,   230,   232,   231,   233,   233,   234,   234,   235,   235,
     236,   236,   237,   237,   238,   239,   241,   240,   242,   242,
     243,   243,   244,   245,   245,   246,   246,   246,   247,   247,
     247,   247,   247,   248,   248,   248,   248,   250,   249,   251,
     252,   252,   253,   253,   253,   255,   254,   256,   257,   257,
     258,   258,   259,   259,   259,   259,   261,   260,   262,   264,
     263,   265,   263,   266,   263,   267,   263,   268,   263,   269,
     263,   270,   263,   263,   263,   271,   271,   271,   272,   272,
     273,   273,   273,   274,   275,   274,   276,   274,   277,   274,
     278,   274,   279,   274,   280,   274,   274,   274,   274,   274,
     281,   281,   282,   282,   282,   282,   283,   283,   284,   285,
     285,   286,   286,   288,   287,   289,   289,   290,   290,   291,
     291,   292,   293,   293,   294,   295,   296,   297,   297,   298,
     298,   299,   299,   299,   300,   300,   301,   301,   302,   302,
     303,   303,   304,   304,   305,   306,   306,   307,   307
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       0,     3,     1,     5,     1,     3,     1,     3,     1,     1,
       0,     4,     0,     5,     0,     5,     0,     5,     0,     5,
       0,     5,     3,     3,     3,     5,     1,     3,     2,     1,
       0,     4,     1,     3,     3,     3,     1,     3,     0,     3,
       0,     4,     0,     3,     0,     4,     0,     3,     0,     4,
       4,     1,     3,     0,     6,     1,     5,     1,     3,     1,
       3,     1,     1,     1,     1,     0,     4,     0,     5,     0,
       5,     0,     5,     0,     5,     0,     5,     3,     3,     3,
       0,     4,     0,     5,     0,     5,     0,     5,     0,     5,
       0,     5,     0,     4,     0,     5,     0,     5,     0,     5,
       0,     5,     0,     5,     0,     4,     0,     5,     0,     5,
       0,     5,     0,     5,     0,     5,     0,     4,     0,     5,
       0,     5,     0,     5,     0,     5,     0,     5,     3,     3,
       3,     4,     4,     4,     4,     4,     3,     2,     3,     3,
       1,     1,     3,     5,     1,     3,     3,     0,     3,     0,
       3,     5,     1,     3,     1,     1,     1,     3,     5,     1,
       3,     3,     0,     3,     0,     3,     5,     1,     3,     1,
       3,     1,     1,     3,     5,     1,     3,     1,     1,     1,
       3,     5,     1,     3,     1,     4,     0,     2,     1,     3,
       3,     1,     5,     1,     3,     1,     1,     2,     1,     2,
       2,     2,     2,     2,     2,     0,     9,     1,     2,     0,
       7,     4,     4,     1,     1,     1,     1,     1,     1,     3,
       1,     2,     0,     0,     6,     0,     0,     7,     0,     7,
       0,     8,     0,     8,     0,     8,     0,     8,     0,     8,
       0,    10,     0,     7,     1,     1,     1,     1,     1,     0,
       4,     0,     3,     5,     1,     3,     0,     5,     0,     3,
       5,     1,     3,     3,     1,     1,     3,     5,     1,     3,
       1,     0,     4,     0,     5,     0,     2,     1,     3,     0,
       4,     3,     0,     3,     5,     1,     3,     1,     1,     1,
       1,     0,     4,     0,     5,     0,     5,     0,     5,     0,
       5,     0,     5,     3,     3,     3,     3,     2,     0,     2,
       1,     1,     0,     5,     0,     2,     1,     3,     4,     4,
       1,     1,     1,     1,     1,     3,     0,     5,     0,     2,
       1,     3,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     1,     1,     1,     1,     1,     0,     4,     3,
       1,     3,     1,     1,     1,     0,     4,     3,     1,     3,
       1,     1,     1,     2,     3,     2,     0,     7,     6,     0,
       5,     0,     5,     0,     5,     0,     5,     0,     5,     0,
       5,     0,     7,     1,     1,     0,     3,     5,     1,     3,
       1,     1,     1,     1,     0,     4,     0,     5,     0,     5,
       0,     5,     0,     5,     0,     5,     3,     3,     3,     2,
       0,     2,     1,     1,     3,     5,     1,     3,     2,     1,
       3,     0,     1,     0,     5,     0,     2,     1,     3,     1,
       1,     4,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     1,
       2,     1,     1,     1,     2,     0,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    50,     0,     2,   505,     1,   507,    51,    48,    52,
     506,    92,    88,    96,     0,   505,    86,   505,   508,   494,
     495,     0,    94,   101,     0,    90,     0,    98,     0,    49,
     506,     0,    54,   103,    93,     0,     0,    89,     0,    97,
       0,     0,    87,   505,    59,     0,     0,     0,     0,     0,
       0,     0,   498,    60,    56,    58,   505,   102,    95,    91,
      99,   245,   505,   100,   241,    53,    64,    68,    62,     0,
      66,    70,     0,   505,    55,   499,   501,     0,     0,   105,
       0,     0,     0,     0,    72,     0,     0,   505,    73,   500,
      57,     0,   505,   505,   505,   243,     0,     0,     0,     0,
       0,     0,   396,   392,   393,   394,   387,   405,   397,   362,
     385,    61,   386,   388,   390,   389,   395,     0,   246,     0,
     107,   505,     0,   503,   502,   383,   397,    65,   384,    69,
      63,    67,    71,    79,    74,   505,    76,    80,   505,   391,
     505,   505,   104,     0,     0,   266,     0,     0,     0,   412,
       0,   265,     0,     0,     0,   247,   248,     0,     0,     0,
       0,   267,     0,   270,     0,   295,   294,   296,   297,   298,
     263,     0,   433,   434,   264,   268,   505,   114,     0,     0,
     112,     0,     0,   142,     0,     0,   130,     0,     0,     0,
     166,     0,     0,   154,   113,     0,     0,     0,   498,   115,
     109,   111,   504,   242,   244,     0,   503,     0,    78,     0,
       0,     0,     0,   364,     0,     0,     0,     0,     0,   413,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     255,   415,   253,   249,   254,   251,   252,   250,   271,   491,
     492,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    26,    25,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,   493,   272,   419,     0,
     106,   146,   134,   170,   158,     0,   119,   150,   138,   174,
     162,     0,   123,   144,   132,   168,   156,     0,   117,     0,
       0,     0,     0,     0,     0,     0,   148,   136,   172,   160,
       0,   121,     0,     0,   152,   140,   176,   164,     0,   125,
       0,   187,     0,     0,   108,   499,     0,    75,    77,     0,
       0,   498,    82,   406,   410,   411,   505,   408,   398,   402,
     403,   505,   400,   404,     0,     0,   498,   366,     0,   372,
     373,   374,     0,   423,     0,   427,   414,   275,     0,   421,
       0,   425,     0,     0,     0,   429,     0,     0,   358,     0,
       0,   460,   269,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   127,     0,   128,   129,     0,   376,   188,   189,     0,
       0,     0,     0,     0,     0,     0,   505,   178,     0,     0,
       0,     0,     0,     0,     0,   186,   179,   180,   110,     0,
       0,     0,     0,   499,   407,   503,   399,   503,   370,   490,
       0,   371,   489,   363,   365,   499,     0,     0,     0,   460,
       0,   460,   358,     0,   460,     0,   460,   261,   262,     0,
     460,   505,     0,     0,     0,     0,   273,     0,     0,     0,
       0,   435,     0,     0,     0,     0,   182,     0,     0,     0,
       0,     0,   184,     0,     0,     0,     0,     0,   181,     0,
     484,   221,   505,   143,   222,   227,   197,   190,   505,   482,
     131,   191,   505,     0,     0,     0,     0,   183,     0,   212,
     205,   505,   482,   167,   206,   236,     0,     0,     0,     0,
     185,     0,   228,   505,   155,   229,   234,   116,    84,    85,
      81,    83,   409,   401,     0,   367,     0,   375,     0,   424,
       0,   428,   276,     0,   422,     0,   426,     0,   430,     0,
     278,     0,   292,   361,   359,   360,   332,   431,     0,   416,
     469,   463,   505,   461,   462,   471,   505,   420,   147,   135,
     171,   159,   120,   151,   139,   175,   163,   124,   145,   133,
     169,   157,   118,     0,   199,     0,   199,   483,   378,   149,
     137,   173,   161,   122,   214,     0,   214,     0,     0,   505,
     238,   153,   141,   177,   165,   126,     0,   369,   368,   280,
     284,   332,   286,   282,   288,   505,     0,   485,     0,     0,
     505,   274,     0,   418,     0,     0,     0,   473,   468,   472,
       0,   223,   505,   225,   505,   198,   192,   505,   194,   196,
       0,     0,   505,   380,   505,   213,   207,   505,   209,   211,
       0,   235,   237,   503,   230,   505,   232,     0,     0,   277,
       0,     0,     0,     0,   486,   314,   505,   279,   315,   321,
     320,     0,   323,   293,     0,   432,   417,   487,   470,   488,
     464,   505,   466,   505,   443,     0,     0,   441,     0,     0,
       0,     0,     0,   442,     0,   436,   498,   444,   438,   440,
       0,   503,     0,     0,   503,     0,   377,   379,   503,     0,
       0,   503,   240,   239,     0,   503,   281,   285,   287,   283,
     289,   259,     0,   257,     0,     0,   290,   505,   340,     0,
       0,   338,     0,     0,     0,     0,     0,     0,   339,     0,
     333,   498,   341,   335,   337,     0,   503,   475,   448,   452,
     446,     0,     0,   450,   454,   459,     0,   499,     0,   224,
     226,   200,   204,   498,   202,   193,   195,   382,   381,     0,
     215,   219,   498,   217,   208,   210,   231,   233,   505,   256,
     258,   316,   505,   318,     0,     0,   325,   345,   349,   343,
       0,     0,     0,   347,   351,   357,     0,   499,     0,   465,
     467,     0,     0,     0,     0,     0,     0,   479,     0,   498,
     477,   480,     0,     0,     0,   456,   457,     0,     0,   458,
     437,   439,     0,     0,   499,     0,     0,   499,     0,     0,
     503,   322,   291,   299,   497,   496,     0,   505,   327,     0,
       0,     0,     0,   355,   353,   354,     0,     0,   356,   334,
     336,     0,     0,   474,   476,   499,     0,     0,     0,     0,
       0,   445,   201,   203,   220,   216,   218,   505,   317,   319,
     308,   324,   326,   503,   329,     0,     0,     0,     0,     0,
     342,     0,   478,   449,   453,   447,   451,   455,     0,   505,
     301,   328,   331,     0,   346,   350,   344,   348,   352,   481,
     505,     0,   505,   300,   330,   260,     0,   309,   498,   311,
       0,     0,     0,   499,   302,     0,   498,   304,   313,   310,
     312,   306,     0,   499,     0,   303,   305,     0,   307
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,   286,     7,     3,     4,     8,    31,    52,    53,
      54,    77,    83,    81,    85,    82,    86,    88,   135,   136,
     499,   208,   341,   762,    15,   154,    24,    38,    21,    36,
      26,    40,    22,    34,    56,    78,   119,   198,   199,   200,
     336,   400,   388,   414,   394,   423,   313,   396,   384,   410,
     390,   419,   310,   395,   383,   409,   389,   418,   330,   398,
     386,   412,   392,   421,   322,   397,   385,   411,   391,   420,
     500,   637,   501,   584,   635,   763,   764,   513,   647,   514,
     594,   645,   772,   773,   493,   632,   494,   524,   655,   525,
     417,   598,   599,   600,    63,    94,    64,   117,   155,   156,
     157,   376,   722,   723,   778,   158,   159,   160,   161,   162,
     163,   164,   165,   378,   556,   166,   452,   611,   167,   616,
     657,   661,   658,   660,   662,   168,   785,   169,   619,   170,
     832,   870,   903,   916,   917,   924,   890,   908,   909,   667,
     782,   668,   725,   673,   727,   836,   837,   838,   893,   621,
     741,   742,   743,   798,   842,   840,   846,   841,   847,   466,
     554,   110,   141,   355,   356,   357,   440,   358,   359,   360,
     407,   502,   641,   642,   643,   127,   111,   112,   113,   128,
     140,   211,   351,   352,   115,   138,   209,   346,   347,   171,
     172,   624,   173,   174,   381,   454,   449,   456,   451,   460,
     622,   567,   696,   697,   698,   758,   814,   812,   817,   813,
     818,   471,   563,   681,   564,   565,   628,   629,   683,   808,
     809,   810,   811,   586,   495,   669,   670,   678,   441,   287,
     175,   839,    74,    75,   122,   123,   124,    10
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -691
static const yytype_int16 yypact[] =
{
      67,  -691,    86,  -691,   105,  -691,  -691,  -691,   288,    44,
     131,   177,   177,   177,   107,   105,  -691,   105,  -691,  -691,
    -691,   144,    95,  -691,   144,    95,   144,    95,   114,  -691,
     181,   148,   721,  -691,  -691,   177,   144,  -691,   144,  -691,
     144,    58,  -691,   105,  -691,   177,   177,   177,   160,   177,
     177,   167,    38,  -691,  -691,  -691,   105,  -691,  -691,  -691,
    -691,  -691,   105,  -691,  -691,  -691,  -691,  -691,  -691,   192,
    -691,  -691,   208,   105,  -691,   721,   131,   178,   211,   231,
     260,   237,   238,   239,  -691,   249,   251,   105,  -691,  -691,
    -691,   185,   105,   105,    56,  -691,    37,    37,    37,    37,
      37,    66,  -691,  -691,  -691,  -691,  -691,  -691,   250,  -691,
    -691,  -691,  -691,  -691,  -691,  -691,  -691,   252,   584,   276,
     806,   105,   264,   260,  -691,  -691,  -691,  -691,  -691,  -691,
    -691,  -691,  -691,  -691,  -691,    56,  -691,   278,   105,  -691,
     105,   105,  -691,   307,   307,  -691,   310,   307,   307,  -691,
     420,  -691,   325,   305,   105,   584,  -691,   105,    38,    38,
      38,  -691,   177,  -691,  1004,  -691,  -691,  -691,  -691,  -691,
    -691,  1004,  -691,  -691,  -691,   291,   105,  -691,   414,   459,
    -691,   480,   295,  -691,   298,   299,  -691,   301,   306,   535,
    -691,   311,   637,  -691,  -691,   316,   323,   326,    38,  -691,
    -691,  -691,  -691,  -691,  -691,   308,   379,    85,  -691,   328,
     210,   329,   271,   200,   112,  1004,  1004,  1004,  1004,  -691,
     347,  1004,  1004,  1004,  1004,  1004,   336,   337,  1004,  1004,
    -691,  -691,   131,  -691,   131,  -691,  -691,  -691,  -691,  -691,
    -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,
    -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,
    -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,
    -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,
    -691,  -691,  -691,  -691,  -691,  -691,  -691,   339,    33,   342,
    -691,  -691,  -691,  -691,  -691,   352,  -691,  -691,  -691,  -691,
    -691,   359,  -691,  -691,  -691,  -691,  -691,   367,  -691,   421,
     372,   423,   177,   389,   392,   392,  -691,  -691,  -691,  -691,
     397,  -691,   398,   400,  -691,  -691,  -691,  -691,   412,  -691,
     418,   177,   415,    58,  -691,   806,   422,  -691,  -691,   430,
     431,    38,  -691,  -691,  -691,  -691,    56,  -691,  -691,  -691,
    -691,    56,  -691,  -691,   954,   428,    38,  -691,   954,  -691,
    -691,   433,   436,  -691,   438,  -691,  -691,  -691,   443,  -691,
     447,  -691,    58,    58,   454,  -691,   462,   143,   469,   489,
      59,   474,  -691,   475,   478,   479,   481,    58,   486,   491,
     494,   496,   501,    58,   504,   505,   507,   508,   509,    58,
     510,  -691,    98,  -691,  -691,    68,  -691,  -691,  -691,   512,
     513,   514,   515,    58,   518,    72,   105,  -691,   520,   521,
     522,   524,    58,   527,   103,  -691,  -691,  -691,  -691,   185,
     529,   538,   536,    85,  -691,   210,  -691,   271,  -691,  -691,
     534,  -691,  -691,  -691,  -691,   200,   537,   541,   577,   474,
     581,   474,   469,   586,   474,   588,   474,  -691,  -691,   590,
     474,   105,   550,   552,   551,   223,  -691,   554,   557,   562,
     139,   566,    98,    68,    72,   103,  -691,    37,    98,    68,
      72,   103,  -691,    37,    98,    68,    72,   103,  -691,    37,
    -691,  -691,   105,  -691,  -691,  -691,  -691,  -691,   105,   619,
    -691,  -691,   105,    98,    68,    72,   103,  -691,    37,  -691,
    -691,   105,   619,  -691,  -691,   621,    98,    68,    72,   103,
    -691,    37,  -691,   105,  -691,  -691,  -691,  -691,  -691,  -691,
    -691,  -691,  -691,  -691,   415,  -691,   233,  -691,   570,  -691,
     572,  -691,  -691,   573,  -691,   575,  -691,   578,  -691,   576,
    -691,   635,  -691,  -691,  -691,  -691,   585,  -691,   640,  -691,
     582,  -691,   105,  -691,  -691,   587,   105,  -691,  -691,  -691,
    -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,
    -691,  -691,  -691,    51,   592,    54,   592,  -691,   636,  -691,
    -691,  -691,  -691,  -691,   594,    61,   594,   589,   591,    56,
    -691,  -691,  -691,  -691,  -691,  -691,    74,  -691,  -691,  -691,
    -691,   585,  -691,  -691,  -691,   105,   145,  -691,   595,   596,
     105,  -691,   587,  -691,   596,   904,    81,  -691,  -691,  -691,
     386,  -691,    56,  -691,   105,  -691,  -691,    56,  -691,  -691,
     598,   597,    56,  -691,   105,  -691,  -691,    56,  -691,  -691,
     646,  -691,  -691,   621,  -691,    56,  -691,   145,   145,  -691,
     145,   145,   145,   642,   600,  -691,   105,  -691,  -691,  -691,
    -691,   601,  -691,  -691,   330,  -691,  -691,  -691,  -691,  -691,
    -691,    56,  -691,   105,  -691,   177,   177,  -691,   177,   608,
     609,   177,   177,  -691,   610,  -691,    38,  -691,  -691,  -691,
     614,   619,   214,   617,   204,   659,  -691,  -691,   636,   215,
     620,   247,  -691,  -691,   622,   619,  -691,  -691,  -691,  -691,
    -691,  -691,    89,  -691,    83,   605,  -691,   105,  -691,   177,
     177,  -691,   177,   628,   629,   630,   177,   177,  -691,   631,
    -691,    38,  -691,  -691,  -691,   632,   685,   364,  -691,  -691,
    -691,   683,   177,  -691,  -691,   177,   638,  1104,   639,  -691,
    -691,  -691,  -691,    38,  -691,  -691,  -691,  -691,  -691,   641,
    -691,  -691,    38,  -691,  -691,  -691,  -691,  -691,   105,  -691,
    -691,  -691,    56,  -691,   904,  1054,   104,  -691,  -691,  -691,
     177,   684,   177,  -691,  -691,   177,   643,   864,   649,  -691,
    -691,   277,   277,   277,   277,   277,   171,  -691,   633,    38,
    -691,  -691,   650,   651,   652,  -691,  -691,   653,   655,  -691,
    -691,  -691,   185,   657,    85,   415,   658,   267,   656,   660,
     693,  -691,  -691,  -691,  -691,  -691,   663,    56,  -691,   671,
     678,   679,   680,  -691,  -691,  -691,   687,   689,  -691,  -691,
    -691,   185,   690,  -691,  -691,   364,    37,    37,    37,    37,
      37,  -691,  -691,  -691,  -691,  -691,  -691,   105,  -691,  -691,
     665,  -691,  -691,   104,   711,    37,    37,    37,    37,    37,
    -691,    58,  -691,  -691,  -691,  -691,  -691,  -691,   686,   105,
     688,  -691,  -691,   233,  -691,  -691,  -691,  -691,  -691,  -691,
     105,   116,   105,  -691,  -691,  -691,   695,  -691,    38,  -691,
      57,   415,   697,   708,  -691,  1054,    38,  -691,  -691,  -691,
    -691,  -691,   694,   177,   701,  -691,  -691,   233,  -691
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -691,  -691,  -338,  -691,  -691,  -691,  -691,  -691,  -691,  -691,
     696,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,   558,
     -75,  -691,  -691,  -196,  -691,    55,  -691,  -691,  -691,  -691,
    -691,  -691,   280,   445,  -691,    -9,  -691,  -691,  -691,   437,
    -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,
    -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,
    -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,
    -183,  -691,  -567,  -691,   182,  -691,   -49,  -112,  -691,  -551,
    -691,   180,  -691,   -50,   -19,  -691,  -550,    -1,  -691,  -552,
    -691,  -691,  -691,   127,  -296,  -691,    19,   -85,  -691,   645,
    -691,  -691,  -691,    63,  -691,  -691,  -691,  -691,  -691,  -691,
    -156,   -98,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,
    -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -690,
    -691,  -691,  -691,  -691,  -140,  -691,  -691,  -691,  -121,  -354,
    -691,  -669,  -691,   169,  -691,  -691,  -691,   -79,  -691,   184,
    -691,  -691,     1,  -691,  -691,  -691,  -691,  -691,  -691,   344,
    -691,  -322,  -691,  -691,  -691,   356,   439,  -691,  -691,  -691,
     487,  -691,  -691,  -691,    96,   -69,  -407,  -452,  -185,   -88,
    -691,  -691,  -691,   366,  -159,  -691,  -691,  -691,   377,   383,
    -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,  -691,
    -691,  -691,  -691,  -691,    60,  -691,  -691,  -691,  -691,  -691,
    -691,    99,  -691,  -691,  -607,  -691,  -691,   191,  -691,  -691,
    -691,   -39,  -691,   312,  -392,   268,  -691,    39,  -602,    42,
     -11,  -691,  -184,  -151,  -123,   -95,     0,   -10
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -486
static const yytype_int16 yytable[] =
{
      23,    23,    23,   114,     9,    30,   238,   235,   236,   237,
     426,   342,   205,   555,   334,    29,   439,    32,   638,   682,
     439,    55,   527,   679,    57,   344,   137,   349,   129,   130,
     131,   132,   526,   633,    66,    67,    68,   427,    70,    71,
     206,     6,    76,    65,   648,   215,   217,   335,   221,   222,
     224,   345,   228,   353,   656,   783,    79,   807,   490,     6,
     133,   496,    80,    16,    55,    19,    20,   133,   509,   125,
      61,     1,   133,    89,   133,   496,   457,   458,   133,   509,
     116,   490,   468,   526,   608,    42,     5,   101,   560,   526,
     664,   476,   118,   120,   379,   526,   380,   482,   126,    95,
     497,   721,    17,   488,   510,   490,    73,   587,     6,   201,
     490,   469,   834,   631,   526,   835,   636,   507,   339,    62,
     587,   202,   914,   646,   350,   121,   520,   526,   134,   498,
     491,   137,   340,   511,    18,   522,   654,   766,   210,   800,
     212,   213,   204,   680,   232,   781,   560,   234,    76,    76,
      76,   760,   664,    28,   779,   219,    33,   432,    35,   492,
     775,   869,   462,   777,   523,   807,   906,   296,   302,   220,
     308,   561,   444,   463,    41,   907,   290,   665,   321,    19,
      20,   329,   679,   833,    18,    19,    20,   852,    76,   145,
     433,   102,   103,    19,    20,   464,   104,   105,    11,   116,
     562,   116,   361,    12,    84,   445,   666,    43,    19,    20,
     133,   496,   607,   288,   526,    13,   102,   106,    19,    20,
      69,   104,   105,   434,   151,   354,    14,    72,   436,   102,
     103,    19,    20,   677,   104,   105,   769,   531,    91,   102,
     103,    19,    20,   107,   104,   105,   108,   339,   339,   109,
     344,   435,   349,   133,   509,   553,   437,   362,   363,   364,
     365,   340,   340,   367,   368,   369,   370,   371,   107,    87,
     374,   375,    61,   761,   770,    92,   345,   102,   353,    19,
      20,   107,   104,   105,   108,    19,    20,   439,   769,    93,
     569,   107,    25,    27,   108,   145,   574,    96,    97,    98,
     339,   404,   579,   716,   717,    11,   718,   719,   720,    99,
      12,   100,   139,   921,   340,    19,    20,   142,    19,    20,
     425,   590,    13,   526,   201,   145,   203,   214,   145,   107,
     151,    76,   126,    14,   602,   176,   207,   230,    19,    20,
     512,   114,   728,   442,   729,   730,    76,   442,   231,   350,
     149,   731,   289,   219,   732,   309,   733,   734,   311,   312,
     151,   314,   570,   151,   153,   735,   315,   220,   575,   736,
     337,   323,    19,    20,   580,   737,   331,   114,   801,   802,
     738,   739,   145,   332,   803,   133,   333,   343,   804,   740,
     366,   348,   677,   591,    19,    20,   372,   373,   684,   512,
     685,   686,   377,   805,   382,   512,   603,   687,   572,   806,
     688,   512,   387,   689,   577,   861,   515,   151,   116,   393,
     582,   690,    19,    20,   116,   691,   116,   399,    19,    20,
     512,   692,   402,   401,   361,   403,   693,   694,   145,   593,
     214,   904,   291,   512,   880,   695,   439,   439,   114,   405,
     292,   226,   605,   568,   116,   293,   406,   413,   415,   573,
     227,   549,   294,   149,   416,   578,    28,    19,    20,    37,
     295,    39,   422,   151,   571,   928,   652,   153,   424,   109,
     576,    58,   429,    59,   589,    60,   581,   297,    19,    20,
     430,   431,   583,   443,   447,   298,   467,   601,   585,   448,
     299,   450,   588,   864,   653,   592,   453,   300,   303,   700,
     455,   595,   756,   771,   703,   301,   304,   459,   604,   707,
     512,   305,   461,   606,   710,   116,   216,   218,   306,   465,
     223,   225,   714,   229,   470,   472,   307,   701,   473,   474,
     528,   475,   704,    19,    20,   757,   477,   708,   539,   529,
     541,   478,   711,   544,   479,   546,   480,   796,   745,   548,
     715,   481,   626,   316,   483,   484,   630,   485,   486,   487,
     489,   317,   503,   504,   505,   506,   318,   439,   508,   823,
     516,   517,   518,   319,   519,   899,   746,   521,   826,   918,
     797,   320,    19,    20,   534,   530,   538,   536,   143,   144,
     540,    11,   145,   537,   146,   543,    12,   545,   147,   547,
     550,   552,   824,   551,   442,   663,   557,   558,    13,   699,
     674,   827,   559,   148,   566,   854,   490,   149,   597,   150,
     609,   771,   610,   612,   702,   613,   512,   151,   614,   152,
     615,   153,   617,   620,   709,    19,    20,   623,   640,   625,
     634,   627,   644,   712,   721,   650,   651,   671,   855,   829,
     672,   726,   706,   744,   705,   324,   724,  -485,   751,   752,
     755,   767,   784,   325,   748,   749,   759,   750,   326,   765,
     753,   754,   774,   747,   776,   327,    76,   830,   790,   791,
     792,   795,   560,   328,   799,   815,   844,   820,   853,   822,
     664,   825,   849,   215,   217,   221,   222,   224,   228,   851,
     856,   857,   858,   859,   872,   860,   862,   865,   787,   788,
     867,   789,   868,   889,   912,   793,   794,   786,   871,    19,
      20,    76,   922,    44,   114,    45,    46,   874,   875,   876,
     877,   816,   873,   892,   819,    47,   699,   878,    48,   879,
     881,   900,   902,    76,   915,   911,   919,   913,   906,   925,
      49,   927,    76,   114,   338,   923,    50,   915,   639,   828,
      51,    90,   428,   442,   442,   863,   649,   866,    79,   843,
     713,   845,   888,   926,   848,   780,   744,   883,   884,   885,
     886,   887,   920,   676,   891,   659,   542,   446,   850,    76,
     233,   535,   408,   533,   768,   114,   894,   895,   896,   897,
     898,   116,   532,   675,    19,    20,   882,   821,   177,   618,
     178,   179,     0,   831,   596,     0,     0,   180,     0,     0,
     181,     0,     0,   182,   183,   184,     0,     0,     0,   114,
     116,   185,   186,   187,   188,   189,     0,   190,   191,     0,
       0,   192,     0,     0,   193,     0,   194,   195,     0,     0,
     196,     0,   197,     0,     0,     0,     0,   118,     0,     0,
       0,     0,    19,    20,     0,     0,   728,     0,   729,   730,
       0,     0,   116,     0,     0,   731,     0,     0,   732,   901,
     733,   734,     0,     0,     0,     0,     0,     0,    76,   735,
     905,     0,   910,   736,   442,     0,    76,     0,     0,   737,
       0,   490,    19,    20,   738,   739,   116,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   250,   251,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,    19,    20,     0,     0,   438,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   250,   251,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,   239,     0,   240,     0,     0,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   250,   251,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,    19,    20,     0,     0,     0,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   250,   251,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,   285,    19,    20,     0,     0,   684,     0,   685,   686,
       0,     0,     0,     0,     0,   687,     0,     0,   688,     0,
       0,   689,     0,     0,     0,     0,     0,     0,     0,   690,
       0,     0,     0,   691,     0,     0,     0,     0,     0,   692,
       0,     0,     0,     0,   693,   694
};

static const yytype_int16 yycheck[] =
{
      11,    12,    13,    91,     4,    15,   162,   158,   159,   160,
     332,   207,   135,   465,   198,    15,   354,    17,   585,   626,
     358,    32,   429,   625,    35,   210,   101,   212,    97,    98,
      99,   100,   424,   583,    45,    46,    47,   333,    49,    50,
     135,     3,    52,    43,   595,   143,   144,   198,   146,   147,
     148,   210,   150,   212,   606,   724,    56,   747,     7,     3,
       6,     7,    62,     8,    75,     8,     9,     6,     7,    32,
      12,     4,     6,    73,     6,     7,   372,   373,     6,     7,
      91,     7,    23,   475,   536,    30,     0,    87,     7,   481,
       7,   387,    92,    93,    61,   487,    63,   393,    61,    80,
      32,    12,    58,   399,    32,     7,    68,   499,     3,   120,
       7,    52,     8,    62,   506,    11,    62,   413,    33,    61,
     512,   121,    65,    62,   212,    69,   422,   519,    62,    61,
      32,   206,    47,    61,     3,    32,    62,   704,   138,   746,
     140,   141,   123,    62,   154,    62,     7,   157,   158,   159,
     160,   701,     7,    46,    65,    43,    12,   341,    63,    61,
     711,   830,    19,   715,    61,   855,    50,   178,   179,    57,
     181,    32,   356,    30,    60,    59,   176,    32,   189,     8,
       9,   192,   784,   785,     3,     8,     9,    16,   198,    18,
     341,     6,     7,     8,     9,    52,    11,    12,    17,   210,
      61,   212,   213,    22,    12,   356,    61,    59,     8,     9,
       6,     7,   534,   171,   606,    34,     6,    32,     8,     9,
      60,    11,    12,   346,    53,    25,    45,    60,   351,     6,
       7,     8,     9,   625,    11,    12,    21,   433,    60,     6,
       7,     8,     9,    58,    11,    12,    61,    33,    33,    64,
     435,   346,   437,     6,     7,    32,   351,   215,   216,   217,
     218,    47,    47,   221,   222,   223,   224,   225,    58,    61,
     228,   229,    12,    59,    59,    64,   435,     6,   437,     8,
       9,    58,    11,    12,    61,     8,     9,   625,    21,    58,
     473,    58,    12,    13,    61,    18,   479,    60,    60,    60,
      33,   312,   485,   657,   658,    17,   660,   661,   662,    60,
      22,    60,    62,   915,    47,     8,     9,    65,     8,     9,
     331,   504,    34,   715,   335,    18,    62,    20,    18,    58,
      53,   341,    61,    45,   517,    59,    58,    12,     8,     9,
     415,   429,    12,   354,    14,    15,   356,   358,    43,   437,
      43,    21,    61,    43,    24,    60,    26,    27,    60,    60,
      53,    60,   474,    53,    57,    35,    60,    57,   480,    39,
      62,    60,     8,     9,   486,    45,    60,   465,    14,    15,
      50,    51,    18,    60,    20,     6,    60,    59,    24,    59,
      43,    62,   784,   505,     8,     9,    60,    60,    12,   474,
      14,    15,    63,    39,    62,   480,   518,    21,   477,    45,
      24,   486,    60,    27,   483,   822,   416,    53,   429,    60,
     489,    35,     8,     9,   435,    39,   437,    60,     8,     9,
     505,    45,    60,    12,   445,    12,    50,    51,    18,   508,
      20,   893,    28,   518,   851,    59,   784,   785,   536,    60,
      36,    31,   521,   472,   465,    41,    64,    60,    60,   478,
      40,   461,    48,    43,    64,   484,    46,     8,     9,    24,
      56,    26,    60,    53,   475,   927,   599,    57,    60,    64,
     481,    36,    60,    38,   503,    40,   487,    28,     8,     9,
      60,    60,   492,    65,    61,    36,     7,   516,   498,    63,
      41,    63,   502,   825,   599,   506,    63,    48,    28,   632,
      63,   511,   696,   709,   637,    56,    36,    63,   519,   642,
     595,    41,    60,   523,   647,   536,   143,   144,    48,    60,
     147,   148,   655,   150,    60,    60,    56,   632,    60,    60,
      11,    60,   637,     8,     9,   696,    60,   642,   449,    11,
     451,    60,   647,   454,    60,   456,    60,   741,   681,   460,
     655,    60,   562,    28,    60,    60,   566,    60,    60,    60,
      60,    36,    60,    60,    60,    60,    41,   915,    60,   763,
      60,    60,    60,    48,    60,   881,   681,    60,   772,   911,
     741,    56,     8,     9,    60,    59,    19,    60,    14,    15,
      19,    17,    18,    62,    20,    19,    22,    19,    24,    19,
      60,    60,   763,    61,   625,   615,    62,    60,    34,   630,
     620,   772,    60,    39,    58,   809,     7,    43,     7,    45,
      60,   827,    60,    60,   634,    60,   711,    53,    60,    55,
      64,    57,     7,    58,   644,     8,     9,     7,    12,    67,
      58,    64,    58,     7,    12,    66,    65,    62,   809,   782,
      64,    60,    65,   674,    66,    28,   666,    67,    60,    60,
      60,    12,    67,    36,   685,   686,    62,   688,    41,    62,
     691,   692,    62,   683,    62,    48,   696,   782,    60,    60,
      60,    60,     7,    56,    62,    12,    12,    59,    65,    60,
       7,    60,    59,   801,   802,   803,   804,   805,   806,    60,
      60,    60,    60,    60,   837,    60,    59,    59,   729,   730,
      64,   732,    62,    58,   908,   736,   737,   727,    65,     8,
       9,   741,   916,    12,   822,    14,    15,    66,    60,    60,
      60,   752,   837,    32,   755,    24,   757,    60,    27,    60,
      60,    65,    64,   763,   910,    60,    59,   908,    50,    65,
      39,    60,   772,   851,   206,   916,    45,   923,   586,   778,
      49,    75,   335,   784,   785,   824,   596,   827,   778,   790,
     653,   792,   867,   923,   795,   722,   797,   856,   857,   858,
     859,   860,   913,   624,   873,   611,   452,   358,   797,   809,
     155,   445,   315,   437,   708,   893,   875,   876,   877,   878,
     879,   822,   435,   622,     8,     9,   855,   757,    12,   551,
      14,    15,    -1,   784,   512,    -1,    -1,    21,    -1,    -1,
      24,    -1,    -1,    27,    28,    29,    -1,    -1,    -1,   927,
     851,    35,    36,    37,    38,    39,    -1,    41,    42,    -1,
      -1,    45,    -1,    -1,    48,    -1,    50,    51,    -1,    -1,
      54,    -1,    56,    -1,    -1,    -1,    -1,   867,    -1,    -1,
      -1,    -1,     8,     9,    -1,    -1,    12,    -1,    14,    15,
      -1,    -1,   893,    -1,    -1,    21,    -1,    -1,    24,   889,
      26,    27,    -1,    -1,    -1,    -1,    -1,    -1,   908,    35,
     900,    -1,   902,    39,   915,    -1,   916,    -1,    -1,    45,
      -1,     7,     8,     9,    50,    51,   927,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,     8,     9,    -1,    -1,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,     8,    -1,    10,    -1,    -1,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,     8,     9,    -1,    -1,    -1,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,     8,     9,    -1,    -1,    12,    -1,    14,    15,
      -1,    -1,    -1,    -1,    -1,    21,    -1,    -1,    24,    -1,
      -1,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    35,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    -1,    -1,    50,    51
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     4,    71,    74,    75,     0,     3,    73,    76,   306,
     307,    17,    22,    34,    45,    94,    95,    58,     3,     8,
       9,    98,   102,   300,    96,   102,   100,   102,    46,   306,
     307,    77,   306,    12,   103,    63,    99,   103,    97,   103,
     101,    60,    95,    59,    12,    14,    15,    24,    27,    39,
      45,    49,    78,    79,    80,   300,   104,   300,   103,   103,
     103,    12,    61,   164,   166,   306,   300,   300,   300,    60,
     300,   300,    60,    68,   302,   303,   307,    81,   105,   306,
     306,    83,    85,    82,    12,    84,    86,    61,    87,   306,
      80,    60,    64,    58,   165,   166,    60,    60,    60,    60,
      60,   306,     6,     7,    11,    12,    32,    58,    61,    64,
     231,   246,   247,   248,   249,   254,   300,   167,   306,   106,
     306,    69,   304,   305,   306,    32,    61,   245,   249,   245,
     245,   245,   245,     6,    62,    88,    89,    90,   255,    62,
     250,   232,    65,    14,    15,    18,    20,    24,    39,    43,
      45,    53,    55,    57,    95,   168,   169,   170,   175,   176,
     177,   178,   179,   180,   181,   182,   185,   188,   195,   197,
     199,   259,   260,   262,   263,   300,    59,    12,    14,    15,
      21,    24,    27,    28,    29,    35,    36,    37,    38,    39,
      41,    42,    45,    48,    50,    51,    54,    56,   107,   108,
     109,   300,   306,    62,   166,   304,   305,    58,    91,   256,
     306,   251,   306,   306,    20,   181,   259,   181,   259,    43,
      57,   181,   181,   259,   181,   259,    31,    40,   181,   259,
      12,    43,   307,   169,   307,   303,   303,   303,   180,     8,
      10,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    72,   299,   299,    61,
     306,    28,    36,    41,    48,    56,   300,    28,    36,    41,
      48,    56,   300,    28,    36,    41,    48,    56,   300,    60,
     122,    60,    60,   116,    60,    60,    28,    36,    41,    48,
      56,   300,   134,    60,    28,    36,    41,    48,    56,   300,
     128,    60,    60,    60,   302,   303,   110,    62,    89,    33,
      47,    92,    93,    59,   248,   254,   257,   258,    62,   248,
     249,   252,   253,   254,    25,   233,   234,   235,   237,   238,
     239,   300,   299,   299,   299,   299,    43,   299,   299,   299,
     299,   299,    60,    60,   299,   299,   171,    63,   183,    61,
      63,   264,    62,   124,   118,   136,   130,    60,   112,   126,
     120,   138,   132,    60,   114,   123,   117,   135,   129,    60,
     111,    12,    60,    12,   300,    60,    64,   240,   240,   125,
     119,   137,   131,    60,   113,    60,    64,   160,   127,   121,
     139,   133,    60,   115,    60,   300,   231,   164,   109,    60,
      60,    60,   302,   303,   304,   305,   304,   305,    12,    72,
     236,   298,   300,    65,   302,   303,   236,    61,    63,   266,
      63,   268,   186,    63,   265,    63,   267,   164,   164,    63,
     269,    60,    19,    30,    52,    60,   229,     7,    23,    52,
      60,   281,    60,    60,    60,    60,   164,    60,    60,    60,
      60,    60,   164,    60,    60,    60,    60,    60,   164,    60,
       7,    32,    61,   154,   156,   294,     7,    32,    61,    90,
     140,   142,   241,    60,    60,    60,    60,   164,    60,     7,
      32,    61,    90,   147,   149,   306,    60,    60,    60,    60,
     164,    60,    32,    61,   157,   159,   294,   246,    11,    11,
      59,    93,   258,   253,    60,   235,    60,    62,    19,   281,
      19,   281,   229,    19,   281,    19,   281,    19,   281,   306,
      60,    61,    60,    32,   230,   247,   184,    62,    60,    60,
       7,    32,    61,   282,   284,   285,    58,   271,   154,   140,
     147,   157,   245,   154,   140,   147,   157,   245,   154,   140,
     147,   157,   245,   306,   143,   306,   293,   294,   306,   154,
     140,   147,   157,   245,   150,   306,   293,     7,   161,   162,
     163,   154,   140,   147,   157,   245,   306,   231,   247,    60,
      60,   187,    60,    60,    60,    64,   189,     7,   295,   198,
      58,   219,   270,     7,   261,    67,   306,    64,   286,   287,
     306,    62,   155,   156,    58,   144,    62,   141,   142,   144,
      12,   242,   243,   244,    58,   151,    62,   148,   149,   151,
      66,    65,   304,   305,    62,   158,   159,   190,   192,   219,
     193,   191,   194,   306,     7,    32,    61,   209,   211,   295,
     296,    62,    64,   213,   306,   287,   213,   294,   297,   298,
      62,   283,   284,   288,    12,    14,    15,    21,    24,    27,
      35,    39,    45,    50,    51,    59,   272,   273,   274,   300,
     304,   305,   306,   304,   305,    66,    65,   304,   305,   306,
     304,   305,     7,   163,   304,   305,   209,   209,   209,   209,
     209,    12,   172,   173,   306,   212,    60,   214,    12,    14,
      15,    21,    24,    26,    27,    35,    39,    45,    50,    51,
      59,   220,   221,   222,   300,   304,   305,   306,   300,   300,
     300,    60,    60,   300,   300,    60,   302,   303,   275,    62,
     156,    59,    93,   145,   146,    62,   142,    12,   244,    21,
      59,    93,   152,   153,    62,   149,    62,   159,   174,    65,
     173,    62,   210,   211,    67,   196,   306,   300,   300,   300,
      60,    60,    60,   300,   300,    60,   302,   303,   223,    62,
     284,    14,    15,    20,    24,    39,    45,   199,   289,   290,
     291,   292,   277,   279,   276,    12,   300,   278,   280,   300,
      59,   274,    60,   302,   303,    60,   302,   303,   105,   304,
     305,   297,   200,   298,     8,    11,   215,   216,   217,   301,
     225,   227,   224,   300,    12,   300,   226,   228,   300,    59,
     222,    60,    16,    65,   302,   303,    60,    60,    60,    60,
      60,   246,    59,   146,   231,    59,   153,    64,    62,   211,
     201,    65,   304,   305,    66,    60,    60,    60,    60,    60,
     246,    60,   291,   245,   245,   245,   245,   245,   167,    58,
     206,   217,    32,   218,   245,   245,   245,   245,   245,   164,
      65,   306,    64,   202,   247,   306,    50,    59,   207,   208,
     306,    60,   302,   303,    65,   180,   203,   204,   231,    59,
     208,   298,   302,   303,   205,    65,   204,    60,   247
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (context, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, yyscanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, context); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, Sdf_TextParserContext *context)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, context)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    Sdf_TextParserContext *context;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (context);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, Sdf_TextParserContext *context)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, context)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    Sdf_TextParserContext *context;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, context);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, Sdf_TextParserContext *context)
#else
static void
yy_reduce_print (yyvsp, yyrule, context)
    YYSTYPE *yyvsp;
    int yyrule;
    Sdf_TextParserContext *context;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , context);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, context); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, Sdf_TextParserContext *context)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, context)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    Sdf_TextParserContext *context;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (context);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (Sdf_TextParserContext *context);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (Sdf_TextParserContext *context)
#else
int
yyparse (context)
    Sdf_TextParserContext *context;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 49:

/* Line 1455 of yacc.c  */
#line 1321 "pxr/usd/sdf/textFileFormat.yy"
    {

        // Store the names of the root prims.
        _SetField(
            SdfPath::AbsoluteRootPath(), SdfChildrenKeys->PrimChildren,
            context->nameChildrenStack.back(), context);
        context->nameChildrenStack.pop_back();
    ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 1332 "pxr/usd/sdf/textFileFormat.yy"
    {
            _MatchMagicIdentifier((yyvsp[(1) - (1)]), context);
            context->nameChildrenStack.push_back(std::vector<TfToken>());

            _CreateSpec(
                SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot, context);

            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 1345 "pxr/usd/sdf/textFileFormat.yy"
    {
            // Abort if error after layer metadata.
            ABORT_IF_ERROR(context->seenError);

            // If we're only reading metadata and we got here, 
            // we're done.
            if (context->metadataOnly)
                YYACCEPT;
        ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 1371 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 1376 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 1378 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 1385 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 1388 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 1391 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 1394 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 1397 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 1400 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 1403 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 1406 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 1409 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 1412 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1417 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1429 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                SdfPath::AbsoluteRootPath(), SdfFieldKeys->SubLayers, 
                context->subLayerPaths, context);
            _SetField(
                SdfPath::AbsoluteRootPath(), SdfFieldKeys->SubLayerOffsets, 
                context->subLayerOffsets, context);

            context->subLayerPaths.clear();
            context->subLayerOffsets.clear();
        ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1448 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->subLayerPaths.push_back(context->layerRefPath);
            context->subLayerOffsets.push_back(context->layerRefOffset);
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1456 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = (yyvsp[(1) - (1)]).Get<std::string>();
            context->layerRefOffset = SdfLayerOffset();
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1474 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetOffset( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 1478 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetScale( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 1494 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken();
        ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 1498 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 1502 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken();
        ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1506 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1510 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken();
        ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1514 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1518 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1528 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1529 "pxr/usd/sdf/textFileFormat.yy"
    { 
            (yyval) = std::string( (yyvsp[(1) - (3)]).Get<std::string>() + '.'
                    + (yyvsp[(3) - (3)]).Get<std::string>() ); 
        ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1536 "pxr/usd/sdf/textFileFormat.yy"
    {
            TfToken name((yyvsp[(1) - (1)]).Get<std::string>());
            if (!SdfPath::IsValidIdentifier(name)) {
                Err(context, "'%s' is not a valid prim name", name.GetText());
            }
            context->path = context->path.AppendChild(name);

            if (_HasSpec(context->path, context)) {
                Err(context, "Duplicate prim '%s'", context->path.GetText());
            } else {
                // Record the existence of this prim.
                _CreateSpec(context->path, SdfSpecTypePrim, context);

                // Add this prim to its parent's name children
                context->nameChildrenStack.back().push_back(name);
            }

            // Create our name children vector and properties vector.
            context->nameChildrenStack.push_back(std::vector<TfToken>());
            context->propertiesStack.push_back(std::vector<TfToken>());

            _SetField(
                context->path, SdfFieldKeys->Specifier, 
                context->specifier, context);

            if (!context->typeName.IsEmpty())
                _SetField(
                    context->path, SdfFieldKeys->TypeName, 
                    context->typeName, context);
        ;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1569 "pxr/usd/sdf/textFileFormat.yy"
    {
            // Store the names of our children
            if (!context->nameChildrenStack.back().empty()) {
                _SetField(
                    context->path, SdfChildrenKeys->PrimChildren,
                    context->nameChildrenStack.back(), context);
            }

            // Store the names of our properties, if there are any
            if (!context->propertiesStack.back().empty()) {
                _SetField(
                    context->path, SdfChildrenKeys->PropertyChildren,
                    context->propertiesStack.back(), context);
            }

            context->nameChildrenStack.pop_back();
            context->propertiesStack.pop_back();
            context->path = context->path.GetParentPath();

            // Abort after each prim if we hit an error.
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 1617 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1622 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePrim, context);
        ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1624 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 1631 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1634 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1637 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1640 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1643 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1646 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1649 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1652 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1655 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1658 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1663 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1670 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Kind, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1677 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission, 
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context), 
                context);
        ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1684 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1688 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1691 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1695 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1698 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1702 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1705 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1709 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1712 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1716 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1719 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1723 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1727 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1729 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1732 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1734 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1737 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1739 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1742 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1744 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1747 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1749 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1752 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1754 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1758 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1760 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1763 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1765 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1768 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1770 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1773 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1775 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1778 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1780 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1783 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1785 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1789 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1793 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1796 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1800 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1803 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1807 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1810 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1814 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1817 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1821 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1824 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1828 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1833 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Relocates, 
                context->relocatesParsingMap, context);
            context->relocatesParsingMap.clear();
        ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1841 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSelection(context);
        ;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1845 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeExplicit, context); 
            context->nameVector.clear();
        ;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1849 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeDeleted, context);
            context->nameVector.clear();
        ;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1853 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAdded, context);
            context->nameVector.clear();
        ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1857 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypePrepended, context);
            context->nameVector.clear();
        ;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1861 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAppended, context);
            context->nameVector.clear();
        ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1865 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeOrdered, context);
            context->nameVector.clear();
        ;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1871 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1876 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1883 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrefixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1891 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SuffixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 1912 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (context->layerRefPath.empty()) {
            Err(context, "Payload asset path must not be empty. If this "
                "is intended to be an internal payload, remove the "
                "'@' delimiters.");
        }

        SdfPayload payload(context->layerRefPath,
                           context->savedPath,
                           context->layerRefOffset);
        context->payloadParsingRefs.push_back(payload);
    ;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1924 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Internal payloads do not begin with an asset path so there's
        // no layer_ref rule, but we need to make sure we reset state the
        // so we don't pick up data from a previously-parsed payload.
        context->layerRefPath.clear();
        context->layerRefOffset = SdfLayerOffset();
        ABORT_IF_ERROR(context->seenError);
      ;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 1932 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!(yyvsp[(1) - (3)]).Get<std::string>().empty()) {
           _PathSetPrim((yyvsp[(1) - (3)]), context);
        }
        else {
            context->savedPath = SdfPath::EmptyPath();
        }        

        SdfPayload payload(std::string(),
                           context->savedPath,
                           context->layerRefOffset);
        context->payloadParsingRefs.push_back(payload);
    ;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 1975 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (context->layerRefPath.empty()) {
            Err(context, "Reference asset path must not be empty. If this "
                "is intended to be an internal reference, remove the "
                "'@' delimiters.");
        }

        SdfReference ref(context->layerRefPath,
                         context->savedPath,
                         context->layerRefOffset);
        ref.SwapCustomData(context->currentDictionaries[0]);
        context->referenceParsingRefs.push_back(ref);
    ;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1988 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Internal references do not begin with an asset path so there's
        // no layer_ref rule, but we need to make sure we reset state the
        // so we don't pick up data from a previously-parsed reference.
        context->layerRefPath.clear();
        context->layerRefOffset = SdfLayerOffset();
        ABORT_IF_ERROR(context->seenError);
      ;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1996 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!(yyvsp[(1) - (3)]).Get<std::string>().empty()) {
           _PathSetPrim((yyvsp[(1) - (3)]), context);
        }
        else {
            context->savedPath = SdfPath::EmptyPath();
        }        

        SdfReference ref(std::string(),
                         context->savedPath,
                         context->layerRefOffset);
        ref.SwapCustomData(context->currentDictionaries[0]);
        context->referenceParsingRefs.push_back(ref);
    ;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 2041 "pxr/usd/sdf/textFileFormat.yy"
    {
        _InheritAppendPath(context);
        ;}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 2059 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SpecializesAppendPath(context);
        ;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 2079 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelocatesAdd((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 2095 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->nameVector.push_back(TfToken((yyvsp[(1) - (1)]).Get<std::string>()));
        ;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 2113 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 2114 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 2115 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 2121 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string name = (yyvsp[(2) - (2)]).Get<std::string>();
        ERROR_IF_NOT_ALLOWED(context, SdfSchema::IsValidVariantIdentifier(name));

        context->currentVariantSetNames.push_back( name );
        context->currentVariantNames.push_back( std::vector<std::string>() );

        context->path = context->path.AppendVariantSelection(name, "");
    ;}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 2129 "pxr/usd/sdf/textFileFormat.yy"
    {

        SdfPath variantSetPath = context->path;
        context->path = context->path.GetParentPath();

        // Create this VariantSetSpec if it does not already exist.
        if (!_HasSpec(variantSetPath, context)) {
            _CreateSpec(variantSetPath, SdfSpecTypeVariantSet, context);

            // Add the name of this variant set to the VariantSets field
            _AppendVectorItem(SdfChildrenKeys->VariantSetChildren,
                              TfToken(context->currentVariantSetNames.back()),
                              context);
        }

        // Author the variant set's variants
        _SetField(
            variantSetPath, SdfChildrenKeys->VariantChildren,
            TfToTokenVector(context->currentVariantNames.back()), context);

        context->currentVariantSetNames.pop_back();
        context->currentVariantNames.pop_back();
    ;}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 2160 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string variantName = (yyvsp[(1) - (1)]).Get<std::string>();
        ERROR_IF_NOT_ALLOWED(
            context, 
            SdfSchema::IsValidVariantIdentifier(variantName));

        context->currentVariantNames.back().push_back(variantName);

        // A variant is basically like a new pseudo-root, so we need to push
        // a new item onto our name children stack to store prims defined
        // within this variant.
        context->nameChildrenStack.push_back(std::vector<TfToken>());
        context->propertiesStack.push_back(std::vector<TfToken>());

        std::string variantSetName = context->currentVariantSetNames.back();
        context->path = context->path.GetParentPath()
            .AppendVariantSelection(variantSetName, variantName);

        _CreateSpec(context->path, SdfSpecTypeVariant, context);

    ;}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 2180 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Store the names of the prims and properties defined in this variant.
        if (!context->nameChildrenStack.back().empty()) {
            _SetField(
                context->path, SdfChildrenKeys->PrimChildren, 
                context->nameChildrenStack.back(), context);
        }
        if (!context->propertiesStack.back().empty()) {
            _SetField(
                context->path, SdfChildrenKeys->PropertyChildren, 
                context->propertiesStack.back(), context);
        }

        context->nameChildrenStack.pop_back();
        context->propertiesStack.pop_back();

        std::string variantSet = context->path.GetVariantSelection().first;
        context->path = 
            context->path.GetParentPath().AppendVariantSelection(variantSet, "");
    ;}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 2203 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 2212 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 2234 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 2237 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue(SdfVariabilityConfig);
    ;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 2243 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->assoc = VtValue();
    ;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 2249 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue((yyvsp[(1) - (1)]).Get<std::string>(), context);
    ;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 2252 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue(std::string((yyvsp[(1) - (3)]).Get<std::string>() + "[]"), context);
    ;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 2258 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue();
        context->custom = false;
    ;}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 2262 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
    ;}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 2268 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (2)]), context);

        if (!context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 2275 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 2280 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 2286 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        _PrimInitAttribute((yyvsp[(3) - (3)]), context);

        if (!context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 2294 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 2299 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 2305 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 2309 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeExplicit, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 2313 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 2317 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAdded, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 2321 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 2325 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypePrepended, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 2329 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 2333 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAppended, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 2337 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 2341 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeDeleted, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 2345 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 2349 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeOrdered, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 2356 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (8)]), context);
        context->mapperTarget = context->savedPath;
        context->path = context->path.AppendMapper(context->mapperTarget);
    ;}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 2361 "pxr/usd/sdf/textFileFormat.yy"
    {
        SdfPath targetPath = context->path.GetTargetPath();
        context->path = context->path.GetParentPath(); // pop mapper

        // Add this mapper to the list of mapper children (keyed by the mapper's
        // connection path) on this attribute.
        //
        // XXX:
        // Conceptually, this is incorrect -- mappers are children of attribute
        // connections, not attributes themselves. This is OK for now and should
        // be fixed by the introduction of real attribute connection specs in Sd.
        _AppendVectorItem<SdfPath>(SdfChildrenKeys->MapperChildren, targetPath,
                                  context);

        context->path = context->path.GetParentPath(); // pop attr
    ;}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 2380 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        ;}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 2383 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            context->path = context->path.GetParentPath(); // pop attr
        ;}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 2405 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string mapperName((yyvsp[(1) - (1)]).Get<std::string>());
        if (_HasSpec(context->path, context)) {
            Err(context, "Duplicate mapper");
        }

        _CreateSpec(context->path, SdfSpecTypeMapper, context);
        _SetField(context->path, SdfFieldKeys->TypeName, mapperName, context);
    ;}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 2425 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfChildrenKeys->MapperArgChildren, 
            context->mapperArgsNameVector, context);
        context->mapperArgsNameVector.clear();
    ;}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 2439 "pxr/usd/sdf/textFileFormat.yy"
    {
            TfToken mapperParamName((yyvsp[(2) - (2)]).Get<std::string>());
            context->mapperArgsNameVector.push_back(mapperParamName);
            context->path = context->path.AppendMapperArg(mapperParamName);

            _CreateSpec(context->path, SdfSpecTypeMapperArg, context);

        ;}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 2446 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->MapperArgValue, 
                context->currentValue, context);
            context->path = context->path.GetParentPath(); // pop mapper arg
        ;}
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 2466 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryArgs, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 2487 "pxr/usd/sdf/textFileFormat.yy"
    {
            _AttributeAppendConnectionPath(context);
        ;}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 2490 "pxr/usd/sdf/textFileFormat.yy"
    {
            _AttributeAppendConnectionPath(context);
        ;}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 2492 "pxr/usd/sdf/textFileFormat.yy"
    {
            // XXX: See comment in relationship_target_and_opt_marker about
            //      markers in reorder/delete statements.
            if (context->connParsingAllowConnectionData) {
                const SdfPath specPath = context->path.AppendTarget(
                    context->connParsingTargetPaths.back());

                // Create the connection spec object if one doesn't already
                // exist to parent the marker data.
                if (!_HasSpec(specPath, context)) {
                    _CreateSpec(specPath, SdfSpecTypeConnection, context);
                }

                _SetField(
                    specPath, SdfFieldKeys->Marker, context->marker, context);
            }
        ;}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 2516 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples = SdfTimeSampleMap();
    ;}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 2532 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (2)]).Get<double>();
    ;}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 2535 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples[ context->timeSampleTime ] = context->currentValue;
    ;}
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 2539 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (3)]).Get<double>();
        context->timeSamples[ context->timeSampleTime ] 
            = VtValue(SdfValueBlock());  
    ;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2569 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 2574 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeAttribute, context);
        ;}
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 2576 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 2583 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 2586 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 2589 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 2592 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 2595 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 2598 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 2601 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 2604 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 2607 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 2610 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 2615 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 2622 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 2629 "pxr/usd/sdf/textFileFormat.yy"
    {
             _SetField(
                 context->path, SdfFieldKeys->DisplayUnit,
                 _GetDisplayUnitFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                 context);
        ;}
    break;

  case 356:

/* Line 1455 of yacc.c  */
#line 2637 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 2642 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken(), context);
        ;}
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 2655 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfFieldKeys->Default,
            context->currentValue, context);
    ;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2660 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfFieldKeys->Default,
            SdfValueBlock(), context);
    ;}
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 2672 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 2675 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2691 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertValue((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 2694 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertDictionary((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 2712 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory((yyvsp[(1) - (1)]), context);
    ;}
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 2718 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitShapedFactory((yyvsp[(1) - (3)]), context);
    ;}
    break;

  case 376:

/* Line 1455 of yacc.c  */
#line 2728 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 377:

/* Line 1455 of yacc.c  */
#line 2731 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 2747 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory(Value(std::string("string")), context);
            _ValueAppendAtomic((yyvsp[(3) - (3)]), context);
            _ValueSetAtomic(context);
            _DictionaryInsertValue((yyvsp[(1) - (3)]), context);
        ;}
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 2760 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->currentValue = VtValue();
        if (context->values.IsRecordingString()) {
            context->values.SetRecordedString("None");
        }
    ;}
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2766 "pxr/usd/sdf/textFileFormat.yy"
    {
        _ValueSetList(context);
    ;}
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 2776 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->currentValue.Swap(context->currentDictionaries[0]);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2781 "pxr/usd/sdf/textFileFormat.yy"
    {
            // This is only here to allow 'None' metadata values for
            // an explicit list operation on an SdfListOp-valued field.
            // We'll reject this value for any other metadata field
            // in _GenericMetadataEnd.
            context->currentValue = VtValue();
            if (context->values.IsRecordingString()) {
                context->values.SetRecordedString("None");
            }
    ;}
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2794 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetAtomic(context);
        ;}
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 2797 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetTuple(context);
        ;}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2800 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetList(context);
        ;}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2803 "pxr/usd/sdf/textFileFormat.yy"
    {
            // Set the recorded string on the ParserValueContext. Normally
            // 'values' is able to keep track of the parsed string, but in this
            // case it doesn't get the BeginList() and EndList() calls so the
            // recorded string would have been "". We want "[]" instead.
            if (context->values.IsRecordingString()) {
                context->values.SetRecordedString("[]");
            }

            _ValueSetShaped(context);
        ;}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2814 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetCurrentToSdfPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2820 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2823 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2826 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs identifiers to be stored as TfToken
            // instead of std::string to be able to distinguish between them.
            _ValueAppendAtomic(TfToken((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 2831 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs asset paths to be stored as
            // SdfAssetPath instead of std::string to be able to distinguish
            // between them
            _ValueAppendAtomic(SdfAssetPath((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 397:

/* Line 1455 of yacc.c  */
#line 2844 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginList();
        ;}
    break;

  case 398:

/* Line 1455 of yacc.c  */
#line 2847 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndList();
        ;}
    break;

  case 405:

/* Line 1455 of yacc.c  */
#line 2872 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginTuple();
        ;}
    break;

  case 406:

/* Line 1455 of yacc.c  */
#line 2874 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndTuple();
        ;}
    break;

  case 412:

/* Line 1455 of yacc.c  */
#line 2897 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 413:

/* Line 1455 of yacc.c  */
#line 2901 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 414:

/* Line 1455 of yacc.c  */
#line 2905 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 415:

/* Line 1455 of yacc.c  */
#line 2909 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 416:

/* Line 1455 of yacc.c  */
#line 2916 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context); 
        ;}
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 2919 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 2928 "pxr/usd/sdf/textFileFormat.yy"
    { 
            _PrimInitRelationship((yyvsp[(2) - (6)]), context);

            // If path is empty, use default c'tor to construct empty path.
            // XXX: 08/04/08 Would be nice if SdfPath would allow 
            // SdfPath("") without throwing a warning.
            std::string pathString = (yyvsp[(6) - (6)]).Get<std::string>();
            SdfPath path = pathString.empty() ? SdfPath() : SdfPath(pathString);

            _SetField(context->path, SdfFieldKeys->Default, path, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 2943 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (2)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 2948 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeExplicit, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2953 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 2956 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeDeleted, context); 
            _PrimEndRelationship(context);
        ;}
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2961 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 2965 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAdded, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 2969 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 2973 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypePrepended, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 2977 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 2981 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAppended, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 2986 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 2989 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeOrdered, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 431:

/* Line 1455 of yacc.c  */
#line 2994 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context);
            context->relParsingAllowTargetData = true;
            _RelationshipAppendTargetPath((yyvsp[(4) - (5)]), context);
            _RelationshipInitTarget(context->relParsingTargetPaths->back(),
                                    context);
        ;}
    break;

  case 432:

/* Line 1455 of yacc.c  */
#line 3001 "pxr/usd/sdf/textFileFormat.yy"
    {
            // This clause only defines relational attributes for a target,
            // it does not add to the relationship target list. However, we 
            // do need to create a relationship target spec to associate the
            // attributes with.
            _PrimEndRelationship(context);
        ;}
    break;

  case 443:

/* Line 1455 of yacc.c  */
#line 3030 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 444:

/* Line 1455 of yacc.c  */
#line 3035 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeRelationship, context);
        ;}
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 3037 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 3044 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 3047 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 3050 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 449:

/* Line 1455 of yacc.c  */
#line 3053 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 3056 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 451:

/* Line 1455 of yacc.c  */
#line 3059 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 452:

/* Line 1455 of yacc.c  */
#line 3062 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 453:

/* Line 1455 of yacc.c  */
#line 3065 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 454:

/* Line 1455 of yacc.c  */
#line 3068 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 455:

/* Line 1455 of yacc.c  */
#line 3071 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 456:

/* Line 1455 of yacc.c  */
#line 3076 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 457:

/* Line 1455 of yacc.c  */
#line 3083 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 458:

/* Line 1455 of yacc.c  */
#line 3091 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 3096 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 463:

/* Line 1455 of yacc.c  */
#line 3110 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 3113 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 469:

/* Line 1455 of yacc.c  */
#line 3129 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipAppendTargetPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 470:

/* Line 1455 of yacc.c  */
#line 3132 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipAppendTargetPath((yyvsp[(1) - (3)]), context);

            // Markers on relationship targets in reorder or delete statements
            // shouldn't cause a relationship target spec to be created.
            //
            // XXX: This probably should be a parser error; markers in these
            //      statements don't make any sense. However, doing this
            //      would require a staged process for backwards compatibility.
            //      For now, we silently ignore markers in unwanted places.
            //      The next stages would be to stop writing out markers in
            //      reorders/deletes, then finally making this an error.
            if (context->relParsingAllowTargetData) {
                const SdfPath specPath = context->path.AppendTarget( 
                    context->relParsingTargetPaths->back() );
                _RelationshipInitTarget(context->relParsingTargetPaths->back(),
                                        context);
                _SetField(
                    specPath, SdfFieldKeys->Marker, VtValue(context->marker), 
                    context);
            }
        ;}
    break;

  case 473:

/* Line 1455 of yacc.c  */
#line 3162 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipInitTarget(context->relParsingTargetPaths->back(), 
                                    context);
            context->path = context->path.AppendTarget( 
                context->relParsingTargetPaths->back() );

            context->propertiesStack.push_back(std::vector<TfToken>());

            if (!context->relParsingAllowTargetData) {
                Err(context, 
                    "Relational attributes cannot be specified in lists of "
                    "targets to be deleted or reordered");
            }
        ;}
    break;

  case 474:

/* Line 1455 of yacc.c  */
#line 3176 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->propertiesStack.back().empty()) {
            _SetField(
                context->path, SdfChildrenKeys->PropertyChildren, 
                context->propertiesStack.back(), context);
        }
        context->propertiesStack.pop_back();

        context->path = context->path.GetParentPath();
    ;}
    break;

  case 479:

/* Line 1455 of yacc.c  */
#line 3200 "pxr/usd/sdf/textFileFormat.yy"
    {
        ;}
    break;

  case 481:

/* Line 1455 of yacc.c  */
#line 3206 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 482:

/* Line 1455 of yacc.c  */
#line 3219 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->savedPath = SdfPath();
    ;}
    break;

  case 484:

/* Line 1455 of yacc.c  */
#line 3226 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrim((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 485:

/* Line 1455 of yacc.c  */
#line 3232 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetProperty((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 486:

/* Line 1455 of yacc.c  */
#line 3238 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrimOrPropertyScenePath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 487:

/* Line 1455 of yacc.c  */
#line 3244 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->marker = context->savedPath.GetString();
        ;}
    break;

  case 488:

/* Line 1455 of yacc.c  */
#line 3247 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->marker = (yyvsp[(1) - (1)]).Get<std::string>();
        ;}
    break;

  case 497:

/* Line 1455 of yacc.c  */
#line 3279 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;



/* Line 1455 of yacc.c  */
#line 6419 "pxr/usd/sdf/textFileFormat.tab.cpp"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (context, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (context, yymsg);
	  }
	else
	  {
	    yyerror (context, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, context);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, context);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (context, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, context);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, context);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 3311 "pxr/usd/sdf/textFileFormat.yy"


//--------------------------------------------------------------------
// textFileFormatYyerror
//--------------------------------------------------------------------
void textFileFormatYyerror(Sdf_TextParserContext *context, const char *msg) 
{
    const std::string nextToken(textFileFormatYyget_text(context->scanner), 
                                textFileFormatYyget_leng(context->scanner));
    const bool isNewlineToken = 
        (nextToken.length() == 1 && nextToken[0] == '\n');

    int errLineNumber = context->menvaLineNo;

    // By this time, menvaLineNo has already been updated to account for
    // nextToken. So, if nextToken is a newline, the error really occurred on
    // the previous line.
    if (isNewlineToken) { 
        errLineNumber -= 1;
    }

    std::string s = TfStringPrintf(
        "%s%s in <%s> on line %i",
        msg,
        isNewlineToken ? 
            "" : TfStringPrintf(" at \'%s\'", nextToken.c_str()).c_str(),
        context->path.GetText(),
        errLineNumber);

    // Append file context, if known.
    if (!context->fileContext.empty()) {
        s += " in file " + context->fileContext;
    }
    s += "\n";

    // Return the line number in the error info.
    TfDiagnosticInfo info(errLineNumber);

    TF_ERROR(info, TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE, s);

    context->seenError = true;
}

static void _ReportParseError(Sdf_TextParserContext *context, 
                              const std::string &text)
{
    if (!context->values.IsRecordingString()) {
        textFileFormatYyerror(context, text.c_str());
    }
}

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
    _flexBuffer = textFileFormatYy_scan_buffer(
        _fileBuffer.get(), size + paddingBytesRequired, _scanner);
}

Sdf_MemoryFlexBuffer::~Sdf_MemoryFlexBuffer()
{
    if (_flexBuffer)
        textFileFormatYy_delete_buffer(_flexBuffer, _scanner);
}

#ifdef SDF_PARSER_DEBUG_MODE
extern int yydebug;
#else
static int yydebug;
#endif // SDF_PARSER_DEBUG_MODE

namespace {
struct _DebugContext {
    explicit _DebugContext(bool state=true) : _old(yydebug) { yydebug = state; }
    ~_DebugContext() { yydebug = _old; }
private:
    bool _old;
};
}

/// Parse a .menva file into an SdfData
bool Sdf_ParseMenva(
     const std::string& fileContext, 
     const std::shared_ptr<ArAsset>& asset,
     const std::string& magicId,
     const std::string& versionString,
     bool metadataOnly,
     SdfDataRefPtr data)
{
    TfAutoMallocTag2 tag("Menva", "Menva_Parse");

    TRACE_FUNCTION();

    // Turn on debugging, if enabled.
    _DebugContext debugCtx;

    // Configure for input file.
    Sdf_TextParserContext context;

    context.data = data;
    context.fileContext = fileContext;
    context.magicIdentifierToken = magicId;
    context.versionString = versionString;
    context.metadataOnly = metadataOnly;
    context.values.errorReporter =
        std::bind(_ReportParseError, &context, std::placeholders::_1);

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
            } catch (boost::bad_get) {
                TF_CODING_ERROR("Bad boost:get<T>() in menva parser.");
                Err(&context, "Internal menva parser error.");
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

/// Parse a .menva string into an SdfData
bool Sdf_ParseMenvaFromString(const std::string & menvaString, 
                             const std::string & magicId,
                             const std::string & versionString,
                             SdfDataRefPtr data)
{
    TfAutoMallocTag2 tag("Menva", "Menva_Parse");

    TRACE_FUNCTION();

    // Configure for input string.
    Sdf_TextParserContext context;

    context.data = data;
    context.magicIdentifierToken = magicId;
    context.versionString = versionString;
    context.values.errorReporter =
        std::bind(_ReportParseError, &context, std::placeholders::_1);

    // Initialize the scanner, allowing it to be reentrant.
    textFileFormatYylex_init(&context.scanner);
    textFileFormatYyset_extra(&context, context.scanner);

    // Run parser.
    yy_buffer_state *buf = textFileFormatYy_scan_string(menvaString.c_str(), 
                                               context.scanner);
    int status = -1;
    try {
        TRACE_SCOPE("textFileFormatYyParse");
        status = textFileFormatYyparse(&context);
    } catch (boost::bad_get) {
        TF_CODING_ERROR("Bad boost:get<T>() in menva parser.");
        Err(&context, "Internal menva parser error.");
    }

    // Clean up.
    textFileFormatYy_delete_buffer(buf, context.scanner);
    textFileFormatYylex_destroy(context.scanner);

    return status == 0;
}

