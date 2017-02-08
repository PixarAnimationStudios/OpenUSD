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

#include "pxr/base/tracelite/trace.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/mallocTag.h"

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <sstream>
#include <string>
#include <sys/mman.h>
#include <unistd.h>
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
        if (!allow) {                                         \
            Err(context, "%s", allow.GetWhyNot().c_str());       \
        }                                                        \
    }

#define ERROR_AND_RETURN_IF_NOT_ALLOWED(context, allowed)        \
    {                                                            \
        const SdfAllowed allow = allowed;                        \
        if (!allow) {                                         \
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
extern int textFileFormatYyget_leng(yyscan_t yyscanner);
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
                "using <%s> instead.  Resaving the menva file will fix "
                "this issue.  (See also bug 68132.)",
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
#line 1209 "pxr/usd/sdf/textFileFormat.tab.cpp"

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
     TOK_ATTRIBUTES = 270,
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
     TOK_PROPERTIES = 293,
     TOK_REFERENCES = 294,
     TOK_RELOCATES = 295,
     TOK_REL = 296,
     TOK_RENAMES = 297,
     TOK_REORDER = 298,
     TOK_ROOTPRIMS = 299,
     TOK_SCALE = 300,
     TOK_SPECIALIZES = 301,
     TOK_SUBLAYERS = 302,
     TOK_SYMMETRYARGUMENTS = 303,
     TOK_SYMMETRYFUNCTION = 304,
     TOK_TIME_SAMPLES = 305,
     TOK_UNIFORM = 306,
     TOK_VARIANTS = 307,
     TOK_VARIANTSET = 308,
     TOK_VARIANTSETS = 309,
     TOK_VARYING = 310
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
#line 1306 "pxr/usd/sdf/textFileFormat.tab.cpp"

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
#define YYLAST   998

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  68
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  208
/* YYNRULES -- Number of rules.  */
#define YYNRULES  444
/* YYNRULES -- Number of states.  */
#define YYNSTATES  785

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   310

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      56,    57,     2,     2,    67,     2,    61,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    64,    66,
       2,    58,     2,     2,    65,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    59,     2,    60,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    62,     2,    63,     2,     2,     2,     2,
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
      55
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
      79,    81,    83,    85,    87,    89,    91,    93,    97,    98,
     102,   104,   110,   112,   116,   118,   122,   124,   126,   127,
     132,   133,   139,   140,   146,   147,   153,   157,   161,   165,
     171,   173,   177,   180,   182,   183,   188,   190,   194,   198,
     202,   204,   208,   209,   213,   214,   219,   220,   224,   225,
     230,   231,   235,   236,   241,   246,   248,   252,   253,   260,
     262,   268,   270,   274,   276,   280,   282,   284,   286,   288,
     289,   294,   295,   301,   302,   308,   309,   315,   319,   323,
     327,   328,   333,   334,   339,   340,   346,   347,   353,   354,
     360,   361,   366,   367,   373,   374,   380,   381,   387,   388,
     393,   394,   400,   401,   407,   408,   414,   418,   422,   426,
     431,   436,   441,   445,   448,   452,   456,   458,   461,   463,
     465,   469,   475,   477,   481,   485,   486,   490,   491,   495,
     501,   503,   507,   509,   513,   515,   517,   521,   527,   529,
     533,   535,   537,   539,   543,   549,   551,   555,   557,   562,
     563,   566,   568,   572,   576,   578,   584,   586,   590,   592,
     594,   597,   599,   602,   605,   608,   611,   614,   617,   618,
     628,   630,   633,   634,   642,   647,   652,   654,   656,   658,
     660,   662,   664,   668,   670,   673,   674,   675,   682,   683,
     684,   692,   693,   701,   702,   711,   712,   721,   722,   731,
     732,   743,   744,   752,   754,   756,   758,   760,   762,   763,
     768,   769,   773,   779,   781,   785,   786,   792,   793,   797,
     803,   805,   809,   813,   815,   817,   821,   827,   829,   833,
     835,   836,   841,   842,   848,   849,   852,   854,   858,   859,
     864,   868,   869,   873,   879,   881,   885,   887,   889,   891,
     893,   894,   899,   900,   906,   907,   913,   914,   920,   924,
     928,   932,   936,   939,   940,   943,   945,   947,   948,   954,
     955,   958,   960,   964,   969,   974,   976,   978,   980,   982,
     984,   988,   989,   995,   996,   999,  1001,  1005,  1009,  1011,
    1013,  1015,  1017,  1019,  1021,  1023,  1025,  1028,  1030,  1032,
    1034,  1036,  1038,  1039,  1044,  1048,  1050,  1054,  1056,  1058,
    1060,  1061,  1066,  1070,  1072,  1076,  1078,  1080,  1082,  1085,
    1089,  1092,  1093,  1101,  1108,  1109,  1115,  1116,  1122,  1123,
    1129,  1130,  1136,  1137,  1145,  1147,  1149,  1150,  1154,  1160,
    1162,  1166,  1168,  1170,  1172,  1174,  1175,  1180,  1181,  1187,
    1188,  1194,  1195,  1201,  1205,  1209,  1213,  1216,  1217,  1220,
    1222,  1224,  1228,  1234,  1236,  1240,  1243,  1245,  1249,  1250,
    1252,  1253,  1259,  1260,  1263,  1265,  1269,  1271,  1273,  1278,
    1279,  1281,  1283,  1285,  1287,  1289,  1291,  1293,  1295,  1297,
    1299,  1301,  1303,  1305,  1307,  1308,  1310,  1313,  1315,  1317,
    1319,  1322,  1323,  1325,  1327
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
      -1,    74,    -1,    74,    90,   274,    -1,    -1,     4,    73,
      71,    -1,   274,    -1,   274,    56,    75,    57,   274,    -1,
     274,    -1,   274,    76,   270,    -1,    78,    -1,    76,   271,
      78,    -1,   268,    -1,    12,    -1,    -1,    77,    79,    58,
     219,    -1,    -1,    23,   268,    80,    58,   218,    -1,    -1,
      14,   268,    81,    58,   218,    -1,    -1,    43,   268,    82,
      58,   218,    -1,    26,    58,    12,    -1,    47,    58,    83,
      -1,    59,   274,    60,    -1,    59,   274,    84,   272,    60,
      -1,    85,    -1,    84,   273,    85,    -1,    86,    87,    -1,
       6,    -1,    -1,    56,    88,   270,    57,    -1,    89,    -1,
      88,   271,    89,    -1,    32,    58,    11,    -1,    45,    58,
      11,    -1,    91,    -1,    90,   275,    91,    -1,    -1,    21,
      92,    99,    -1,    -1,    21,    98,    93,    99,    -1,    -1,
      16,    94,    99,    -1,    -1,    16,    98,    95,    99,    -1,
      -1,    33,    96,    99,    -1,    -1,    33,    98,    97,    99,
      -1,    43,    44,    58,   141,    -1,   268,    -1,    98,    61,
     268,    -1,    -1,    12,   100,   101,    62,   144,    63,    -1,
     274,    -1,   274,    56,   102,    57,   274,    -1,   274,    -1,
     274,   103,   270,    -1,   105,    -1,   103,   271,   105,    -1,
     268,    -1,    20,    -1,    48,    -1,    12,    -1,    -1,   104,
     106,    58,   219,    -1,    -1,    23,   268,   107,    58,   218,
      -1,    -1,    14,   268,   108,    58,   218,    -1,    -1,    43,
     268,   109,    58,   218,    -1,    26,    58,    12,    -1,    28,
      58,    12,    -1,    34,    58,   268,    -1,    -1,    35,   110,
      58,   123,    -1,    -1,    27,   111,    58,   131,    -1,    -1,
      23,    27,   112,    58,   131,    -1,    -1,    14,    27,   113,
      58,   131,    -1,    -1,    43,    27,   114,    58,   131,    -1,
      -1,    46,   115,    58,   134,    -1,    -1,    23,    46,   116,
      58,   134,    -1,    -1,    14,    46,   117,    58,   134,    -1,
      -1,    43,    46,   118,    58,   134,    -1,    -1,    39,   119,
      58,   124,    -1,    -1,    23,    39,   120,    58,   124,    -1,
      -1,    14,    39,   121,    58,   124,    -1,    -1,    43,    39,
     122,    58,   124,    -1,    40,    58,   137,    -1,    52,    58,
     204,    -1,    54,    58,   141,    -1,    23,    54,    58,   141,
      -1,    14,    54,    58,   141,    -1,    43,    54,    58,   141,
      -1,    49,    58,   268,    -1,    49,    58,    -1,    36,    58,
     213,    -1,    37,    58,   213,    -1,    31,    -1,    86,   262,
      -1,    31,    -1,   126,    -1,    59,   274,    60,    -1,    59,
     274,   125,   272,    60,    -1,   126,    -1,   125,   273,   126,
      -1,    86,   262,   128,    -1,    -1,     7,   127,   128,    -1,
      -1,    56,   274,    57,    -1,    56,   274,   129,   270,    57,
      -1,   130,    -1,   129,   271,   130,    -1,    89,    -1,    20,
      58,   204,    -1,    31,    -1,   133,    -1,    59,   274,    60,
      -1,    59,   274,   132,   272,    60,    -1,   133,    -1,   132,
     273,   133,    -1,   263,    -1,    31,    -1,   136,    -1,    59,
     274,    60,    -1,    59,   274,   135,   272,    60,    -1,   136,
      -1,   135,   273,   136,    -1,   263,    -1,    62,   274,   138,
      63,    -1,    -1,   139,   272,    -1,   140,    -1,   139,   273,
     140,    -1,     7,    64,     7,    -1,   143,    -1,    59,   274,
     142,   272,    60,    -1,   143,    -1,   142,   273,   143,    -1,
      12,    -1,   274,    -1,   274,   145,    -1,   146,    -1,   145,
     146,    -1,   154,   271,    -1,   152,   271,    -1,   153,   271,
      -1,    91,   275,    -1,   147,   275,    -1,    -1,    53,    12,
     148,    58,   274,    62,   274,   149,    63,    -1,   150,    -1,
     150,   149,    -1,    -1,    12,   151,   101,    62,   144,    63,
     274,    -1,    43,    30,    58,   141,    -1,    43,    38,    58,
     141,    -1,   174,    -1,   236,    -1,    51,    -1,    17,    -1,
     155,    -1,   268,    -1,   268,    59,    60,    -1,   157,    -1,
     156,   157,    -1,    -1,    -1,   158,   267,   160,   202,   161,
     194,    -1,    -1,    -1,    19,   158,   267,   163,   202,   164,
     194,    -1,    -1,   158,   267,    61,    18,    58,   166,   184,
      -1,    -1,    14,   158,   267,    61,    18,    58,   167,   184,
      -1,    -1,    23,   158,   267,    61,    18,    58,   168,   184,
      -1,    -1,    43,   158,   267,    61,    18,    58,   169,   184,
      -1,    -1,   158,   267,    61,    29,    59,   264,    60,    58,
     171,   175,    -1,    -1,   158,   267,    61,    50,    58,   173,
     188,    -1,   162,    -1,   159,    -1,   165,    -1,   170,    -1,
     172,    -1,    -1,   266,   176,   181,   177,    -1,    -1,    62,
     274,    63,    -1,    62,   274,   178,   270,    63,    -1,   179,
      -1,   178,   271,   179,    -1,    -1,   157,   266,   180,    58,
     220,    -1,    -1,    56,   274,    57,    -1,    56,   274,   182,
     270,    57,    -1,   183,    -1,   182,   271,   183,    -1,    48,
      58,   204,    -1,    31,    -1,   186,    -1,    59,   274,    60,
      -1,    59,   274,   185,   272,    60,    -1,   186,    -1,   185,
     273,   186,    -1,   264,    -1,    -1,   264,   187,    65,   265,
      -1,    -1,    62,   189,   274,   190,    63,    -1,    -1,   191,
     272,    -1,   192,    -1,   191,   273,   192,    -1,    -1,   269,
      64,   193,   220,    -1,   269,    64,    31,    -1,    -1,    56,
     274,    57,    -1,    56,   274,   195,   270,    57,    -1,   197,
      -1,   195,   271,   197,    -1,   268,    -1,    20,    -1,    48,
      -1,    12,    -1,    -1,   196,   198,    58,   219,    -1,    -1,
      23,   268,   199,    58,   218,    -1,    -1,    14,   268,   200,
      58,   218,    -1,    -1,    43,   268,   201,    58,   218,    -1,
      26,    58,    12,    -1,    34,    58,   268,    -1,    25,    58,
     268,    -1,    49,    58,   268,    -1,    49,    58,    -1,    -1,
      58,   203,    -1,   220,    -1,    31,    -1,    -1,    62,   205,
     274,   206,    63,    -1,    -1,   207,   270,    -1,   208,    -1,
     207,   271,   208,    -1,   210,   209,    58,   220,    -1,    24,
     209,    58,   204,    -1,    12,    -1,   266,    -1,   211,    -1,
     212,    -1,   268,    -1,   268,    59,    60,    -1,    -1,    62,
     214,   274,   215,    63,    -1,    -1,   216,   272,    -1,   217,
      -1,   216,   273,   217,    -1,    12,    64,    12,    -1,    31,
      -1,   222,    -1,   204,    -1,   220,    -1,    31,    -1,   221,
      -1,   227,    -1,   222,    -1,    59,    60,    -1,     7,    -1,
      11,    -1,    12,    -1,   268,    -1,     6,    -1,    -1,    59,
     223,   224,    60,    -1,   274,   225,   272,    -1,   226,    -1,
     225,   273,   226,    -1,   221,    -1,   222,    -1,   227,    -1,
      -1,    56,   228,   229,    57,    -1,   274,   230,   272,    -1,
     231,    -1,   230,   273,   231,    -1,   221,    -1,   227,    -1,
      41,    -1,    19,    41,    -1,    19,    55,    41,    -1,    55,
      41,    -1,    -1,   232,   267,    61,    50,    58,   234,   188,
      -1,   232,   267,    61,    22,    58,     7,    -1,    -1,   232,
     267,   237,   250,   242,    -1,    -1,    23,   232,   267,   238,
     250,    -1,    -1,    14,   232,   267,   239,   250,    -1,    -1,
      43,   232,   267,   240,   250,    -1,    -1,   232,   267,    59,
       7,    60,   241,   256,    -1,   233,    -1,   235,    -1,    -1,
      56,   274,    57,    -1,    56,   274,   243,   270,    57,    -1,
     245,    -1,   243,   271,   245,    -1,   268,    -1,    20,    -1,
      48,    -1,    12,    -1,    -1,   244,   246,    58,   219,    -1,
      -1,    23,   268,   247,    58,   218,    -1,    -1,    14,   268,
     248,    58,   218,    -1,    -1,    43,   268,   249,    58,   218,
      -1,    26,    58,    12,    -1,    34,    58,   268,    -1,    49,
      58,   268,    -1,    49,    58,    -1,    -1,    58,   251,    -1,
     253,    -1,    31,    -1,    59,   274,    60,    -1,    59,   274,
     252,   272,    60,    -1,   253,    -1,   252,   273,   253,    -1,
     254,   255,    -1,     7,    -1,     7,    65,   265,    -1,    -1,
     256,    -1,    -1,    62,   257,   274,   258,    63,    -1,    -1,
     259,   270,    -1,   260,    -1,   259,   271,   260,    -1,   174,
      -1,   261,    -1,    43,    15,    58,   141,    -1,    -1,   263,
      -1,     7,    -1,     7,    -1,   263,    -1,   266,    -1,   268,
      -1,    70,    -1,     8,    -1,    10,    -1,    70,    -1,     8,
      -1,     9,    -1,    11,    -1,     8,    -1,    -1,   271,    -1,
      66,   274,    -1,   275,    -1,   274,    -1,   273,    -1,    67,
     274,    -1,    -1,   275,    -1,     3,    -1,   275,     3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1226,  1226,  1229,  1230,  1231,  1232,  1233,  1234,  1235,
    1236,  1237,  1238,  1239,  1240,  1241,  1242,  1243,  1244,  1245,
    1246,  1247,  1248,  1249,  1250,  1251,  1252,  1253,  1254,  1255,
    1256,  1257,  1258,  1259,  1260,  1261,  1262,  1263,  1264,  1265,
    1266,  1267,  1268,  1269,  1270,  1271,  1279,  1280,  1291,  1291,
    1303,  1304,  1316,  1317,  1321,  1322,  1326,  1330,  1335,  1335,
    1344,  1344,  1350,  1350,  1356,  1356,  1364,  1371,  1375,  1376,
    1390,  1391,  1395,  1403,  1410,  1412,  1416,  1417,  1421,  1425,
    1432,  1433,  1441,  1441,  1445,  1445,  1449,  1449,  1453,  1453,
    1457,  1457,  1461,  1461,  1465,  1475,  1476,  1483,  1483,  1543,
    1544,  1548,  1549,  1553,  1554,  1558,  1559,  1560,  1564,  1569,
    1569,  1578,  1578,  1584,  1584,  1590,  1590,  1598,  1605,  1612,
    1620,  1620,  1629,  1629,  1634,  1634,  1639,  1639,  1644,  1644,
    1650,  1650,  1655,  1655,  1660,  1660,  1665,  1665,  1671,  1671,
    1678,  1678,  1685,  1685,  1692,  1692,  1701,  1709,  1713,  1717,
    1721,  1725,  1731,  1736,  1743,  1751,  1760,  1761,  1765,  1766,
    1767,  1768,  1772,  1773,  1777,  1790,  1790,  1814,  1816,  1817,
    1821,  1822,  1826,  1827,  1831,  1832,  1833,  1834,  1838,  1839,
    1843,  1849,  1850,  1851,  1852,  1856,  1857,  1861,  1867,  1870,
    1872,  1876,  1877,  1881,  1887,  1888,  1892,  1893,  1897,  1905,
    1906,  1910,  1911,  1915,  1916,  1917,  1918,  1919,  1923,  1923,
    1957,  1958,  1962,  1962,  2005,  2014,  2027,  2028,  2036,  2039,
    2045,  2051,  2054,  2060,  2064,  2070,  2077,  2070,  2088,  2096,
    2088,  2107,  2107,  2115,  2115,  2123,  2123,  2131,  2131,  2142,
    2142,  2166,  2166,  2178,  2179,  2180,  2181,  2182,  2191,  2191,
    2208,  2210,  2211,  2220,  2221,  2225,  2225,  2240,  2242,  2243,
    2247,  2248,  2252,  2261,  2262,  2263,  2264,  2268,  2269,  2273,
    2276,  2276,  2302,  2302,  2307,  2309,  2313,  2314,  2318,  2318,
    2325,  2337,  2339,  2340,  2344,  2345,  2349,  2350,  2351,  2355,
    2360,  2360,  2369,  2369,  2375,  2375,  2381,  2381,  2389,  2396,
    2403,  2411,  2416,  2423,  2425,  2429,  2434,  2446,  2446,  2454,
    2456,  2460,  2461,  2465,  2468,  2476,  2477,  2481,  2482,  2486,
    2492,  2502,  2502,  2510,  2512,  2516,  2517,  2521,  2534,  2540,
    2550,  2554,  2555,  2568,  2571,  2574,  2577,  2588,  2594,  2597,
    2600,  2605,  2618,  2618,  2627,  2631,  2632,  2636,  2637,  2638,
    2646,  2646,  2653,  2657,  2658,  2662,  2663,  2671,  2675,  2679,
    2683,  2690,  2690,  2702,  2717,  2717,  2727,  2727,  2735,  2735,
    2744,  2744,  2752,  2752,  2766,  2767,  2770,  2772,  2773,  2777,
    2778,  2782,  2783,  2784,  2788,  2793,  2793,  2802,  2802,  2808,
    2808,  2814,  2814,  2822,  2829,  2837,  2842,  2849,  2851,  2855,
    2856,  2859,  2862,  2866,  2867,  2871,  2875,  2878,  2902,  2904,
    2908,  2908,  2934,  2936,  2940,  2941,  2946,  2948,  2952,  2965,
    2968,  2972,  2978,  2984,  2987,  2998,  2999,  3005,  3006,  3007,
    3012,  3013,  3018,  3019,  3022,  3024,  3028,  3029,  3033,  3034,
    3038,  3041,  3043,  3047,  3048
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
  "TOK_NUMBER", "TOK_STRING", "TOK_ABSTRACT", "TOK_ADD", "TOK_ATTRIBUTES",
  "TOK_CLASS", "TOK_CONFIG", "TOK_CONNECT", "TOK_CUSTOM", "TOK_CUSTOMDATA",
  "TOK_DEF", "TOK_DEFAULT", "TOK_DELETE", "TOK_DICTIONARY",
  "TOK_DISPLAYUNIT", "TOK_DOC", "TOK_INHERITS", "TOK_KIND", "TOK_MAPPER",
  "TOK_NAMECHILDREN", "TOK_NONE", "TOK_OFFSET", "TOK_OVER",
  "TOK_PERMISSION", "TOK_PAYLOAD", "TOK_PREFIX_SUBSTITUTIONS",
  "TOK_SUFFIX_SUBSTITUTIONS", "TOK_PROPERTIES", "TOK_REFERENCES",
  "TOK_RELOCATES", "TOK_REL", "TOK_RENAMES", "TOK_REORDER",
  "TOK_ROOTPRIMS", "TOK_SCALE", "TOK_SPECIALIZES", "TOK_SUBLAYERS",
  "TOK_SYMMETRYARGUMENTS", "TOK_SYMMETRYFUNCTION", "TOK_TIME_SAMPLES",
  "TOK_UNIFORM", "TOK_VARIANTS", "TOK_VARIANTSET", "TOK_VARIANTSETS",
  "TOK_VARYING", "'('", "')'", "'='", "'['", "']'", "'.'", "'{'", "'}'",
  "':'", "'@'", "';'", "','", "$accept", "menva_file", "keyword",
  "layer_metadata_form", "layer", "$@1", "layer_metadata_opt",
  "layer_metadata_list_opt", "layer_metadata_list", "layer_metadata_key",
  "layer_metadata", "$@2", "$@3", "$@4", "$@5", "sublayer_list",
  "sublayer_list_int", "sublayer_stmt", "layer_ref", "layer_offset_opt",
  "layer_offset_int", "layer_offset_stmt", "prim_list", "prim_stmt", "$@6",
  "$@7", "$@8", "$@9", "$@10", "$@11", "prim_type_name", "prim_stmt_int",
  "$@12", "prim_metadata_opt", "prim_metadata_list_opt",
  "prim_metadata_list", "prim_metadata_key", "prim_metadata", "$@13",
  "$@14", "$@15", "$@16", "$@17", "$@18", "$@19", "$@20", "$@21", "$@22",
  "$@23", "$@24", "$@25", "$@26", "$@27", "$@28", "$@29", "payload_item",
  "reference_list", "reference_list_int", "reference_list_item", "$@30",
  "reference_params_opt", "reference_params_int", "reference_params_item",
  "inherit_list", "inherit_list_int", "inherit_list_item",
  "specializes_list", "specializes_list_int", "specializes_list_item",
  "relocates_map", "relocates_stmt_list_opt", "relocates_stmt_list",
  "relocates_stmt", "name_list", "name_list_int", "name_list_item",
  "prim_contents_list_opt", "prim_contents_list",
  "prim_contents_list_item", "variantset_stmt", "$@31", "variant_list",
  "variant_stmt", "$@32", "prim_child_order_stmt",
  "prim_property_order_stmt", "prim_property", "prim_attr_variability",
  "prim_attr_qualifiers", "prim_attr_type", "prim_attribute_full_type",
  "prim_attribute_default", "$@33", "$@34", "prim_attribute_fallback",
  "$@35", "$@36", "prim_attribute_connect", "$@37", "$@38", "$@39", "$@40",
  "prim_attribute_mapper", "$@41", "prim_attribute_time_samples", "$@42",
  "prim_attribute", "attribute_mapper_rhs", "$@43",
  "attribute_mapper_params_opt", "attribute_mapper_params_list",
  "attribute_mapper_param", "$@44", "attribute_mapper_metadata_opt",
  "attribute_mapper_metadata_list", "attribute_mapper_metadata",
  "connect_rhs", "connect_list", "connect_item", "$@45",
  "time_samples_rhs", "$@46", "time_sample_list", "time_sample_list_int",
  "time_sample", "$@47", "attribute_metadata_list_opt",
  "attribute_metadata_list", "attribute_metadata_key",
  "attribute_metadata", "$@48", "$@49", "$@50", "$@51",
  "attribute_assignment_opt", "attribute_value", "typed_dictionary",
  "$@52", "typed_dictionary_list_opt", "typed_dictionary_list",
  "typed_dictionary_element", "dictionary_key", "dictionary_value_type",
  "dictionary_value_scalar_type", "dictionary_value_shaped_type",
  "string_dictionary", "$@53", "string_dictionary_list_opt",
  "string_dictionary_list", "string_dictionary_element",
  "metadata_listop_list", "metadata_value", "typed_value",
  "typed_value_atomic", "typed_value_list", "$@54", "typed_value_list_int",
  "typed_value_list_items", "typed_value_list_item", "typed_value_tuple",
  "$@55", "typed_value_tuple_int", "typed_value_tuple_items",
  "typed_value_tuple_item", "prim_relationship_type",
  "prim_relationship_time_samples", "$@56", "prim_relationship_default",
  "prim_relationship", "$@57", "$@58", "$@59", "$@60", "$@61",
  "relationship_metadata_list_opt", "relationship_metadata_list",
  "relationship_metadata_key", "relationship_metadata", "$@62", "$@63",
  "$@64", "$@65", "relationship_assignment_opt", "relationship_rhs",
  "relationship_target_list", "relationship_target",
  "relationship_target_and_opt_marker", "relational_attributes_opt",
  "relational_attributes", "$@66", "relational_attributes_list_opt",
  "relational_attributes_list", "relational_attributes_list_item",
  "relational_attributes_order_stmt", "prim_path_opt", "prim_path",
  "property_path", "marker", "name", "namespaced_name", "identifier",
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
     305,   306,   307,   308,   309,   310,    40,    41,    61,    91,
      93,    46,   123,   125,    58,    64,    59,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,    68,    69,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    71,    71,    73,    72,
      74,    74,    75,    75,    76,    76,    77,    78,    79,    78,
      80,    78,    81,    78,    82,    78,    78,    78,    83,    83,
      84,    84,    85,    86,    87,    87,    88,    88,    89,    89,
      90,    90,    92,    91,    93,    91,    94,    91,    95,    91,
      96,    91,    97,    91,    91,    98,    98,   100,    99,   101,
     101,   102,   102,   103,   103,   104,   104,   104,   105,   106,
     105,   107,   105,   108,   105,   109,   105,   105,   105,   105,
     110,   105,   111,   105,   112,   105,   113,   105,   114,   105,
     115,   105,   116,   105,   117,   105,   118,   105,   119,   105,
     120,   105,   121,   105,   122,   105,   105,   105,   105,   105,
     105,   105,   105,   105,   105,   105,   123,   123,   124,   124,
     124,   124,   125,   125,   126,   127,   126,   128,   128,   128,
     129,   129,   130,   130,   131,   131,   131,   131,   132,   132,
     133,   134,   134,   134,   134,   135,   135,   136,   137,   138,
     138,   139,   139,   140,   141,   141,   142,   142,   143,   144,
     144,   145,   145,   146,   146,   146,   146,   146,   148,   147,
     149,   149,   151,   150,   152,   153,   154,   154,   155,   155,
     156,   157,   157,   158,   158,   160,   161,   159,   163,   164,
     162,   166,   165,   167,   165,   168,   165,   169,   165,   171,
     170,   173,   172,   174,   174,   174,   174,   174,   176,   175,
     177,   177,   177,   178,   178,   180,   179,   181,   181,   181,
     182,   182,   183,   184,   184,   184,   184,   185,   185,   186,
     187,   186,   189,   188,   190,   190,   191,   191,   193,   192,
     192,   194,   194,   194,   195,   195,   196,   196,   196,   197,
     198,   197,   199,   197,   200,   197,   201,   197,   197,   197,
     197,   197,   197,   202,   202,   203,   203,   205,   204,   206,
     206,   207,   207,   208,   208,   209,   209,   210,   210,   211,
     212,   214,   213,   215,   215,   216,   216,   217,   218,   218,
     219,   219,   219,   220,   220,   220,   220,   220,   221,   221,
     221,   221,   223,   222,   224,   225,   225,   226,   226,   226,
     228,   227,   229,   230,   230,   231,   231,   232,   232,   232,
     232,   234,   233,   235,   237,   236,   238,   236,   239,   236,
     240,   236,   241,   236,   236,   236,   242,   242,   242,   243,
     243,   244,   244,   244,   245,   246,   245,   247,   245,   248,
     245,   249,   245,   245,   245,   245,   245,   250,   250,   251,
     251,   251,   251,   252,   252,   253,   254,   254,   255,   255,
     257,   256,   258,   258,   259,   259,   260,   260,   261,   262,
     262,   263,   264,   265,   265,   266,   266,   267,   267,   267,
     268,   268,   269,   269,   270,   270,   271,   271,   272,   272,
     273,   274,   274,   275,   275
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     0,     3,
       1,     5,     1,     3,     1,     3,     1,     1,     0,     4,
       0,     5,     0,     5,     0,     5,     3,     3,     3,     5,
       1,     3,     2,     1,     0,     4,     1,     3,     3,     3,
       1,     3,     0,     3,     0,     4,     0,     3,     0,     4,
       0,     3,     0,     4,     4,     1,     3,     0,     6,     1,
       5,     1,     3,     1,     3,     1,     1,     1,     1,     0,
       4,     0,     5,     0,     5,     0,     5,     3,     3,     3,
       0,     4,     0,     4,     0,     5,     0,     5,     0,     5,
       0,     4,     0,     5,     0,     5,     0,     5,     0,     4,
       0,     5,     0,     5,     0,     5,     3,     3,     3,     4,
       4,     4,     3,     2,     3,     3,     1,     2,     1,     1,
       3,     5,     1,     3,     3,     0,     3,     0,     3,     5,
       1,     3,     1,     3,     1,     1,     3,     5,     1,     3,
       1,     1,     1,     3,     5,     1,     3,     1,     4,     0,
       2,     1,     3,     3,     1,     5,     1,     3,     1,     1,
       2,     1,     2,     2,     2,     2,     2,     2,     0,     9,
       1,     2,     0,     7,     4,     4,     1,     1,     1,     1,
       1,     1,     3,     1,     2,     0,     0,     6,     0,     0,
       7,     0,     7,     0,     8,     0,     8,     0,     8,     0,
      10,     0,     7,     1,     1,     1,     1,     1,     0,     4,
       0,     3,     5,     1,     3,     0,     5,     0,     3,     5,
       1,     3,     3,     1,     1,     3,     5,     1,     3,     1,
       0,     4,     0,     5,     0,     2,     1,     3,     0,     4,
       3,     0,     3,     5,     1,     3,     1,     1,     1,     1,
       0,     4,     0,     5,     0,     5,     0,     5,     3,     3,
       3,     3,     2,     0,     2,     1,     1,     0,     5,     0,
       2,     1,     3,     4,     4,     1,     1,     1,     1,     1,
       3,     0,     5,     0,     2,     1,     3,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     1,     1,     1,
       1,     1,     0,     4,     3,     1,     3,     1,     1,     1,
       0,     4,     3,     1,     3,     1,     1,     1,     2,     3,
       2,     0,     7,     6,     0,     5,     0,     5,     0,     5,
       0,     5,     0,     7,     1,     1,     0,     3,     5,     1,
       3,     1,     1,     1,     1,     0,     4,     0,     5,     0,
       5,     0,     5,     3,     3,     3,     2,     0,     2,     1,
       1,     3,     5,     1,     3,     2,     1,     3,     0,     1,
       0,     5,     0,     2,     1,     3,     1,     1,     4,     0,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     0,     1,     2,     1,     1,     1,
       2,     0,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    48,     0,     2,   441,     1,   443,    49,    46,    50,
     442,    86,    82,    90,     0,   441,    80,   441,   444,   430,
     431,     0,    88,    95,     0,    84,     0,    92,     0,    47,
     442,     0,    52,    97,    87,     0,     0,    83,     0,    91,
       0,     0,    81,   441,    57,     0,     0,     0,     0,     0,
     434,    58,    54,    56,   441,    96,    89,    85,    93,   198,
     441,    94,   194,    51,    62,    60,     0,    64,     0,   441,
      53,   435,   437,     0,     0,    99,     0,     0,     0,    66,
       0,   441,    67,   436,    55,     0,   441,   441,   441,   196,
       0,     0,     0,     0,   341,   337,   338,   339,   332,   350,
     342,   307,   330,    59,   331,   333,   335,   334,   340,     0,
     199,     0,   101,   441,     0,   439,   438,   328,   342,    63,
     329,    61,    65,    73,    68,   441,    70,    74,   441,   336,
     441,   441,    98,     0,   219,     0,     0,   357,     0,   218,
       0,     0,     0,   200,   201,     0,     0,     0,     0,   220,
       0,   223,     0,   244,   243,   245,   246,   247,   216,     0,
     374,   375,   217,   221,   441,   108,     0,   106,     0,     0,
     122,     0,     0,   120,     0,     0,   138,     0,     0,   130,
     107,     0,     0,     0,   434,   109,   103,   105,   440,   195,
     197,     0,   439,     0,    72,     0,     0,     0,     0,   309,
       0,     0,     0,   358,     0,     0,     0,     0,     0,     0,
       0,     0,   208,   360,   206,   202,   207,   204,   205,   203,
     224,   427,   428,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    25,    24,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,   429,   225,   364,     0,
     100,   126,   142,   134,     0,   113,   124,   140,   132,     0,
     111,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     128,   144,   136,     0,   115,     0,   153,     0,     0,   102,
     435,     0,    69,    71,     0,     0,   434,    76,   351,   355,
     356,   441,   353,   343,   347,   348,   441,   345,   349,     0,
       0,   434,   311,     0,   317,   318,   319,     0,   368,   359,
     228,     0,   366,     0,     0,     0,   370,     0,     0,   303,
       0,     0,   397,   222,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   117,     0,   118,   119,     0,   321,
     154,   155,     0,   441,   146,     0,     0,     0,     0,     0,
       0,   152,   147,   148,   104,     0,     0,     0,     0,   435,
     352,   439,   344,   439,   315,   426,     0,   316,   425,   308,
     310,   435,     0,     0,     0,   397,   303,     0,   397,   214,
     215,     0,   397,   441,     0,     0,     0,     0,   226,     0,
       0,     0,     0,   376,     0,     0,     0,   150,     0,     0,
       0,     0,   149,     0,   421,   174,   441,   123,   175,   180,
     156,   419,   121,   441,   165,   158,   441,   419,   139,   159,
     189,     0,     0,     0,   151,     0,   181,   441,   131,   182,
     187,   110,    78,    79,    75,    77,   354,   346,     0,   312,
       0,   320,     0,   369,   229,     0,   367,     0,   371,     0,
     231,     0,   241,   306,   304,   305,   281,   372,     0,   361,
     406,   400,   441,   398,   399,   408,   441,   365,   127,   143,
     135,   114,   125,   141,   133,   112,     0,   157,   420,   323,
     167,     0,   167,     0,     0,   441,   191,   129,   145,   137,
     116,     0,   314,   313,   233,   281,   235,   237,   441,     0,
     422,     0,     0,   441,   227,     0,   363,     0,     0,     0,
     410,   405,   409,     0,   176,   441,   178,     0,     0,   441,
     325,   441,   166,   160,   441,   162,   164,     0,   188,   190,
     439,   183,   441,   185,     0,   230,     0,     0,     0,   263,
     441,   232,   264,   269,     0,   272,   242,     0,   373,   362,
     423,   407,   424,   401,   441,   403,   441,   384,     0,   382,
       0,     0,     0,     0,   383,     0,   377,   434,   385,   379,
     381,     0,   439,     0,   322,   324,   439,     0,     0,   439,
     193,   192,     0,   439,   234,   236,   238,   212,     0,   210,
       0,     0,   239,   441,   289,     0,   287,     0,     0,     0,
       0,     0,   288,     0,   282,   434,   290,   284,   286,     0,
     439,   412,   389,   387,     0,     0,   391,   396,     0,   435,
       0,   177,   179,   327,   326,     0,   168,   172,   434,   170,
     161,   163,   184,   186,   441,   209,   211,   265,   441,   267,
       0,     0,   274,   294,   292,     0,     0,     0,   296,   302,
       0,   435,     0,   402,   404,     0,     0,     0,     0,   416,
       0,   434,   414,   417,     0,     0,   393,   394,     0,   395,
     378,   380,     0,     0,     0,   435,     0,     0,   439,   271,
     240,   248,   433,   432,     0,   441,   276,     0,     0,     0,
     300,   298,   299,     0,   301,   283,   285,     0,     0,   411,
     413,   435,     0,     0,     0,   386,   173,   169,   171,   441,
     266,   268,   257,   273,   275,   439,   278,     0,     0,     0,
     291,     0,   415,   390,   388,   392,     0,   441,   250,   277,
     280,     0,   295,   293,   297,   418,   441,     0,   441,   249,
     279,   213,     0,   258,   434,   260,     0,     0,     0,   435,
     251,     0,   434,   253,   262,   259,   261,   255,     0,   435,
       0,   252,   254,     0,   256
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,   266,     7,     3,     4,     8,    31,    50,    51,
      52,    73,    78,    77,    80,    82,   125,   126,   437,   194,
     306,   647,    15,   142,    24,    38,    21,    36,    26,    40,
      22,    34,    54,    74,   111,   184,   185,   186,   301,   353,
     348,   369,   285,   282,   349,   344,   365,   295,   351,   346,
     367,   288,   350,   345,   366,   432,   438,   544,   439,   500,
     542,   648,   649,   427,   535,   428,   448,   552,   449,   364,
     504,   505,   506,    61,    88,    62,   109,   143,   144,   145,
     337,   608,   609,   654,   146,   147,   148,   149,   150,   151,
     152,   153,   339,   476,   154,   396,   515,   155,   519,   554,
     556,   557,   156,   661,   157,   522,   158,   700,   732,   759,
     772,   773,   780,   748,   764,   765,   561,   658,   562,   611,
     566,   613,   704,   705,   706,   751,   524,   625,   626,   627,
     672,   709,   708,   713,   408,   474,   102,   131,   320,   321,
     322,   386,   323,   324,   325,   360,   433,   538,   539,   540,
     119,   103,   104,   105,   120,   130,   197,   316,   317,   107,
     128,   195,   311,   312,   159,   160,   527,   161,   162,   342,
     398,   395,   402,   525,   487,   587,   588,   589,   640,   685,
     684,   688,   413,   483,   574,   484,   485,   531,   532,   576,
     680,   681,   682,   683,   497,   429,   563,   571,   387,   267,
     163,   707,    70,    71,   114,   115,   116,    10
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -589
static const yytype_int16 yypact[] =
{
      52,  -589,    71,  -589,    63,  -589,  -589,  -589,   294,    33,
      74,   152,   152,   152,   135,    63,  -589,    63,  -589,  -589,
    -589,    83,   114,  -589,    83,   114,    83,   114,   142,  -589,
      51,   155,   608,  -589,  -589,   152,    83,  -589,    83,  -589,
      83,   138,  -589,    63,  -589,   152,   152,   158,   152,   162,
      42,  -589,  -589,  -589,    63,  -589,  -589,  -589,  -589,  -589,
      63,  -589,  -589,  -589,  -589,  -589,   218,  -589,   186,    63,
    -589,   608,    74,   198,   197,   206,   255,   216,   222,  -589,
     226,    63,  -589,  -589,  -589,   196,    63,    63,    36,  -589,
      21,    21,    21,    38,  -589,  -589,  -589,  -589,  -589,  -589,
     233,  -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,   234,
     422,   246,   677,    63,   245,   255,  -589,  -589,  -589,  -589,
    -589,  -589,  -589,  -589,  -589,    36,  -589,   269,    63,  -589,
      63,    63,  -589,   327,  -589,   425,   327,  -589,   311,  -589,
     295,   292,    63,   422,  -589,    63,    42,    42,    42,  -589,
     152,  -589,   895,  -589,  -589,  -589,  -589,  -589,  -589,   895,
    -589,  -589,  -589,   275,    63,  -589,   260,  -589,   445,   280,
    -589,   281,   287,  -589,   296,   298,  -589,   299,   474,  -589,
    -589,   300,   303,   309,    42,  -589,  -589,  -589,  -589,  -589,
    -589,   293,   369,   124,  -589,   329,   242,   316,   220,   156,
      24,   895,   895,  -589,   350,   895,   895,   895,   334,   340,
     895,   895,  -589,  -589,    74,  -589,    74,  -589,  -589,  -589,
    -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,
    -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,
    -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,
    -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,
    -589,  -589,  -589,  -589,  -589,  -589,  -589,   341,   185,   343,
    -589,  -589,  -589,  -589,   346,  -589,  -589,  -589,  -589,   348,
    -589,   389,   353,   397,   152,   354,   352,   352,   355,   359,
    -589,  -589,  -589,   361,  -589,   365,   152,   363,   138,  -589,
     677,   368,  -589,  -589,   370,   374,    42,  -589,  -589,  -589,
    -589,    36,  -589,  -589,  -589,  -589,    36,  -589,  -589,   847,
     377,    42,  -589,   847,  -589,  -589,   356,   383,  -589,  -589,
    -589,   385,  -589,   138,   138,   386,  -589,   392,    40,   393,
     410,    37,   394,  -589,   402,   404,   406,   138,   409,   411,
     416,   420,   138,   427,  -589,    57,  -589,  -589,    55,  -589,
    -589,  -589,   147,    63,  -589,   429,   430,   432,   138,   434,
     151,  -589,  -589,  -589,  -589,   196,   457,   482,   437,   124,
    -589,   242,  -589,   220,  -589,  -589,   444,  -589,  -589,  -589,
    -589,   156,   447,   435,   491,   394,   393,   492,   394,  -589,
    -589,   493,   394,    63,   449,   455,   458,   112,  -589,   461,
     460,   465,   188,   459,    57,   147,   151,  -589,    21,    57,
     147,   151,  -589,    21,  -589,  -589,    63,  -589,  -589,  -589,
    -589,   517,  -589,    63,  -589,  -589,    63,   517,  -589,  -589,
     518,    57,   147,   151,  -589,    21,  -589,    63,  -589,  -589,
    -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,   363,  -589,
     103,  -589,   468,  -589,  -589,   469,  -589,   471,  -589,   472,
    -589,   523,  -589,  -589,  -589,  -589,   475,  -589,   526,  -589,
     473,  -589,    63,  -589,  -589,   480,    63,  -589,  -589,  -589,
    -589,  -589,  -589,  -589,  -589,  -589,    46,  -589,  -589,   524,
     481,    44,   481,   483,   485,    36,  -589,  -589,  -589,  -589,
    -589,    66,  -589,  -589,  -589,   475,  -589,  -589,    63,   204,
    -589,   484,   487,    63,  -589,   480,  -589,   487,   799,    68,
    -589,  -589,  -589,   351,  -589,    36,  -589,   486,   488,    36,
    -589,    63,  -589,  -589,    36,  -589,  -589,   538,  -589,  -589,
     518,  -589,    36,  -589,   204,  -589,   204,   204,   534,  -589,
      63,  -589,  -589,   490,   495,  -589,  -589,   252,  -589,  -589,
    -589,  -589,  -589,  -589,    36,  -589,    63,  -589,   152,  -589,
     152,   496,   499,   152,  -589,   500,  -589,    42,  -589,  -589,
    -589,   501,   517,   547,  -589,  -589,   524,   272,   506,   264,
    -589,  -589,   508,   517,  -589,  -589,  -589,  -589,   497,   534,
      85,   505,  -589,    63,  -589,   152,  -589,   152,   515,   516,
     520,   152,  -589,   521,  -589,    42,  -589,  -589,  -589,   522,
     568,   489,  -589,  -589,   569,   152,  -589,   152,   527,   743,
     525,  -589,  -589,  -589,  -589,   528,  -589,  -589,    42,  -589,
    -589,  -589,  -589,  -589,    63,  -589,  -589,  -589,    36,  -589,
     799,   943,   141,  -589,  -589,   152,   573,   152,  -589,   152,
     532,   710,   533,  -589,  -589,   232,   232,   232,   273,  -589,
     529,    42,  -589,  -589,   535,   539,  -589,  -589,   540,  -589,
    -589,  -589,   196,   363,   542,   131,   541,   536,   523,  -589,
    -589,  -589,  -589,  -589,   537,    36,  -589,   543,   544,   550,
    -589,  -589,  -589,   551,  -589,  -589,  -589,   196,   554,  -589,
    -589,   489,    21,    21,    21,  -589,  -589,  -589,  -589,    63,
    -589,  -589,   545,  -589,  -589,   141,   549,    21,    21,    21,
    -589,   138,  -589,  -589,  -589,  -589,   555,    63,   552,  -589,
    -589,   103,  -589,  -589,  -589,  -589,    63,    43,    63,  -589,
    -589,  -589,   563,  -589,    42,  -589,    39,   363,   548,   575,
    -589,   943,    42,  -589,  -589,  -589,  -589,  -589,   562,   152,
     572,  -589,  -589,   103,  -589
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -589,  -589,  -310,  -589,  -589,  -589,  -589,  -589,  -589,  -589,
     556,  -589,  -589,  -589,  -589,  -589,  -589,   441,   -67,  -589,
    -589,  -181,  -589,   140,  -589,  -589,  -589,  -589,  -589,  -589,
     319,   357,  -589,   -17,  -589,  -589,  -589,   339,  -589,  -589,
    -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,
    -589,  -589,  -589,  -589,  -589,  -589,  -206,  -589,  -468,  -589,
     139,  -589,   -55,  -101,  -589,  -465,  -100,  -589,  -462,  -589,
    -589,  -589,    92,  -271,  -589,   -30,   -85,  -589,   502,  -589,
    -589,    48,  -589,  -589,  -589,  -589,  -589,  -589,  -589,  -147,
    -113,  -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,  -589,
    -589,  -589,  -589,  -589,  -589,  -589,  -588,  -589,  -589,  -589,
    -589,  -127,  -589,  -589,  -589,  -116,  -332,  -589,  -568,  -589,
     132,  -589,  -589,  -589,   -71,  -589,   150,  -589,  -589,    -4,
    -589,  -589,  -589,  -589,   276,  -589,  -286,  -589,  -589,  -589,
     277,   360,  -589,  -589,  -589,   382,  -589,  -589,  -589,    77,
     -76,  -369,  -378,  -168,   -81,  -589,  -589,  -589,   291,  -160,
    -589,  -589,  -589,   297,   251,  -589,  -589,  -589,  -589,  -589,
    -589,  -589,  -589,  -589,  -589,  -589,  -589,    41,  -589,  -589,
    -589,  -589,  -209,  -589,  -589,  -488,  -589,  -589,   154,  -589,
    -589,  -589,   -46,  -589,   239,  -338,   211,    17,  -514,    32,
     -11,  -589,  -177,  -129,  -117,  -115,    53,   -10
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -271
static const yytype_int16 yytable[] =
{
      23,    23,    23,   220,   106,    30,   451,   299,   191,   385,
     192,   372,   307,   385,   572,   121,   122,   217,   218,   219,
     201,    53,   205,   206,    55,   210,   127,   373,   309,   475,
     314,   536,   450,   545,    64,    65,   310,    67,   318,     6,
      72,   575,   659,   679,   123,     6,    89,    19,    20,   553,
     123,   434,   117,   424,    18,   300,     1,     9,   404,   410,
      53,   123,   399,   400,   424,   203,     6,    11,    29,   405,
      32,     5,    12,   424,   108,   480,   417,    18,   450,   204,
     118,   422,   513,   450,    13,   190,   430,   411,   425,    17,
     406,   762,   520,   498,    14,    33,    63,   444,   124,   498,
     763,   187,   770,   113,   543,   450,   534,    75,    69,    94,
      95,    19,    20,    76,    96,    97,   426,   315,    94,    95,
      19,    20,    83,    96,    97,   127,   551,   642,   573,   378,
     731,   651,   214,   679,    93,   216,    72,    72,    72,   110,
     112,   653,   674,   473,   390,   657,   572,   701,    16,   702,
      59,   645,   703,   123,   434,   275,   304,   280,   424,    99,
      19,    20,   100,   304,    19,    20,   188,   294,    99,   305,
      42,   100,   512,   450,    72,    35,   305,   379,   435,    28,
     319,   196,   446,   198,   199,   108,   463,   108,   326,   466,
     570,   268,   391,   468,   380,   480,   381,    60,   455,   382,
      41,   383,    94,    95,    19,    20,   436,    96,    97,   489,
     447,   520,    43,   309,   493,   314,    66,   270,   385,   481,
      68,   310,   604,   318,   605,   606,    94,    98,    19,    20,
      79,    96,    97,   327,   328,   559,   508,   330,   331,   332,
      19,    20,   335,   336,   340,    81,   341,   482,    94,   134,
      19,    20,    99,    96,    97,   100,    85,   777,   101,    86,
      19,    20,    87,   560,   614,   450,   615,    59,    19,    20,
     123,   434,   616,   357,    90,   617,    99,   618,   619,   118,
      91,    19,    20,   139,    92,   371,   620,   271,   718,   187,
     134,   431,   645,   129,   106,   621,    72,   132,    99,   272,
     622,   623,   315,   164,   304,   189,   273,   212,   388,   624,
      11,    72,   388,   488,   274,    12,   490,   305,   492,    19,
      20,   494,   570,   725,   139,   193,   106,    13,   134,   646,
     200,    25,    27,   213,   269,    19,    20,    14,   281,   283,
     507,   208,   491,   509,   134,   284,   200,   495,   740,   209,
     385,   385,   137,   302,   286,    28,   287,   289,   296,    19,
      20,   297,   139,   577,   108,   578,   141,   298,   137,   510,
     108,   579,   108,   760,   580,   123,   313,   581,   139,   106,
     326,    37,   141,    39,   202,   582,   308,   207,   549,   211,
     550,   329,   333,    56,   583,    57,   108,    58,   334,   584,
     585,   354,   338,   343,   347,   784,   352,   726,   586,   356,
     638,   355,   358,   362,   359,   393,   440,   409,   591,   368,
     592,   363,   595,   370,   596,   101,   375,   598,   376,   599,
      19,    20,   377,    19,    20,   602,   133,   603,    11,   134,
     389,   135,   134,    12,   394,   136,   397,   401,   670,   108,
     403,   407,   412,    19,    20,    13,   469,   629,   639,   630,
     414,   385,   415,   137,   416,   138,   203,   418,   452,   419,
     755,   694,   276,   139,   420,   140,   139,   141,   421,   496,
     204,   774,    19,    20,   277,   423,   499,   441,   442,   501,
     443,   278,   445,   453,   454,   461,   671,    19,    20,   279,
     511,   290,   458,   675,   720,   460,   134,   470,   676,   462,
     465,   467,   677,   291,   471,   486,   472,   388,   478,   695,
     292,   477,   590,   479,   424,   503,   514,   516,   293,   517,
     520,   523,   678,   526,   518,   529,   537,   541,   528,   533,
     139,   697,   530,   698,   564,   600,   607,   547,   548,   565,
     593,   594,   721,   612,   634,  -270,   628,   635,   637,   643,
     655,   641,   201,   205,   206,   210,   650,   632,   652,   633,
     660,   558,   636,   665,   666,   480,   567,    72,   667,   669,
     750,   686,   673,   692,   690,   711,   693,   768,   734,   715,
     735,   717,   719,   722,   597,   778,   730,   723,   724,   727,
     733,   747,   737,   729,   663,   775,   664,   736,   738,   739,
     668,   106,   741,   610,   758,    72,    19,    20,   756,   771,
      44,   767,    45,   762,   687,   781,   689,    84,   590,   631,
     783,    46,   771,   303,    47,   769,   106,   696,    72,   374,
     728,   546,   601,   779,   746,   215,   743,   744,   745,   388,
     388,    48,   782,   776,   710,    49,   712,   656,   714,   569,
     628,   752,   753,   754,   749,   555,   662,   716,   459,   361,
     106,    72,   464,   644,   457,   742,   502,   699,   456,   568,
     691,   108,   521,   392,     0,    19,    20,     0,     0,   165,
       0,   166,     0,     0,     0,     0,     0,   167,     0,     0,
     168,     0,   106,   169,   170,   171,   108,    75,     0,     0,
       0,   172,   173,   174,   175,     0,   176,   177,    19,    20,
     178,     0,   614,   179,   615,   180,   181,     0,     0,   182,
     616,   183,     0,   617,     0,   618,   619,     0,     0,     0,
     108,     0,     0,     0,   620,     0,     0,     0,     0,     0,
       0,    19,    20,   621,    72,   577,     0,   578,   622,   623,
     388,     0,    72,   579,     0,     0,   580,     0,     0,   581,
       0,     0,   108,     0,     0,     0,     0,   582,     0,     0,
       0,     0,   110,     0,     0,     0,   583,     0,     0,     0,
       0,   584,   585,     0,     0,     0,     0,     0,     0,     0,
     757,     0,     0,     0,     0,     0,   424,    19,    20,   761,
       0,   766,   223,   224,   225,   226,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,    19,    20,     0,     0,   384,
     223,   224,   225,   226,   227,   228,   229,   230,   231,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,   251,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   221,     0,   222,     0,     0,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,    19,    20,     0,     0,     0,   223,   224,   225,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,   251,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265
};

static const yytype_int16 yycheck[] =
{
      11,    12,    13,   150,    85,    15,   375,   184,   125,   319,
     125,   297,   193,   323,   528,    91,    92,   146,   147,   148,
     133,    32,   135,   136,    35,   138,    93,   298,   196,   407,
     198,   496,   370,   501,    45,    46,   196,    48,   198,     3,
      50,   529,   610,   631,     6,     3,    76,     8,     9,   511,
       6,     7,    31,     7,     3,   184,     4,     4,    18,    22,
      71,     6,   333,   334,     7,    41,     3,    16,    15,    29,
      17,     0,    21,     7,    85,     7,   347,     3,   416,    55,
      59,   352,   460,   421,    33,   115,    31,    50,    31,    56,
      50,    48,     7,   431,    43,    12,    43,   368,    60,   437,
      57,   112,    63,    67,    60,   443,    60,    54,    66,     6,
       7,     8,     9,    60,    11,    12,    59,   198,     6,     7,
       8,     9,    69,    11,    12,   192,    60,   592,    60,   306,
     698,   599,   142,   721,    81,   145,   146,   147,   148,    86,
      87,   603,   630,    31,   321,    60,   660,   661,     8,     8,
      12,    20,    11,     6,     7,   166,    32,   168,     7,    56,
       8,     9,    59,    32,     8,     9,   113,   178,    56,    45,
      30,    59,   458,   511,   184,    61,    45,   306,    31,    44,
      24,   128,    31,   130,   131,   196,   395,   198,   199,   398,
     528,   159,   321,   402,   311,     7,   311,    59,   379,   316,
      58,   316,     6,     7,     8,     9,    59,    11,    12,   415,
      59,     7,    57,   381,   420,   383,    58,   164,   528,    31,
      58,   381,   554,   383,   556,   557,     6,    31,     8,     9,
      12,    11,    12,   201,   202,    31,   442,   205,   206,   207,
       8,     9,   210,   211,    59,    59,    61,    59,     6,    17,
       8,     9,    56,    11,    12,    59,    58,   771,    62,    62,
       8,     9,    56,    59,    12,   603,    14,    12,     8,     9,
       6,     7,    20,   284,    58,    23,    56,    25,    26,    59,
      58,     8,     9,    51,    58,   296,    34,    27,    15,   300,
      17,   358,    20,    60,   375,    43,   306,    63,    56,    39,
      48,    49,   383,    57,    32,    60,    46,    12,   319,    57,
      16,   321,   323,   414,    54,    21,   416,    45,   419,     8,
       9,   421,   660,   692,    51,    56,   407,    33,    17,    57,
      19,    12,    13,    41,    59,     8,     9,    43,    58,    58,
     441,    30,   418,   443,    17,    58,    19,   423,   717,    38,
     660,   661,    41,    60,    58,    44,    58,    58,    58,     8,
       9,    58,    51,    12,   375,    14,    55,    58,    41,   445,
     381,    20,   383,   751,    23,     6,    60,    26,    51,   460,
     391,    24,    55,    26,   133,    34,    57,   136,   505,   138,
     505,    41,    58,    36,    43,    38,   407,    40,    58,    48,
      49,    12,    61,    60,    58,   783,    58,   693,    57,    12,
     587,    58,    58,    58,    62,    59,   363,     7,   535,    58,
     535,    62,   539,    58,   539,    62,    58,   544,    58,   544,
       8,     9,    58,     8,     9,   552,    14,   552,    16,    17,
      63,    19,    17,    21,    61,    23,    61,    61,   625,   460,
      58,    58,    58,     8,     9,    33,   403,   574,   587,   574,
      58,   771,    58,    41,    58,    43,    41,    58,    11,    58,
     741,   648,    27,    51,    58,    53,    51,    55,    58,   426,
      55,   767,     8,     9,    39,    58,   433,    58,    58,   436,
      58,    46,    58,    11,    57,    60,   625,     8,     9,    54,
     447,    27,    58,    14,   681,    58,    17,    58,    19,    18,
      18,    18,    23,    39,    59,    56,    58,   528,    58,   648,
      46,    60,   533,    58,     7,     7,    58,    58,    54,    58,
       7,    56,    43,     7,    62,   482,    12,    56,    65,   486,
      51,   658,    62,   658,    60,     7,    12,    64,    63,    62,
      64,    63,   681,    58,    58,    65,   567,    58,    58,    12,
      63,    60,   675,   676,   677,   678,    60,   578,    60,   580,
      65,   518,   583,    58,    58,     7,   523,   587,    58,    58,
      31,    12,    60,    58,    57,    12,    58,   764,   705,    57,
     705,    58,    63,    58,   541,   772,    60,    58,    58,    57,
      63,    56,    58,    62,   615,    57,   617,    64,    58,    58,
     621,   692,    58,   560,    62,   625,     8,     9,    63,   766,
      12,    58,    14,    48,   635,    63,   637,    71,   639,   576,
      58,    23,   779,   192,    26,   764,   717,   654,   648,   300,
     695,   502,   550,   772,   729,   143,   722,   723,   724,   660,
     661,    43,   779,   769,   665,    47,   667,   609,   669,   527,
     671,   737,   738,   739,   735,   515,   613,   671,   391,   287,
     751,   681,   396,   596,   383,   721,   437,   660,   381,   525,
     639,   692,   471,   323,    -1,     8,     9,    -1,    -1,    12,
      -1,    14,    -1,    -1,    -1,    -1,    -1,    20,    -1,    -1,
      23,    -1,   783,    26,    27,    28,   717,   654,    -1,    -1,
      -1,    34,    35,    36,    37,    -1,    39,    40,     8,     9,
      43,    -1,    12,    46,    14,    48,    49,    -1,    -1,    52,
      20,    54,    -1,    23,    -1,    25,    26,    -1,    -1,    -1,
     751,    -1,    -1,    -1,    34,    -1,    -1,    -1,    -1,    -1,
      -1,     8,     9,    43,   764,    12,    -1,    14,    48,    49,
     771,    -1,   772,    20,    -1,    -1,    23,    -1,    -1,    26,
      -1,    -1,   783,    -1,    -1,    -1,    -1,    34,    -1,    -1,
      -1,    -1,   729,    -1,    -1,    -1,    43,    -1,    -1,    -1,
      -1,    48,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     747,    -1,    -1,    -1,    -1,    -1,     7,     8,     9,   756,
      -1,   758,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,     8,     9,    -1,    -1,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,     8,    -1,    10,    -1,    -1,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,     8,     9,    -1,    -1,    -1,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     4,    69,    72,    73,     0,     3,    71,    74,   274,
     275,    16,    21,    33,    43,    90,    91,    56,     3,     8,
       9,    94,    98,   268,    92,    98,    96,    98,    44,   274,
     275,    75,   274,    12,    99,    61,    95,    99,    93,    99,
      97,    58,    91,    57,    12,    14,    23,    26,    43,    47,
      76,    77,    78,   268,   100,   268,    99,    99,    99,    12,
      59,   141,   143,   274,   268,   268,    58,   268,    58,    66,
     270,   271,   275,    79,   101,   274,   274,    81,    80,    12,
      82,    59,    83,   274,    78,    58,    62,    56,   142,   143,
      58,    58,    58,   274,     6,     7,    11,    12,    31,    56,
      59,    62,   204,   219,   220,   221,   222,   227,   268,   144,
     274,   102,   274,    67,   272,   273,   274,    31,    59,   218,
     222,   218,   218,     6,    60,    84,    85,    86,   228,    60,
     223,   205,    63,    14,    17,    19,    23,    41,    43,    51,
      53,    55,    91,   145,   146,   147,   152,   153,   154,   155,
     156,   157,   158,   159,   162,   165,   170,   172,   174,   232,
     233,   235,   236,   268,    57,    12,    14,    20,    23,    26,
      27,    28,    34,    35,    36,    37,    39,    40,    43,    46,
      48,    49,    52,    54,   103,   104,   105,   268,   274,    60,
     143,   272,   273,    56,    87,   229,   274,   224,   274,   274,
      19,   158,   232,    41,    55,   158,   158,   232,    30,    38,
     158,   232,    12,    41,   275,   146,   275,   271,   271,   271,
     157,     8,    10,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    70,   267,   267,    59,
     274,    27,    39,    46,    54,   268,    27,    39,    46,    54,
     268,    58,   111,    58,    58,   110,    58,    58,   119,    58,
      27,    39,    46,    54,   268,   115,    58,    58,    58,   270,
     271,   106,    60,    85,    32,    45,    88,    89,    57,   221,
     227,   230,   231,    60,   221,   222,   225,   226,   227,    24,
     206,   207,   208,   210,   211,   212,   268,   267,   267,    41,
     267,   267,   267,    58,    58,   267,   267,   148,    61,   160,
      59,    61,   237,    60,   113,   121,   117,    58,   108,   112,
     120,   116,    58,   107,    12,    58,    12,   268,    58,    62,
     213,   213,    58,    62,   137,   114,   122,   118,    58,   109,
      58,   268,   204,   141,   105,    58,    58,    58,   270,   271,
     272,   273,   272,   273,    12,    70,   209,   266,   268,    63,
     270,   271,   209,    59,    61,   239,   163,    61,   238,   141,
     141,    61,   240,    58,    18,    29,    50,    58,   202,     7,
      22,    50,    58,   250,    58,    58,    58,   141,    58,    58,
      58,    58,   141,    58,     7,    31,    59,   131,   133,   263,
      31,    86,   123,   214,     7,    31,    59,    86,   124,   126,
     274,    58,    58,    58,   141,    58,    31,    59,   134,   136,
     263,   219,    11,    11,    57,    89,   231,   226,    58,   208,
      58,    60,    18,   250,   202,    18,   250,    18,   250,   274,
      58,    59,    58,    31,   203,   220,   161,    60,    58,    58,
       7,    31,    59,   251,   253,   254,    56,   242,   131,   124,
     134,   218,   131,   124,   134,   218,   274,   262,   263,   274,
     127,   274,   262,     7,   138,   139,   140,   131,   124,   134,
     218,   274,   204,   220,    58,   164,    58,    58,    62,   166,
       7,   264,   173,    56,   194,   241,     7,   234,    65,   274,
      62,   255,   256,   274,    60,   132,   133,    12,   215,   216,
     217,    56,   128,    60,   125,   126,   128,    64,    63,   272,
     273,    60,   135,   136,   167,   194,   168,   169,   274,    31,
      59,   184,   186,   264,    60,    62,   188,   274,   256,   188,
     263,   265,   266,    60,   252,   253,   257,    12,    14,    20,
      23,    26,    34,    43,    48,    49,    57,   243,   244,   245,
     268,   272,   273,    64,    63,   272,   273,   274,   272,   273,
       7,   140,   272,   273,   184,   184,   184,    12,   149,   150,
     274,   187,    58,   189,    12,    14,    20,    23,    25,    26,
      34,    43,    48,    49,    57,   195,   196,   197,   268,   272,
     273,   274,   268,   268,    58,    58,   268,    58,   270,   271,
     246,    60,   133,    12,   217,    20,    57,    89,   129,   130,
      60,   126,    60,   136,   151,    63,   149,    60,   185,   186,
      65,   171,   274,   268,   268,    58,    58,    58,   268,    58,
     270,   271,   198,    60,   253,    14,    19,    23,    43,   174,
     258,   259,   260,   261,   248,   247,    12,   268,   249,   268,
      57,   245,    58,    58,   270,   271,   101,   272,   273,   265,
     175,   266,     8,    11,   190,   191,   192,   269,   200,   199,
     268,    12,   268,   201,   268,    57,   197,    58,    15,    63,
     270,   271,    58,    58,    58,   219,   204,    57,   130,    62,
      60,   186,   176,    63,   272,   273,    64,    58,    58,    58,
     219,    58,   260,   218,   218,   218,   144,    56,   181,   192,
      31,   193,   218,   218,   218,   141,    63,   274,    62,   177,
     220,   274,    48,    57,   182,   183,   274,    58,   270,   271,
      63,   157,   178,   179,   204,    57,   183,   266,   270,   271,
     180,    63,   179,    58,   220
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
        case 47:

/* Line 1455 of yacc.c  */
#line 1280 "pxr/usd/sdf/textFileFormat.yy"
    {

        // Store the names of the root prims.
        _SetField(
            SdfPath::AbsoluteRootPath(), SdfChildrenKeys->PrimChildren,
            context->nameChildrenStack.back(), context);
        context->nameChildrenStack.pop_back();
    ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 1291 "pxr/usd/sdf/textFileFormat.yy"
    {
            _MatchMagicIdentifier((yyvsp[(1) - (1)]), context);
            context->nameChildrenStack.push_back(std::vector<TfToken>());

            _CreateSpec(
                SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot, context);

            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 1304 "pxr/usd/sdf/textFileFormat.yy"
    {
            // Abort if error after layer metadata.
            ABORT_IF_ERROR(context->seenError);

            // If we're only reading metadata and we got here, 
            // we're done.
            if (context->metadataOnly)
                YYACCEPT;
        ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 1330 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 1335 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 1337 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 1344 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 1347 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 1350 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 1353 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 1356 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePseudoRoot, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 1359 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePseudoRoot, context);
        ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 1364 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 1376 "pxr/usd/sdf/textFileFormat.yy"
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

  case 72:

/* Line 1455 of yacc.c  */
#line 1395 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->subLayerPaths.push_back(context->layerRefPath);
            context->subLayerOffsets.push_back(context->layerRefOffset);
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 1403 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = (yyvsp[(1) - (1)]).Get<std::string>();
            context->layerRefOffset = SdfLayerOffset();
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1421 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetOffset( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1425 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefOffset.SetScale( (yyvsp[(3) - (3)]).Get<double>() );
            ABORT_IF_ERROR(context->seenError);
        ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 1441 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken();
        ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1445 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierDef;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 1449 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken();
        ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 1453 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierClass;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 1457 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken();
        ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 1461 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specifier = SdfSpecifierOver;
            context->typeName = TfToken((yyvsp[(2) - (2)]).Get<std::string>());
        ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1465 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 1475 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1476 "pxr/usd/sdf/textFileFormat.yy"
    { 
            (yyval) = std::string( (yyvsp[(1) - (3)]).Get<std::string>() + '.'
                    + (yyvsp[(3) - (3)]).Get<std::string>() ); 
        ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1483 "pxr/usd/sdf/textFileFormat.yy"
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

  case 98:

/* Line 1455 of yacc.c  */
#line 1516 "pxr/usd/sdf/textFileFormat.yy"
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

  case 108:

/* Line 1455 of yacc.c  */
#line 1564 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment, 
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1569 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypePrim, context);
        ;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 1571 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 1578 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 1581 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 1584 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 1587 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1590 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypePrim, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1593 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypePrim, context);
        ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 1598 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation, 
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1605 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Kind, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1612 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission, 
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context), 
                context);
        ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1620 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
        ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1623 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Payload, 
                SdfPayload(context->layerRefPath, context->savedPath), context);
        ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1629 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1631 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1634 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1636 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1639 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1641 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1644 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->inheritParsingTargetPaths.clear();
        ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1646 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetInheritListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1650 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1652 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1655 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1657 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1660 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1662 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1665 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->specializesParsingTargetPaths.clear();
        ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1667 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetSpecializesListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1671 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1675 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeExplicit, context);
        ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1678 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1682 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeDeleted, context);
        ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1685 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1689 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeAdded, context);
        ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1692 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->layerRefPath = std::string();
            context->savedPath = SdfPath();
            context->referenceParsingRefs.clear();
        ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1696 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetReferenceListItems(SdfListOpTypeOrdered, context);
        ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1701 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Relocates, 
                context->relocatesParsingMap, context);
            context->relocatesParsingMap.clear();
        ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1709 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSelection(context);
        ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1713 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeExplicit, context); 
            context->nameVector.clear();
        ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1717 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeDeleted, context);
            context->nameVector.clear();
        ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1721 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeAdded, context);
            context->nameVector.clear();
        ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1725 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimSetVariantSetNamesListItems(SdfListOpTypeOrdered, context);
            context->nameVector.clear();
        ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1731 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1736 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1743 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrefixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1751 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SuffixSubstitutions, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1777 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (context->layerRefPath.empty()) {
            Err(context, "Reference asset path must not be empty. If this "
                "is intended to be an internal reference, remove the "
                "'@@'.");
        }

        SdfReference ref(context->layerRefPath,
                         context->savedPath,
                         context->layerRefOffset);
        ref.SwapCustomData(context->currentDictionaries[0]);
        context->referenceParsingRefs.push_back(ref);
    ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1790 "pxr/usd/sdf/textFileFormat.yy"
    {
        // Internal references do not begin with an asset path so there's
        // no layer_ref rule, but we need to make sure we reset state the
        // so we don't pick up data from a previously-parsed reference.
        context->layerRefPath.clear();
        context->layerRefOffset = SdfLayerOffset();
        ABORT_IF_ERROR(context->seenError);
      ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1798 "pxr/usd/sdf/textFileFormat.yy"
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

  case 180:

/* Line 1455 of yacc.c  */
#line 1843 "pxr/usd/sdf/textFileFormat.yy"
    {
        _InheritAppendPath(context);
        ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1861 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SpecializesAppendPath(context);
        ;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1881 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelocatesAdd((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 1897 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->nameVector.push_back(TfToken((yyvsp[(1) - (1)]).Get<std::string>()));
        ;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1915 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 1916 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 1917 "pxr/usd/sdf/textFileFormat.yy"
    {;}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 1923 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string name = (yyvsp[(2) - (2)]).Get<std::string>();
        ERROR_IF_NOT_ALLOWED(context, SdfSchema::IsValidVariantIdentifier(name));

        context->currentVariantSetNames.push_back( name );
        context->currentVariantNames.push_back( std::vector<std::string>() );

        context->path = context->path.AppendVariantSelection(name, "");
    ;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1931 "pxr/usd/sdf/textFileFormat.yy"
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

  case 212:

/* Line 1455 of yacc.c  */
#line 1962 "pxr/usd/sdf/textFileFormat.yy"
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

  case 213:

/* Line 1455 of yacc.c  */
#line 1982 "pxr/usd/sdf/textFileFormat.yy"
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

  case 214:

/* Line 1455 of yacc.c  */
#line 2005 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PrimOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 2014 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 2036 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 2039 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue(SdfVariabilityConfig);
    ;}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 2045 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->assoc = VtValue();
    ;}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 2051 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue((yyvsp[(1) - (1)]).Get<std::string>(), context);
    ;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 2054 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetupValue(std::string((yyvsp[(1) - (3)]).Get<std::string>() + "[]"), context);
    ;}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 2060 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->variability = VtValue();
        context->custom = false;
    ;}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 2064 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
    ;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 2070 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (2)]), context);

        if (!context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 2077 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 2082 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 2088 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        _PrimInitAttribute((yyvsp[(3) - (3)]), context);

        if (!context->values.valueTypeIsValid) {
            context->values.StartRecordingString();
        }
    ;}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 2096 "pxr/usd/sdf/textFileFormat.yy"
    {
        if (!context->values.valueTypeIsValid) {
            context->values.StopRecordingString();
        }
    ;}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 2101 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 2107 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 2111 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeExplicit, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 2115 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = true;
    ;}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 2119 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeAdded, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 2123 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 2127 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeDeleted, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 2131 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(3) - (6)]), context);
        context->connParsingTargetPaths.clear();
        context->connParsingAllowConnectionData = false;
    ;}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 2135 "pxr/usd/sdf/textFileFormat.yy"
    {
        _AttributeSetConnectionTargetsList(SdfListOpTypeOrdered, context);
        context->path = context->path.GetParentPath();
    ;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 2142 "pxr/usd/sdf/textFileFormat.yy"
    {
        _PrimInitAttribute((yyvsp[(2) - (8)]), context);
        context->mapperTarget = context->savedPath;
        context->path = context->path.AppendMapper(context->mapperTarget);
    ;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 2147 "pxr/usd/sdf/textFileFormat.yy"
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

  case 241:

/* Line 1455 of yacc.c  */
#line 2166 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitAttribute((yyvsp[(2) - (5)]), context);
        ;}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 2169 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            context->path = context->path.GetParentPath(); // pop attr
        ;}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 2191 "pxr/usd/sdf/textFileFormat.yy"
    {
        const std::string mapperName((yyvsp[(1) - (1)]).Get<std::string>());
        if (_HasSpec(context->path, context)) {
            Err(context, "Duplicate mapper");
        }

        _CreateSpec(context->path, SdfSpecTypeMapper, context);
        _SetField(context->path, SdfFieldKeys->TypeName, mapperName, context);
    ;}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 2211 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfChildrenKeys->MapperArgChildren, 
            context->mapperArgsNameVector, context);
        context->mapperArgsNameVector.clear();
    ;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 2225 "pxr/usd/sdf/textFileFormat.yy"
    {
            TfToken mapperParamName((yyvsp[(2) - (2)]).Get<std::string>());
            context->mapperArgsNameVector.push_back(mapperParamName);
            context->path = context->path.AppendMapperArg(mapperParamName);

            _CreateSpec(context->path, SdfSpecTypeMapperArg, context);

        ;}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 2232 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->MapperArgValue, 
                context->currentValue, context);
            context->path = context->path.GetParentPath(); // pop mapper arg
        ;}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 2252 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryArgs, 
                context->currentDictionaries[0], context);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 2273 "pxr/usd/sdf/textFileFormat.yy"
    {
            _AttributeAppendConnectionPath(context);
        ;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 2276 "pxr/usd/sdf/textFileFormat.yy"
    {
            _AttributeAppendConnectionPath(context);
        ;}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 2278 "pxr/usd/sdf/textFileFormat.yy"
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

  case 272:

/* Line 1455 of yacc.c  */
#line 2302 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples = SdfTimeSampleMap();
    ;}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 2318 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (2)]).Get<double>();
    ;}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 2321 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSamples[ context->timeSampleTime ] = context->currentValue;
    ;}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 2325 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->timeSampleTime = (yyvsp[(1) - (3)]).Get<double>();
        context->timeSamples[ context->timeSampleTime ] 
            = VtValue(SdfValueBlock());  
    ;}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 2355 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 2360 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeAttribute, context);
        ;}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 2362 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 2369 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 2372 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 2375 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 2378 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 2381 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeAttribute, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 2384 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeAttribute, context);
        ;}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 2389 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 2396 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 2403 "pxr/usd/sdf/textFileFormat.yy"
    {
             _SetField(
                 context->path, SdfFieldKeys->DisplayUnit,
                 _GetDisplayUnitFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                 context);
        ;}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 2411 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 2416 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken(), context);
        ;}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 2429 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfFieldKeys->Default,
            context->currentValue, context);
    ;}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 2434 "pxr/usd/sdf/textFileFormat.yy"
    {
        _SetField(
            context->path, SdfFieldKeys->Default,
            SdfValueBlock(), context);
    ;}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 2446 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 2449 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 2465 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertValue((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 2468 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInsertDictionary((yyvsp[(2) - (4)]), context);
        ;}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 2486 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory((yyvsp[(1) - (1)]), context);
    ;}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 2492 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitShapedFactory((yyvsp[(1) - (3)]), context);
    ;}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 2502 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryBegin(context);
        ;}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 2505 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryEnd(context);
        ;}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 2521 "pxr/usd/sdf/textFileFormat.yy"
    {
            _DictionaryInitScalarFactory(Value(std::string("string")), context);
            _ValueAppendAtomic((yyvsp[(3) - (3)]), context);
            _ValueSetAtomic(context);
            _DictionaryInsertValue((yyvsp[(1) - (3)]), context);
        ;}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 2534 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->currentValue = VtValue();
        if (context->values.IsRecordingString()) {
            context->values.SetRecordedString("None");
        }
    ;}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 2540 "pxr/usd/sdf/textFileFormat.yy"
    {
        _ValueSetList(context);
    ;}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 2550 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->currentValue.Swap(context->currentDictionaries[0]);
            context->currentDictionaries[0].clear();
        ;}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 2555 "pxr/usd/sdf/textFileFormat.yy"
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

  case 333:

/* Line 1455 of yacc.c  */
#line 2568 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetAtomic(context);
        ;}
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 2571 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetTuple(context);
        ;}
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 2574 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetList(context);
        ;}
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 2577 "pxr/usd/sdf/textFileFormat.yy"
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

  case 337:

/* Line 1455 of yacc.c  */
#line 2588 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueSetCurrentToSdfPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 2594 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 339:

/* Line 1455 of yacc.c  */
#line 2597 "pxr/usd/sdf/textFileFormat.yy"
    {
            _ValueAppendAtomic((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 340:

/* Line 1455 of yacc.c  */
#line 2600 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs identifiers to be stored as TfToken
            // instead of std::string to be able to distinguish between them.
            _ValueAppendAtomic(TfToken((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 341:

/* Line 1455 of yacc.c  */
#line 2605 "pxr/usd/sdf/textFileFormat.yy"
    {
            // The ParserValueContext needs asset paths to be stored as
            // SdfAssetPath instead of std::string to be able to distinguish
            // between them
            _ValueAppendAtomic(SdfAssetPath((yyvsp[(1) - (1)]).Get<std::string>()), context);
        ;}
    break;

  case 342:

/* Line 1455 of yacc.c  */
#line 2618 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginList();
        ;}
    break;

  case 343:

/* Line 1455 of yacc.c  */
#line 2621 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndList();
        ;}
    break;

  case 350:

/* Line 1455 of yacc.c  */
#line 2646 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.BeginTuple();
        ;}
    break;

  case 351:

/* Line 1455 of yacc.c  */
#line 2648 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->values.EndTuple();
        ;}
    break;

  case 357:

/* Line 1455 of yacc.c  */
#line 2671 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 358:

/* Line 1455 of yacc.c  */
#line 2675 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityUniform);
    ;}
    break;

  case 359:

/* Line 1455 of yacc.c  */
#line 2679 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = true;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 360:

/* Line 1455 of yacc.c  */
#line 2683 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->custom = false;
        context->variability = VtValue(SdfVariabilityVarying);
    ;}
    break;

  case 361:

/* Line 1455 of yacc.c  */
#line 2690 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context); 
        ;}
    break;

  case 362:

/* Line 1455 of yacc.c  */
#line 2693 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->TimeSamples,
                context->timeSamples, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 363:

/* Line 1455 of yacc.c  */
#line 2702 "pxr/usd/sdf/textFileFormat.yy"
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

  case 364:

/* Line 1455 of yacc.c  */
#line 2717 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (2)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 365:

/* Line 1455 of yacc.c  */
#line 2722 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeExplicit, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 366:

/* Line 1455 of yacc.c  */
#line 2727 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 367:

/* Line 1455 of yacc.c  */
#line 2730 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeDeleted, context); 
            _PrimEndRelationship(context);
        ;}
    break;

  case 368:

/* Line 1455 of yacc.c  */
#line 2735 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
            context->relParsingAllowTargetData = true;
        ;}
    break;

  case 369:

/* Line 1455 of yacc.c  */
#line 2739 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeAdded, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 370:

/* Line 1455 of yacc.c  */
#line 2744 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(3) - (3)]), context);
        ;}
    break;

  case 371:

/* Line 1455 of yacc.c  */
#line 2747 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipSetTargetsList(SdfListOpTypeOrdered, context);
            _PrimEndRelationship(context);
        ;}
    break;

  case 372:

/* Line 1455 of yacc.c  */
#line 2752 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PrimInitRelationship((yyvsp[(2) - (5)]), context);
            context->relParsingAllowTargetData = true;
            _RelationshipAppendTargetPath((yyvsp[(4) - (5)]), context);
            _RelationshipInitTarget(context->relParsingTargetPaths->back(),
                                    context);
        ;}
    break;

  case 373:

/* Line 1455 of yacc.c  */
#line 2759 "pxr/usd/sdf/textFileFormat.yy"
    {
            // This clause only defines relational attributes for a target,
            // it does not add to the relationship target list. However, we 
            // do need to create a relationship target spec to associate the
            // attributes with.
            _PrimEndRelationship(context);
        ;}
    break;

  case 384:

/* Line 1455 of yacc.c  */
#line 2788 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Comment,
                (yyvsp[(1) - (1)]).Get<std::string>(), context);
        ;}
    break;

  case 385:

/* Line 1455 of yacc.c  */
#line 2793 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(1) - (1)]), SdfSpecTypeRelationship, context);
        ;}
    break;

  case 386:

/* Line 1455 of yacc.c  */
#line 2795 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 387:

/* Line 1455 of yacc.c  */
#line 2802 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeDeleted;
        ;}
    break;

  case 388:

/* Line 1455 of yacc.c  */
#line 2805 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 389:

/* Line 1455 of yacc.c  */
#line 2808 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeAdded;
        ;}
    break;

  case 390:

/* Line 1455 of yacc.c  */
#line 2811 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 391:

/* Line 1455 of yacc.c  */
#line 2814 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataStart((yyvsp[(2) - (2)]), SdfSpecTypeRelationship, context);
            context->listOpType = SdfListOpTypeOrdered;
        ;}
    break;

  case 392:

/* Line 1455 of yacc.c  */
#line 2817 "pxr/usd/sdf/textFileFormat.yy"
    {
            _GenericMetadataEnd(SdfSpecTypeRelationship, context);
        ;}
    break;

  case 393:

/* Line 1455 of yacc.c  */
#line 2822 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Documentation,
                (yyvsp[(3) - (3)]).Get<std::string>(), context);
        ;}
    break;

  case 394:

/* Line 1455 of yacc.c  */
#line 2829 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->Permission,
                _GetPermissionFromString((yyvsp[(3) - (3)]).Get<std::string>(), context),
                context);
        ;}
    break;

  case 395:

/* Line 1455 of yacc.c  */
#line 2837 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction,
                TfToken((yyvsp[(3) - (3)]).Get<std::string>()), context);
        ;}
    break;

  case 396:

/* Line 1455 of yacc.c  */
#line 2842 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->SymmetryFunction, 
                TfToken(), context);
        ;}
    break;

  case 400:

/* Line 1455 of yacc.c  */
#line 2856 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 401:

/* Line 1455 of yacc.c  */
#line 2859 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->relParsingTargetPaths = SdfPathVector();
        ;}
    break;

  case 406:

/* Line 1455 of yacc.c  */
#line 2875 "pxr/usd/sdf/textFileFormat.yy"
    {
            _RelationshipAppendTargetPath((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 407:

/* Line 1455 of yacc.c  */
#line 2878 "pxr/usd/sdf/textFileFormat.yy"
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

  case 410:

/* Line 1455 of yacc.c  */
#line 2908 "pxr/usd/sdf/textFileFormat.yy"
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

  case 411:

/* Line 1455 of yacc.c  */
#line 2922 "pxr/usd/sdf/textFileFormat.yy"
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

  case 416:

/* Line 1455 of yacc.c  */
#line 2946 "pxr/usd/sdf/textFileFormat.yy"
    {
        ;}
    break;

  case 418:

/* Line 1455 of yacc.c  */
#line 2952 "pxr/usd/sdf/textFileFormat.yy"
    {
            _SetField(
                context->path, SdfFieldKeys->PropertyOrder, 
                context->nameVector, context);
            context->nameVector.clear();
        ;}
    break;

  case 419:

/* Line 1455 of yacc.c  */
#line 2965 "pxr/usd/sdf/textFileFormat.yy"
    {
        context->savedPath = SdfPath();
    ;}
    break;

  case 421:

/* Line 1455 of yacc.c  */
#line 2972 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetPrim((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 422:

/* Line 1455 of yacc.c  */
#line 2978 "pxr/usd/sdf/textFileFormat.yy"
    {
            _PathSetProperty((yyvsp[(1) - (1)]), context);
        ;}
    break;

  case 423:

/* Line 1455 of yacc.c  */
#line 2984 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->marker = context->savedPath.GetString();
        ;}
    break;

  case 424:

/* Line 1455 of yacc.c  */
#line 2987 "pxr/usd/sdf/textFileFormat.yy"
    {
            context->marker = (yyvsp[(1) - (1)]).Get<std::string>();
        ;}
    break;

  case 433:

/* Line 1455 of yacc.c  */
#line 3019 "pxr/usd/sdf/textFileFormat.yy"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;



/* Line 1455 of yacc.c  */
#line 5723 "pxr/usd/sdf/textFileFormat.tab.cpp"
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
#line 3051 "pxr/usd/sdf/textFileFormat.yy"


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
    Sdf_MemoryFlexBuffer(FILE* file, const std::string& name, yyscan_t scanner);
    ~Sdf_MemoryFlexBuffer();

    yy_buffer_state *GetBuffer() { return _flexBuffer; }

private:
    yy_buffer_state *_flexBuffer;

    std::unique_ptr<char[]> _fileBuffer;
    size_t _fileBufferSize;

    yyscan_t _scanner;
};

Sdf_MemoryFlexBuffer::Sdf_MemoryFlexBuffer(FILE* file, 
                                           const std::string& name,
                                           yyscan_t scanner)
    : _flexBuffer(nullptr)
    , _scanner(scanner)
{
    int64_t fileSize = ArchGetFileLength(file);
    if (fileSize == -1) {
        TF_RUNTIME_ERROR("Error retrieving file size for @%s@: %s", 
                         name.c_str(), strerror(errno));
        return;
    }

    // flex requires 2 bytes of null padding at the end of any buffers it is
    // given.  We'll allocate a buffer with 2 padding bytes, then read the
    // entire file in.
    static const size_t paddingBytesRequired = 2;

    std::unique_ptr<char[]> buffer(new char[fileSize + paddingBytesRequired]);

    fseek(file, 0, SEEK_SET);
    if (fread(buffer.get(), fileSize, 1, file) == 0) {
        TF_RUNTIME_ERROR("Failed to read file contents @%s@: %s",
                         name.c_str(), feof(file) ?
                         "premature end-of-file" : ArchStrerror().c_str());
        return;
    }

    // Set null padding.
    memset(buffer.get() + fileSize, '\0', paddingBytesRequired);
    _fileBuffer = std::move(buffer);
    _flexBuffer = textFileFormatYy_scan_buffer(
        _fileBuffer.get(), fileSize + paddingBytesRequired, _scanner);
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
bool Sdf_ParseMenva(const std::string & fileContext, FILE *fin,
                   const std::string & magicId,
                   const std::string & versionString,
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
    context.values.errorReporter = boost::bind(_ReportParseError, &context, _1);

    // Initialize the scanner, allowing it to be reentrant.
    textFileFormatYylex_init(&context.scanner);
    textFileFormatYyset_extra(&context, context.scanner);

    int status = -1;
    {
        Sdf_MemoryFlexBuffer input(fin, fileContext, context.scanner);
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
    context.values.errorReporter = boost::bind(_ReportParseError, &context, _1);

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

