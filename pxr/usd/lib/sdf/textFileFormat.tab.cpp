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
#line 1243 "pxr/usd/sdf/textFileFormat.tab.cpp"

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
     TOK_CLASS = 271,
     TOK_CONFIG = 272,
     TOK_CONNECT = 273,
     TOK_CUSTOM = 274,
     TOK_CUSTOMDATA = 275,
     TOK_DEF = 276,
     TOK_DEFAULT = 277,
     TOK_DELETE = 278,
     TOK_DICTIONARY = 279,
     TOK_DISPLAYUNIT = 280,
     TOK_DOC = 281,
     TOK_INHERITS = 282,
     TOK_KIND = 283,
     TOK_MAPPER = 284,
     TOK_NAMECHILDREN = 285,
     TOK_NONE = 286,
     TOK_OFFSET = 287,
     TOK_OVER = 288,
     TOK_PERMISSION = 289,
     TOK_PAYLOAD = 290,
     TOK_PREFIX_SUBSTITUTIONS = 291,
     TOK_SUFFIX_SUBSTITUTIONS = 292,
     TOK_PREPEND = 293,
     TOK_PROPERTIES = 294,
     TOK_REFERENCES = 295,
     TOK_RELOCATES = 296,
     TOK_REL = 297,
     TOK_RENAMES = 298,
     TOK_REORDER = 299,
     TOK_ROOTPRIMS = 300,
     TOK_SCALE = 301,
     TOK_SPECIALIZES = 302,
     TOK_SUBLAYERS = 303,
     TOK_SYMMETRYARGUMENTS = 304,
     TOK_SYMMETRYFUNCTION = 305,
     TOK_TIME_SAMPLES = 306,
     TOK_UNIFORM = 307,
     TOK_VARIANTS = 308,
     TOK_VARIANTSET = 309,
     TOK_VARIANTSETS = 310,
     TOK_VARYING = 311
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
#line 1341 "pxr/usd/sdf/textFileFormat.tab.cpp"

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
#define YYLAST   1083

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  68
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  227
/* YYNRULES -- Number of rules.  */
#define YYNRULES  489
/* YYNRULES -- Number of states.  */
#define YYNSTATES  894

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   311

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      57,    58,     2,     2,    67,     2,    62,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    65,    66,
       2,    59,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    60,     2,    61,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    63,     2,    64,     2,     2,     2,     2,
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
      55,    56
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
      79,    81,    83,    85,    87,    89,    91,    93,    95,    99,
     100,   104,   106,   112,   114,   118,   120,   124,   126,   128,
     129,   134,   135,   141,   142,   148,   149,   155,   156,   162,
     163,   169,   173,   177,   181,   187,   189,   193,   196,   198,
     199,   204,   206,   210,   214,   218,   220,   224,   225,   229,
     230,   235,   236,   240,   241,   246,   247,   251,   252,   257,
     262,   264,   268,   269,   276,   278,   284,   286,   290,   292,
     296,   298,   300,   302,   304,   305,   310,   311,   317,   318,
     324,   325,   331,   332,   338,   339,   345,   349,   353,   357,
     358,   363,   364,   370,   371,   377,   378,   384,   385,   391,
     392,   398,   399,   404,   405,   411,   412,   418,   419,   425,
     426,   432,   433,   439,   440,   445,   446,   452,   453,   459,
     460,   466,   467,   473,   474,   480,   481,   486,   487,   493,
     494,   500,   501,   507,   508,   514,   515,   521,   525,   529,
     533,   538,   543,   548,   553,   558,   562,   565,   569,   573,
     575,   577,   581,   587,   589,   593,   597,   598,   602,   603,
     607,   613,   615,   619,   621,   623,   625,   629,   635,   637,
     641,   645,   646,   650,   651,   655,   661,   663,   667,   669,
     673,   675,   677,   681,   687,   689,   693,   695,   697,   699,
     703,   709,   711,   715,   717,   722,   723,   726,   728,   732,
     736,   738,   744,   746,   750,   752,   754,   757,   759,   762,
     765,   768,   771,   774,   777,   778,   788,   790,   793,   794,
     802,   807,   812,   814,   816,   818,   820,   822,   824,   828,
     830,   833,   834,   835,   842,   843,   844,   852,   853,   861,
     862,   871,   872,   881,   882,   891,   892,   901,   902,   911,
     912,   923,   924,   932,   934,   936,   938,   940,   942,   943,
     948,   949,   953,   959,   961,   965,   966,   972,   973,   977,
     983,   985,   989,   993,   995,   997,  1001,  1007,  1009,  1013,
    1015,  1016,  1022,  1023,  1026,  1028,  1032,  1033,  1038,  1042,
    1043,  1047,  1053,  1055,  1059,  1061,  1063,  1065,  1067,  1068,
    1073,  1074,  1080,  1081,  1087,  1088,  1094,  1095,  1101,  1102,
    1108,  1112,  1116,  1120,  1124,  1127,  1128,  1131,  1133,  1135,
    1136,  1142,  1143,  1146,  1148,  1152,  1157,  1162,  1164,  1166,
    1168,  1170,  1172,  1176,  1177,  1183,  1184,  1187,  1189,  1193,
    1197,  1199,  1201,  1203,  1205,  1207,  1209,  1211,  1213,  1216,
    1218,  1220,  1222,  1224,  1226,  1227,  1232,  1236,  1238,  1242,
    1244,  1246,  1248,  1249,  1254,  1258,  1260,  1264,  1266,  1268,
    1270,  1273,  1277,  1280,  1281,  1289,  1296,  1297,  1303,  1304,
    1310,  1311,  1317,  1318,  1324,  1325,  1331,  1332,  1338,  1344,
    1346,  1348,  1349,  1353,  1359,  1361,  1365,  1367,  1369,  1371,
    1373,  1374,  1379,  1380,  1386,  1387,  1393,  1394,  1400,  1401,
    1407,  1408,  1414,  1418,  1422,  1426,  1429,  1430,  1433,  1435,
    1437,  1441,  1447,  1449,  1453,  1455,  1456,  1458,  1460,  1462,
    1464,  1466,  1468,  1470,  1472,  1474,  1476,  1478,  1480,  1482,
    1483,  1485,  1488,  1490,  1492,  1494,  1497,  1498,  1500,  1502
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      69,     0,    -1,    72,    -1,    13,    -1,    14,    -1,    15,
      -1,    16,    -1,    17,    -1,    18,    -1,    19,    -1,    20,
      -1,    21,    -1,    22,    -1,    23,    -1,    24,    -1,    25,
      -1,    26,    -1,    27,    -1,    28,    -1,    29,    -1,    30,
      -1,    31,    -1,    32,    -1,    33,    -1,    35,    -1,    34,
      -1,    36,    -1,    37,    -1,    38,    -1,    39,    -1,    40,
      -1,    41,    -1,    42,    -1,    43,    -1,    44,    -1,    45,
      -1,    46,    -1,    47,    -1,    48,    -1,    49,    -1,    50,
      -1,    51,    -1,    52,    -1,    53,    -1,    54,    -1,    55,
      -1,    56,    -1,    74,    -1,    74,    92,   293,    -1,    -1,
       4,    73,    71,    -1,   293,    -1,   293,    57,    75,    58,
     293,    -1,   293,    -1,   293,    76,   289,    -1,    78,    -1,
      76,   290,    78,    -1,   287,    -1,    12,    -1,    -1,    77,
      79,    59,   243,    -1,    -1,    23,   287,    80,    59,   242,
      -1,    -1,    14,   287,    81,    59,   242,    -1,    -1,    38,
     287,    82,    59,   242,    -1,    -1,    15,   287,    83,    59,
     242,    -1,    -1,    44,   287,    84,    59,   242,    -1,    26,
      59,    12,    -1,    48,    59,    85,    -1,    60,   293,    61,
      -1,    60,   293,    86,   291,    61,    -1,    87,    -1,    86,
     292,    87,    -1,    88,    89,    -1,     6,    -1,    -1,    57,
      90,   289,    58,    -1,    91,    -1,    90,   290,    91,    -1,
      32,    59,    11,    -1,    46,    59,    11,    -1,    93,    -1,
      92,   294,    93,    -1,    -1,    21,    94,   101,    -1,    -1,
      21,   100,    95,   101,    -1,    -1,    16,    96,   101,    -1,
      -1,    16,   100,    97,   101,    -1,    -1,    33,    98,   101,
      -1,    -1,    33,   100,    99,   101,    -1,    44,    45,    59,
     162,    -1,   287,    -1,   100,    62,   287,    -1,    -1,    12,
     102,   103,    63,   165,    64,    -1,   293,    -1,   293,    57,
     104,    58,   293,    -1,   293,    -1,   293,   105,   289,    -1,
     107,    -1,   105,   290,   107,    -1,   287,    -1,    20,    -1,
      49,    -1,    12,    -1,    -1,   106,   108,    59,   243,    -1,
      -1,    23,   287,   109,    59,   242,    -1,    -1,    14,   287,
     110,    59,   242,    -1,    -1,    38,   287,   111,    59,   242,
      -1,    -1,    15,   287,   112,    59,   242,    -1,    -1,    44,
     287,   113,    59,   242,    -1,    26,    59,    12,    -1,    28,
      59,    12,    -1,    34,    59,   287,    -1,    -1,    35,   114,
      59,   138,    -1,    -1,    23,    35,   115,    59,   138,    -1,
      -1,    14,    35,   116,    59,   138,    -1,    -1,    38,    35,
     117,    59,   138,    -1,    -1,    15,    35,   118,    59,   138,
      -1,    -1,    44,    35,   119,    59,   138,    -1,    -1,    27,
     120,    59,   152,    -1,    -1,    23,    27,   121,    59,   152,
      -1,    -1,    14,    27,   122,    59,   152,    -1,    -1,    38,
      27,   123,    59,   152,    -1,    -1,    15,    27,   124,    59,
     152,    -1,    -1,    44,    27,   125,    59,   152,    -1,    -1,
      47,   126,    59,   155,    -1,    -1,    23,    47,   127,    59,
     155,    -1,    -1,    14,    47,   128,    59,   155,    -1,    -1,
      38,    47,   129,    59,   155,    -1,    -1,    15,    47,   130,
      59,   155,    -1,    -1,    44,    47,   131,    59,   155,    -1,
      -1,    40,   132,    59,   145,    -1,    -1,    23,    40,   133,
      59,   145,    -1,    -1,    14,    40,   134,    59,   145,    -1,
      -1,    38,    40,   135,    59,   145,    -1,    -1,    15,    40,
     136,    59,   145,    -1,    -1,    44,    40,   137,    59,   145,
      -1,    41,    59,   158,    -1,    53,    59,   228,    -1,    55,
      59,   162,    -1,    23,    55,    59,   162,    -1,    14,    55,
      59,   162,    -1,    38,    55,    59,   162,    -1,    15,    55,
      59,   162,    -1,    44,    55,    59,   162,    -1,    50,    59,
     287,    -1,    50,    59,    -1,    36,    59,   237,    -1,    37,
      59,   237,    -1,    31,    -1,   140,    -1,    60,   293,    61,
      -1,    60,   293,   139,   291,    61,    -1,   140,    -1,   139,
     292,   140,    -1,    88,   281,   142,    -1,    -1,     7,   141,
     142,    -1,    -1,    57,   293,    58,    -1,    57,   293,   143,
     289,    58,    -1,   144,    -1,   143,   290,   144,    -1,    91,
      -1,    31,    -1,   147,    -1,    60,   293,    61,    -1,    60,
     293,   146,   291,    61,    -1,   147,    -1,   146,   292,   147,
      -1,    88,   281,   149,    -1,    -1,     7,   148,   149,    -1,
      -1,    57,   293,    58,    -1,    57,   293,   150,   289,    58,
      -1,   151,    -1,   150,   290,   151,    -1,    91,    -1,    20,
      59,   228,    -1,    31,    -1,   154,    -1,    60,   293,    61,
      -1,    60,   293,   153,   291,    61,    -1,   154,    -1,   153,
     292,   154,    -1,   282,    -1,    31,    -1,   157,    -1,    60,
     293,    61,    -1,    60,   293,   156,   291,    61,    -1,   157,
      -1,   156,   292,   157,    -1,   282,    -1,    63,   293,   159,
      64,    -1,    -1,   160,   291,    -1,   161,    -1,   160,   292,
     161,    -1,     7,    65,     7,    -1,   164,    -1,    60,   293,
     163,   291,    61,    -1,   164,    -1,   163,   292,   164,    -1,
      12,    -1,   293,    -1,   293,   166,    -1,   167,    -1,   166,
     167,    -1,   175,   290,    -1,   173,   290,    -1,   174,   290,
      -1,    93,   294,    -1,   168,   294,    -1,    -1,    54,    12,
     169,    59,   293,    63,   293,   170,    64,    -1,   171,    -1,
     170,   171,    -1,    -1,    12,   172,   103,    63,   165,    64,
     293,    -1,    44,    30,    59,   162,    -1,    44,    39,    59,
     162,    -1,   197,    -1,   260,    -1,    52,    -1,    17,    -1,
     176,    -1,   287,    -1,   287,    60,    61,    -1,   178,    -1,
     177,   178,    -1,    -1,    -1,   179,   286,   181,   226,   182,
     216,    -1,    -1,    -1,    19,   179,   286,   184,   226,   185,
     216,    -1,    -1,   179,   286,    62,    18,    59,   187,   207,
      -1,    -1,    14,   179,   286,    62,    18,    59,   188,   207,
      -1,    -1,    38,   179,   286,    62,    18,    59,   189,   207,
      -1,    -1,    15,   179,   286,    62,    18,    59,   190,   207,
      -1,    -1,    23,   179,   286,    62,    18,    59,   191,   207,
      -1,    -1,    44,   179,   286,    62,    18,    59,   192,   207,
      -1,    -1,   179,   286,    62,    29,    60,   283,    61,    59,
     194,   198,    -1,    -1,   179,   286,    62,    51,    59,   196,
     210,    -1,   183,    -1,   180,    -1,   186,    -1,   193,    -1,
     195,    -1,    -1,   285,   199,   204,   200,    -1,    -1,    63,
     293,    64,    -1,    63,   293,   201,   289,    64,    -1,   202,
      -1,   201,   290,   202,    -1,    -1,   178,   285,   203,    59,
     244,    -1,    -1,    57,   293,    58,    -1,    57,   293,   205,
     289,    58,    -1,   206,    -1,   205,   290,   206,    -1,    49,
      59,   228,    -1,    31,    -1,   209,    -1,    60,   293,    61,
      -1,    60,   293,   208,   291,    61,    -1,   209,    -1,   208,
     292,   209,    -1,   284,    -1,    -1,    63,   211,   293,   212,
      64,    -1,    -1,   213,   291,    -1,   214,    -1,   213,   292,
     214,    -1,    -1,   288,    65,   215,   244,    -1,   288,    65,
      31,    -1,    -1,    57,   293,    58,    -1,    57,   293,   217,
     289,    58,    -1,   219,    -1,   217,   290,   219,    -1,   287,
      -1,    20,    -1,    49,    -1,    12,    -1,    -1,   218,   220,
      59,   243,    -1,    -1,    23,   287,   221,    59,   242,    -1,
      -1,    14,   287,   222,    59,   242,    -1,    -1,    38,   287,
     223,    59,   242,    -1,    -1,    15,   287,   224,    59,   242,
      -1,    -1,    44,   287,   225,    59,   242,    -1,    26,    59,
      12,    -1,    34,    59,   287,    -1,    25,    59,   287,    -1,
      50,    59,   287,    -1,    50,    59,    -1,    -1,    59,   227,
      -1,   244,    -1,    31,    -1,    -1,    63,   229,   293,   230,
      64,    -1,    -1,   231,   289,    -1,   232,    -1,   231,   290,
     232,    -1,   234,   233,    59,   244,    -1,    24,   233,    59,
     228,    -1,    12,    -1,   285,    -1,   235,    -1,   236,    -1,
     287,    -1,   287,    60,    61,    -1,    -1,    63,   238,   293,
     239,    64,    -1,    -1,   240,   291,    -1,   241,    -1,   240,
     292,   241,    -1,    12,    65,    12,    -1,    31,    -1,   246,
      -1,   228,    -1,   244,    -1,    31,    -1,   245,    -1,   251,
      -1,   246,    -1,    60,    61,    -1,     7,    -1,    11,    -1,
      12,    -1,   287,    -1,     6,    -1,    -1,    60,   247,   248,
      61,    -1,   293,   249,   291,    -1,   250,    -1,   249,   292,
     250,    -1,   245,    -1,   246,    -1,   251,    -1,    -1,    57,
     252,   253,    58,    -1,   293,   254,   291,    -1,   255,    -1,
     254,   292,   255,    -1,   245,    -1,   251,    -1,    42,    -1,
      19,    42,    -1,    19,    56,    42,    -1,    56,    42,    -1,
      -1,   256,   286,    62,    51,    59,   258,   210,    -1,   256,
     286,    62,    22,    59,     7,    -1,    -1,   256,   286,   261,
     277,   267,    -1,    -1,    23,   256,   286,   262,   277,    -1,
      -1,    14,   256,   286,   263,   277,    -1,    -1,    38,   256,
     286,   264,   277,    -1,    -1,    15,   256,   286,   265,   277,
      -1,    -1,    44,   256,   286,   266,   277,    -1,   256,   286,
      60,     7,    61,    -1,   257,    -1,   259,    -1,    -1,    57,
     293,    58,    -1,    57,   293,   268,   289,    58,    -1,   270,
      -1,   268,   290,   270,    -1,   287,    -1,    20,    -1,    49,
      -1,    12,    -1,    -1,   269,   271,    59,   243,    -1,    -1,
      23,   287,   272,    59,   242,    -1,    -1,    14,   287,   273,
      59,   242,    -1,    -1,    38,   287,   274,    59,   242,    -1,
      -1,    15,   287,   275,    59,   242,    -1,    -1,    44,   287,
     276,    59,   242,    -1,    26,    59,    12,    -1,    34,    59,
     287,    -1,    50,    59,   287,    -1,    50,    59,    -1,    -1,
      59,   278,    -1,   280,    -1,    31,    -1,    60,   293,    61,
      -1,    60,   293,   279,   291,    61,    -1,   280,    -1,   279,
     292,   280,    -1,     7,    -1,    -1,   282,    -1,     7,    -1,
       7,    -1,     7,    -1,   287,    -1,    70,    -1,     8,    -1,
      10,    -1,    70,    -1,     8,    -1,     9,    -1,    11,    -1,
       8,    -1,    -1,   290,    -1,    66,   293,    -1,   294,    -1,
     293,    -1,   292,    -1,    67,   293,    -1,    -1,   294,    -1,
       3,    -1,   294,     3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1261,  1261,  1264,  1265,  1266,  1267,  1268,  1269,  1270,
    1271,  1272,  1273,  1274,  1275,  1276,  1277,  1278,  1279,  1280,
    1281,  1282,  1283,  1284,  1285,  1286,  1287,  1288,  1289,  1290,
    1291,  1292,  1293,  1294,  1295,  1296,  1297,  1298,  1299,  1300,
    1301,  1302,  1303,  1304,  1305,  1306,  1307,  1315,  1316,  1327,
    1327,  1339,  1340,  1352,  1353,  1357,  1358,  1362,  1366,  1371,
    1371,  1380,  1380,  1386,  1386,  1392,  1392,  1398,  1398,  1404,
    1404,  1412,  1419,  1423,  1424,  1438,  1439,  1443,  1451,  1458,
    1460,  1464,  1465,  1469,  1473,  1480,  1481,  1489,  1489,  1493,
    1493,  1497,  1497,  1501,  1501,  1505,  1505,  1509,  1509,  1513,
    1523,  1524,  1531,  1531,  1591,  1592,  1596,  1597,  1601,  1602,
    1606,  1607,  1608,  1612,  1617,  1617,  1626,  1626,  1632,  1632,
    1638,  1638,  1644,  1644,  1650,  1650,  1658,  1665,  1672,  1679,
    1679,  1686,  1686,  1693,  1693,  1700,  1700,  1707,  1707,  1714,
    1714,  1722,  1722,  1727,  1727,  1732,  1732,  1737,  1737,  1742,
    1742,  1747,  1747,  1753,  1753,  1758,  1758,  1763,  1763,  1768,
    1768,  1773,  1773,  1778,  1778,  1784,  1784,  1791,  1791,  1798,
    1798,  1805,  1805,  1812,  1812,  1819,  1819,  1828,  1836,  1840,
    1844,  1848,  1852,  1856,  1860,  1866,  1871,  1878,  1886,  1895,
    1896,  1897,  1898,  1902,  1903,  1907,  1919,  1919,  1942,  1944,
    1945,  1949,  1950,  1954,  1958,  1959,  1960,  1961,  1965,  1966,
    1970,  1983,  1983,  2007,  2009,  2010,  2014,  2015,  2019,  2020,
    2024,  2025,  2026,  2027,  2031,  2032,  2036,  2042,  2043,  2044,
    2045,  2049,  2050,  2054,  2060,  2063,  2065,  2069,  2070,  2074,
    2080,  2081,  2085,  2086,  2090,  2098,  2099,  2103,  2104,  2108,
    2109,  2110,  2111,  2112,  2116,  2116,  2150,  2151,  2155,  2155,
    2198,  2207,  2220,  2221,  2229,  2232,  2238,  2244,  2247,  2253,
    2257,  2263,  2270,  2263,  2281,  2289,  2281,  2300,  2300,  2308,
    2308,  2316,  2316,  2324,  2324,  2332,  2332,  2340,  2340,  2351,
    2351,  2375,  2375,  2387,  2388,  2389,  2390,  2391,  2400,  2400,
    2417,  2419,  2420,  2429,  2430,  2434,  2434,  2449,  2451,  2452,
    2456,  2457,  2461,  2470,  2471,  2472,  2473,  2477,  2478,  2482,
    2492,  2492,  2497,  2499,  2503,  2504,  2508,  2508,  2515,  2527,
    2529,  2530,  2534,  2535,  2539,  2540,  2541,  2545,  2550,  2550,
    2559,  2559,  2565,  2565,  2571,  2571,  2577,  2577,  2583,  2583,
    2591,  2598,  2605,  2613,  2618,  2625,  2627,  2631,  2636,  2648,
    2648,  2656,  2658,  2662,  2663,  2667,  2670,  2678,  2679,  2683,
    2684,  2688,  2694,  2704,  2704,  2712,  2714,  2718,  2719,  2723,
    2736,  2742,  2752,  2756,  2757,  2770,  2773,  2776,  2779,  2790,
    2796,  2799,  2802,  2807,  2820,  2820,  2829,  2833,  2834,  2838,
    2839,  2840,  2848,  2848,  2855,  2859,  2860,  2864,  2865,  2873,
    2877,  2881,  2885,  2892,  2892,  2904,  2919,  2919,  2929,  2929,
    2937,  2937,  2945,  2945,  2953,  2953,  2962,  2962,  2970,  2977,
    2978,  2981,  2983,  2984,  2988,  2989,  2993,  2994,  2995,  2999,
    3004,  3004,  3013,  3013,  3019,  3019,  3025,  3025,  3031,  3031,
    3037,  3037,  3045,  3052,  3060,  3065,  3072,  3074,  3078,  3079,
    3082,  3085,  3089,  3090,  3094,  3104,  3107,  3111,  3117,  3123,
    3134,  3135,  3141,  3142,  3143,  3148,  3149,  3154,  3155,  3158,
    3160,  3164,  3165,  3169,  3170,  3174,  3177,  3179,  3183,  3184
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
  "TOK_CLASS", "TOK_CONFIG", "TOK_CONNECT", "TOK_CUSTOM", "TOK_CUSTOMDATA",
  "TOK_DEF", "TOK_DEFAULT", "TOK_DELETE", "TOK_DICTIONARY",
  "TOK_DISPLAYUNIT", "TOK_DOC", "TOK_INHERITS", "TOK_KIND", "TOK_MAPPER",
  "TOK_NAMECHILDREN", "TOK_NONE", "TOK_OFFSET", "TOK_OVER",
  "TOK_PERMISSION", "TOK_PAYLOAD", "TOK_PREFIX_SUBSTITUTIONS",
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
  "relationship_metadata_list_opt", "relationship_metadata_list",
  "relationship_metadata_key", "relationship_metadata", "$@82", "$@83",
  "$@84", "$@85", "$@86", "$@87", "relationship_assignment_opt",
  "relationship_rhs", "relationship_target_list", "relationship_target",
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
     305,   306,   307,   308,   309,   310,   311,    40,    41,    61,
      91,    93,    46,   123,   125,    58,    59,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,    68,    69,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    71,    71,    73,
      72,    74,    74,    75,    75,    76,    76,    77,    78,    79,
      78,    80,    78,    81,    78,    82,    78,    83,    78,    84,
      78,    78,    78,    85,    85,    86,    86,    87,    88,    89,
      89,    90,    90,    91,    91,    92,    92,    94,    93,    95,
      93,    96,    93,    97,    93,    98,    93,    99,    93,    93,
     100,   100,   102,   101,   103,   103,   104,   104,   105,   105,
     106,   106,   106,   107,   108,   107,   109,   107,   110,   107,
     111,   107,   112,   107,   113,   107,   107,   107,   107,   114,
     107,   115,   107,   116,   107,   117,   107,   118,   107,   119,
     107,   120,   107,   121,   107,   122,   107,   123,   107,   124,
     107,   125,   107,   126,   107,   127,   107,   128,   107,   129,
     107,   130,   107,   131,   107,   132,   107,   133,   107,   134,
     107,   135,   107,   136,   107,   137,   107,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   107,   107,   107,   138,
     138,   138,   138,   139,   139,   140,   141,   140,   142,   142,
     142,   143,   143,   144,   145,   145,   145,   145,   146,   146,
     147,   148,   147,   149,   149,   149,   150,   150,   151,   151,
     152,   152,   152,   152,   153,   153,   154,   155,   155,   155,
     155,   156,   156,   157,   158,   159,   159,   160,   160,   161,
     162,   162,   163,   163,   164,   165,   165,   166,   166,   167,
     167,   167,   167,   167,   169,   168,   170,   170,   172,   171,
     173,   174,   175,   175,   176,   176,   177,   178,   178,   179,
     179,   181,   182,   180,   184,   185,   183,   187,   186,   188,
     186,   189,   186,   190,   186,   191,   186,   192,   186,   194,
     193,   196,   195,   197,   197,   197,   197,   197,   199,   198,
     200,   200,   200,   201,   201,   203,   202,   204,   204,   204,
     205,   205,   206,   207,   207,   207,   207,   208,   208,   209,
     211,   210,   212,   212,   213,   213,   215,   214,   214,   216,
     216,   216,   217,   217,   218,   218,   218,   219,   220,   219,
     221,   219,   222,   219,   223,   219,   224,   219,   225,   219,
     219,   219,   219,   219,   219,   226,   226,   227,   227,   229,
     228,   230,   230,   231,   231,   232,   232,   233,   233,   234,
     234,   235,   236,   238,   237,   239,   239,   240,   240,   241,
     242,   242,   243,   243,   243,   244,   244,   244,   244,   244,
     245,   245,   245,   245,   247,   246,   248,   249,   249,   250,
     250,   250,   252,   251,   253,   254,   254,   255,   255,   256,
     256,   256,   256,   258,   257,   259,   261,   260,   262,   260,
     263,   260,   264,   260,   265,   260,   266,   260,   260,   260,
     260,   267,   267,   267,   268,   268,   269,   269,   269,   270,
     271,   270,   272,   270,   273,   270,   274,   270,   275,   270,
     276,   270,   270,   270,   270,   270,   277,   277,   278,   278,
     278,   278,   279,   279,   280,   281,   281,   282,   283,   284,
     285,   285,   286,   286,   286,   287,   287,   288,   288,   289,
     289,   290,   290,   291,   291,   292,   293,   293,   294,   294
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     0,
       3,     1,     5,     1,     3,     1,     3,     1,     1,     0,
       4,     0,     5,     0,     5,     0,     5,     0,     5,     0,
       5,     3,     3,     3,     5,     1,     3,     2,     1,     0,
       4,     1,     3,     3,     3,     1,     3,     0,     3,     0,
       4,     0,     3,     0,     4,     0,     3,     0,     4,     4,
       1,     3,     0,     6,     1,     5,     1,     3,     1,     3,
       1,     1,     1,     1,     0,     4,     0,     5,     0,     5,
       0,     5,     0,     5,     0,     5,     3,     3,     3,     0,
       4,     0,     5,     0,     5,     0,     5,     0,     5,     0,
       5,     0,     4,     0,     5,     0,     5,     0,     5,     0,
       5,     0,     5,     0,     4,     0,     5,     0,     5,     0,
       5,     0,     5,     0,     5,     0,     4,     0,     5,     0,
       5,     0,     5,     0,     5,     0,     5,     3,     3,     3,
       4,     4,     4,     4,     4,     3,     2,     3,     3,     1,
       1,     3,     5,     1,     3,     3,     0,     3,     0,     3,
       5,     1,     3,     1,     1,     1,     3,     5,     1,     3,
       3,     0,     3,     0,     3,     5,     1,     3,     1,     3,
       1,     1,     3,     5,     1,     3,     1,     1,     1,     3,
       5,     1,     3,     1,     4,     0,     2,     1,     3,     3,
       1,     5,     1,     3,     1,     1,     2,     1,     2,     2,
       2,     2,     2,     2,     0,     9,     1,     2,     0,     7,
       4,     4,     1,     1,     1,     1,     1,     1,     3,     1,
       2,     0,     0,     6,     0,     0,     7,     0,     7,     0,
       8,     0,     8,     0,     8,     0,     8,     0,     8,     0,
      10,     0,     7,     1,     1,     1,     1,     1,     0,     4,
       0,     3,     5,     1,     3,     0,     5,     0,     3,     5,
       1,     3,     3,     1,     1,     3,     5,     1,     3,     1,
       0,     5,     0,     2,     1,     3,     0,     4,     3,     0,
       3,     5,     1,     3,     1,     1,     1,     1,     0,     4,
       0,     5,     0,     5,     0,     5,     0,     5,     0,     5,
       3,     3,     3,     3,     2,     0,     2,     1,     1,     0,
       5,     0,     2,     1,     3,     4,     4,     1,     1,     1,
       1,     1,     3,     0,     5,     0,     2,     1,     3,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     1,
       1,     1,     1,     1,     0,     4,     3,     1,     3,     1,
       1,     1,     0,     4,     3,     1,     3,     1,     1,     1,
       2,     3,     2,     0,     7,     6,     0,     5,     0,     5,
       0,     5,     0,     5,     0,     5,     0,     5,     5,     1,
       1,     0,     3,     5,     1,     3,     1,     1,     1,     1,
       0,     4,     0,     5,     0,     5,     0,     5,     0,     5,
       0,     5,     3,     3,     3,     2,     0,     2,     1,     1,
       3,     5,     1,     3,     1,     0,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       1,     2,     1,     1,     1,     2,     0,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    49,     0,     2,   486,     1,   488,    50,    47,    51,
     487,    91,    87,    95,     0,   486,    85,   486,   489,   475,
     476,     0,    93,   100,     0,    89,     0,    97,     0,    48,
     487,     0,    53,   102,    92,     0,     0,    88,     0,    96,
       0,     0,    86,   486,    58,     0,     0,     0,     0,     0,
       0,     0,   479,    59,    55,    57,   486,   101,    94,    90,
      98,   244,   486,    99,   240,    52,    63,    67,    61,     0,
      65,    69,     0,   486,    54,   480,   482,     0,     0,   104,
       0,     0,     0,     0,    71,     0,     0,   486,    72,   481,
      56,     0,   486,   486,   486,   242,     0,     0,     0,     0,
       0,     0,   393,   389,   390,   391,   384,   402,   394,   359,
     382,    60,   383,   385,   387,   386,   392,     0,   245,     0,
     106,   486,     0,   484,   483,   380,   394,    64,   381,    68,
      62,    66,    70,    78,    73,   486,    75,    79,   486,   388,
     486,   486,   103,     0,     0,   265,     0,     0,     0,   409,
       0,   264,     0,     0,     0,   246,   247,     0,     0,     0,
       0,   266,     0,   269,     0,   294,   293,   295,   296,   297,
     262,     0,   429,   430,   263,   267,   486,   113,     0,     0,
     111,     0,     0,   141,     0,     0,   129,     0,     0,     0,
     165,     0,     0,   153,   112,     0,     0,     0,   479,   114,
     108,   110,   485,   241,   243,     0,   484,     0,    77,     0,
       0,     0,     0,   361,     0,     0,     0,     0,     0,   410,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     254,   412,   252,   248,   253,   250,   251,   249,   270,   472,
     473,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    25,    24,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,   474,   271,   416,     0,   105,
     145,   133,   169,   157,     0,   118,   149,   137,   173,   161,
       0,   122,   143,   131,   167,   155,     0,   116,     0,     0,
       0,     0,     0,     0,     0,   147,   135,   171,   159,     0,
     120,     0,     0,   151,   139,   175,   163,     0,   124,     0,
     186,     0,     0,   107,   480,     0,    74,    76,     0,     0,
     479,    81,   403,   407,   408,   486,   405,   395,   399,   400,
     486,   397,   401,     0,     0,   479,   363,     0,   369,   370,
     371,     0,   420,     0,   424,   411,   274,     0,   418,     0,
     422,     0,     0,     0,   426,     0,     0,   355,     0,     0,
     456,   268,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     126,     0,   127,   128,     0,   373,   187,   188,     0,     0,
       0,     0,     0,     0,     0,   486,   177,     0,     0,     0,
       0,     0,     0,     0,   185,   178,   179,   109,     0,     0,
       0,     0,   480,   404,   484,   396,   484,   367,   471,     0,
     368,   470,   360,   362,   480,     0,     0,     0,   456,     0,
     456,   355,     0,   456,     0,   456,   260,   261,     0,   456,
     486,     0,     0,     0,     0,   272,     0,     0,     0,     0,
     431,     0,     0,     0,     0,   181,     0,     0,     0,     0,
       0,   183,     0,     0,     0,     0,     0,   180,     0,   467,
     220,   486,   142,   221,   226,   196,   189,   486,   465,   130,
     190,   486,     0,     0,     0,     0,   182,     0,   211,   204,
     486,   465,   166,   205,   235,     0,     0,     0,     0,   184,
       0,   227,   486,   154,   228,   233,   115,    83,    84,    80,
      82,   406,   398,     0,   364,     0,   372,     0,   421,     0,
     425,   275,     0,   419,     0,   423,     0,   427,     0,   277,
       0,   291,   358,   356,   357,   329,   428,     0,   413,   464,
     459,   486,   457,   458,   486,   417,   146,   134,   170,   158,
     119,   150,   138,   174,   162,   123,   144,   132,   168,   156,
     117,     0,   198,     0,   198,   466,   375,   148,   136,   172,
     160,   121,   213,     0,   213,     0,     0,   486,   237,   152,
     140,   176,   164,   125,     0,   366,   365,   279,   283,   329,
     285,   281,   287,   486,     0,   468,     0,     0,   486,   273,
     415,     0,     0,     0,   222,   486,   224,   486,   197,   191,
     486,   193,   195,     0,     0,   486,   377,   486,   212,   206,
     486,   208,   210,     0,   234,   236,   484,   229,   486,   231,
       0,     0,   276,     0,     0,     0,     0,   469,   313,   486,
     278,   314,   319,     0,   320,   292,     0,   414,   460,   486,
     462,   439,     0,     0,   437,     0,     0,     0,     0,     0,
     438,     0,   432,   479,   440,   434,   436,     0,   484,     0,
       0,   484,     0,   374,   376,   484,     0,     0,   484,   239,
     238,     0,   484,   280,   284,   286,   282,   288,   258,     0,
     256,     0,   289,   486,   337,     0,     0,   335,     0,     0,
       0,     0,     0,     0,   336,     0,   330,   479,   338,   332,
     334,     0,   484,   444,   448,   442,     0,     0,   446,   450,
     455,     0,   480,     0,   223,   225,   199,   203,   479,   201,
     192,   194,   379,   378,     0,   214,   218,   479,   216,   207,
     209,   230,   232,   486,   255,   257,   315,   486,   317,     0,
     322,   342,   346,   340,     0,     0,     0,   344,   348,   354,
       0,   480,     0,   461,   463,     0,     0,     0,   452,   453,
       0,     0,   454,   433,   435,     0,     0,   480,     0,     0,
     480,     0,     0,   484,   290,   298,   478,   477,     0,   486,
     324,     0,     0,     0,     0,   352,   350,   351,     0,     0,
     353,   331,   333,     0,     0,     0,     0,     0,     0,   441,
     200,   202,   219,   215,   217,   486,   316,   318,   307,   321,
     323,   484,   326,     0,     0,     0,     0,     0,   339,   445,
     449,   443,   447,   451,     0,   486,   300,   325,   328,     0,
     343,   347,   341,   345,   349,   486,     0,   486,   299,   327,
     259,     0,   308,   479,   310,     0,     0,     0,   480,   301,
       0,   479,   303,   312,   309,   311,   305,     0,   480,     0,
     302,   304,     0,   306
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,   285,     7,     3,     4,     8,    31,    52,    53,
      54,    77,    83,    81,    85,    82,    86,    88,   135,   136,
     498,   208,   340,   747,    15,   154,    24,    38,    21,    36,
      26,    40,    22,    34,    56,    78,   119,   198,   199,   200,
     335,   399,   387,   413,   393,   422,   312,   395,   383,   409,
     389,   418,   309,   394,   382,   408,   388,   417,   329,   397,
     385,   411,   391,   420,   321,   396,   384,   410,   390,   419,
     499,   630,   500,   582,   628,   748,   749,   512,   640,   513,
     592,   638,   757,   758,   492,   625,   493,   523,   648,   524,
     416,   596,   597,   598,    63,    94,    64,   117,   155,   156,
     157,   375,   709,   710,   763,   158,   159,   160,   161,   162,
     163,   164,   165,   377,   555,   166,   451,   609,   167,   614,
     650,   654,   651,   653,   655,   168,   769,   169,   617,   170,
     804,   838,   868,   881,   882,   889,   856,   873,   874,   660,
     767,   661,   665,   713,   808,   809,   810,   859,   619,   727,
     728,   729,   782,   814,   812,   818,   813,   819,   465,   553,
     110,   141,   354,   355,   356,   439,   357,   358,   359,   406,
     501,   634,   635,   636,   127,   111,   112,   113,   128,   140,
     211,   350,   351,   115,   138,   209,   345,   346,   171,   172,
     621,   173,   174,   380,   453,   448,   455,   450,   459,   565,
     683,   684,   685,   743,   787,   785,   790,   786,   791,   470,
     562,   669,   563,   584,   494,   616,   662,   440,   286,   116,
     811,    74,    75,   122,   123,   124,    10
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -758
static const yytype_int16 yypact[] =
{
      83,  -758,    42,  -758,    57,  -758,  -758,  -758,   293,    63,
     119,    68,    68,    68,    81,    57,  -758,    57,  -758,  -758,
    -758,   129,   109,  -758,   129,   109,   129,   109,    84,  -758,
     532,   115,   848,  -758,  -758,    68,   129,  -758,   129,  -758,
     129,    44,  -758,    57,  -758,    68,    68,    68,   125,    68,
      68,   132,    34,  -758,  -758,  -758,    57,  -758,  -758,  -758,
    -758,  -758,    57,  -758,  -758,  -758,  -758,  -758,  -758,   177,
    -758,  -758,   134,    57,  -758,   848,   119,   149,   138,   162,
     209,   176,   183,   189,  -758,   192,   198,    57,  -758,  -758,
    -758,   232,    57,    57,    29,  -758,    51,    51,    51,    51,
      51,    40,  -758,  -758,  -758,  -758,  -758,  -758,   207,  -758,
    -758,  -758,  -758,  -758,  -758,  -758,  -758,   211,   517,   213,
     800,    57,   221,   209,  -758,  -758,  -758,  -758,  -758,  -758,
    -758,  -758,  -758,  -758,  -758,    29,  -758,   229,    57,  -758,
      57,    57,  -758,   264,   264,  -758,   288,   264,   264,  -758,
     282,  -758,   275,   246,    57,   517,  -758,    57,    34,    34,
      34,  -758,    68,  -758,   978,  -758,  -758,  -758,  -758,  -758,
    -758,   978,  -758,  -758,  -758,   233,    57,  -758,   348,   359,
    -758,   383,   239,  -758,   243,   244,  -758,   249,   251,   408,
    -758,   252,   537,  -758,  -758,   254,   269,   270,    34,  -758,
    -758,  -758,  -758,  -758,  -758,   256,   325,     9,  -758,   274,
     268,   297,    59,    71,    33,   978,   978,   978,   978,  -758,
     294,   978,   978,   978,   978,   978,   300,   302,   978,   978,
    -758,  -758,   119,  -758,   119,  -758,  -758,  -758,  -758,  -758,
    -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,
    -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,
    -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,
    -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,
    -758,  -758,  -758,  -758,  -758,  -758,   307,    21,   301,  -758,
    -758,  -758,  -758,  -758,   311,  -758,  -758,  -758,  -758,  -758,
     312,  -758,  -758,  -758,  -758,  -758,   314,  -758,   364,   323,
     384,    68,   338,   339,   339,  -758,  -758,  -758,  -758,   352,
    -758,   354,   356,  -758,  -758,  -758,  -758,   361,  -758,   362,
      68,   363,    44,  -758,   800,   365,  -758,  -758,   368,   370,
      34,  -758,  -758,  -758,  -758,    29,  -758,  -758,  -758,  -758,
      29,  -758,  -758,   929,   351,    34,  -758,   929,  -758,  -758,
     371,   360,  -758,   374,  -758,  -758,  -758,   375,  -758,   379,
    -758,    44,    44,   385,  -758,   390,   146,   393,   446,   147,
     395,  -758,   398,   412,   417,   423,    44,   427,   430,   431,
     435,   437,    44,   439,   445,   449,   452,   454,    44,   457,
    -758,    38,  -758,  -758,    87,  -758,  -758,  -758,   458,   462,
     463,   468,    44,   469,   151,    57,  -758,   471,   485,   488,
     490,    44,   492,   117,  -758,  -758,  -758,  -758,   232,   513,
     541,   496,     9,  -758,   268,  -758,    59,  -758,  -758,   498,
    -758,  -758,  -758,  -758,    71,   503,   502,   550,   395,   556,
     395,   393,   557,   395,   563,   395,  -758,  -758,   564,   395,
      57,   528,   530,   529,   247,  -758,   534,   539,   542,   152,
     545,    38,    87,   151,   117,  -758,    51,    38,    87,   151,
     117,  -758,    51,    38,    87,   151,   117,  -758,    51,  -758,
    -758,    57,  -758,  -758,  -758,  -758,  -758,    57,   596,  -758,
    -758,    57,    38,    87,   151,   117,  -758,    51,  -758,  -758,
      57,   596,  -758,  -758,   597,    38,    87,   151,   117,  -758,
      51,  -758,    57,  -758,  -758,  -758,  -758,  -758,  -758,  -758,
    -758,  -758,  -758,   363,  -758,   258,  -758,   554,  -758,   555,
    -758,  -758,   565,  -758,   566,  -758,   567,  -758,   546,  -758,
     608,  -758,  -758,  -758,  -758,   570,  -758,   621,  -758,  -758,
    -758,    57,  -758,  -758,    57,  -758,  -758,  -758,  -758,  -758,
    -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,
    -758,    41,   574,    47,   574,  -758,   620,  -758,  -758,  -758,
    -758,  -758,   579,    52,   579,   573,   576,    29,  -758,  -758,
    -758,  -758,  -758,  -758,    54,  -758,  -758,  -758,  -758,   570,
    -758,  -758,  -758,    57,   156,  -758,   580,   581,    57,  -758,
    -758,   581,    56,   465,  -758,    29,  -758,    57,  -758,  -758,
      29,  -758,  -758,   577,   582,    29,  -758,    57,  -758,  -758,
      29,  -758,  -758,   636,  -758,  -758,   597,  -758,    29,  -758,
     156,   156,  -758,   156,   156,   156,   633,  -758,  -758,    57,
    -758,  -758,  -758,   588,  -758,  -758,   340,  -758,  -758,    29,
    -758,  -758,    68,    68,  -758,    68,   589,   590,    68,    68,
    -758,   591,  -758,    34,  -758,  -758,  -758,   592,   596,   142,
     593,   255,   639,  -758,  -758,   620,   160,   595,   278,  -758,
    -758,   609,   596,  -758,  -758,  -758,  -758,  -758,  -758,    28,
    -758,    67,  -758,    57,  -758,    68,    68,  -758,    68,   598,
     599,   610,    68,    68,  -758,   613,  -758,    34,  -758,  -758,
    -758,   612,   645,  -758,  -758,  -758,   663,    68,  -758,  -758,
      68,   618,   651,   625,  -758,  -758,  -758,  -758,    34,  -758,
    -758,  -758,  -758,  -758,   627,  -758,  -758,    34,  -758,  -758,
    -758,  -758,  -758,    57,  -758,  -758,  -758,    29,  -758,  1027,
      80,  -758,  -758,  -758,    68,   668,    68,  -758,  -758,    68,
     630,   585,   631,  -758,  -758,   634,   635,   638,  -758,  -758,
     640,   643,  -758,  -758,  -758,   232,   650,     9,   363,   655,
     214,   629,   637,   696,  -758,  -758,  -758,  -758,   652,    29,
    -758,   644,   656,   659,   660,  -758,  -758,  -758,   661,   662,
    -758,  -758,  -758,   232,    51,    51,    51,    51,    51,  -758,
    -758,  -758,  -758,  -758,  -758,    57,  -758,  -758,   657,  -758,
    -758,    80,   692,    51,    51,    51,    51,    51,  -758,  -758,
    -758,  -758,  -758,  -758,   669,    57,   664,  -758,  -758,   258,
    -758,  -758,  -758,  -758,  -758,    57,   127,    57,  -758,  -758,
    -758,   665,  -758,    34,  -758,    35,   363,   667,   685,  -758,
    1027,    34,  -758,  -758,  -758,  -758,  -758,   671,    68,   677,
    -758,  -758,   258,  -758
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -758,  -758,  -337,  -758,  -758,  -758,  -758,  -758,  -758,  -758,
     666,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,   531,
     -92,  -758,  -758,  -204,  -758,   197,  -758,  -758,  -758,  -758,
    -758,  -758,   191,   420,  -758,   -24,  -758,  -758,  -758,   410,
    -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,
    -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,
    -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,
    -139,  -758,  -561,  -758,   158,  -758,   -52,  -132,  -758,  -560,
    -758,   153,  -758,   -54,   -32,  -758,  -554,     1,  -758,  -573,
    -758,  -758,  -758,   102,  -226,  -758,   -18,   -86,  -758,   600,
    -758,  -758,  -758,    45,  -758,  -758,  -758,  -758,  -758,  -758,
    -158,   318,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,
    -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,
    -758,  -758,  -758,  -758,  -138,  -758,  -758,  -758,  -127,   -33,
    -758,  -682,   135,  -758,  -758,  -758,   -88,  -758,   148,  -758,
    -758,   -22,  -758,  -758,  -758,  -758,  -758,  -758,   309,  -758,
    -320,  -758,  -758,  -758,   317,   405,  -758,  -758,  -758,   450,
    -758,  -758,  -758,    72,   -48,  -415,  -450,  -187,   -85,  -758,
    -758,  -758,   330,  -184,  -758,  -758,  -758,   335,   257,  -758,
    -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,  -758,
    -758,  -758,    30,  -758,  -758,  -758,  -758,  -758,  -758,   130,
    -758,  -758,  -592,   260,  -408,  -758,  -758,  -757,     8,   -11,
    -758,  -190,  -141,  -130,  -128,    69,    -5
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
      23,    23,    23,   341,   238,   205,   114,   206,   333,   137,
      30,   425,   805,   526,   554,   525,   438,   235,   236,   237,
     438,    55,   631,   343,    57,   348,   344,   626,   352,   768,
     670,   649,     6,   641,    66,    67,    68,     6,    70,    71,
     708,   338,     5,    19,    20,   489,   133,    76,   489,   129,
     130,   131,   132,   133,   495,   339,    61,   334,   133,   508,
       6,   489,    95,   559,    55,   102,   525,    19,    20,   490,
     104,   105,   525,     9,   657,   219,    19,    20,   525,    19,
      20,   378,   125,   379,    29,   606,    32,     1,   806,   220,
     585,   807,   764,   133,   495,   353,   121,   525,   491,   879,
      73,   134,   624,   585,    62,   204,   426,   175,   629,   201,
     525,   126,    65,   639,   137,   647,   107,   668,   496,   126,
      17,   837,    18,   886,   489,    79,    28,   349,   766,   762,
     751,    80,   175,   175,   745,   175,   175,   175,   760,   175,
     784,    33,    89,    41,   175,   456,   457,   497,   521,   232,
     431,   175,   234,    76,    76,    76,   101,   133,   508,   559,
     475,   118,   120,   657,   461,   443,   481,   295,   301,   467,
     307,    35,   487,    43,   338,   462,   871,   522,   320,   287,
     754,   328,   509,   560,    69,   872,   506,   658,   339,    84,
     202,    72,   338,    76,    87,   519,   525,   463,   468,   432,
     746,    92,   360,    25,    27,    16,   339,   210,    91,   212,
     213,   510,   561,   605,   444,   433,   659,   434,   755,    93,
     435,    61,   436,   361,   362,   363,   364,    42,   530,   366,
     367,   368,   369,   370,   754,    96,   373,   374,   102,   103,
      19,    20,    97,   104,   105,   289,   338,   343,    98,   348,
     344,    99,   352,   102,   103,    19,    20,   100,   104,   105,
     339,   133,   495,   106,   102,   103,    19,    20,   139,   104,
     105,   176,    19,    20,   102,   142,    19,    20,   552,   104,
     105,   145,   203,   214,   133,   508,   207,   230,   231,   107,
      19,    20,   108,   288,   525,   109,    19,    20,   308,   145,
     403,   214,   310,   311,   107,   145,   149,   108,   313,    11,
     314,   322,   226,   330,    12,   107,   151,   336,   108,   424,
     153,   227,   511,   201,   149,   107,    13,    28,   331,   332,
     219,   133,   342,   567,   151,    76,   365,    14,   153,   572,
     151,   568,   441,   114,   220,   577,   441,   573,    19,    20,
      76,   349,   714,   578,   715,   716,    19,    20,   347,   371,
     717,   372,   381,   718,   588,   719,   720,    19,    20,   376,
     386,   392,   589,   398,   721,   290,   400,   600,   722,   114,
     829,   511,   401,   291,   723,   601,   296,   511,   292,   724,
     725,    19,    20,   511,   297,   293,   402,   404,   726,   298,
     216,   218,   405,   294,   223,   225,   299,   229,   848,   869,
     302,   412,   511,   414,   300,   442,    19,    20,   303,   415,
     421,   423,   447,   304,   428,   511,   109,   429,   570,   430,
     305,   446,   438,   360,   575,   315,   449,   452,   306,   566,
     580,   454,   893,   316,    37,   571,    39,   458,   317,   460,
     114,   576,   464,   466,   469,   318,    58,   471,    59,   591,
      60,   215,   217,   319,   221,   222,   224,   645,   228,   646,
     587,   472,   603,    19,    20,   569,   473,   671,   832,   672,
     673,   574,   474,   599,   514,   674,   476,   579,   675,   477,
     478,   676,   756,   741,   479,   687,   480,   688,   482,   677,
     690,   511,   691,   678,   483,   694,   590,   695,   484,   679,
     697,   485,   698,   486,   680,   681,   488,   502,   701,   602,
     702,   503,   504,   682,   527,    19,    20,   505,   507,   548,
     515,   143,   144,    11,   145,    18,   146,   780,    12,   731,
     147,   732,   742,   438,   516,    19,    20,   517,    11,   518,
      13,   520,   528,    12,   529,   148,   883,   533,   796,   149,
     581,   150,   535,   536,   323,    13,   583,   799,   537,   151,
     586,   152,   324,   153,   539,   542,    14,   325,   538,   593,
     540,   544,   546,   543,   326,   545,   781,   549,   551,   547,
     550,   604,   327,    19,    20,   556,   756,   714,   557,   715,
     716,   558,   564,   489,   595,   717,   511,   797,   718,   613,
     719,   720,   686,   607,   608,   615,   800,   703,   704,   721,
     705,   706,   707,   722,   610,   611,   612,   618,   620,   723,
     622,   627,   633,   623,   724,   725,   637,   802,   643,   803,
     644,   663,   692,   699,   664,   708,   693,   712,   736,   737,
     740,   752,   559,   744,   750,   730,   759,   774,   775,    19,
      20,   733,   734,   671,   735,   672,   673,   738,   739,   776,
     761,   674,   779,   783,   675,   788,   793,   676,    76,   840,
     816,   841,   656,   877,   795,   677,   798,   666,   821,   678,
     823,   887,   835,   824,   825,   679,   689,   826,   836,   827,
     680,   681,   828,   657,   771,   772,   696,   773,   830,   842,
     114,   777,   778,   833,   855,   843,   839,   880,   844,   845,
     846,   847,    76,   858,   876,   884,   789,   867,   711,   792,
     880,   686,   878,   865,   871,   890,   892,   337,   114,   801,
     888,    90,   632,    76,   427,   831,   834,   642,   700,   854,
     891,   885,    76,   857,   765,   233,   667,   652,   441,   822,
     541,   534,   445,   815,   407,   817,   532,   753,   820,   531,
     730,   594,   794,     0,   114,     0,   849,   850,   851,   852,
     853,     0,   770,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   860,   861,   862,   863,   864,
       0,     0,     0,     0,     0,     0,     0,   114,    19,    20,
       0,     0,   177,     0,   178,   179,     0,     0,     0,     0,
     180,     0,     0,   181,     0,     0,   182,   183,   184,     0,
       0,     0,    79,     0,   185,   186,   187,   188,   189,     0,
     190,   191,     0,     0,   192,     0,     0,   193,     0,   194,
     195,     0,     0,   196,     0,   197,    19,    20,     0,     0,
      44,     0,    45,    46,   175,     0,     0,     0,    76,   441,
       0,    47,     0,     0,    48,     0,    76,   175,     0,     0,
       0,     0,     0,     0,     0,     0,    49,     0,     0,     0,
       0,     0,    50,     0,     0,     0,    51,     0,     0,     0,
       0,     0,     0,     0,   118,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   866,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   870,     0,   875,    19,    20,     0,
       0,   437,   241,   242,   243,   244,   245,   246,   247,   248,
     249,   250,   251,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,   271,   272,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   239,     0,   240,     0,
       0,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,    19,    20,     0,     0,     0,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284
};

static const yytype_int16 yycheck[] =
{
      11,    12,    13,   207,   162,   135,    91,   135,   198,   101,
      15,   331,   769,   428,   464,   423,   353,   158,   159,   160,
     357,    32,   583,   210,    35,   212,   210,   581,   212,   711,
     622,   604,     3,   593,    45,    46,    47,     3,    49,    50,
      12,    32,     0,     8,     9,     7,     6,    52,     7,    97,
      98,    99,   100,     6,     7,    46,    12,   198,     6,     7,
       3,     7,    80,     7,    75,     6,   474,     8,     9,    31,
      11,    12,   480,     4,     7,    42,     8,     9,   486,     8,
       9,    60,    31,    62,    15,   535,    17,     4,     8,    56,
     498,    11,    64,     6,     7,    24,    67,   505,    60,    64,
      66,    61,    61,   511,    60,   123,   332,   118,    61,   120,
     518,    60,    43,    61,   206,    61,    57,    61,    31,    60,
      57,   803,     3,   880,     7,    56,    45,   212,    61,   702,
     691,    62,   143,   144,   688,   146,   147,   148,   698,   150,
     732,    12,    73,    59,   155,   371,   372,    60,    31,   154,
     340,   162,   157,   158,   159,   160,    87,     6,     7,     7,
     386,    92,    93,     7,    18,   355,   392,   178,   179,    22,
     181,    62,   398,    58,    32,    29,    49,    60,   189,   171,
      20,   192,    31,    31,    59,    58,   412,    31,    46,    12,
     121,    59,    32,   198,    60,   421,   604,    51,    51,   340,
      58,    63,   213,    12,    13,     8,    46,   138,    59,   140,
     141,    60,    60,   533,   355,   345,    60,   345,    58,    57,
     350,    12,   350,   215,   216,   217,   218,    30,   432,   221,
     222,   223,   224,   225,    20,    59,   228,   229,     6,     7,
       8,     9,    59,    11,    12,   176,    32,   434,    59,   436,
     434,    59,   436,     6,     7,     8,     9,    59,    11,    12,
      46,     6,     7,    31,     6,     7,     8,     9,    61,    11,
      12,    58,     8,     9,     6,    64,     8,     9,    31,    11,
      12,    17,    61,    19,     6,     7,    57,    12,    42,    57,
       8,     9,    60,    60,   702,    63,     8,     9,    59,    17,
     311,    19,    59,    59,    57,    17,    42,    60,    59,    16,
      59,    59,    30,    59,    21,    57,    52,    61,    60,   330,
      56,    39,   414,   334,    42,    57,    33,    45,    59,    59,
      42,     6,    58,   472,    52,   340,    42,    44,    56,   478,
      52,   473,   353,   428,    56,   484,   357,   479,     8,     9,
     355,   436,    12,   485,    14,    15,     8,     9,    61,    59,
      20,    59,    61,    23,   503,    25,    26,     8,     9,    62,
      59,    59,   504,    59,    34,    27,    12,   516,    38,   464,
     795,   473,    59,    35,    44,   517,    27,   479,    40,    49,
      50,     8,     9,   485,    35,    47,    12,    59,    58,    40,
     143,   144,    63,    55,   147,   148,    47,   150,   823,   859,
      27,    59,   504,    59,    55,    64,     8,     9,    35,    63,
      59,    59,    62,    40,    59,   517,    63,    59,   476,    59,
      47,    60,   769,   444,   482,    27,    62,    62,    55,   471,
     488,    62,   892,    35,    24,   477,    26,    62,    40,    59,
     535,   483,    59,     7,    59,    47,    36,    59,    38,   507,
      40,   143,   144,    55,   146,   147,   148,   597,   150,   597,
     502,    59,   520,     8,     9,   474,    59,    12,   798,    14,
      15,   480,    59,   515,   415,    20,    59,   486,    23,    59,
      59,    26,   696,   683,    59,   625,    59,   625,    59,    34,
     630,   593,   630,    38,    59,   635,   505,   635,    59,    44,
     640,    59,   640,    59,    49,    50,    59,    59,   648,   518,
     648,    59,    59,    58,    11,     8,     9,    59,    59,   460,
      59,    14,    15,    16,    17,     3,    19,   727,    21,   669,
      23,   669,   683,   880,    59,     8,     9,    59,    16,    59,
      33,    59,    11,    21,    58,    38,   876,    59,   748,    42,
     491,    44,    59,    61,    27,    33,   497,   757,    18,    52,
     501,    54,    35,    56,    18,    18,    44,    40,   448,   510,
     450,    18,    18,   453,    47,   455,   727,    59,    59,   459,
      60,   522,    55,     8,     9,    61,   800,    12,    59,    14,
      15,    59,    57,     7,     7,    20,   698,   748,    23,    63,
      25,    26,   623,    59,    59,     7,   757,   650,   651,    34,
     653,   654,   655,    38,    59,    59,    59,    57,     7,    44,
     561,    57,    12,   564,    49,    50,    57,   767,    65,   767,
      64,    61,    65,     7,    63,    12,    64,    59,    59,    59,
      59,    12,     7,    61,    61,   666,    61,    59,    59,     8,
       9,   672,   673,    12,   675,    14,    15,   678,   679,    59,
      61,    20,    59,    61,    23,    12,    58,    26,   683,   809,
      12,   809,   613,   873,    59,    34,    59,   618,    58,    38,
      59,   881,    63,    59,    59,    44,   627,    59,    61,    59,
      49,    50,    59,     7,   715,   716,   637,   718,    58,    65,
     795,   722,   723,    58,    57,    59,    64,   875,    59,    59,
      59,    59,   727,    31,    59,    58,   737,    63,   659,   740,
     888,   742,   873,    64,    49,    64,    59,   206,   823,   763,
     881,    75,   584,   748,   334,   797,   800,   594,   646,   835,
     888,   878,   757,   841,   709,   155,   621,   609,   769,   781,
     451,   444,   357,   774,   314,   776,   436,   695,   779,   434,
     781,   511,   742,    -1,   859,    -1,   824,   825,   826,   827,
     828,    -1,   713,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   843,   844,   845,   846,   847,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   892,     8,     9,
      -1,    -1,    12,    -1,    14,    15,    -1,    -1,    -1,    -1,
      20,    -1,    -1,    23,    -1,    -1,    26,    27,    28,    -1,
      -1,    -1,   763,    -1,    34,    35,    36,    37,    38,    -1,
      40,    41,    -1,    -1,    44,    -1,    -1,    47,    -1,    49,
      50,    -1,    -1,    53,    -1,    55,     8,     9,    -1,    -1,
      12,    -1,    14,    15,   875,    -1,    -1,    -1,   873,   880,
      -1,    23,    -1,    -1,    26,    -1,   881,   888,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,    -1,
      -1,    -1,    44,    -1,    -1,    -1,    48,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   835,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   855,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   865,    -1,   867,     8,     9,    -1,
      -1,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,     8,    -1,    10,    -1,
      -1,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,     8,     9,    -1,    -1,    -1,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     4,    69,    72,    73,     0,     3,    71,    74,   293,
     294,    16,    21,    33,    44,    92,    93,    57,     3,     8,
       9,    96,   100,   287,    94,   100,    98,   100,    45,   293,
     294,    75,   293,    12,   101,    62,    97,   101,    95,   101,
      99,    59,    93,    58,    12,    14,    15,    23,    26,    38,
      44,    48,    76,    77,    78,   287,   102,   287,   101,   101,
     101,    12,    60,   162,   164,   293,   287,   287,   287,    59,
     287,   287,    59,    66,   289,   290,   294,    79,   103,   293,
     293,    81,    83,    80,    12,    82,    84,    60,    85,   293,
      78,    59,    63,    57,   163,   164,    59,    59,    59,    59,
      59,   293,     6,     7,    11,    12,    31,    57,    60,    63,
     228,   243,   244,   245,   246,   251,   287,   165,   293,   104,
     293,    67,   291,   292,   293,    31,    60,   242,   246,   242,
     242,   242,   242,     6,    61,    86,    87,    88,   252,    61,
     247,   229,    64,    14,    15,    17,    19,    23,    38,    42,
      44,    52,    54,    56,    93,   166,   167,   168,   173,   174,
     175,   176,   177,   178,   179,   180,   183,   186,   193,   195,
     197,   256,   257,   259,   260,   287,    58,    12,    14,    15,
      20,    23,    26,    27,    28,    34,    35,    36,    37,    38,
      40,    41,    44,    47,    49,    50,    53,    55,   105,   106,
     107,   287,   293,    61,   164,   291,   292,    57,    89,   253,
     293,   248,   293,   293,    19,   179,   256,   179,   256,    42,
      56,   179,   179,   256,   179,   256,    30,    39,   179,   256,
      12,    42,   294,   167,   294,   290,   290,   290,   178,     8,
      10,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    70,   286,   286,    60,   293,
      27,    35,    40,    47,    55,   287,    27,    35,    40,    47,
      55,   287,    27,    35,    40,    47,    55,   287,    59,   120,
      59,    59,   114,    59,    59,    27,    35,    40,    47,    55,
     287,   132,    59,    27,    35,    40,    47,    55,   287,   126,
      59,    59,    59,   289,   290,   108,    61,    87,    32,    46,
      90,    91,    58,   245,   251,   254,   255,    61,   245,   246,
     249,   250,   251,    24,   230,   231,   232,   234,   235,   236,
     287,   286,   286,   286,   286,    42,   286,   286,   286,   286,
     286,    59,    59,   286,   286,   169,    62,   181,    60,    62,
     261,    61,   122,   116,   134,   128,    59,   110,   124,   118,
     136,   130,    59,   112,   121,   115,   133,   127,    59,   109,
      12,    59,    12,   287,    59,    63,   237,   237,   123,   117,
     135,   129,    59,   111,    59,    63,   158,   125,   119,   137,
     131,    59,   113,    59,   287,   228,   162,   107,    59,    59,
      59,   289,   290,   291,   292,   291,   292,    12,    70,   233,
     285,   287,    64,   289,   290,   233,    60,    62,   263,    62,
     265,   184,    62,   262,    62,   264,   162,   162,    62,   266,
      59,    18,    29,    51,    59,   226,     7,    22,    51,    59,
     277,    59,    59,    59,    59,   162,    59,    59,    59,    59,
      59,   162,    59,    59,    59,    59,    59,   162,    59,     7,
      31,    60,   152,   154,   282,     7,    31,    60,    88,   138,
     140,   238,    59,    59,    59,    59,   162,    59,     7,    31,
      60,    88,   145,   147,   293,    59,    59,    59,    59,   162,
      59,    31,    60,   155,   157,   282,   243,    11,    11,    58,
      91,   255,   250,    59,   232,    59,    61,    18,   277,    18,
     277,   226,    18,   277,    18,   277,    18,   277,   293,    59,
      60,    59,    31,   227,   244,   182,    61,    59,    59,     7,
      31,    60,   278,   280,    57,   267,   152,   138,   145,   155,
     242,   152,   138,   145,   155,   242,   152,   138,   145,   155,
     242,   293,   141,   293,   281,   282,   293,   152,   138,   145,
     155,   242,   148,   293,   281,     7,   159,   160,   161,   152,
     138,   145,   155,   242,   293,   228,   244,    59,    59,   185,
      59,    59,    59,    63,   187,     7,   283,   196,    57,   216,
       7,   258,   293,   293,    61,   153,   154,    57,   142,    61,
     139,   140,   142,    12,   239,   240,   241,    57,   149,    61,
     146,   147,   149,    65,    64,   291,   292,    61,   156,   157,
     188,   190,   216,   191,   189,   192,   293,     7,    31,    60,
     207,   209,   284,    61,    63,   210,   293,   210,    61,   279,
     280,    12,    14,    15,    20,    23,    26,    34,    38,    44,
      49,    50,    58,   268,   269,   270,   287,   291,   292,   293,
     291,   292,    65,    64,   291,   292,   293,   291,   292,     7,
     161,   291,   292,   207,   207,   207,   207,   207,    12,   170,
     171,   293,    59,   211,    12,    14,    15,    20,    23,    25,
      26,    34,    38,    44,    49,    50,    58,   217,   218,   219,
     287,   291,   292,   287,   287,   287,    59,    59,   287,   287,
      59,   289,   290,   271,    61,   154,    58,    91,   143,   144,
      61,   140,    12,   241,    20,    58,    91,   150,   151,    61,
     147,    61,   157,   172,    64,   171,    61,   208,   209,   194,
     293,   287,   287,   287,    59,    59,    59,   287,   287,    59,
     289,   290,   220,    61,   280,   273,   275,   272,    12,   287,
     274,   276,   287,    58,   270,    59,   289,   290,    59,   289,
     290,   103,   291,   292,   198,   285,     8,    11,   212,   213,
     214,   288,   222,   224,   221,   287,    12,   287,   223,   225,
     287,    58,   219,    59,    59,    59,    59,    59,    59,   243,
      58,   144,   228,    58,   151,    63,    61,   209,   199,    64,
     291,   292,    65,    59,    59,    59,    59,    59,   243,   242,
     242,   242,   242,   242,   165,    57,   204,   214,    31,   215,
     242,   242,   242,   242,   242,    64,   293,    63,   200,   244,
     293,    49,    58,   205,   206,   293,    59,   289,   290,    64,
     178,   201,   202,   228,    58,   206,   285,   289,   290,   203,
      64,   202,    59,   244
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
        case 48:

/* Line 1455 of yacc.c  */
#line 1316 "pxr/usd/sdf/textFileFormat.yy"
    {

        // Store the names of the root prims.
        _SetField(
            SdfPath::AbsoluteRootPath(), SdfChildrenKeys->PrimChildren,
            context->nameChildrenStack.back(), context);
        context->nameChildrenStack.pop_back();
    ;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 1327 "pxr/usd/sdf/textFileFormat.yy"
    {
            _MatchMagicIdentifier((yyvsp[(1) - (1)]), context);
            context->nameChildrenStack.push_back(std::vector<TfToken>());

            _CreateSpec(
                SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot, context);

            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 1340 "pxr/usd/sdf/textFileFormat.yy"
    {
            // Abort if error after layer metadata.
            ABORT_IF_ERROR(context->seenError);

            // If we're only reading metadata and we got here, 
            // we're done.
            if (context->metadataOnly)
                YYACCEPT;
        ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 1366 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 1371 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 1373 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 1380 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 1383 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 1386 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 1389 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 1392 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 1395 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 1398 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 1401 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 1404 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 1407 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 1412 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1424 "pxr/usd/sdf/textFileFormat.yy"
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

  case 77:

/* Line 1455 of yacc.c  */
#line 1443 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->subLayerPaths.push_back(context->layerRefPath);
            context->subLayerOffsets.push_back(context->layerRefOffset);
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1451 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = (yyvsp[(1) - (1)]).Get<std::string>();
            context->layerRefOffset = SdfLayerOffset();
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1469 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetOffset( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1473 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetScale( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 1489 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken();
        ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 1493 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 1497 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken();
        ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 1501 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 1505 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken();
        ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1509 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1513 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1523 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1524 "pxr/usd/sdf/textFileFormat.yy"
    { 
            (yyval) = std::string( (yyvsp[(1) - (3)]).Get<std::string>() + '.'
                    + (yyvsp[(3) - (3)]).Get<std::string>() ); 
        ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1531 "pxr/usd/sdf/textFileFormat.yy"
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

  case 103:

/* Line 1455 of yacc.c  */
#line 1564 "pxr/usd/sdf/textFileFormat.yy"
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

  case 113:

/* Line 1455 of yacc.c  */
#line 1612 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 1617 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePrim, context);
        ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1619 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1626 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 1629 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1632 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1635 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1638 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1641 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1644 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1647 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1650 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1653 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1658 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1665 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Kind, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1672 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission, 
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context), 
                context);
        ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1679 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1683 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1686 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1690 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1693 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1697 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1700 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1704 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1707 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1711 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1714 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->payloadParsingRefs.clear();
        ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1718 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetPayloadListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1722 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1724 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1727 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1729 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1732 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1734 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1737 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1739 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1742 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1744 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1747 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1749 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1753 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1755 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1758 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1760 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1763 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1765 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1768 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1770 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1773 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1775 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1778 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1780 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1784 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1788 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1791 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1795 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1798 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1802 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1805 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1809 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypePrepended, context);
        ;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1812 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1816 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeAppended, context);
        ;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1819 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1823 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1828 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Relocates, 
                context->relocatesParsingMap, context);
            context->relocatesParsingMap.clear();
        ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1836 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSelection(context);
        ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1840 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeExplicit, context); 
            context->nameVector.clear();
        ;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1844 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeDeleted, context);
            context->nameVector.clear();
        ;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1848 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAdded, context);
            context->nameVector.clear();
        ;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1852 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypePrepended, context);
            context->nameVector.clear();
        ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1856 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAppended, context);
            context->nameVector.clear();
        ;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1860 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeOrdered, context);
            context->nameVector.clear();
        ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1866 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1871 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1878 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrefixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1886 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SuffixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1907 "pxr/usd/sdf/textFileFormat.yy"
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

  case 196:

/* Line 1455 of yacc.c  */
#line 1919 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Internal payloads do not begin with an asset path so there's
        // no layer_ref rule, but we need to make sure we reset state the
        // so we don't pick up data from a previously-parsed payload.
        context->layerRefPath.clear();
        context->layerRefOffset = SdfLayerOffset();
        ABORT_IF_ERROR(context->seenError);
      ;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1927 "pxr/usd/sdf/textFileFormat.yy"
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

  case 210:

/* Line 1455 of yacc.c  */
#line 1970 "pxr/usd/sdf/textFileFormat.yy"
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

  case 211:

/* Line 1455 of yacc.c  */
#line 1983 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Internal references do not begin with an asset path so there's
        // no layer_ref rule, but we need to make sure we reset state the
        // so we don't pick up data from a previously-parsed reference.
        context->layerRefPath.clear();
        context->layerRefOffset = SdfLayerOffset();
        ABORT_IF_ERROR(context->seenError);
      ;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1991 "pxr/usd/sdf/textFileFormat.yy"
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

  case 226:

/* Line 1455 of yacc.c  */
#line 2036 "pxr/usd/sdf/textFileFormat.yy"
    {
        _InheritAppendPath(context);
        ;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 2054 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SpecializesAppendPath(context);
        ;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 2074 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelocatesAdd((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 2090 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->nameVector.push_back(TfToken((yyvsp[(1) - (1)]).Get<std::string>()));
        ;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 2108 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 2109 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 2110 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 2116 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string name = (yyvsp[(2) - (2)]).Get<std::string>();
        ERROR_IF_NOT_ALLOWED(context, SdfSchema::IsValidVariantIdentifier(name));

        context->currentVariantSetNames.push_back( name );
        context->currentVariantNames.push_back( std::vector<std::string>() );

        context->path = context->path.AppendVariantSelection(name, "");
    ;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 2124 "pxr/usd/sdf/textFileFormat.yy"
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

  case 258:

/* Line 1455 of yacc.c  */
#line 2155 "pxr/usd/sdf/textFileFormat.yy"
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

  case 259:

/* Line 1455 of yacc.c  */
#line 2175 "pxr/usd/sdf/textFileFormat.yy"
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

  case 260:

/* Line 1455 of yacc.c  */
#line 2198 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 2207 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 2229 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 2232 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue(SdfVariabilityConfig);
    ;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 2238 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->assoc = VtValue();
    ;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 2244 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue((yyvsp[(1) - (1)]).Get<std::string>(), context);
    ;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 2247 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue(std::string((yyvsp[(1) - (3)]).Get<std::string>() + "[]"), context);
    ;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 2253 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue();
        context->custom = false;
    ;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 2257 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
    ;}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 2263 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (2)]), context);

        if (!context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 2270 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 2275 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 2281 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        _PrimInitAttribute((yyvsp[(3) - (3)]), context);

        if (!context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 2289 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 2294 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 2300 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 2304 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeExplicit, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 2308 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 2312 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAdded, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 2316 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 2320 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypePrepended, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 2324 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 2328 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAppended, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 2332 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 2336 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeDeleted, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 2340 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 2344 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeOrdered, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 2351 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (8)]), context);
        context->mapperTarget = context->savedPath;
        context->path = context->path.AppendMapper(context->mapperTarget);
    ;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 2356 "pxr/usd/sdf/textFileFormat.yy"
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

  case 291:

/* Line 1455 of yacc.c  */
#line 2375 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        ;}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 2378 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            context->path = context->path.GetParentPath(); // pop attr
        ;}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 2400 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string mapperName((yyvsp[(1) - (1)]).Get<std::string>());
        if (_HasSpec(context->path, context)) {
            Err(context, "Duplicate mapper");
        }

        _CreateSpec(context->path, SdfSpecTypeMapper, context);
        _SetField(context->path, SdfFieldKeys->TypeName, mapperName, context);
    ;}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 2420 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfChildrenKeys->MapperArgChildren, 
            context->mapperArgsNameVector, context);
        context->mapperArgsNameVector.clear();
    ;}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 2434 "pxr/usd/sdf/textFileFormat.yy"
    {
            TfToken mapperParamName((yyvsp[(2) - (2)]).Get<std::string>());
            context->mapperArgsNameVector.push_back(mapperParamName);
            context->path = context->path.AppendMapperArg(mapperParamName);

            _CreateSpec(context->path, SdfSpecTypeMapperArg, context);

        ;}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 2441 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->MapperArgValue, 
                context->currentValue, context);
            context->path = context->path.GetParentPath(); // pop mapper arg
        ;}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 2461 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryArgs, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 2482 "pxr/usd/sdf/textFileFormat.yy"
    {
            _AttributeAppendConnectionPath(context);
        ;}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 2492 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples = SdfTimeSampleMap();
    ;}
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 2508 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (2)]).Get<double>();
    ;}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 2511 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples[ context->timeSampleTime ] = context->currentValue;
    ;}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 2515 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (3)]).Get<double>();
        context->timeSamples[ context->timeSampleTime ] 
            = VtValue(SdfValueBlock());  
    ;}
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 2545 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 2550 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeAttribute, context);
        ;}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 2552 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2559 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 2562 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 2565 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 2568 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 344:

/* Line 1455 of yacc.c  */
#line 2571 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 345:

/* Line 1455 of yacc.c  */
#line 2574 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 346:

/* Line 1455 of yacc.c  */
#line 2577 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 347:

/* Line 1455 of yacc.c  */
#line 2580 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 348:

/* Line 1455 of yacc.c  */
#line 2583 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 349:

/* Line 1455 of yacc.c  */
#line 2586 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 2591 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 2598 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 352:

/* Line 1455 of yacc.c  */
#line 2605 "pxr/usd/sdf/textFileFormat.yy"
    {
             _SetField(
                 context->path, SdfFieldKeys->DisplayUnit,
                 _GetDisplayUnitFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                 context);
        ;}
    break;

  case 353:

/* Line 1455 of yacc.c  */
#line 2613 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 354:

/* Line 1455 of yacc.c  */
#line 2618 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken(), context);
        ;}
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 2631 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfFieldKeys->Default,
            context->currentValue, context);
    ;}
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 2636 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfFieldKeys->Default,
            SdfValueBlock(), context);
    ;}
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 2648 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 2651 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 2667 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertValue((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2670 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertDictionary((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2688 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory((yyvsp[(1) - (1)]), context);
    ;}
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2694 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitShapedFactory((yyvsp[(1) - (3)]), context);
    ;}
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 2704 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 374:

/* Line 1455 of yacc.c  */
#line 2707 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 379:

/* Line 1455 of yacc.c  */
#line 2723 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory(Value(std::string("string")), context);
            _ValueAppendAtomic((yyvsp[(3) - (3)]), context);
            _ValueSetAtomic(context);
            _DictionaryInsertValue((yyvsp[(1) - (3)]), context);
        ;}
    break;

  case 380:

/* Line 1455 of yacc.c  */
#line 2736 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->currentValue = VtValue();
        if (context->values.IsRecordingString()) {
            context->values.SetRecordedString("None");
        }
    ;}
    break;

  case 381:

/* Line 1455 of yacc.c  */
#line 2742 "pxr/usd/sdf/textFileFormat.yy"
    {
        _ValueSetList(context);
    ;}
    break;

  case 382:

/* Line 1455 of yacc.c  */
#line 2752 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->currentValue.Swap(context->currentDictionaries[0]);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2757 "pxr/usd/sdf/textFileFormat.yy"
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

  case 385:

/* Line 1455 of yacc.c  */
#line 2770 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetAtomic(context);
        ;}
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 2773 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetTuple(context);
        ;}
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2776 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetList(context);
        ;}
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2779 "pxr/usd/sdf/textFileFormat.yy"
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

  case 389:

/* Line 1455 of yacc.c  */
#line 2790 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetCurrentToSdfPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2796 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2799 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2802 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs identifiers to be stored as TfToken
            // instead of std::string to be able to distinguish between them.
            _ValueAppendAtomic(TfToken((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2807 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs asset paths to be stored as
            // SdfAssetPath instead of std::string to be able to distinguish
            // between them
            _ValueAppendAtomic(SdfAssetPath((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2820 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginList();
        ;}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2823 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndList();
        ;}
    break;

  case 402:

/* Line 1455 of yacc.c  */
#line 2848 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginTuple();
        ;}
    break;

  case 403:

/* Line 1455 of yacc.c  */
#line 2850 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndTuple();
        ;}
    break;

  case 409:

/* Line 1455 of yacc.c  */
#line 2873 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 410:

/* Line 1455 of yacc.c  */
#line 2877 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 411:

/* Line 1455 of yacc.c  */
#line 2881 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 412:

/* Line 1455 of yacc.c  */
#line 2885 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 413:

/* Line 1455 of yacc.c  */
#line 2892 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context); 
        ;}
    break;

  case 414:

/* Line 1455 of yacc.c  */
#line 2895 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 415:

/* Line 1455 of yacc.c  */
#line 2904 "pxr/usd/sdf/textFileFormat.yy"
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

  case 416:

/* Line 1455 of yacc.c  */
#line 2919 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (2)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 417:

/* Line 1455 of yacc.c  */
#line 2924 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeExplicit, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 2929 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 2932 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeDeleted, context); 
            _PrimEndRelationship(context);
        ;}
    break;

  case 420:

/* Line 1455 of yacc.c  */
#line 2937 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2941 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAdded, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 2945 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2949 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypePrepended, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 2953 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 425:

/* Line 1455 of yacc.c  */
#line 2957 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAppended, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 426:

/* Line 1455 of yacc.c  */
#line 2962 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 427:

/* Line 1455 of yacc.c  */
#line 2965 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeOrdered, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 428:

/* Line 1455 of yacc.c  */
#line 2970 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context);
            context->relParsingAllowTargetData = true;
            _RelationshipAppendTargetPath((yyvsp[(4) - (5)]), context);
            _RelationshipInitTarget(context->relParsingTargetPaths->back(),
                                    context);
        ;}
    break;

  case 439:

/* Line 1455 of yacc.c  */
#line 2999 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 440:

/* Line 1455 of yacc.c  */
#line 3004 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeRelationship, context);
        ;}
    break;

  case 441:

/* Line 1455 of yacc.c  */
#line 3006 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 442:

/* Line 1455 of yacc.c  */
#line 3013 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 443:

/* Line 1455 of yacc.c  */
#line 3016 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 444:

/* Line 1455 of yacc.c  */
#line 3019 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 445:

/* Line 1455 of yacc.c  */
#line 3022 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 446:

/* Line 1455 of yacc.c  */
#line 3025 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypePrepended;
        ;}
    break;

  case 447:

/* Line 1455 of yacc.c  */
#line 3028 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 448:

/* Line 1455 of yacc.c  */
#line 3031 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAppended;
        ;}
    break;

  case 449:

/* Line 1455 of yacc.c  */
#line 3034 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 450:

/* Line 1455 of yacc.c  */
#line 3037 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 451:

/* Line 1455 of yacc.c  */
#line 3040 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 452:

/* Line 1455 of yacc.c  */
#line 3045 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 453:

/* Line 1455 of yacc.c  */
#line 3052 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 454:

/* Line 1455 of yacc.c  */
#line 3060 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 455:

/* Line 1455 of yacc.c  */
#line 3065 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 459:

/* Line 1455 of yacc.c  */
#line 3079 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 460:

/* Line 1455 of yacc.c  */
#line 3082 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 464:

/* Line 1455 of yacc.c  */
#line 3094 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipAppendTargetPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 465:

/* Line 1455 of yacc.c  */
#line 3104 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->savedPath = SdfPath();
    ;}
    break;

  case 467:

/* Line 1455 of yacc.c  */
#line 3111 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrim((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 468:

/* Line 1455 of yacc.c  */
#line 3117 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetProperty((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 469:

/* Line 1455 of yacc.c  */
#line 3123 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrimOrPropertyScenePath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 478:

/* Line 1455 of yacc.c  */
#line 3155 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;



/* Line 1455 of yacc.c  */
#line 6227 "pxr/usd/sdf/textFileFormat.tab.cpp"
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
#line 3187 "pxr/usd/sdf/textFileFormat.yy"


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

