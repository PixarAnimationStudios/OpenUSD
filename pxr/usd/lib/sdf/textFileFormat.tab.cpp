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
#define YYLAST   1085

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  69
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  236
/* YYNRULES -- Number of rules.  */
#define YYNRULES  503
/* YYNRULES -- Number of states.  */
#define YYNSTATES  921

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
      58,    59,     2,     2,    68,     2,    63,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    66,    67,
       2,    60,     2,     2,     2,     2,     2,     2,     2,     2,
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
    1015,  1017,  1018,  1024,  1025,  1028,  1030,  1034,  1035,  1040,
    1044,  1045,  1049,  1055,  1057,  1061,  1063,  1065,  1067,  1069,
    1070,  1075,  1076,  1082,  1083,  1089,  1090,  1096,  1097,  1103,
    1104,  1110,  1114,  1118,  1122,  1126,  1129,  1130,  1133,  1135,
    1137,  1138,  1144,  1145,  1148,  1150,  1154,  1159,  1164,  1166,
    1168,  1170,  1172,  1174,  1178,  1179,  1185,  1186,  1189,  1191,
    1195,  1199,  1201,  1203,  1205,  1207,  1209,  1211,  1213,  1215,
    1218,  1220,  1222,  1224,  1226,  1228,  1229,  1234,  1238,  1240,
    1244,  1246,  1248,  1250,  1251,  1256,  1260,  1262,  1266,  1268,
    1270,  1272,  1275,  1279,  1282,  1283,  1291,  1298,  1299,  1305,
    1306,  1312,  1313,  1319,  1320,  1326,  1327,  1333,  1334,  1340,
    1341,  1349,  1351,  1353,  1354,  1358,  1364,  1366,  1370,  1372,
    1374,  1376,  1378,  1379,  1384,  1385,  1391,  1392,  1398,  1399,
    1405,  1406,  1412,  1413,  1419,  1423,  1427,  1431,  1434,  1435,
    1438,  1440,  1442,  1446,  1452,  1454,  1458,  1459,  1463,  1464,
    1466,  1467,  1473,  1474,  1477,  1479,  1483,  1485,  1487,  1492,
    1493,  1495,  1497,  1499,  1501,  1503,  1505,  1507,  1509,  1511,
    1513,  1515,  1517,  1519,  1520,  1522,  1525,  1527,  1529,  1531,
    1534,  1535,  1537,  1539
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      70,     0,    -1,    73,    -1,    13,    -1,    14,    -1,    15,
      -1,    16,    -1,    17,    -1,    18,    -1,    19,    -1,    20,
      -1,    21,    -1,    22,    -1,    23,    -1,    24,    -1,    25,
      -1,    26,    -1,    27,    -1,    28,    -1,    29,    -1,    30,
      -1,    31,    -1,    32,    -1,    33,    -1,    34,    -1,    36,
      -1,    35,    -1,    37,    -1,    38,    -1,    39,    -1,    40,
      -1,    41,    -1,    42,    -1,    43,    -1,    44,    -1,    45,
      -1,    46,    -1,    47,    -1,    48,    -1,    49,    -1,    50,
      -1,    51,    -1,    52,    -1,    53,    -1,    54,    -1,    55,
      -1,    56,    -1,    57,    -1,    75,    -1,    75,    93,   303,
      -1,    -1,     4,    74,    72,    -1,   303,    -1,   303,    58,
      76,    59,   303,    -1,   303,    -1,   303,    77,   299,    -1,
      79,    -1,    77,   300,    79,    -1,   297,    -1,    12,    -1,
      -1,    78,    80,    60,   244,    -1,    -1,    24,   297,    81,
      60,   243,    -1,    -1,    14,   297,    82,    60,   243,    -1,
      -1,    39,   297,    83,    60,   243,    -1,    -1,    15,   297,
      84,    60,   243,    -1,    -1,    45,   297,    85,    60,   243,
      -1,    27,    60,    12,    -1,    49,    60,    86,    -1,    61,
     303,    62,    -1,    61,   303,    87,   301,    62,    -1,    88,
      -1,    87,   302,    88,    -1,    89,    90,    -1,     6,    -1,
      -1,    58,    91,   299,    59,    -1,    92,    -1,    91,   300,
      92,    -1,    33,    60,    11,    -1,    47,    60,    11,    -1,
      94,    -1,    93,   304,    94,    -1,    -1,    22,    95,   102,
      -1,    -1,    22,   101,    96,   102,    -1,    -1,    17,    97,
     102,    -1,    -1,    17,   101,    98,   102,    -1,    -1,    34,
      99,   102,    -1,    -1,    34,   101,   100,   102,    -1,    45,
      46,    60,   163,    -1,   297,    -1,   101,    63,   297,    -1,
      -1,    12,   103,   104,    64,   166,    65,    -1,   303,    -1,
     303,    58,   105,    59,   303,    -1,   303,    -1,   303,   106,
     299,    -1,   108,    -1,   106,   300,   108,    -1,   297,    -1,
      21,    -1,    50,    -1,    12,    -1,    -1,   107,   109,    60,
     244,    -1,    -1,    24,   297,   110,    60,   243,    -1,    -1,
      14,   297,   111,    60,   243,    -1,    -1,    39,   297,   112,
      60,   243,    -1,    -1,    15,   297,   113,    60,   243,    -1,
      -1,    45,   297,   114,    60,   243,    -1,    27,    60,    12,
      -1,    29,    60,    12,    -1,    35,    60,   297,    -1,    -1,
      36,   115,    60,   139,    -1,    -1,    24,    36,   116,    60,
     139,    -1,    -1,    14,    36,   117,    60,   139,    -1,    -1,
      39,    36,   118,    60,   139,    -1,    -1,    15,    36,   119,
      60,   139,    -1,    -1,    45,    36,   120,    60,   139,    -1,
      -1,    28,   121,    60,   153,    -1,    -1,    24,    28,   122,
      60,   153,    -1,    -1,    14,    28,   123,    60,   153,    -1,
      -1,    39,    28,   124,    60,   153,    -1,    -1,    15,    28,
     125,    60,   153,    -1,    -1,    45,    28,   126,    60,   153,
      -1,    -1,    48,   127,    60,   156,    -1,    -1,    24,    48,
     128,    60,   156,    -1,    -1,    14,    48,   129,    60,   156,
      -1,    -1,    39,    48,   130,    60,   156,    -1,    -1,    15,
      48,   131,    60,   156,    -1,    -1,    45,    48,   132,    60,
     156,    -1,    -1,    41,   133,    60,   146,    -1,    -1,    24,
      41,   134,    60,   146,    -1,    -1,    14,    41,   135,    60,
     146,    -1,    -1,    39,    41,   136,    60,   146,    -1,    -1,
      15,    41,   137,    60,   146,    -1,    -1,    45,    41,   138,
      60,   146,    -1,    42,    60,   159,    -1,    54,    60,   229,
      -1,    56,    60,   163,    -1,    24,    56,    60,   163,    -1,
      14,    56,    60,   163,    -1,    39,    56,    60,   163,    -1,
      15,    56,    60,   163,    -1,    45,    56,    60,   163,    -1,
      51,    60,   297,    -1,    51,    60,    -1,    37,    60,   238,
      -1,    38,    60,   238,    -1,    32,    -1,   141,    -1,    61,
     303,    62,    -1,    61,   303,   140,   301,    62,    -1,   141,
      -1,   140,   302,   141,    -1,    89,   291,   143,    -1,    -1,
       7,   142,   143,    -1,    -1,    58,   303,    59,    -1,    58,
     303,   144,   299,    59,    -1,   145,    -1,   144,   300,   145,
      -1,    92,    -1,    32,    -1,   148,    -1,    61,   303,    62,
      -1,    61,   303,   147,   301,    62,    -1,   148,    -1,   147,
     302,   148,    -1,    89,   291,   150,    -1,    -1,     7,   149,
     150,    -1,    -1,    58,   303,    59,    -1,    58,   303,   151,
     299,    59,    -1,   152,    -1,   151,   300,   152,    -1,    92,
      -1,    21,    60,   229,    -1,    32,    -1,   155,    -1,    61,
     303,    62,    -1,    61,   303,   154,   301,    62,    -1,   155,
      -1,   154,   302,   155,    -1,   292,    -1,    32,    -1,   158,
      -1,    61,   303,    62,    -1,    61,   303,   157,   301,    62,
      -1,   158,    -1,   157,   302,   158,    -1,   292,    -1,    64,
     303,   160,    65,    -1,    -1,   161,   301,    -1,   162,    -1,
     161,   302,   162,    -1,     7,    66,     7,    -1,   165,    -1,
      61,   303,   164,   301,    62,    -1,   165,    -1,   164,   302,
     165,    -1,    12,    -1,   303,    -1,   303,   167,    -1,   168,
      -1,   167,   168,    -1,   176,   300,    -1,   174,   300,    -1,
     175,   300,    -1,    94,   304,    -1,   169,   304,    -1,    -1,
      55,    12,   170,    60,   303,    64,   303,   171,    65,    -1,
     172,    -1,   171,   172,    -1,    -1,    12,   173,   104,    64,
     166,    65,   303,    -1,    45,    31,    60,   163,    -1,    45,
      40,    60,   163,    -1,   198,    -1,   261,    -1,    53,    -1,
      18,    -1,   177,    -1,   297,    -1,   297,    61,    62,    -1,
     179,    -1,   178,   179,    -1,    -1,    -1,   180,   296,   182,
     227,   183,   217,    -1,    -1,    -1,    20,   180,   296,   185,
     227,   186,   217,    -1,    -1,   180,   296,    63,    19,    60,
     188,   208,    -1,    -1,    14,   180,   296,    63,    19,    60,
     189,   208,    -1,    -1,    39,   180,   296,    63,    19,    60,
     190,   208,    -1,    -1,    15,   180,   296,    63,    19,    60,
     191,   208,    -1,    -1,    24,   180,   296,    63,    19,    60,
     192,   208,    -1,    -1,    45,   180,   296,    63,    19,    60,
     193,   208,    -1,    -1,   180,   296,    63,    30,    61,   293,
      62,    60,   195,   199,    -1,    -1,   180,   296,    63,    52,
      60,   197,   211,    -1,   184,    -1,   181,    -1,   187,    -1,
     194,    -1,   196,    -1,    -1,   295,   200,   205,   201,    -1,
      -1,    64,   303,    65,    -1,    64,   303,   202,   299,    65,
      -1,   203,    -1,   202,   300,   203,    -1,    -1,   179,   295,
     204,    60,   245,    -1,    -1,    58,   303,    59,    -1,    58,
     303,   206,   299,    59,    -1,   207,    -1,   206,   300,   207,
      -1,    50,    60,   229,    -1,    32,    -1,   210,    -1,    61,
     303,    62,    -1,    61,   303,   209,   301,    62,    -1,   210,
      -1,   209,   302,   210,    -1,   294,    -1,    -1,    64,   212,
     303,   213,    65,    -1,    -1,   214,   301,    -1,   215,    -1,
     214,   302,   215,    -1,    -1,   298,    66,   216,   245,    -1,
     298,    66,    32,    -1,    -1,    58,   303,    59,    -1,    58,
     303,   218,   299,    59,    -1,   220,    -1,   218,   300,   220,
      -1,   297,    -1,    21,    -1,    50,    -1,    12,    -1,    -1,
     219,   221,    60,   244,    -1,    -1,    24,   297,   222,    60,
     243,    -1,    -1,    14,   297,   223,    60,   243,    -1,    -1,
      39,   297,   224,    60,   243,    -1,    -1,    15,   297,   225,
      60,   243,    -1,    -1,    45,   297,   226,    60,   243,    -1,
      27,    60,    12,    -1,    35,    60,   297,    -1,    26,    60,
     297,    -1,    51,    60,   297,    -1,    51,    60,    -1,    -1,
      60,   228,    -1,   245,    -1,    32,    -1,    -1,    64,   230,
     303,   231,    65,    -1,    -1,   232,   299,    -1,   233,    -1,
     232,   300,   233,    -1,   235,   234,    60,   245,    -1,    25,
     234,    60,   229,    -1,    12,    -1,   295,    -1,   236,    -1,
     237,    -1,   297,    -1,   297,    61,    62,    -1,    -1,    64,
     239,   303,   240,    65,    -1,    -1,   241,   301,    -1,   242,
      -1,   241,   302,   242,    -1,    12,    66,    12,    -1,    32,
      -1,   247,    -1,   229,    -1,   245,    -1,    32,    -1,   246,
      -1,   252,    -1,   247,    -1,    61,    62,    -1,     7,    -1,
      11,    -1,    12,    -1,   297,    -1,     6,    -1,    -1,    61,
     248,   249,    62,    -1,   303,   250,   301,    -1,   251,    -1,
     250,   302,   251,    -1,   246,    -1,   247,    -1,   252,    -1,
      -1,    58,   253,   254,    59,    -1,   303,   255,   301,    -1,
     256,    -1,   255,   302,   256,    -1,   246,    -1,   252,    -1,
      43,    -1,    20,    43,    -1,    20,    57,    43,    -1,    57,
      43,    -1,    -1,   257,   296,    63,    52,    60,   259,   211,
      -1,   257,   296,    63,    23,    60,     7,    -1,    -1,   257,
     296,   262,   279,   269,    -1,    -1,    24,   257,   296,   263,
     279,    -1,    -1,    14,   257,   296,   264,   279,    -1,    -1,
      39,   257,   296,   265,   279,    -1,    -1,    15,   257,   296,
     266,   279,    -1,    -1,    45,   257,   296,   267,   279,    -1,
      -1,   257,   296,    61,     7,    62,   268,   285,    -1,   258,
      -1,   260,    -1,    -1,    58,   303,    59,    -1,    58,   303,
     270,   299,    59,    -1,   272,    -1,   270,   300,   272,    -1,
     297,    -1,    21,    -1,    50,    -1,    12,    -1,    -1,   271,
     273,    60,   244,    -1,    -1,    24,   297,   274,    60,   243,
      -1,    -1,    14,   297,   275,    60,   243,    -1,    -1,    39,
     297,   276,    60,   243,    -1,    -1,    15,   297,   277,    60,
     243,    -1,    -1,    45,   297,   278,    60,   243,    -1,    27,
      60,    12,    -1,    35,    60,   297,    -1,    51,    60,   297,
      -1,    51,    60,    -1,    -1,    60,   280,    -1,   282,    -1,
      32,    -1,    61,   303,    62,    -1,    61,   303,   281,   301,
      62,    -1,   282,    -1,   281,   302,   282,    -1,    -1,     7,
     283,   284,    -1,    -1,   285,    -1,    -1,    64,   286,   303,
     287,    65,    -1,    -1,   288,   299,    -1,   289,    -1,   288,
     300,   289,    -1,   198,    -1,   290,    -1,    45,    16,    60,
     163,    -1,    -1,   292,    -1,     7,    -1,     7,    -1,     7,
      -1,   297,    -1,    71,    -1,     8,    -1,    10,    -1,    71,
      -1,     8,    -1,     9,    -1,    11,    -1,     8,    -1,    -1,
     300,    -1,    67,   303,    -1,   304,    -1,   303,    -1,   302,
      -1,    68,   303,    -1,    -1,   304,    -1,     3,    -1,   304,
       3,    -1
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
    2487,  2497,  2497,  2502,  2504,  2508,  2509,  2513,  2513,  2520,
    2532,  2534,  2535,  2539,  2540,  2544,  2545,  2546,  2550,  2555,
    2555,  2564,  2564,  2570,  2570,  2576,  2576,  2582,  2582,  2588,
    2588,  2596,  2603,  2610,  2618,  2623,  2630,  2632,  2636,  2641,
    2653,  2653,  2661,  2663,  2667,  2668,  2672,  2675,  2683,  2684,
    2688,  2689,  2693,  2699,  2709,  2709,  2717,  2719,  2723,  2724,
    2728,  2741,  2747,  2757,  2761,  2762,  2775,  2778,  2781,  2784,
    2795,  2801,  2804,  2807,  2812,  2825,  2825,  2834,  2838,  2839,
    2843,  2844,  2845,  2853,  2853,  2860,  2864,  2865,  2869,  2870,
    2878,  2882,  2886,  2890,  2897,  2897,  2909,  2924,  2924,  2934,
    2934,  2942,  2942,  2950,  2950,  2958,  2958,  2967,  2967,  2975,
    2975,  2989,  2990,  2993,  2995,  2996,  3000,  3001,  3005,  3006,
    3007,  3011,  3016,  3016,  3025,  3025,  3031,  3031,  3037,  3037,
    3043,  3043,  3049,  3049,  3057,  3064,  3072,  3077,  3084,  3086,
    3090,  3091,  3094,  3097,  3101,  3102,  3106,  3106,  3112,  3114,
    3118,  3118,  3144,  3146,  3150,  3151,  3156,  3158,  3162,  3175,
    3178,  3182,  3188,  3194,  3205,  3206,  3212,  3213,  3214,  3219,
    3220,  3225,  3226,  3229,  3231,  3235,  3236,  3240,  3241,  3245,
    3248,  3250,  3254,  3255
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
  "'.'", "'{'", "'}'", "':'", "';'", "','", "$accept", "menva_file",
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
  "connect_item", "time_samples_rhs", "$@63", "time_sample_list",
  "time_sample_list_int", "time_sample", "$@64",
  "attribute_metadata_list_opt", "attribute_metadata_list",
  "attribute_metadata_key", "attribute_metadata", "$@65", "$@66", "$@67",
  "$@68", "$@69", "$@70", "attribute_assignment_opt", "attribute_value",
  "typed_dictionary", "$@71", "typed_dictionary_list_opt",
  "typed_dictionary_list", "typed_dictionary_element", "dictionary_key",
  "dictionary_value_type", "dictionary_value_scalar_type",
  "dictionary_value_shaped_type", "string_dictionary", "$@72",
  "string_dictionary_list_opt", "string_dictionary_list",
  "string_dictionary_element", "metadata_listop_list", "metadata_value",
  "typed_value", "typed_value_atomic", "typed_value_list", "$@73",
  "typed_value_list_int", "typed_value_list_items",
  "typed_value_list_item", "typed_value_tuple", "$@74",
  "typed_value_tuple_int", "typed_value_tuple_items",
  "typed_value_tuple_item", "prim_relationship_type",
  "prim_relationship_time_samples", "$@75", "prim_relationship_default",
  "prim_relationship", "$@76", "$@77", "$@78", "$@79", "$@80", "$@81",
  "$@82", "relationship_metadata_list_opt", "relationship_metadata_list",
  "relationship_metadata_key", "relationship_metadata", "$@83", "$@84",
  "$@85", "$@86", "$@87", "$@88", "relationship_assignment_opt",
  "relationship_rhs", "relationship_target_list", "relationship_target",
  "$@89", "relational_attributes_opt", "relational_attributes", "$@90",
  "relational_attributes_list_opt", "relational_attributes_list",
  "relational_attributes_list_item", "relational_attributes_order_stmt",
  "prim_path_opt", "prim_path", "property_path",
  "prim_or_property_scene_path", "name", "namespaced_name", "identifier",
  "extended_number", "stmtsep_opt", "stmtsep", "listsep_opt", "listsep",
  "newlines_opt", "newlines", 0
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
      61,    91,    93,    46,   123,   125,    58,    59,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,    69,    70,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    72,    72,
      74,    73,    75,    75,    76,    76,    77,    77,    78,    79,
      80,    79,    81,    79,    82,    79,    83,    79,    84,    79,
      85,    79,    79,    79,    86,    86,    87,    87,    88,    89,
      90,    90,    91,    91,    92,    92,    93,    93,    95,    94,
      96,    94,    97,    94,    98,    94,    99,    94,   100,    94,
      94,   101,   101,   103,   102,   104,   104,   105,   105,   106,
     106,   107,   107,   107,   108,   109,   108,   110,   108,   111,
     108,   112,   108,   113,   108,   114,   108,   108,   108,   108,
     115,   108,   116,   108,   117,   108,   118,   108,   119,   108,
     120,   108,   121,   108,   122,   108,   123,   108,   124,   108,
     125,   108,   126,   108,   127,   108,   128,   108,   129,   108,
     130,   108,   131,   108,   132,   108,   133,   108,   134,   108,
     135,   108,   136,   108,   137,   108,   138,   108,   108,   108,
     108,   108,   108,   108,   108,   108,   108,   108,   108,   108,
     139,   139,   139,   139,   140,   140,   141,   142,   141,   143,
     143,   143,   144,   144,   145,   146,   146,   146,   146,   147,
     147,   148,   149,   148,   150,   150,   150,   151,   151,   152,
     152,   153,   153,   153,   153,   154,   154,   155,   156,   156,
     156,   156,   157,   157,   158,   159,   160,   160,   161,   161,
     162,   163,   163,   164,   164,   165,   166,   166,   167,   167,
     168,   168,   168,   168,   168,   170,   169,   171,   171,   173,
     172,   174,   175,   176,   176,   177,   177,   178,   179,   179,
     180,   180,   182,   183,   181,   185,   186,   184,   188,   187,
     189,   187,   190,   187,   191,   187,   192,   187,   193,   187,
     195,   194,   197,   196,   198,   198,   198,   198,   198,   200,
     199,   201,   201,   201,   202,   202,   204,   203,   205,   205,
     205,   206,   206,   207,   208,   208,   208,   208,   209,   209,
     210,   212,   211,   213,   213,   214,   214,   216,   215,   215,
     217,   217,   217,   218,   218,   219,   219,   219,   220,   221,
     220,   222,   220,   223,   220,   224,   220,   225,   220,   226,
     220,   220,   220,   220,   220,   220,   227,   227,   228,   228,
     230,   229,   231,   231,   232,   232,   233,   233,   234,   234,
     235,   235,   236,   237,   239,   238,   240,   240,   241,   241,
     242,   243,   243,   244,   244,   244,   245,   245,   245,   245,
     245,   246,   246,   246,   246,   248,   247,   249,   250,   250,
     251,   251,   251,   253,   252,   254,   255,   255,   256,   256,
     257,   257,   257,   257,   259,   258,   260,   262,   261,   263,
     261,   264,   261,   265,   261,   266,   261,   267,   261,   268,
     261,   261,   261,   269,   269,   269,   270,   270,   271,   271,
     271,   272,   273,   272,   274,   272,   275,   272,   276,   272,
     277,   272,   278,   272,   272,   272,   272,   272,   279,   279,
     280,   280,   280,   280,   281,   281,   283,   282,   284,   284,
     286,   285,   287,   287,   288,   288,   289,   289,   290,   291,
     291,   292,   293,   294,   295,   295,   296,   296,   296,   297,
     297,   298,   298,   299,   299,   300,   300,   301,   301,   302,
     303,   303,   304,   304
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
       1,     0,     5,     0,     2,     1,     3,     0,     4,     3,
       0,     3,     5,     1,     3,     1,     1,     1,     1,     0,
       4,     0,     5,     0,     5,     0,     5,     0,     5,     0,
       5,     3,     3,     3,     3,     2,     0,     2,     1,     1,
       0,     5,     0,     2,     1,     3,     4,     4,     1,     1,
       1,     1,     1,     3,     0,     5,     0,     2,     1,     3,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       1,     1,     1,     1,     1,     0,     4,     3,     1,     3,
       1,     1,     1,     0,     4,     3,     1,     3,     1,     1,
       1,     2,     3,     2,     0,     7,     6,     0,     5,     0,
       5,     0,     5,     0,     5,     0,     5,     0,     5,     0,
       7,     1,     1,     0,     3,     5,     1,     3,     1,     1,
       1,     1,     0,     4,     0,     5,     0,     5,     0,     5,
       0,     5,     0,     5,     3,     3,     3,     2,     0,     2,
       1,     1,     3,     5,     1,     3,     0,     3,     0,     1,
       0,     5,     0,     2,     1,     3,     1,     1,     4,     0,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     1,     2,     1,     1,     1,     2,
       0,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    50,     0,     2,   500,     1,   502,    51,    48,    52,
     501,    92,    88,    96,     0,   500,    86,   500,   503,   489,
     490,     0,    94,   101,     0,    90,     0,    98,     0,    49,
     501,     0,    54,   103,    93,     0,     0,    89,     0,    97,
       0,     0,    87,   500,    59,     0,     0,     0,     0,     0,
       0,     0,   493,    60,    56,    58,   500,   102,    95,    91,
      99,   245,   500,   100,   241,    53,    64,    68,    62,     0,
      66,    70,     0,   500,    55,   494,   496,     0,     0,   105,
       0,     0,     0,     0,    72,     0,     0,   500,    73,   495,
      57,     0,   500,   500,   500,   243,     0,     0,     0,     0,
       0,     0,   394,   390,   391,   392,   385,   403,   395,   360,
     383,    61,   384,   386,   388,   387,   393,     0,   246,     0,
     107,   500,     0,   498,   497,   381,   395,    65,   382,    69,
      63,    67,    71,    79,    74,   500,    76,    80,   500,   389,
     500,   500,   104,     0,     0,   266,     0,     0,     0,   410,
       0,   265,     0,     0,     0,   247,   248,     0,     0,     0,
       0,   267,     0,   270,     0,   295,   294,   296,   297,   298,
     263,     0,   431,   432,   264,   268,   500,   114,     0,     0,
     112,     0,     0,   142,     0,     0,   130,     0,     0,     0,
     166,     0,     0,   154,   113,     0,     0,     0,   493,   115,
     109,   111,   499,   242,   244,     0,   498,     0,    78,     0,
       0,     0,     0,   362,     0,     0,     0,     0,     0,   411,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     255,   413,   253,   249,   254,   251,   252,   250,   271,   486,
     487,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    26,    25,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,   488,   272,   417,     0,
     106,   146,   134,   170,   158,     0,   119,   150,   138,   174,
     162,     0,   123,   144,   132,   168,   156,     0,   117,     0,
       0,     0,     0,     0,     0,     0,   148,   136,   172,   160,
       0,   121,     0,     0,   152,   140,   176,   164,     0,   125,
       0,   187,     0,     0,   108,   494,     0,    75,    77,     0,
       0,   493,    82,   404,   408,   409,   500,   406,   396,   400,
     401,   500,   398,   402,     0,     0,   493,   364,     0,   370,
     371,   372,     0,   421,     0,   425,   412,   275,     0,   419,
       0,   423,     0,     0,     0,   427,     0,     0,   356,     0,
       0,   458,   269,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   127,     0,   128,   129,     0,   374,   188,   189,     0,
       0,     0,     0,     0,     0,     0,   500,   178,     0,     0,
       0,     0,     0,     0,     0,   186,   179,   180,   110,     0,
       0,     0,     0,   494,   405,   498,   397,   498,   368,   485,
       0,   369,   484,   361,   363,   494,     0,     0,     0,   458,
       0,   458,   356,     0,   458,     0,   458,   261,   262,     0,
     458,   500,     0,     0,     0,     0,   273,     0,     0,     0,
       0,   433,     0,     0,     0,     0,   182,     0,     0,     0,
       0,     0,   184,     0,     0,     0,     0,     0,   181,     0,
     481,   221,   500,   143,   222,   227,   197,   190,   500,   479,
     131,   191,   500,     0,     0,     0,     0,   183,     0,   212,
     205,   500,   479,   167,   206,   236,     0,     0,     0,     0,
     185,     0,   228,   500,   155,   229,   234,   116,    84,    85,
      81,    83,   407,   399,     0,   365,     0,   373,     0,   422,
       0,   426,   276,     0,   420,     0,   424,     0,   428,     0,
     278,     0,   292,   359,   357,   358,   330,   429,     0,   414,
     466,   461,   500,   459,   460,   500,   418,   147,   135,   171,
     159,   120,   151,   139,   175,   163,   124,   145,   133,   169,
     157,   118,     0,   199,     0,   199,   480,   376,   149,   137,
     173,   161,   122,   214,     0,   214,     0,     0,   500,   238,
     153,   141,   177,   165,   126,     0,   367,   366,   280,   284,
     330,   286,   282,   288,   500,     0,   482,     0,     0,   500,
     274,     0,   416,     0,   468,     0,     0,   223,   500,   225,
     500,   198,   192,   500,   194,   196,     0,     0,   500,   378,
     500,   213,   207,   500,   209,   211,     0,   235,   237,   498,
     230,   500,   232,     0,     0,   277,     0,     0,     0,     0,
     483,   314,   500,   279,   315,   320,     0,   321,   293,     0,
     470,   430,   415,   467,   469,   462,   500,   464,   441,     0,
       0,   439,     0,     0,     0,     0,     0,   440,     0,   434,
     493,   442,   436,   438,     0,   498,     0,     0,   498,     0,
     375,   377,   498,     0,     0,   498,   240,   239,     0,   498,
     281,   285,   287,   283,   289,   259,     0,   257,     0,   290,
     500,   338,     0,     0,   336,     0,     0,     0,     0,     0,
       0,   337,     0,   331,   493,   339,   333,   335,   500,     0,
     498,   446,   450,   444,     0,     0,   448,   452,   457,     0,
     494,     0,   224,   226,   200,   204,   493,   202,   193,   195,
     380,   379,     0,   215,   219,   493,   217,   208,   210,   231,
     233,   500,   256,   258,   316,   500,   318,     0,   323,   343,
     347,   341,     0,     0,     0,   345,   349,   355,     0,   494,
       0,   472,   463,   465,     0,     0,     0,   454,   455,     0,
       0,   456,   435,   437,     0,     0,   494,     0,     0,   494,
       0,     0,   498,   291,   299,   492,   491,     0,   500,   325,
       0,     0,     0,     0,   353,   351,   352,     0,     0,   354,
     332,   334,     0,     0,     0,     0,     0,     0,     0,   476,
       0,   493,   474,   477,     0,     0,     0,     0,     0,   443,
     201,   203,   220,   216,   218,   500,   317,   319,   308,   322,
     324,   498,   327,     0,     0,     0,     0,     0,   340,     0,
     471,   473,   494,   447,   451,   445,   449,   453,     0,   500,
     301,   326,   329,     0,   344,   348,   342,   346,   350,     0,
     475,   500,     0,   500,   300,   328,   478,   260,     0,   309,
     493,   311,     0,     0,     0,   494,   302,     0,   493,   304,
     313,   310,   312,   306,     0,   494,     0,   303,   305,     0,
     307
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,   286,     7,     3,     4,     8,    31,    52,    53,
      54,    77,    83,    81,    85,    82,    86,    88,   135,   136,
     499,   208,   341,   755,    15,   154,    24,    38,    21,    36,
      26,    40,    22,    34,    56,    78,   119,   198,   199,   200,
     336,   400,   388,   414,   394,   423,   313,   396,   384,   410,
     390,   419,   310,   395,   383,   409,   389,   418,   330,   398,
     386,   412,   392,   421,   322,   397,   385,   411,   391,   420,
     500,   633,   501,   583,   631,   756,   757,   513,   643,   514,
     593,   641,   765,   766,   493,   628,   494,   524,   651,   525,
     417,   597,   598,   599,    63,    94,    64,   117,   155,   156,
     157,   376,   716,   717,   771,   158,   159,   160,   161,   162,
     163,   164,   165,   378,   556,   166,   452,   610,   167,   615,
     653,   657,   654,   656,   658,   168,   777,   169,   618,   170,
     813,   858,   894,   908,   909,   916,   880,   900,   901,   663,
     775,   664,   668,   720,   817,   818,   819,   883,   620,   734,
     735,   736,   790,   823,   821,   827,   822,   828,   466,   554,
     110,   141,   355,   356,   357,   440,   358,   359,   360,   407,
     502,   637,   638,   639,   127,   111,   112,   113,   128,   140,
     211,   351,   352,   115,   138,   209,   346,   347,   171,   172,
     623,   173,   174,   381,   454,   449,   456,   451,   460,   621,
     566,   690,   691,   692,   751,   796,   794,   799,   795,   800,
     471,   563,   676,   564,   624,   673,   671,   738,   840,   841,
     842,   843,   585,   495,   617,   665,   441,   287,   175,   820,
      74,    75,   122,   123,   124,    10
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -762
static const yytype_int16 yypact[] =
{
      45,  -762,    81,  -762,   103,  -762,  -762,  -762,   227,    74,
     113,   304,   304,   304,    78,   103,  -762,   103,  -762,  -762,
    -762,   125,   110,  -762,   125,   110,   125,   110,   136,  -762,
     406,   141,   865,  -762,  -762,   304,   125,  -762,   125,  -762,
     125,    67,  -762,   103,  -762,   304,   304,   304,   144,   304,
     304,   150,    40,  -762,  -762,  -762,   103,  -762,  -762,  -762,
    -762,  -762,   103,  -762,  -762,  -762,  -762,  -762,  -762,   204,
    -762,  -762,   158,   103,  -762,   865,   113,   174,   183,   193,
     248,   218,   224,   257,  -762,   278,   284,   103,  -762,  -762,
    -762,    62,   103,   103,    37,  -762,   123,   123,   123,   123,
     123,    48,  -762,  -762,  -762,  -762,  -762,  -762,   219,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,   254,   362,   290,
     582,   103,   295,   248,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,    37,  -762,   301,   103,  -762,
     103,   103,  -762,   347,   347,  -762,   390,   347,   347,  -762,
     437,  -762,   349,   319,   103,   362,  -762,   103,    40,    40,
      40,  -762,   304,  -762,   978,  -762,  -762,  -762,  -762,  -762,
    -762,   978,  -762,  -762,  -762,   307,   103,  -762,   267,   479,
    -762,   533,   309,  -762,   315,   328,  -762,   334,   342,   645,
    -762,   343,   651,  -762,  -762,   346,   350,   351,    40,  -762,
    -762,  -762,  -762,  -762,  -762,   311,   407,   192,  -762,   363,
     229,   359,   163,   280,   234,   978,   978,   978,   978,  -762,
     392,   978,   978,   978,   978,   978,   379,   381,   978,   978,
    -762,  -762,   113,  -762,   113,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,   389,   239,   394,
    -762,  -762,  -762,  -762,  -762,   410,  -762,  -762,  -762,  -762,
    -762,   411,  -762,  -762,  -762,  -762,  -762,   412,  -762,   454,
     413,   463,   304,   419,   417,   417,  -762,  -762,  -762,  -762,
     422,  -762,   425,   427,  -762,  -762,  -762,  -762,   429,  -762,
     432,   304,   431,    67,  -762,   582,   433,  -762,  -762,   436,
     440,    40,  -762,  -762,  -762,  -762,    37,  -762,  -762,  -762,
    -762,    37,  -762,  -762,   928,   447,    40,  -762,   928,  -762,
    -762,   441,   438,  -762,   453,  -762,  -762,  -762,   456,  -762,
     458,  -762,    67,    67,   465,  -762,   457,   157,   466,   522,
     156,   470,  -762,   472,   473,   478,   486,    67,   489,   491,
     492,   497,   498,    67,   506,   508,   510,   511,   512,    67,
     513,  -762,    58,  -762,  -762,   132,  -762,  -762,  -762,   515,
     516,   519,   520,    67,   524,   153,   103,  -762,   525,   527,
     528,   532,    67,   535,   133,  -762,  -762,  -762,  -762,    62,
     526,   587,   534,   192,  -762,   229,  -762,   163,  -762,  -762,
     539,  -762,  -762,  -762,  -762,   280,   540,   546,   583,   470,
     594,   470,   466,   603,   470,   606,   470,  -762,  -762,   607,
     470,   103,   541,   567,   569,   221,  -762,   572,   571,   575,
     154,   579,    58,   132,   153,   133,  -762,   123,    58,   132,
     153,   133,  -762,   123,    58,   132,   153,   133,  -762,   123,
    -762,  -762,   103,  -762,  -762,  -762,  -762,  -762,   103,   632,
    -762,  -762,   103,    58,   132,   153,   133,  -762,   123,  -762,
    -762,   103,   632,  -762,  -762,   633,    58,   132,   153,   133,
    -762,   123,  -762,   103,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,   431,  -762,   145,  -762,   581,  -762,
     584,  -762,  -762,   585,  -762,   588,  -762,   589,  -762,   578,
    -762,   636,  -762,  -762,  -762,  -762,   592,  -762,   640,  -762,
    -762,  -762,   103,  -762,  -762,   103,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,    51,   593,    38,   593,  -762,   652,  -762,  -762,
    -762,  -762,  -762,   599,    60,   599,   586,   598,    37,  -762,
    -762,  -762,  -762,  -762,  -762,    68,  -762,  -762,  -762,  -762,
     592,  -762,  -762,  -762,   103,   159,  -762,   604,   601,   103,
    -762,   612,  -762,   601,   612,    80,   313,  -762,    37,  -762,
     103,  -762,  -762,    37,  -762,  -762,   611,   602,    37,  -762,
     103,  -762,  -762,    37,  -762,  -762,   663,  -762,  -762,   633,
    -762,    37,  -762,   159,   159,  -762,   159,   159,   159,   666,
    -762,  -762,   103,  -762,  -762,  -762,   623,  -762,  -762,   259,
    -762,  -762,  -762,  -762,  -762,  -762,    37,  -762,  -762,   304,
     304,  -762,   304,   624,   625,   304,   304,  -762,   628,  -762,
      40,  -762,  -762,  -762,   629,   632,   164,   634,   236,   682,
    -762,  -762,   652,   477,   635,   326,  -762,  -762,   638,   632,
    -762,  -762,  -762,  -762,  -762,  -762,    66,  -762,    96,  -762,
     103,  -762,   304,   304,  -762,   304,   642,   643,   646,   304,
     304,  -762,   648,  -762,    40,  -762,  -762,  -762,   103,   647,
     688,  -762,  -762,  -762,   693,   304,  -762,  -762,   304,   654,
     836,   650,  -762,  -762,  -762,  -762,    40,  -762,  -762,  -762,
    -762,  -762,   655,  -762,  -762,    40,  -762,  -762,  -762,  -762,
    -762,   103,  -762,  -762,  -762,    37,  -762,  1028,   184,  -762,
    -762,  -762,   304,   704,   304,  -762,  -762,   304,   661,   814,
     662,   321,  -762,  -762,   665,   667,   668,  -762,  -762,   670,
     671,  -762,  -762,  -762,    62,   664,   192,   431,   673,    39,
     669,   674,   719,  -762,  -762,  -762,  -762,   675,    37,  -762,
     672,   681,   684,   687,  -762,  -762,  -762,   690,   691,  -762,
    -762,  -762,    62,   237,   237,   237,   237,   237,   298,  -762,
     677,    40,  -762,  -762,   123,   123,   123,   123,   123,  -762,
    -762,  -762,  -762,  -762,  -762,   103,  -762,  -762,   694,  -762,
    -762,   184,   703,   123,   123,   123,   123,   123,  -762,   698,
    -762,  -762,   321,  -762,  -762,  -762,  -762,  -762,   683,   103,
     689,  -762,  -762,   145,  -762,  -762,  -762,  -762,  -762,    67,
    -762,   103,   139,   103,  -762,  -762,  -762,  -762,   699,  -762,
      40,  -762,    47,   431,   695,   710,  -762,  1028,    40,  -762,
    -762,  -762,  -762,  -762,   696,   304,   705,  -762,  -762,   145,
    -762
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -762,  -762,  -340,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
     692,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,   557,
     -89,  -762,  -762,  -197,  -762,    33,  -762,  -762,  -762,  -762,
    -762,  -762,   341,   424,  -762,    -7,  -762,  -762,  -762,   434,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
     -92,  -762,  -553,  -762,   185,  -762,   -38,   -91,  -762,  -564,
    -762,   177,  -762,   -35,   -40,  -762,  -562,   -22,  -762,  -573,
    -762,  -762,  -762,   126,  -311,  -762,   -32,   -78,  -762,   626,
    -762,  -762,  -762,    63,  -762,  -762,  -762,  -762,  -762,  -762,
    -159,   -51,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -754,
    -762,  -762,  -762,  -762,  -127,  -762,  -762,  -762,  -116,   -94,
    -762,  -685,   167,  -762,  -762,  -762,   -70,  -762,   182,  -762,
    -762,     5,  -762,  -762,  -762,  -762,  -762,  -762,   352,  -762,
    -321,  -762,  -762,  -762,   357,   445,  -762,  -762,  -762,   480,
    -762,  -762,  -762,   104,   -47,  -412,  -452,  -187,   -87,  -762,
    -762,  -762,   368,  -183,  -762,  -762,  -762,   372,   149,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,    59,  -762,  -762,  -762,  -762,  -762,  -762,
      94,  -762,  -762,  -597,  -762,  -762,   186,  -762,  -762,  -762,
     -64,  -762,   299,  -398,  -762,  -762,  -761,    41,   -11,  -762,
    -179,  -151,  -129,  -120,    42,   -10
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
      23,    23,    23,   238,   114,    30,   205,   235,   236,   237,
     342,   426,   137,   555,   439,   206,   814,   527,   439,   334,
     629,    55,   427,   344,    57,   349,   526,   345,   677,   353,
     644,   634,   652,   776,    66,    67,    68,   839,    70,    71,
       6,    16,    76,     6,   133,   496,     9,   335,    95,     1,
     129,   130,   131,   132,   133,    19,    20,    29,   490,    32,
     762,   457,   458,    42,    55,   490,   133,   509,   102,   103,
      19,    20,   339,   104,   105,   490,   476,   526,   715,    61,
     116,     5,   482,   526,   607,    65,   340,   560,   488,   526,
     491,   204,   215,   217,   106,   221,   222,   224,    79,   228,
     632,   586,   507,   660,    80,   121,     6,    73,   526,   201,
     134,   520,   906,   627,   586,    89,    18,   137,   839,   492,
     107,   526,   642,   108,    28,   350,   109,   857,    62,   101,
     650,   772,    17,   753,   118,   120,   770,    33,   133,   496,
     490,   768,   675,   793,   232,   759,   913,   234,    76,    76,
      76,   102,   103,    19,    20,   125,   104,   105,   774,   133,
     509,   560,   432,   202,   497,   522,   660,   296,   302,   102,
     308,    19,    20,    35,   104,   105,   462,   444,   321,   468,
     210,   329,   212,   213,   126,   510,   561,   463,    76,   898,
     433,   661,   815,   498,   523,   816,    41,   339,   899,   116,
      43,   116,   361,   107,    69,   445,   108,   526,   469,   464,
      72,   340,   288,   606,   511,   562,    84,   434,   290,    87,
     662,   107,   436,   754,   126,   339,   435,   102,   103,    19,
      20,   437,   104,   105,    91,   102,   531,    19,    20,   340,
     104,   105,   133,   496,    11,    19,    20,    92,   344,    12,
     349,    93,   345,   553,   353,   145,   362,   363,   364,   365,
      61,    13,   367,   368,   369,   370,   371,    19,    20,   374,
     375,   721,    14,   722,   723,    19,    20,   219,    96,   107,
     724,   139,   108,   725,    97,   726,   727,   107,    19,    20,
     151,   220,   216,   218,   728,   291,   223,   225,   729,   229,
     379,   404,   380,   292,   730,   354,    19,    20,   293,   731,
     732,   526,    19,    20,   869,   294,   145,    98,   733,   142,
     425,    19,    20,   295,   201,   678,   512,   679,   680,    19,
      20,    76,   133,   509,   681,   833,   834,   682,    99,   145,
     683,   835,   114,   442,   100,   836,    76,   442,   684,   176,
     350,   151,   685,    25,    27,    19,    20,   203,   686,   207,
     837,   230,   231,   687,   688,   145,   838,   214,   289,   309,
      19,    20,   689,   337,   151,   311,   143,   144,   114,    11,
     145,   568,   146,   569,    12,   512,   147,   573,   312,   574,
     149,   512,   849,   578,   314,   579,    13,   512,    19,    20,
     151,   148,   315,   323,   153,   149,   331,   150,   145,    18,
     332,   333,   589,   133,   590,   151,   512,   152,   116,   153,
     868,   348,   343,    11,   116,   601,   116,   602,    12,   512,
     571,   895,   567,   219,   361,   366,   576,   439,   572,   372,
      13,   373,   581,   151,   577,    19,    20,   220,    37,   114,
      39,    14,   377,   570,   116,   145,   382,   214,   515,   575,
      58,   592,    59,   588,    60,   580,   401,   920,   226,   648,
     387,   393,   399,   402,   604,   403,   600,   227,   649,   405,
     149,   406,   413,    28,   591,   415,   852,    19,    20,   422,
     151,   416,   424,   429,   153,   109,   430,   603,   762,   694,
     431,   448,   447,   549,   697,   512,   764,   297,   695,   701,
     339,   749,   443,   698,   704,   298,   450,   461,   702,   453,
     299,   455,   708,   705,   340,   116,   465,   300,   459,   467,
     470,   709,   472,   473,   582,   301,   763,   528,   474,   750,
     584,    19,    20,   539,   587,   541,   475,   739,   544,   477,
     546,   478,   479,   594,   548,   788,   740,   480,   481,   710,
     711,   303,   712,   713,   714,   605,   483,   439,   484,   304,
     485,   486,   487,   489,   305,   503,   504,   805,   896,   505,
     506,   306,   910,   789,   508,   516,   808,   517,   518,   307,
      19,    20,   519,   530,   177,   521,   178,   179,   529,   534,
     536,   550,   538,   180,   625,   806,   181,   626,   537,   182,
     183,   184,   764,   540,   809,   693,   512,   185,   186,   187,
     188,   189,   543,   190,   191,   545,   547,   192,   551,   552,
     193,   558,   194,   195,   557,   559,   196,   565,   197,   490,
     596,   608,   614,   616,   609,   611,   811,   622,   612,   613,
     619,   630,   646,    19,    20,   812,   659,   640,   737,    19,
      20,   669,   871,   647,   636,   667,   666,   700,   741,   742,
     706,   743,   696,   316,   746,   747,   670,   699,   715,   324,
      76,   317,   703,   719,   744,   745,   318,   325,   748,   860,
     872,   752,   326,   319,   760,   560,   758,   767,   861,   327,
     769,   320,   782,   783,   718,   797,   784,   328,   787,   792,
     804,   779,   780,   802,   781,   807,   825,   114,   785,   786,
     830,   904,   832,   850,    76,   844,   660,   845,   846,   914,
     847,   848,   853,   855,   798,   882,   856,   801,   862,   693,
     859,   863,   870,   907,   864,   114,    76,   865,   891,   905,
     866,   867,   879,   893,   911,    76,   907,   915,   889,   903,
     898,   917,   778,   338,   810,   919,   442,    90,   851,   428,
     635,   824,   645,   826,   854,   707,   829,   878,   737,   773,
     791,   233,   215,   217,   221,   222,   224,   228,   918,   912,
     672,   881,   655,   116,   831,   408,   114,   873,   874,   875,
     876,   877,   535,   446,   542,   533,   761,   532,   890,   803,
     674,   595,     0,    79,     0,     0,   884,   885,   886,   887,
     888,   116,    19,    20,     0,     0,   721,     0,   722,   723,
       0,    76,   114,     0,     0,   724,     0,     0,   725,     0,
     726,   727,     0,     0,    19,    20,     0,     0,   678,   728,
     679,   680,     0,   729,     0,     0,     0,   681,     0,   730,
     682,     0,     0,   683,   731,   732,     0,     0,     0,     0,
       0,   684,   116,    19,    20,   685,     0,    44,     0,    45,
      46,   686,     0,     0,     0,     0,   687,   688,     0,    47,
      76,     0,    48,     0,     0,     0,   442,   118,    76,     0,
       0,     0,     0,     0,    49,     0,     0,     0,   116,     0,
      50,     0,     0,     0,    51,     0,     0,     0,     0,     0,
       0,   892,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   897,     0,   902,    19,    20,     0,     0,
     438,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   285,   239,     0,   240,     0,
       0,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   285,    19,    20,     0,     0,
       0,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   285
};

static const yytype_int16 yycheck[] =
{
      11,    12,    13,   162,    91,    15,   135,   158,   159,   160,
     207,   332,   101,   465,   354,   135,   777,   429,   358,   198,
     582,    32,   333,   210,    35,   212,   424,   210,   625,   212,
     594,   584,   605,   718,    45,    46,    47,   791,    49,    50,
       3,     8,    52,     3,     6,     7,     4,   198,    80,     4,
      97,    98,    99,   100,     6,     8,     9,    15,     7,    17,
      21,   372,   373,    30,    75,     7,     6,     7,     6,     7,
       8,     9,    33,    11,    12,     7,   387,   475,    12,    12,
      91,     0,   393,   481,   536,    43,    47,     7,   399,   487,
      32,   123,   143,   144,    32,   146,   147,   148,    56,   150,
      62,   499,   413,     7,    62,    68,     3,    67,   506,   120,
      62,   422,    65,    62,   512,    73,     3,   206,   872,    61,
      58,   519,    62,    61,    46,   212,    64,   812,    61,    87,
      62,    65,    58,   695,    92,    93,   709,    12,     6,     7,
       7,   705,    62,   740,   154,   698,   907,   157,   158,   159,
     160,     6,     7,     8,     9,    32,    11,    12,    62,     6,
       7,     7,   341,   121,    32,    32,     7,   178,   179,     6,
     181,     8,     9,    63,    11,    12,    19,   356,   189,    23,
     138,   192,   140,   141,    61,    32,    32,    30,   198,    50,
     341,    32,     8,    61,    61,    11,    60,    33,    59,   210,
      59,   212,   213,    58,    60,   356,    61,   605,    52,    52,
      60,    47,   171,   534,    61,    61,    12,   346,   176,    61,
      61,    58,   351,    59,    61,    33,   346,     6,     7,     8,
       9,   351,    11,    12,    60,     6,   433,     8,     9,    47,
      11,    12,     6,     7,    17,     8,     9,    64,   435,    22,
     437,    58,   435,    32,   437,    18,   215,   216,   217,   218,
      12,    34,   221,   222,   223,   224,   225,     8,     9,   228,
     229,    12,    45,    14,    15,     8,     9,    43,    60,    58,
      21,    62,    61,    24,    60,    26,    27,    58,     8,     9,
      53,    57,   143,   144,    35,    28,   147,   148,    39,   150,
      61,   312,    63,    36,    45,    25,     8,     9,    41,    50,
      51,   709,     8,     9,    16,    48,    18,    60,    59,    65,
     331,     8,     9,    56,   335,    12,   415,    14,    15,     8,
       9,   341,     6,     7,    21,    14,    15,    24,    60,    18,
      27,    20,   429,   354,    60,    24,   356,   358,    35,    59,
     437,    53,    39,    12,    13,     8,     9,    62,    45,    58,
      39,    12,    43,    50,    51,    18,    45,    20,    61,    60,
       8,     9,    59,    62,    53,    60,    14,    15,   465,    17,
      18,   473,    20,   474,    22,   474,    24,   479,    60,   480,
      43,   480,   804,   485,    60,   486,    34,   486,     8,     9,
      53,    39,    60,    60,    57,    43,    60,    45,    18,     3,
      60,    60,   504,     6,   505,    53,   505,    55,   429,    57,
     832,    62,    59,    17,   435,   517,   437,   518,    22,   518,
     477,   883,   472,    43,   445,    43,   483,   777,   478,    60,
      34,    60,   489,    53,   484,     8,     9,    57,    24,   536,
      26,    45,    63,   475,   465,    18,    62,    20,   416,   481,
      36,   508,    38,   503,    40,   487,    12,   919,    31,   598,
      60,    60,    60,    60,   521,    12,   516,    40,   598,    60,
      43,    64,    60,    46,   506,    60,   807,     8,     9,    60,
      53,    64,    60,    60,    57,    64,    60,   519,    21,   628,
      60,    63,    61,   461,   633,   594,   703,    28,   628,   638,
      33,   690,    65,   633,   643,    36,    63,    60,   638,    63,
      41,    63,   651,   643,    47,   536,    60,    48,    63,     7,
      60,   651,    60,    60,   492,    56,    59,    11,    60,   690,
     498,     8,     9,   449,   502,   451,    60,   676,   454,    60,
     456,    60,    60,   511,   460,   734,   676,    60,    60,   653,
     654,    28,   656,   657,   658,   523,    60,   907,    60,    36,
      60,    60,    60,    60,    41,    60,    60,   756,   889,    60,
      60,    48,   903,   734,    60,    60,   765,    60,    60,    56,
       8,     9,    60,    59,    12,    60,    14,    15,    11,    60,
      60,    60,    19,    21,   562,   756,    24,   565,    62,    27,
      28,    29,   809,    19,   765,   626,   705,    35,    36,    37,
      38,    39,    19,    41,    42,    19,    19,    45,    61,    60,
      48,    60,    50,    51,    62,    60,    54,    58,    56,     7,
       7,    60,    64,     7,    60,    60,   775,     7,    60,    60,
      58,    58,    66,     8,     9,   775,   614,    58,   669,     8,
       9,   619,   841,    65,    12,    64,    62,    65,   679,   680,
       7,   682,   630,    28,   685,   686,    64,    66,    12,    28,
     690,    36,   640,    60,    60,    60,    41,    36,    60,   818,
     841,    62,    41,    48,    12,     7,    62,    62,   818,    48,
      62,    56,    60,    60,   662,    12,    60,    56,    60,    62,
      60,   722,   723,    59,   725,    60,    12,   804,   729,   730,
      59,   900,    60,    59,   734,    60,     7,    60,    60,   908,
      60,    60,    59,    64,   745,    32,    62,   748,    66,   750,
      65,    60,    65,   902,    60,   832,   756,    60,    65,   900,
      60,    60,    58,    64,    59,   765,   915,   908,    60,    60,
      50,    65,   720,   206,   771,    60,   777,    75,   806,   335,
     585,   782,   595,   784,   809,   649,   787,   855,   789,   716,
     738,   155,   833,   834,   835,   836,   837,   838,   915,   905,
     623,   861,   610,   804,   789,   315,   883,   844,   845,   846,
     847,   848,   445,   358,   452,   437,   702,   435,   872,   750,
     624,   512,    -1,   771,    -1,    -1,   863,   864,   865,   866,
     867,   832,     8,     9,    -1,    -1,    12,    -1,    14,    15,
      -1,   841,   919,    -1,    -1,    21,    -1,    -1,    24,    -1,
      26,    27,    -1,    -1,     8,     9,    -1,    -1,    12,    35,
      14,    15,    -1,    39,    -1,    -1,    -1,    21,    -1,    45,
      24,    -1,    -1,    27,    50,    51,    -1,    -1,    -1,    -1,
      -1,    35,   883,     8,     9,    39,    -1,    12,    -1,    14,
      15,    45,    -1,    -1,    -1,    -1,    50,    51,    -1,    24,
     900,    -1,    27,    -1,    -1,    -1,   907,   855,   908,    -1,
      -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,   919,    -1,
      45,    -1,    -1,    -1,    49,    -1,    -1,    -1,    -1,    -1,
      -1,   879,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   891,    -1,   893,     8,     9,    -1,    -1,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,     8,    -1,    10,    -1,
      -1,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,     8,     9,    -1,    -1,
      -1,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     4,    70,    73,    74,     0,     3,    72,    75,   303,
     304,    17,    22,    34,    45,    93,    94,    58,     3,     8,
       9,    97,   101,   297,    95,   101,    99,   101,    46,   303,
     304,    76,   303,    12,   102,    63,    98,   102,    96,   102,
     100,    60,    94,    59,    12,    14,    15,    24,    27,    39,
      45,    49,    77,    78,    79,   297,   103,   297,   102,   102,
     102,    12,    61,   163,   165,   303,   297,   297,   297,    60,
     297,   297,    60,    67,   299,   300,   304,    80,   104,   303,
     303,    82,    84,    81,    12,    83,    85,    61,    86,   303,
      79,    60,    64,    58,   164,   165,    60,    60,    60,    60,
      60,   303,     6,     7,    11,    12,    32,    58,    61,    64,
     229,   244,   245,   246,   247,   252,   297,   166,   303,   105,
     303,    68,   301,   302,   303,    32,    61,   243,   247,   243,
     243,   243,   243,     6,    62,    87,    88,    89,   253,    62,
     248,   230,    65,    14,    15,    18,    20,    24,    39,    43,
      45,    53,    55,    57,    94,   167,   168,   169,   174,   175,
     176,   177,   178,   179,   180,   181,   184,   187,   194,   196,
     198,   257,   258,   260,   261,   297,    59,    12,    14,    15,
      21,    24,    27,    28,    29,    35,    36,    37,    38,    39,
      41,    42,    45,    48,    50,    51,    54,    56,   106,   107,
     108,   297,   303,    62,   165,   301,   302,    58,    90,   254,
     303,   249,   303,   303,    20,   180,   257,   180,   257,    43,
      57,   180,   180,   257,   180,   257,    31,    40,   180,   257,
      12,    43,   304,   168,   304,   300,   300,   300,   179,     8,
      10,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    71,   296,   296,    61,
     303,    28,    36,    41,    48,    56,   297,    28,    36,    41,
      48,    56,   297,    28,    36,    41,    48,    56,   297,    60,
     121,    60,    60,   115,    60,    60,    28,    36,    41,    48,
      56,   297,   133,    60,    28,    36,    41,    48,    56,   297,
     127,    60,    60,    60,   299,   300,   109,    62,    88,    33,
      47,    91,    92,    59,   246,   252,   255,   256,    62,   246,
     247,   250,   251,   252,    25,   231,   232,   233,   235,   236,
     237,   297,   296,   296,   296,   296,    43,   296,   296,   296,
     296,   296,    60,    60,   296,   296,   170,    63,   182,    61,
      63,   262,    62,   123,   117,   135,   129,    60,   111,   125,
     119,   137,   131,    60,   113,   122,   116,   134,   128,    60,
     110,    12,    60,    12,   297,    60,    64,   238,   238,   124,
     118,   136,   130,    60,   112,    60,    64,   159,   126,   120,
     138,   132,    60,   114,    60,   297,   229,   163,   108,    60,
      60,    60,   299,   300,   301,   302,   301,   302,    12,    71,
     234,   295,   297,    65,   299,   300,   234,    61,    63,   264,
      63,   266,   185,    63,   263,    63,   265,   163,   163,    63,
     267,    60,    19,    30,    52,    60,   227,     7,    23,    52,
      60,   279,    60,    60,    60,    60,   163,    60,    60,    60,
      60,    60,   163,    60,    60,    60,    60,    60,   163,    60,
       7,    32,    61,   153,   155,   292,     7,    32,    61,    89,
     139,   141,   239,    60,    60,    60,    60,   163,    60,     7,
      32,    61,    89,   146,   148,   303,    60,    60,    60,    60,
     163,    60,    32,    61,   156,   158,   292,   244,    11,    11,
      59,    92,   256,   251,    60,   233,    60,    62,    19,   279,
      19,   279,   227,    19,   279,    19,   279,    19,   279,   303,
      60,    61,    60,    32,   228,   245,   183,    62,    60,    60,
       7,    32,    61,   280,   282,    58,   269,   153,   139,   146,
     156,   243,   153,   139,   146,   156,   243,   153,   139,   146,
     156,   243,   303,   142,   303,   291,   292,   303,   153,   139,
     146,   156,   243,   149,   303,   291,     7,   160,   161,   162,
     153,   139,   146,   156,   243,   303,   229,   245,    60,    60,
     186,    60,    60,    60,    64,   188,     7,   293,   197,    58,
     217,   268,     7,   259,   283,   303,   303,    62,   154,   155,
      58,   143,    62,   140,   141,   143,    12,   240,   241,   242,
      58,   150,    62,   147,   148,   150,    66,    65,   301,   302,
      62,   157,   158,   189,   191,   217,   192,   190,   193,   303,
       7,    32,    61,   208,   210,   294,    62,    64,   211,   303,
      64,   285,   211,   284,   285,    62,   281,   282,    12,    14,
      15,    21,    24,    27,    35,    39,    45,    50,    51,    59,
     270,   271,   272,   297,   301,   302,   303,   301,   302,    66,
      65,   301,   302,   303,   301,   302,     7,   162,   301,   302,
     208,   208,   208,   208,   208,    12,   171,   172,   303,    60,
     212,    12,    14,    15,    21,    24,    26,    27,    35,    39,
      45,    50,    51,    59,   218,   219,   220,   297,   286,   301,
     302,   297,   297,   297,    60,    60,   297,   297,    60,   299,
     300,   273,    62,   155,    59,    92,   144,   145,    62,   141,
      12,   242,    21,    59,    92,   151,   152,    62,   148,    62,
     158,   173,    65,   172,    62,   209,   210,   195,   303,   297,
     297,   297,    60,    60,    60,   297,   297,    60,   299,   300,
     221,   303,    62,   282,   275,   277,   274,    12,   297,   276,
     278,   297,    59,   272,    60,   299,   300,    60,   299,   300,
     104,   301,   302,   199,   295,     8,    11,   213,   214,   215,
     298,   223,   225,   222,   297,    12,   297,   224,   226,   297,
      59,   220,    60,    14,    15,    20,    24,    39,    45,   198,
     287,   288,   289,   290,    60,    60,    60,    60,    60,   244,
      59,   145,   229,    59,   152,    64,    62,   210,   200,    65,
     301,   302,    66,    60,    60,    60,    60,    60,   244,    16,
      65,   299,   300,   243,   243,   243,   243,   243,   166,    58,
     205,   215,    32,   216,   243,   243,   243,   243,   243,    60,
     289,    65,   303,    64,   201,   245,   163,   303,    50,    59,
     206,   207,   303,    60,   299,   300,    65,   179,   202,   203,
     229,    59,   207,   295,   299,   300,   204,    65,   203,    60,
     245
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
#line 2497 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples = SdfTimeSampleMap();
    ;}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 2513 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (2)]).Get<double>();
    ;}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 2516 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples[ context->timeSampleTime ] = context->currentValue;
    ;}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 2520 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (3)]).Get<double>();
        context->timeSamples[ context->timeSampleTime ] 
            = VtValue(SdfValueBlock());  
    ;}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 2550 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 2555 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeAttribute, context);
        ;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2557 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 2564 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 2567 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 2570 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 2573 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 2576 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 2579 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 2582 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 2585 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 2588 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 2591 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 2596 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 2603 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 2610 "pxr/usd/sdf/textFileFormat.yy"
    {
             _SetField(
                 context->path, SdfFieldKeys->DisplayUnit,
                 _GetDisplayUnitFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                 context);
        ;}
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 2618 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 355:

/* Line 1455 of yacc.c  */
#line 2623 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken(), context);
        ;}
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 2636 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfFieldKeys->Default,
            context->currentValue, context);
    ;}
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 2641 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfFieldKeys->Default,
            SdfValueBlock(), context);
    ;}
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 2653 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2656 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2672 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertValue((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2675 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertDictionary((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2693 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory((yyvsp[(1) - (1)]), context);
    ;}
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 2699 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitShapedFactory((yyvsp[(1) - (3)]), context);
    ;}
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 2709 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 375:

/* Line 1455 of yacc.c  */
#line 2712 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 2728 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory(Value(std::string("string")), context);
            _ValueAppendAtomic((yyvsp[(3) - (3)]), context);
            _ValueSetAtomic(context);
            _DictionaryInsertValue((yyvsp[(1) - (3)]), context);
        ;}
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 2741 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->currentValue = VtValue();
        if (context->values.IsRecordingString()) {
            context->values.SetRecordedString("None");
        }
    ;}
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 2747 "pxr/usd/sdf/textFileFormat.yy"
    {
        _ValueSetList(context);
    ;}
    break;

  case 383:

/* Line 1455 of yacc.c  */
#line 2757 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->currentValue.Swap(context->currentDictionaries[0]);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 2762 "pxr/usd/sdf/textFileFormat.yy"
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

  case 386:

/* Line 1455 of yacc.c  */
#line 2775 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetAtomic(context);
        ;}
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2778 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetTuple(context);
        ;}
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2781 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetList(context);
        ;}
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 2784 "pxr/usd/sdf/textFileFormat.yy"
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

  case 390:

/* Line 1455 of yacc.c  */
#line 2795 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetCurrentToSdfPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2801 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2804 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2807 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs identifiers to be stored as TfToken
            // instead of std::string to be able to distinguish between them.
            _ValueAppendAtomic(TfToken((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2812 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs asset paths to be stored as
            // SdfAssetPath instead of std::string to be able to distinguish
            // between them
            _ValueAppendAtomic(SdfAssetPath((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2825 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginList();
        ;}
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 2828 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndList();
        ;}
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 2853 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginTuple();
        ;}
    break;

  case 404:

/* Line 1455 of yacc.c  */
#line 2855 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndTuple();
        ;}
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 2878 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 411:

/* Line 1455 of yacc.c  */
#line 2882 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 412:

/* Line 1455 of yacc.c  */
#line 2886 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 413:

/* Line 1455 of yacc.c  */
#line 2890 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 414:

/* Line 1455 of yacc.c  */
#line 2897 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context); 
        ;}
    break;

  case 415:

/* Line 1455 of yacc.c  */
#line 2900 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 416:

/* Line 1455 of yacc.c  */
#line 2909 "pxr/usd/sdf/textFileFormat.yy"
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

  case 417:

/* Line 1455 of yacc.c  */
#line 2924 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (2)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 2929 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeExplicit, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 2934 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 2937 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeDeleted, context); 
            _PrimEndRelationship(context);
        ;}
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2942 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 2946 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAdded, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2950 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 2954 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypePrepended, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 2958 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 2962 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAppended, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 2967 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 2970 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeOrdered, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 429:

/* Line 1455 of yacc.c  */
#line 2975 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context);
            context->relParsingAllowTargetData = true;
            _RelationshipAppendTargetPath((yyvsp[(4) - (5)]), context);
            _RelationshipInitTarget(context->relParsingTargetPaths->back(),
                                    context);
        ;}
    break;

  case 430:

/* Line 1455 of yacc.c  */
#line 2982 "pxr/usd/sdf/textFileFormat.yy"
    {
            // This clause only defines relational attributes for a target,
            // it does not add to the relationship target list. However, we 
            // do need to create a relationship target spec to associate the
            // attributes with.
            _PrimEndRelationship(context);
        ;}
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 3011 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 442:

/* Line 1455 of yacc.c  */
#line 3016 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeRelationship, context);
        ;}
    break;

  case 443:

/* Line 1455 of yacc.c  */
#line 3018 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 444:

/* Line 1455 of yacc.c  */
#line 3025 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 3028 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 3031 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 3034 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 3037 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 449:

/* Line 1455 of yacc.c  */
#line 3040 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 3043 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 451:

/* Line 1455 of yacc.c  */
#line 3046 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 452:

/* Line 1455 of yacc.c  */
#line 3049 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 453:

/* Line 1455 of yacc.c  */
#line 3052 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 454:

/* Line 1455 of yacc.c  */
#line 3057 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 455:

/* Line 1455 of yacc.c  */
#line 3064 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 456:

/* Line 1455 of yacc.c  */
#line 3072 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 457:

/* Line 1455 of yacc.c  */
#line 3077 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 461:

/* Line 1455 of yacc.c  */
#line 3091 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 462:

/* Line 1455 of yacc.c  */
#line 3094 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 466:

/* Line 1455 of yacc.c  */
#line 3106 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipAppendTargetPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 470:

/* Line 1455 of yacc.c  */
#line 3118 "pxr/usd/sdf/textFileFormat.yy"
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

  case 471:

/* Line 1455 of yacc.c  */
#line 3132 "pxr/usd/sdf/textFileFormat.yy"
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

  case 476:

/* Line 1455 of yacc.c  */
#line 3156 "pxr/usd/sdf/textFileFormat.yy"
    {
        ;}
    break;

  case 478:

/* Line 1455 of yacc.c  */
#line 3162 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 479:

/* Line 1455 of yacc.c  */
#line 3175 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->savedPath = SdfPath();
    ;}
    break;

  case 481:

/* Line 1455 of yacc.c  */
#line 3182 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrim((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 482:

/* Line 1455 of yacc.c  */
#line 3188 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetProperty((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 483:

/* Line 1455 of yacc.c  */
#line 3194 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrimOrPropertyScenePath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 492:

/* Line 1455 of yacc.c  */
#line 3226 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;



/* Line 1455 of yacc.c  */
#line 6326 "pxr/usd/sdf/textFileFormat.tab.cpp"
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
#line 3258 "pxr/usd/sdf/textFileFormat.yy"


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

