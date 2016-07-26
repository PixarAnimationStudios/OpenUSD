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
///
/// \file sdf/fileIO_Common.h 


#ifndef SDF_FILEIO_COMMON_H
#define SDF_FILEIO_COMMON_H

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/mapperSpec.h"
#include "pxr/usd/sdf/mapperArgSpec.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/variantSetSpec.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

#include <algorithm>
#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////
// Simple FileIO Utilities

class Sdf_FileIOUtility {

public:

    // === Stream output helpers

    // Non-formatted string output
    static void Puts(std::ostream &out,
                size_t indent, const std::string &str);
    // Printf-style formatted string output
    static void Write(std::ostream &out,
                size_t indent, const char *fmt, ...);

    static bool OpenParensIfNeeded(std::ostream &out,
                bool didParens, bool multiLine);
    static void CloseParensIfNeeded(std::ostream &out,
                size_t indent, bool didParens, bool multiLine);

    static void WriteQuotedString(std::ostream &out,
                size_t indent, const std::string &str);

    static void WriteAssetPath(std::ostream &out,
                size_t indent, const std::string &str);

    static void WriteDefaultValue(std::ostream &out,
                size_t indent, VtValue value);

    static void WriteSdfPath(std::ostream &out,
                size_t indent, const SdfPath &path,
                const std::string &markerName = "");

    static bool WriteNameVector(std::ostream &out,
                size_t indent, const std::vector<std::string> &vec);
    static bool WriteNameVector(std::ostream &out,
                size_t indent, const std::vector<TfToken> &vec);

    static bool WriteTimeSamples(std::ostream &out,
                size_t indent, const SdfTimeSampleMap &);

    static bool WriteRelocates(std::ostream &out,
                size_t indent, bool multiLine,
                const SdfRelocatesMap &reloMap);

    static void WriteDictionary(std::ostream &out,
                size_t indent, bool multiLine,
                const VtDictionary &dictionary,
                bool stringValuesOnly=false);

    template <class T>
    static void WriteListOp(std::ostream &out,
                size_t indent,
                const TfToken& fieldName,
                const SdfListOp<T>& listOp);

    static void WriteLayerOffset(std::ostream &out,
                size_t indent, bool multiline,
                const SdfLayerOffset& offset);

    // === String production and transformation helpers

    /// Quote \p str, adding quotes before and after and escaping
    /// unprintable characters and the quote character itself.  If
    /// the string contains newlines it's quoted with triple quotes
    /// and the newlines are not escaped.
    static std::string Quote(const std::string &str);
    static std::string Quote(const TfToken &token);

    // Create a string from a value
    static std::string StringFromVtValue(const VtValue &value);

    // Convert enums to a strings for use in menva syntax.
    // Note that in some cases we use empty strings to represent the
    // default values of these enums.
    static const char* Stringify( SdfPermission val );
    static const char* Stringify( SdfSpecifier val );
    static const char* Stringify( SdfVariability val );

private:

    // Helper types to write a VtDictionary so that its keys are ordered.
    struct _StringLessThan {
        bool operator()(const std::string *lhs, const std::string *rhs) const {
            return *lhs < *rhs;
        }
    };
    typedef std::map<const std::string *, const VtValue *, _StringLessThan>
        _OrderedDictionary;

    static void _WriteDictionary(std::ostream &out,
                size_t indent, bool multiLine, _OrderedDictionary &dictionary,
                bool stringValuesOnly);
};

////////////////////////////////////////////////////////////////////////
// Helpers for determining if a field should be included in a spec's
// metadata section.

struct Sdf_IsMetadataField
{
    Sdf_IsMetadataField(const SdfSpecType specType)
        : _specDef(SdfSchema::GetInstance().GetSpecDefinition(specType))
    { }

    bool operator()(const TfToken& field) const
    { 
        // Allow fields tagged explicitly as metadata, or fields
        // that are invalid, as these may be unrecognized plugin
        // metadata fields. In this case, there may be a string
        // representation that needs to be written out.
        return (not _specDef->IsValidField(field) or 
                    _specDef->IsMetadataField(field)); 
    }

    const SdfSchema::SpecDefinition* _specDef;
};

////////////////////////////////////////////////////////////////////////

static bool
Sdf_WritePrimPreamble(
    const SdfPrimSpec &prim, std::ostream &out, size_t indent)
{
    SdfSpecifier spec = prim.GetSpecifier();
    bool writeTypeName = true;
    if (not SdfIsDefiningSpecifier(spec)) {
        // For non-defining specifiers, we write typeName only if we have
        // a setting.
        writeTypeName = prim.HasField(SdfFieldKeys->TypeName);
    }

    TfToken typeName;
    if (writeTypeName) {
        typeName = prim.GetTypeName();
        if (typeName == SdfTokens->AnyTypeToken){
            typeName = TfToken();
        }
    }

    Sdf_FileIOUtility::Write( out, indent, "%s%s%s ",
            Sdf_FileIOUtility::Stringify(spec),
            not typeName.IsEmpty() ? " " : "",
            not typeName.IsEmpty() ? typeName.GetText() : "" );
    Sdf_FileIOUtility::WriteQuotedString( out, 0, prim.GetName().c_str() );

    return true;
}

template <class ListOpType>
static bool
Sdf_WriteIfListOp(
    std::ostream& out, size_t indent,
    const TfToken& field, const VtValue& value)
{
    if (value.IsHolding<ListOpType>()) {
        Sdf_FileIOUtility::WriteListOp(
            out, indent, field, value.UncheckedGet<ListOpType>());
        return true;
    }
    return false;
}

static void
Sdf_WriteSimpleField(
    std::ostream &out, size_t indent,
    const SdfSpec& spec, const TfToken& field)
{
    const VtValue& value = spec.GetField(field);

    if (Sdf_WriteIfListOp<SdfIntListOp>(out, indent, field, value) or
        Sdf_WriteIfListOp<SdfInt64ListOp>(out, indent, field, value) or
        Sdf_WriteIfListOp<SdfUIntListOp>(out, indent, field, value) or
        Sdf_WriteIfListOp<SdfUInt64ListOp>(out, indent, field, value) or
        Sdf_WriteIfListOp<SdfStringListOp>(out, indent, field, value) or
        Sdf_WriteIfListOp<SdfTokenListOp>(out, indent, field, value)) {
        return;
    }

    bool isUnregisteredValue = value.IsHolding<SdfUnregisteredValue>();

    if (isUnregisteredValue) {
        // The value boxed inside a SdfUnregisteredValue can either be a
        // std::string, a VtDictionary, or an SdfUnregisteredValueListOp.
        const VtValue &boxedValue = value.Get<SdfUnregisteredValue>().GetValue();
        if (boxedValue.IsHolding<SdfUnregisteredValueListOp>()) {
            Sdf_FileIOUtility::WriteListOp(
                out, indent, field, 
                boxedValue.UncheckedGet<SdfUnregisteredValueListOp>());
        }
        else {
            Sdf_FileIOUtility::Write(out, indent, "%s = ", field.GetText());
            if (boxedValue.IsHolding<VtDictionary>()) {
                Sdf_FileIOUtility::WriteDictionary(out, indent, true, boxedValue.Get<VtDictionary>());
            } 
            else if (boxedValue.IsHolding<std::string>()) {
                Sdf_FileIOUtility::Write(out, 0, "%s\n", boxedValue.Get<std::string>().c_str());
            }
        }
        return;
    }

    Sdf_FileIOUtility::Write(out, indent, "%s = ", field.GetText());
    if (value.IsHolding<VtDictionary>()) {
        Sdf_FileIOUtility::WriteDictionary(out, indent, true, value.Get<VtDictionary>());
    } 
    else if (value.IsHolding<bool>()) {
        Sdf_FileIOUtility::Write(out, 0, "%s\n", TfStringify(value.Get<bool>()).c_str());
    }
    else {
        Sdf_FileIOUtility::Write(out, 0, "%s\n", Sdf_FileIOUtility::StringFromVtValue(value).c_str());
    }
}

// Predicate for determining fields that should be included in a
// prim's metadata section.
struct Sdf_IsPrimMetadataField : public Sdf_IsMetadataField
{
    Sdf_IsPrimMetadataField() : Sdf_IsMetadataField(SdfSpecTypePrim) { }
    
    bool operator()(const TfToken& field) const
    { 
        // Typename is registered as metadata for a prim, but is written
        // outside the metadata section.
        if (field == SdfFieldKeys->TypeName) {
            return false;
        }

        return (Sdf_IsMetadataField::operator()(field) or
            field == SdfFieldKeys->Payload or
            field == SdfFieldKeys->References or
            field == SdfFieldKeys->Relocates or 
            field == SdfFieldKeys->InheritPaths or
            field == SdfFieldKeys->Specializes or
            field == SdfFieldKeys->VariantSetNames or
            field == SdfFieldKeys->VariantSelection);
    }
};

static bool
Sdf_WritePrimMetadata(
    const SdfPrimSpec &prim, std::ostream &out, size_t indent)
{
    // Partition this prim's fields so that all fields to write out are
    // in the range [fields.begin(), metadataFieldsEnd).
    TfTokenVector fields = prim.ListFields();
    TfTokenVector::iterator metadataFieldsEnd = 
        std::partition(fields.begin(), fields.end(), Sdf_IsPrimMetadataField());

    // Comment isn't tagged as a metadata field but gets special cased
    // because it wants to be at the top of the metadata section.
    std::string comment = prim.GetComment();
    bool hasComment     = !comment.empty();

    bool didParens = false;

    // As long as there's anything to write in the metadata section, we'll
    // always use the multi-line format.
    bool multiLine = hasComment or (fields.begin() != metadataFieldsEnd);

    // Write comment at the top of the metadata section for readability.
    if (hasComment) {
        didParens = 
            Sdf_FileIOUtility::OpenParensIfNeeded(out, didParens, multiLine);
        Sdf_FileIOUtility::WriteQuotedString(out, indent+1, comment);
        Sdf_FileIOUtility::Puts(out, 0, "\n");
    }

    // Write out remaining fields in the metadata section in dictionary-sorted
    // order.
    std::sort(fields.begin(), metadataFieldsEnd, TfDictionaryLessThan());
    for (TfTokenVector::const_iterator fieldIt = fields.begin();
         fieldIt != metadataFieldsEnd; ++fieldIt) {

        didParens = 
            Sdf_FileIOUtility::OpenParensIfNeeded(out, didParens, multiLine);

        const TfToken& field = *fieldIt;

        if (field == SdfFieldKeys->Documentation) {
            Sdf_FileIOUtility::Puts(out, indent+1, "doc = ");
            Sdf_FileIOUtility::WriteQuotedString(out, 0, prim.GetDocumentation());
            Sdf_FileIOUtility::Puts(out, 0, "\n");
        }
        else if (field == SdfFieldKeys->Permission) {
            if (multiLine) {
                Sdf_FileIOUtility::Write(out, indent+1, "permission = %s\n",
                    Sdf_FileIOUtility::Stringify(prim.GetPermission()) );
            } else {
                Sdf_FileIOUtility::Write(out, 0, "permission = %s",
                    Sdf_FileIOUtility::Stringify(prim.GetPermission()) );
            }
        }
        else if (field == SdfFieldKeys->SymmetryFunction) {
            Sdf_FileIOUtility::Write(out, multiLine ? indent+1 : 0, "symmetryFunction = %s%s",
                prim.GetSymmetryFunction().GetText(),
                multiLine ? "\n" : "");
        }
        else if (field == SdfFieldKeys->Payload) {
            if (multiLine) {
                Sdf_FileIOUtility::Puts(out, indent+1, "");
            }
            Sdf_FileIOUtility::Puts(out, 0, "payload = ");
            if (SdfPayload payload = prim.GetPayload()) {
                Sdf_FileIOUtility::WriteAssetPath(out, 0, payload.GetAssetPath());
                if (not payload.GetPrimPath().IsEmpty())
                    Sdf_FileIOUtility::WriteSdfPath(out, 0, payload.GetPrimPath());
            } else {
                Sdf_FileIOUtility::Puts(out, 0, "None");
            }
            if (multiLine) {
                Sdf_FileIOUtility::Puts(out, 0, "\n");
            }
        }
        else if (field == SdfFieldKeys->References) {
            const VtValue v = prim.GetField(field);
            if (not Sdf_WriteIfListOp<SdfReferenceListOp>(
                    out, indent+1, TfToken("references"), v)) {
                TF_CODING_ERROR(
                    "'%s' field holding unexpected type '%s'",
                    field.GetText(), v.GetTypeName().c_str());
            }
        }
        else if (field == SdfFieldKeys->VariantSetNames) {
            SdfVariantSetNamesProxy variantSetNameList = prim.GetVariantSetNameList();
            if (variantSetNameList.IsExplicit()) {
                // Explicit list
                SdfVariantSetNamesProxy::ListProxy setNames = variantSetNameList.GetExplicitItems();
                Sdf_FileIOUtility::Puts(out, indent+1, "variantSets = ");
                Sdf_FileIOUtility::WriteNameVector(out, indent+1, setNames);
                Sdf_FileIOUtility::Puts(out, 0, "\n");
            } else {
                // List operations
                SdfVariantSetNamesProxy::ListProxy setNames = variantSetNameList.GetDeletedItems();
                if (not setNames.empty()) {
                    Sdf_FileIOUtility::Puts(out, indent+1, "delete variantSets = ");
                    Sdf_FileIOUtility::WriteNameVector(out, indent+1, setNames);
                    Sdf_FileIOUtility::Puts(out, 0, "\n");
                }
                setNames = variantSetNameList.GetAddedItems();
                if (not setNames.empty()) {
                    Sdf_FileIOUtility::Puts(out, indent+1, "add variantSets = ");
                    Sdf_FileIOUtility::WriteNameVector(out, indent+1, setNames);
                    Sdf_FileIOUtility::Puts(out, 0, "\n");
                }
                setNames = variantSetNameList.GetOrderedItems();
                if (not setNames.empty()) {
                    Sdf_FileIOUtility::Puts(out, indent+1, "reorder variantSets = ");
                    Sdf_FileIOUtility::WriteNameVector(out, indent+1, setNames);
                    Sdf_FileIOUtility::Puts(out, 0, "\n");
                }
            }
        }
        else if (field == SdfFieldKeys->InheritPaths) {
            const VtValue v = prim.GetField(field);
            if (not Sdf_WriteIfListOp<SdfPathListOp>(
                    out, indent+1, TfToken("inherits"), v)) {
                TF_CODING_ERROR(
                    "'%s' field holding unexpected type '%s'",
                    field.GetText(), v.GetTypeName().c_str());
            }
        }
        else if (field == SdfFieldKeys->Specializes) {
            const VtValue v = prim.GetField(field);
            if (not Sdf_WriteIfListOp<SdfPathListOp>(
                    out, indent+1, TfToken("specializes"), v)) {
                TF_CODING_ERROR(
                    "'%s' field holding unexpected type '%s'",
                    field.GetText(), v.GetTypeName().c_str());
            }
        }
        else if (field == SdfFieldKeys->Relocates) {

            // Relativize all paths in the relocates.
            SdfRelocatesMap result;
            SdfPath primPath = prim.GetPath();
            
            SdfRelocatesMap finalRelocates;
            const SdfRelocatesMapProxy relocates = prim.GetRelocates();
            TF_FOR_ALL(mapIt, relocates) {
                finalRelocates[mapIt->first.MakeRelativePath(primPath)] =
                    mapIt->second.MakeRelativePath(primPath);
            }

            Sdf_FileIOUtility::WriteRelocates(
                out, indent+1, multiLine, finalRelocates);
        }
        else if (field == SdfFieldKeys->PrefixSubstitutions) {
            VtDictionary prefixSubstitutions = prim.GetPrefixSubstitutions();
            Sdf_FileIOUtility::Puts(out, indent+1, "prefixSubstitutions = ");
            Sdf_FileIOUtility::WriteDictionary(out, indent+1, multiLine,
                          prefixSubstitutions, /* stringValuesOnly = */ true );
        }
        else if (field == SdfFieldKeys->VariantSelection) {
            SdfVariantSelectionMap refVariants = prim.GetVariantSelections();
            if (refVariants.size() > 0) {
                VtDictionary dictionary;
                TF_FOR_ALL(it, refVariants) {
                    dictionary[it->first] = VtValue(it->second);
                }
                Sdf_FileIOUtility::Puts(out, indent+1, "variants = ");
                Sdf_FileIOUtility::WriteDictionary(out, indent+1, multiLine, dictionary);
            }
        }
        else {
            Sdf_WriteSimpleField(out, indent+1, prim, field);
        }

    } // end for each field

    Sdf_FileIOUtility::CloseParensIfNeeded(out, indent, didParens, multiLine);

    return true;
}

namespace {
struct _SortByNameThenType {
    template <class T>
    bool operator()(T const &lhs, T const &rhs) const {
        // If the names are identical, order by spectype.  This puts Attributes
        // before Relationships (if identically named).
        std::string const &lhsName = lhs->GetName();
        std::string const &rhsName = rhs->GetName();
        return (lhsName == rhsName and lhs->GetSpecType() < rhs->GetSpecType())
            or TfDictionaryLessThan()(lhsName, rhsName);
    }
};
}

static bool
Sdf_WritePrimProperties(
    const SdfPrimSpec &prim, std::ostream &out, size_t indent)
{
    std::vector<SdfPropertySpecHandle> properties =
        prim.GetProperties().values_as<std::vector<SdfPropertySpecHandle> >();
    std::sort(properties.begin(), properties.end(), _SortByNameThenType());
    TF_FOR_ALL(it, properties) {
        (*it)->WriteToStream(out, indent+1);
    }

    return true;
}

static bool
Sdf_WritePrimNamespaceReorders(
    const SdfPrimSpec &prim, std::ostream &out, size_t indent)
{
    const std::vector<TfToken>& propertyNames = prim.GetPropertyOrder();

    if ( propertyNames.size() > 1 ) {
        Sdf_FileIOUtility::Puts( out, indent+1, "reorder properties = " );
        Sdf_FileIOUtility::WriteNameVector( out, indent+1, propertyNames );
        Sdf_FileIOUtility::Puts( out, 0, "\n" );
    }

    const std::vector<TfToken>& childrenNames = prim.GetNameChildrenOrder();

    if ( childrenNames.size() > 1 ) {

        Sdf_FileIOUtility::Puts( out, indent+1, "reorder nameChildren = " );
        Sdf_FileIOUtility::WriteNameVector( out, indent+1, childrenNames );
        Sdf_FileIOUtility::Puts( out, 0, "\n" );
    }

    return true;
}

static bool
Sdf_WritePrimChildren(
    const SdfPrimSpec &prim, std::ostream &out, size_t indent)
{
    bool newline = false;
    TF_FOR_ALL(i, prim.GetNameChildren()) {
        if (newline)
            Sdf_FileIOUtility::Puts(out, 0, "\n");
        else
            newline = true;
        (*i)->WriteToStream(out, indent+1);
    }

    return true;
}

static bool
Sdf_WritePrimVariantSets(
    const SdfPrimSpec &prim, std::ostream &out, size_t indent)
{
    SdfVariantSetsProxy variantSets = prim.GetVariantSets();
    if (variantSets) {
        TF_FOR_ALL(it, variantSets) {
            SdfVariantSetSpecHandle variantSet = it->second;
            variantSet->WriteToStream(out, indent+1);
        }
    }
    return true;
}

static bool
Sdf_WritePrimBody(
    const SdfPrimSpec &prim, std::ostream &out, size_t indent)
{
    Sdf_WritePrimNamespaceReorders( prim, out, indent );

    Sdf_WritePrimProperties( prim, out, indent );

    if (!prim.GetProperties().empty() && !prim.GetNameChildren().empty())
        Sdf_FileIOUtility::Puts(out, 0, "\n");

    Sdf_WritePrimChildren( prim, out, indent );

    Sdf_WritePrimVariantSets( prim, out, indent );

    return true;
}

static inline bool
Sdf_WritePrim(
    const SdfPrimSpec &prim, std::ostream &out, size_t indent)
{
    Sdf_WritePrimPreamble( prim, out, indent );
    Sdf_WritePrimMetadata( prim, out, indent );

    Sdf_FileIOUtility::Puts(out, 0, "\n");
    Sdf_FileIOUtility::Puts(out, indent, "{\n");

    Sdf_WritePrimBody( prim, out, indent );

    Sdf_FileIOUtility::Puts(out, indent, "}\n");

    return true;
}

static bool
Sdf_WriteConnectionStatement(
    std::ostream &out,
            size_t indent, const SdfConnectionsProxy::ListProxy &connections,
            const std::string &opStr,
            const std::string &variabilityStr,
            const std::string &typeStr, const std::string &nameStr,
    const SdfAttributeSpec* attrOwner)
{
    Sdf_FileIOUtility::Write(out, indent, "%s%s%s %s.connect = ",
                            opStr.c_str(),
                            variabilityStr.c_str(),
                            typeStr.c_str(), nameStr.c_str());

    if (connections.size() == 0) {
        Sdf_FileIOUtility::Puts(out, 0, "None\n");
    } 
    else if (connections.size() == 1) {
        Sdf_FileIOUtility::WriteSdfPath(out, 0, connections.front(),
            (attrOwner ? attrOwner->GetConnectionMarker(connections.front()) : ""));
        Sdf_FileIOUtility::Puts(out, 0, "\n");
    } 
    else {
        Sdf_FileIOUtility::Puts(out, 0, "[\n");
        TF_FOR_ALL(it, connections) {
            Sdf_FileIOUtility::WriteSdfPath(out, indent+1, (*it),
                (attrOwner ? attrOwner->GetConnectionMarker(*it) : ""));
            Sdf_FileIOUtility::Puts(out, 0, ",\n");
        }
        Sdf_FileIOUtility::Puts(out, indent, "]\n");
    }
    return true;
}

static bool
Sdf_WriteConnectionList(
    std::ostream &out,
                       size_t indent, const SdfConnectionsProxy &connList,
                       const std::string &variabilityStr,
                       const std::string &typeStr, const std::string &nameStr,
    const SdfAttributeSpec *attrOwner)
{
    if (connList.IsExplicit()) {
        SdfConnectionsProxy::ListProxy vec = connList.GetExplicitItems();
        Sdf_WriteConnectionStatement(out, indent, vec, "",
                                    variabilityStr,
                                    typeStr, nameStr, attrOwner);
    } else {
        SdfConnectionsProxy::ListProxy vec = connList.GetDeletedItems();
        if (not vec.empty()) {
            Sdf_WriteConnectionStatement(out, indent, vec, "delete ",
                                              variabilityStr, typeStr, nameStr,
                                              NULL);
        }
        vec = connList.GetAddedItems();
        if (not vec.empty()) {
            Sdf_WriteConnectionStatement(out, indent, vec, "add ",
                                        variabilityStr, typeStr,
                                        nameStr, attrOwner);
        }
        vec = connList.GetOrderedItems();
        if (not vec.empty()) {
            Sdf_WriteConnectionStatement(out, indent, vec, "reorder ",
                                              variabilityStr, typeStr, nameStr,
                                              NULL);
        }
    }
    return true;
}

// Predicate for determining fields that should be included in an
// attribute's metadata section.
struct Sdf_IsAttributeMetadataField : public Sdf_IsMetadataField
{
    Sdf_IsAttributeMetadataField() : Sdf_IsMetadataField(SdfSpecTypeAttribute)
    { }
    
    bool operator()(const TfToken& field) const
    {
        return (Sdf_IsMetadataField::operator()(field) or
            field == SdfFieldKeys->DisplayUnit);
    }
};

static inline bool
Sdf_WriteAttribute(
    const SdfAttributeSpec &attr, std::ostream &out, size_t indent)
{
    std::string variabilityStr =
        Sdf_FileIOUtility::Stringify( attr.GetVariability() );
    if (not variabilityStr.empty())
        variabilityStr += ' ';

    bool hasComment           = !attr.GetComment().empty();
    bool hasDefault           = attr.HasField(SdfFieldKeys->Default);
    bool hasCustomDeclaration = attr.IsCustom();
    bool hasConnections       = attr.HasField(SdfFieldKeys->ConnectionPaths);
    bool hasTimeSamples       = attr.HasField(SdfFieldKeys->TimeSamples);

    // The mapper children field contains all connection paths for which
    // mappers exist, so for efficiency purposes we can directly grab that 
    // field instead of going through the mapper proxy.
    SdfPathVector mapperPaths = 
        attr.GetFieldAs<SdfPathVector>(SdfChildrenKeys->MapperChildren);
    bool hasMappers = !mapperPaths.empty();

    const SdfPathVector markerPaths = attr.GetConnectionMarkerPaths();
    bool hasMarkers = !markerPaths.empty();

    std::string typeName =
        SdfValueTypeNames->GetSerializationName(attr.GetTypeName()).GetString();

    // Partition this attribute's fields so that all fields to write in the
    // metadata section are in the range [fields.begin(), metadataFieldsEnd).
    TfTokenVector fields = attr.ListFields();
    TfTokenVector::iterator metadataFieldsEnd = std::partition(
        fields.begin(), fields.end(), Sdf_IsAttributeMetadataField());

    // As long as there's anything to write in the metadata section, we'll
    // always use the multi-line format.
    bool hasInfo = hasComment or (fields.begin() != metadataFieldsEnd);
    bool multiLine = hasInfo;

    bool didParens = false;

    // Write the basic line if we have info or a default or if we
    // have nothing else to write.
    if (hasInfo || hasDefault || hasCustomDeclaration ||
        (!hasConnections &&
         !hasMappers && !hasMarkers && !hasTimeSamples))
    {
        VtValue value;

        if(hasDefault)
            value = attr.GetDefaultValue();

        Sdf_FileIOUtility::Write( out, indent, "%s%s%s %s",
            (hasCustomDeclaration ? "custom " : ""),
            variabilityStr.c_str(),
            typeName.c_str(),
            attr.GetName().c_str() );

        // If we have a default value, write it...
        if (not value.IsEmpty()) {
            Sdf_FileIOUtility::WriteDefaultValue(out, indent, value);
        }

        // Write comment at the top of the metadata section for readability.
        if (hasComment) {
            didParens = Sdf_FileIOUtility::OpenParensIfNeeded(out, didParens, multiLine);
            Sdf_FileIOUtility::WriteQuotedString(out, indent+1, attr.GetComment());
            Sdf_FileIOUtility::Puts(out, 0, "\n");
        }

        // Write out remaining fields in the metadata section in
        // dictionary-sorted order.
        std::sort(fields.begin(), metadataFieldsEnd, TfDictionaryLessThan());
        for (TfTokenVector::const_iterator fieldIt = fields.begin();
             fieldIt != metadataFieldsEnd; ++fieldIt) {

            didParens = 
                Sdf_FileIOUtility::OpenParensIfNeeded(out, didParens, multiLine);

            const TfToken& field = *fieldIt;

            if (field == SdfFieldKeys->Documentation) {
                Sdf_FileIOUtility::Puts(out, indent+1, "doc = ");
                Sdf_FileIOUtility::WriteQuotedString(out, 0, attr.GetDocumentation());
                Sdf_FileIOUtility::Puts(out, 0, "\n");
            }
            else if (field == SdfFieldKeys->Permission) {
                Sdf_FileIOUtility::Write(out, multiLine ? indent+1 : 0, "permission = %s%s",
                                    Sdf_FileIOUtility::Stringify(attr.GetPermission()),
                                    multiLine ? "\n" : "");
            }
            else if (field == SdfFieldKeys->SymmetryFunction) {
                Sdf_FileIOUtility::Write(out, multiLine ? indent+1 : 0, "symmetryFunction = %s%s",
                                    attr.GetSymmetryFunction().GetText(),
                                    multiLine ? "\n" : "");
            }
            else if (field == SdfFieldKeys->DisplayUnit) {
                Sdf_FileIOUtility::Write(out, multiLine ? indent+1 : 0, "displayUnit = %s%s",
                                    SdfGetNameForUnit(attr.GetDisplayUnit()).c_str(),
                                    multiLine ? "\n" : "");
            }
            else {
                Sdf_WriteSimpleField(out, indent+1, attr, field);
            }

        } // end for each field

        Sdf_FileIOUtility::CloseParensIfNeeded(out, indent, didParens, multiLine);
        Sdf_FileIOUtility::Puts(out, 0, "\n");
    }

    if (hasTimeSamples) {
        Sdf_FileIOUtility::Write(out, indent, "%s%s %s.timeSamples = {\n",
                                variabilityStr.c_str(),
                                typeName.c_str(), attr.GetName().c_str() );
        Sdf_FileIOUtility::WriteTimeSamples( out, indent,
                                            attr.GetTimeSampleMap() );
        Sdf_FileIOUtility::Puts(out, indent, "}\n");
    }

    if (hasConnections) {
        Sdf_WriteConnectionList(out, indent, attr.GetConnectionPathList(),
                               variabilityStr, typeName,
                               attr.GetName(), &attr);
    }

    std::sort(mapperPaths.begin(), mapperPaths.end());
    TF_FOR_ALL(it, mapperPaths) {
        const SdfPath mapperPath = attr.GetPath().AppendMapper(*it);
        const SdfMapperSpecHandle mapper = TfStatic_cast<SdfMapperSpecHandle>(
            attr.GetLayer()->GetObjectAtPath(mapperPath));
        if (not TF_VERIFY(mapper)) {
            continue;
        }

        SdfMapperParametersMap params;
        const SdfMapperArgsProxy args = mapper->GetArgs();
        TF_FOR_ALL(argIt, args) {
            const SdfMapperArgSpecHandle arg = argIt->second;
            params[argIt->first] = arg->GetValue();
        }
        
        const VtDictionary symmetryArgs = 
            mapper->GetFieldAs<VtDictionary>(SdfFieldKeys->SymmetryArgs);

        const std::string name = mapper->GetTypeName();

        if (!name.empty()) {
            Sdf_FileIOUtility::Write(out, indent, "%s%s %s.mapper[ ",
                                    variabilityStr.c_str(),
                                    typeName.c_str(), attr.GetName().c_str() );
            Sdf_FileIOUtility::WriteSdfPath(out, 0, (*it));
            Sdf_FileIOUtility::Write(out, 0, " ] = %s", name.c_str() );
            if (not symmetryArgs.empty()) {
                Sdf_FileIOUtility::Write(out, 0, " (\n");
                Sdf_FileIOUtility::Write(out, indent+1, "symmetryArguments = ");
                Sdf_FileIOUtility::WriteDictionary(out, indent+1,
                                                  true, symmetryArgs);
                Sdf_FileIOUtility::Write(out, indent, ")");
            }
            if (not params.empty()) {
                Sdf_FileIOUtility::Write(out, 0, " {\n");
                TF_FOR_ALL(paramIt, params) {
                    const TfToken& name =
                        SdfValueTypeNames->GetSerializationName(paramIt->second);
                    Sdf_FileIOUtility::Write(out, indent+1, "%s %s = %s\n",
                                            name.GetText(),
                                            paramIt->first.c_str(),
                                            Sdf_FileIOUtility::StringFromVtValue(paramIt->second).c_str() );
                }
                Sdf_FileIOUtility::Write(out, indent, "}");
            }
            Sdf_FileIOUtility::Write(out, 0, "\n");
        }
    }

    return true;
}

enum Sdf_WriteFlag {
    Sdf_WriteFlagDefault = 0,
    Sdf_WriteFlagAttributes = 1,
    Sdf_WriteFlagNoLastNewline = 2,
};

inline Sdf_WriteFlag operator |(Sdf_WriteFlag a, Sdf_WriteFlag b)
{
    return (Sdf_WriteFlag)(static_cast<int>(a) | static_cast<int>(b));
}

static bool
Sdf_WriteRelationshipTargetList(
    const SdfRelationshipSpec &rel,
            const SdfTargetsProxy::ListProxy &targetPaths,
            std::ostream &out, size_t indent, Sdf_WriteFlag flags)
{
    if (targetPaths.size() > 1) {
        Sdf_FileIOUtility::Write(out, 0," = [\n");
        ++indent;
    } else {
        Sdf_FileIOUtility::Write(out, 0," = ");
    }

    for (size_t i=0; i < targetPaths.size(); ++i) {
        if (targetPaths.size() > 1) {
            Sdf_FileIOUtility::Write(out, indent, "");
        }
        Sdf_FileIOUtility::WriteSdfPath( out, 0, targetPaths[i],
                    rel.GetTargetMarker(targetPaths[i]));
        if (flags & Sdf_WriteFlagAttributes) {

            std::vector< SdfAttributeSpecHandle > attrs =
                rel.GetAttributesForTargetPath( targetPaths[i] ).values();

            std::vector< TfToken > attrOrderNames =
                rel.GetAttributeOrderForTargetPath( targetPaths[i] );

            if ( not attrs.empty() || attrOrderNames.size() > 1 ) {

                Sdf_FileIOUtility::Write(out, 0, " {\n");

                if ( attrOrderNames.size() > 1 ) {
                    Sdf_FileIOUtility::Write(
                        out, indent+1, "reorder attributes = " );
                    Sdf_FileIOUtility::WriteNameVector(
                        out, indent+1, attrOrderNames );
                    Sdf_FileIOUtility::Write( out, 0, "\n" );
                }

                TF_FOR_ALL(it, attrs)
                    (*it)->WriteToStream(out, indent+1);
                Sdf_FileIOUtility::Write(out, indent, "}");
            }
        }
        if (targetPaths.size() > 1) {
            Sdf_FileIOUtility::Write(out, 0,",\n");
        }
    }

    if (targetPaths.size() > 1) {
        --indent;
        Sdf_FileIOUtility::Write(out, indent, "]");
    }
    if (not(flags & Sdf_WriteFlagNoLastNewline)) {
        Sdf_FileIOUtility::Write(out, 0,"\n");
    }
    return true;
}

static bool
Sdf_WriteRelationalAttributesForTarget(
    const SdfRelationshipSpec &rel,
            const SdfPath &targetPath,
            std::ostream &out, size_t indent)
{
    std::vector< SdfAttributeSpecHandle > attrs =
        rel.GetAttributesForTargetPath( targetPath ).values();
    std::vector< TfToken > attrOrderNames =
        rel.GetAttributeOrderForTargetPath( targetPath );
    if (not attrs.empty() or attrOrderNames.size() > 1) {
        Sdf_FileIOUtility::Write(out, indent, "rel ");

        Sdf_FileIOUtility::Write(
            out, 0, rel.GetName().c_str());
        Sdf_FileIOUtility::Write(
            out, 0, "[");
        Sdf_FileIOUtility::WriteSdfPath(
            out, 0, targetPath);
        Sdf_FileIOUtility::Write(
            out, 0, "] {\n");

        if ( attrOrderNames.size() > 1 ) {
            Sdf_FileIOUtility::Write(
                out, indent+1, "reorder attributes = " );
            Sdf_FileIOUtility::WriteNameVector(
                out, indent+1, attrOrderNames );
            Sdf_FileIOUtility::Write( out, 0, "\n" );
        }

        TF_FOR_ALL(it, attrs)
            (*it)->WriteToStream(out, indent+1);
        Sdf_FileIOUtility::Write(out, indent, "}\n");
    }

    return true;
}

// Predicate for determining fields that should be included in an
// relationship's metadata section.
struct Sdf_IsRelationshipMetadataField : public Sdf_IsMetadataField
{
    Sdf_IsRelationshipMetadataField() 
        : Sdf_IsMetadataField(SdfSpecTypeRelationship) { }
    
    bool operator()(const TfToken& field) const
    {
        return Sdf_IsMetadataField::operator()(field);
    }
};

static inline bool
Sdf_WriteRelationship(
    const SdfRelationshipSpec &rel, std::ostream &out, size_t indent)
{
    // When a new metadata field is added to the spec, it will be automatically
    // written out generically, so you probably don't need to add a special case
    // here. If you need to special-case the output of a metadata field, you will
    // also need to prevent the automatic output by adding the token inside
    // Sdf_GetGenericRelationshipMetadataFields().
    //
    // These special cases below were all kept to prevent reordering in existing
    // menva files, which would create noise in file diffs.
    bool hasComment           = !rel.GetComment().empty();
    bool hasTargets           = rel.HasField(SdfFieldKeys->TargetPaths);
    bool hasDefaultValue      = rel.HasField(SdfFieldKeys->Default);
    bool hasTimeSamples       = rel.HasField(SdfFieldKeys->TimeSamples);

    bool hasCustom            = rel.IsCustom();
    SdfPathVector markerPaths  = rel.GetTargetMarkerPaths();
    bool hasMarkers           = !markerPaths.empty();

    // Partition this attribute's fields so that all fields to write in the
    // metadata section are in the range [fields.begin(), metadataFieldsEnd).
    TfTokenVector fields = rel.ListFields();
    TfTokenVector::iterator metadataFieldsEnd = std::partition(
        fields.begin(), fields.end(), Sdf_IsRelationshipMetadataField());

    bool hasInfo = hasComment or (fields.begin() != metadataFieldsEnd);
    bool multiLine = hasInfo;

    bool didParens = false;

    bool hasExplicitTargets = false;
    bool hasTargetListOps = false;
    if (hasTargets) {
        SdfTargetsProxy targetPathList = rel.GetTargetPathList();
        hasExplicitTargets = targetPathList.IsExplicit() and
                             targetPathList.HasKeys();
        hasTargetListOps   = not targetPathList.IsExplicit() and
                             targetPathList.HasKeys();
    }

    // If relationship is a varying relationship, use varying keyword.
    bool isVarying = (rel.GetVariability() == SdfVariabilityVarying);
    std::string varyingStr = isVarying ? "varying " : ""; // the space in "varying " is required...

    // Figure out if there are any rel attrs to write
    bool hasRelAttrs = false;
    SdfPathVector attrTargetPaths = rel.GetAttributeTargetPaths();
    SdfPathVector attrOrderTargetPaths = rel.GetAttributeOrderTargetPaths();
    // Just combine the lists of paths with attrs and paths with attr orders
    attrTargetPaths.insert(attrTargetPaths.end(), attrOrderTargetPaths.begin(),
                attrOrderTargetPaths.end());
    TF_FOR_ALL(pathIt, attrTargetPaths) {
        hasRelAttrs =
                    ((not rel.GetAttributesForTargetPath(*pathIt).empty()) or
                    (rel.GetAttributeOrderForTargetPath(*pathIt).size() > 1));
        if (hasRelAttrs) {
            break;
        }
    }

    // We'll keep track of the target paths that we have written attribute
    // blocks for (as part of explicit targets or added targets) so we'll
    // know at the end which ones we need to write out as override blocks
    // for targets we don't otherwise have opinions about.
    std::set<SdfPath> targetsWhoseAttrsAreWritten;

    // Write the basic line if we have info or a default (i.e. explicit
    // targets) or if we have nothing else to write and we're not custom
    if (hasInfo || (hasTargets && hasExplicitTargets) ||
        (!hasTargetListOps && !hasRelAttrs && !hasMarkers && !rel.IsCustom()))
    {

        if (hasCustom) {
            Sdf_FileIOUtility::Write( out, indent, "custom %srel %s",
                                     varyingStr.c_str(),
                                     rel.GetName().c_str() );
        } else {
            Sdf_FileIOUtility::Write( out, indent, "%srel %s",
                                     varyingStr.c_str(),
                                     rel.GetName().c_str() );
        }

        if (hasTargets && hasExplicitTargets) {
            SdfTargetsProxy targetPathList = rel.GetTargetPathList();
            SdfTargetsProxy::ListProxy targetPaths = targetPathList.GetExplicitItems();
            if (targetPaths.size() == 0) {
                Sdf_FileIOUtility::Write(out, 0, " = None");
            } else {
                // Write explicit targets
                Sdf_WriteRelationshipTargetList(rel, targetPaths, out, indent,
                            Sdf_WriteFlagAttributes | Sdf_WriteFlagNoLastNewline);
                targetsWhoseAttrsAreWritten.insert(targetPaths.begin(),
                            targetPaths.end());
            }
        }

        // Write comment at the top of the metadata section for readability.
        if (hasComment) {
            didParens = Sdf_FileIOUtility::OpenParensIfNeeded(out, didParens, multiLine);
            Sdf_FileIOUtility::WriteQuotedString(out, indent+1, rel.GetComment());
            Sdf_FileIOUtility::Write(out, 0, "\n");
        }

        // Write out remaining fields in the metadata section in
        // dictionary-sorted order.
        std::sort(fields.begin(), metadataFieldsEnd, TfDictionaryLessThan());
        for (TfTokenVector::const_iterator fieldIt = fields.begin();
             fieldIt != metadataFieldsEnd; ++fieldIt) {

            didParens = 
                Sdf_FileIOUtility::OpenParensIfNeeded(out, didParens, multiLine);

            const TfToken& field = *fieldIt;

            if (field == SdfFieldKeys->Documentation) {
                Sdf_FileIOUtility::Write(out, indent+1, "doc = ");
                Sdf_FileIOUtility::WriteQuotedString(out, 0, rel.GetDocumentation());
                Sdf_FileIOUtility::Write(out, 0, "\n");
            }
            else if (field == SdfFieldKeys->Permission) {
                if (multiLine) {
                    Sdf_FileIOUtility::Write(out, indent+1, "permission = %s\n",
                            Sdf_FileIOUtility::Stringify(rel.GetPermission()));
                } else {
                    Sdf_FileIOUtility::Write(out, 0, "permission = %s",
                            Sdf_FileIOUtility::Stringify(rel.GetPermission()));
                }
            }
            else if (field == SdfFieldKeys->SymmetryFunction) {
                Sdf_FileIOUtility::Write(out, multiLine ? indent+1 : 0, "symmetryFunction = %s%s",
                                    rel.GetSymmetryFunction().GetText(),
                                    multiLine ? "\n" : "");
            }
            else {
                Sdf_WriteSimpleField(out, indent+1, rel, field);                
            }

        } // end for each field

        Sdf_FileIOUtility::CloseParensIfNeeded(out, indent, didParens, multiLine);
        Sdf_FileIOUtility::Write(out, 0,"\n");
    }
    else if (hasCustom) {
        // If we did not write out the "basic" line AND we are custom,
        // we need to add a custom decl line, because we won't include
        // custom in any of the output below
        Sdf_FileIOUtility::Write( out, indent, "custom %srel %s\n",
                                 varyingStr.c_str(),        
                                 rel.GetName().c_str() );
    }

    if (hasTargets && hasTargetListOps) {
        // Write deleted targets
        SdfTargetsProxy targetPathList = rel.GetTargetPathList();
        SdfTargetsProxy::ListProxy targetPaths = targetPathList.GetDeletedItems();
        if (not targetPaths.empty()) {
            Sdf_FileIOUtility::Write( out, indent, "delete %srel %s",
                varyingStr.c_str(), rel.GetName().c_str());
            Sdf_WriteRelationshipTargetList(rel, targetPaths, out, indent, Sdf_WriteFlagDefault);
        }

        // Write added targets
        targetPaths = targetPathList.GetAddedItems();
        if (not targetPaths.empty()) {
            Sdf_FileIOUtility::Write( out, indent, "add %srel %s",
                varyingStr.c_str(), rel.GetName().c_str());
            Sdf_WriteRelationshipTargetList(rel, targetPaths, out, indent, Sdf_WriteFlagAttributes);
            targetsWhoseAttrsAreWritten.insert(targetPaths.begin(),
                        targetPaths.end());
        }

        // Write ordered targets
        targetPaths = targetPathList.GetOrderedItems();
        if (not targetPaths.empty()) {
            Sdf_FileIOUtility::Write( out, indent, "reorder %srel %s",
                varyingStr.c_str(), rel.GetName().c_str());
            Sdf_WriteRelationshipTargetList(rel, targetPaths, out, indent, Sdf_WriteFlagDefault);
        }
    }

    // Write out relational attributes for targets we haven't handled above
    TF_FOR_ALL(pathIt, attrTargetPaths) {
        if (targetsWhoseAttrsAreWritten.count(*pathIt) == 0) {
            // We have not written attributes for this one.
            Sdf_WriteRelationalAttributesForTarget(rel, *pathIt, out, indent);
            targetsWhoseAttrsAreWritten.insert(*pathIt);
        }
    }

    if (hasTimeSamples) {
        Sdf_FileIOUtility::Write(out, indent, "%srel %s.timeSamples = {\n",
                                varyingStr.c_str(),
                                rel.GetName().c_str());
        Sdf_FileIOUtility::WriteTimeSamples( out, indent,
                                            rel.GetTimeSampleMap() );
        Sdf_FileIOUtility::Puts(out, indent, "}\n");
    }

    // Write out the default value for the relationship if we have one...
    if (hasDefaultValue)
    {
        VtValue value = rel.GetDefaultValue();
        if (not value.IsEmpty())
        {
            Sdf_FileIOUtility::Write(out, indent, "%srel %s.default = ",
                                    varyingStr.c_str(),
                                    rel.GetName().c_str());
            Sdf_FileIOUtility::WriteDefaultValue(out, 0, value);
            Sdf_FileIOUtility::Puts(out, indent, "\n");
        }
    }

    return true;
}

#endif
