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
//
// FileIO_Common.cpp

#include "pxr/usd/sdf/fileIO_Common.h"
#include "pxr/usd/sdf/reference.h"

#include <boost/assign.hpp>
#include <fstream>
#include <sstream>
#include <cctype>

using std::map;
using std::ostream;
using std::string;
using std::vector;

static const char *_IndentString = "    ";

// basically duplicate Vt::Array::_StreamRecursive() but specialize for
// string arrays by putting in quotes
template <class T>
static void
_StringFromVtStringArray(
    string *valueStr,
    const VtArray<T> &valArray)
{
    valueStr->append("[");
    if (typename VtArray<T>::const_pointer d = valArray.cdata()) {
        if (const size_t n = valArray.size()) {
            valueStr->append(Sdf_FileIOUtility::Quote(d[0]));
            for (size_t i = 1; i != n; ++i) {
                valueStr->append(", ");
                valueStr->append(Sdf_FileIOUtility::Quote(d[i]));
            }
        }
    }
    valueStr->append("]");
}

// Helper to created a quoted string if the given value is holding a 
// string-valued type (specified by T).
template <class T>
static bool
_StringFromVtStringValue(string* valueStr, const VtValue& value)
{
    if (value.IsHolding<T>()) {
        *valueStr = Sdf_FileIOUtility::Quote( value.UncheckedGet<T>() );
        return true;
    } 
    else if (value.IsHolding<VtArray<T> >()) {
        const VtArray<T>& valArray = value.UncheckedGet<VtArray<T> >();
        _StringFromVtStringArray(valueStr,valArray);
        return true;
    }
    return false;
}

// ------------------------------------------------------------
// Helpers functions for writing SdfListOp<T>. Consumers can
// specialize the _ListOpWriter struct for custom behavior based
// on the element type of the list op.
namespace
{

template <class T> 
struct _ListOpWriter
{
    static constexpr bool ItemPerLine = false;
    static constexpr bool SingleItemRequiresBrackets(const T& item)
    {
        return true;
    }
    static void Write(ostream& out, size_t indent, const T& item)
    {
        Sdf_FileIOUtility::Write(out, indent, TfStringify(item).c_str());
    }
};

template <>
struct _ListOpWriter<string>
{
    static constexpr bool ItemPerLine = false;
    static constexpr bool SingleItemRequiresBrackets(const string& s)
    {
        return true;
    }
    static void Write(ostream& out, size_t indent, const string& s)
    {
        Sdf_FileIOUtility::WriteQuotedString(out, indent, s);
    }
};

template <>
struct _ListOpWriter<TfToken>
{
    static constexpr bool ItemPerLine = false;
    static constexpr bool SingleItemRequiresBrackets(const TfToken& s)
    {
        return true;
    }
    static void Write(ostream& out, size_t indent, const TfToken& s)
    {
        Sdf_FileIOUtility::WriteQuotedString(out, indent, s.GetString());
    }
};

template <>
struct _ListOpWriter<SdfPath>
{
    static constexpr bool ItemPerLine = true;
    static constexpr bool SingleItemRequiresBrackets(const SdfPath& path)
    {
        return false;
    }
    static void Write(ostream& out, size_t indent, const SdfPath& path)
    {
        Sdf_FileIOUtility::WriteSdfPath(out, indent, path);
    }
};

template <>
struct _ListOpWriter<SdfReference>
{
    static constexpr bool ItemPerLine = true;
    static bool SingleItemRequiresBrackets(const SdfReference& ref)
    {
        return not ref.GetCustomData().empty();
    }
    static void Write(ostream& out, size_t indent, const SdfReference& ref)
    {
        bool multiLineRefMetaData = not ref.GetCustomData().empty();
    
        Sdf_FileIOUtility::Write(out, indent, "");

        if (not ref.GetAssetPath().empty()) {
            Sdf_FileIOUtility::WriteAssetPath(out, 0, ref.GetAssetPath());
            if (not ref.GetPrimPath().IsEmpty())
                Sdf_FileIOUtility::WriteSdfPath(out, 0, ref.GetPrimPath());
        }
        else {
            // If this is an internal reference, we always have to write
            // out a path, even if it's empty since that encodes a reference
            // to the default prim.
            Sdf_FileIOUtility::WriteSdfPath(out, 0, ref.GetPrimPath());
        }

        if (multiLineRefMetaData) {
            Sdf_FileIOUtility::Puts(out, 0, " (\n");
        }
        Sdf_FileIOUtility::WriteLayerOffset(
            out, indent+1, multiLineRefMetaData, ref.GetLayerOffset());
        if (not ref.GetCustomData().empty()) {
            Sdf_FileIOUtility::Puts(out, indent+1, "customData = ");
            Sdf_FileIOUtility::WriteDictionary(
                out, indent+1, /* multiline = */ true, ref.GetCustomData());
        }
        if (multiLineRefMetaData) {
            Sdf_FileIOUtility::Puts(out, indent, ")");
        }
    }
};

template <class ListOpList>
void
_WriteListOpList(
    ostream& out, size_t indent,
    const string& name, const ListOpList& listOpList, 
    const string& op = string())
{
    typedef _ListOpWriter<typename ListOpList::value_type> _Writer;

    Sdf_FileIOUtility::Write(out, indent, "%s%s%s = ", 
                            op.c_str(), op.empty() ? "" : " ", name.c_str());

    if (listOpList.empty()) {
        Sdf_FileIOUtility::Puts(out, 0, "None\n");
    }
    else if (listOpList.size() == 1 and 
             not _Writer::SingleItemRequiresBrackets(listOpList.front())) {
        _Writer::Write(out, 0, listOpList.front());
        Sdf_FileIOUtility::Puts(out, 0, "\n");
    }
    else {
        const bool itemPerLine =  _Writer::ItemPerLine;

        Sdf_FileIOUtility::Puts(out, 0, itemPerLine ? "[\n" : "[");
        TF_FOR_ALL(it, listOpList) {
            _Writer::Write(out, itemPerLine ? indent + 1 : 0, *it);
            if (it.GetNext()) {
                Sdf_FileIOUtility::Puts(out, 0, itemPerLine ? ",\n" : ", ");
            }
            else {
                Sdf_FileIOUtility::Puts(out, 0, itemPerLine ? "\n" : "");
            }
        }
        Sdf_FileIOUtility::Puts(out, itemPerLine ? indent : 0, "]\n");
    }
}

template <class ListOp>
void
_WriteListOp(
    ostream &out, size_t indent, 
    const std::string& name, const ListOp& listOp)
{
    if (listOp.IsExplicit()) {
        _WriteListOpList(out, indent, name, listOp.GetExplicitItems());
    } 
    else {
        if (not listOp.GetDeletedItems().empty()) {
            _WriteListOpList(out, indent, name, 
                             listOp.GetDeletedItems(), "delete");
        }
        if (not listOp.GetAddedItems().empty()) {
            _WriteListOpList(out, indent, name, 
                             listOp.GetAddedItems(), "add"); 
        }
        if (not listOp.GetOrderedItems().empty()) {
            _WriteListOpList(out, indent, name, 
                             listOp.GetOrderedItems(), "reorder");
        }
    }
}

} // end anonymous namespace
// ------------------------------------------------------------

void
Sdf_FileIOUtility::Puts(ostream &out, size_t indent, const std::string &str)
{
    for (size_t i=0; i < indent; ++i)
        out << _IndentString;

    out << str;
}

void
Sdf_FileIOUtility::Write(ostream &out, 
            size_t indent, const char *fmt, ...)
{
    for (size_t i=0; i < indent; ++i)
        out << _IndentString;

    va_list ap;
    va_start(ap, fmt);
    out << TfVStringPrintf(fmt, ap);
    va_end(ap);
}

bool
Sdf_FileIOUtility::OpenParensIfNeeded(ostream &out, 
            bool didParens, bool multiLine)
{
    if (!didParens) {
        Puts(out, 0, multiLine ? " (\n" : " (");
    } else if (!multiLine) {
        Puts(out, 0, "; ");
    }
    return true;
}

void
Sdf_FileIOUtility::CloseParensIfNeeded(ostream &out, 
            size_t indent, bool didParens, bool multiLine)
{
    if (didParens) {
        Puts(out, multiLine ? indent : 0, ")");
    }
}

void
Sdf_FileIOUtility::WriteQuotedString(ostream &out, 
            size_t indent, const string &str)
{
    Puts(out, indent, Quote(str));
}

void
Sdf_FileIOUtility::WriteAssetPath(ostream &out, 
                                 size_t indent, const string &str) {
    Write(out, indent, "@%s@", str.c_str());
}

void
Sdf_FileIOUtility::WriteDefaultValue(
    std::ostream &out, size_t indent, VtValue value)
{
    // ---
    // Special case for SdfPath value types
    // ---

    if (value.IsHolding<SdfPath>()) {
        WriteSdfPath(out, indent, value.Get<SdfPath>() );
        return;
    }

    // ---
    // General case value to string conversion and write-out.
    // ---
    std::string valueString = Sdf_FileIOUtility::StringFromVtValue(value);
    Sdf_FileIOUtility::Write(out, 0, " = %s", valueString.c_str());
}

void
Sdf_FileIOUtility::WriteSdfPath(ostream &out, 
            size_t indent, const SdfPath &path, const string &markerName) {

    if (ARCH_LIKELY(markerName.empty())) {
        Write(out, indent, "<%s>", path.GetString().c_str());
    } else {
        // Unexpected! That used to mean, an explicitly authored current marker.
        if (markerName == "None") {
            TF_RUNTIME_ERROR(
                "Encountered 'None' marker, this should not happen.");
            WriteSdfPath(out, indent, path, "current");
        } else if (markerName == "authored") {
            TF_RUNTIME_ERROR("Authored markers can't be authored in menva as "
                             "by object modelling.");
            WriteSdfPath(out, indent, path);
        } else {
            const char *fmt = SdfPath::IsBuiltInMarker(markerName)
                ? "<%s> @ %s" : "<%s> @ <%s>";
            Write(out, indent, fmt, path.GetText(), markerName.c_str());
        }
    }
}

template <class StrType>
static bool
_WriteNameVector(ostream &out, size_t indent, const vector<StrType> &vec)
{
    size_t i, c = vec.size();
    if (c>1) {
        Sdf_FileIOUtility::Puts(out, 0, "[");
    }
    for (i=0; i<c; i++) {
        if (i > 0) {
            Sdf_FileIOUtility::Puts(out, 0, ", ");
        }
        Sdf_FileIOUtility::WriteQuotedString(out, 0, vec[i]);
    }
    if (c>1) {
        Sdf_FileIOUtility::Puts(out, 0, "]");
    }
    return true;
}

bool
Sdf_FileIOUtility::WriteNameVector(ostream &out, 
            size_t indent, const vector<string> &vec)
{
    return _WriteNameVector(out, indent, vec);
}

bool
Sdf_FileIOUtility::WriteNameVector(ostream &out, 
            size_t indent, const vector<TfToken> &vec)
{
    return _WriteNameVector(out, indent, vec);
}

bool
Sdf_FileIOUtility::WriteTimeSamples(ostream &out, size_t indent,
                                    const SdfTimeSampleMap & samples)
{
    TF_FOR_ALL(i, samples) {
        Write(out, indent+1, "%g: ", i->first);
        if (i->second.IsHolding<SdfPath>()) {
            WriteSdfPath(out, 0, i->second.Get<SdfPath>() );
        } else {
            Puts(out, 0, StringFromVtValue( i->second ));
        }
        out << ",\n";
    }
    return true;
}

bool 
Sdf_FileIOUtility::WriteRelocates(ostream &out, 
            size_t indent, bool multiLine,
            const SdfRelocatesMap &reloMap)
{
    Write(out, indent, "relocates = %s", multiLine ? "{\n" : "{ ");
    size_t itemCount = reloMap.size();
    TF_FOR_ALL(it, reloMap) {
        WriteSdfPath(out, indent+1, it->first);
        Puts(out, 0, ": ");
        WriteSdfPath(out, 0, it->second);
        if (--itemCount > 0) {
            Puts(out, 0, ", ");
        }
        if (multiLine) {
            Puts(out, 0, "\n");
        }
    }
    if (multiLine) {
        Puts(out, indent, "}\n");
    }
    else {
        Puts(out, 0, " }");
    }
    
    return true;
}

void
Sdf_FileIOUtility::_WriteDictionary(ostream &out,
            size_t indent, bool multiLine,
            Sdf_FileIOUtility::_OrderedDictionary &dictionary,
            bool stringValuesOnly)
{
    Puts(out, 0, multiLine ? "{\n" : "{ ");
    size_t counter = dictionary.size();
    TF_FOR_ALL(i, dictionary) {
        counter--;
        const VtValue &value = *i->second;
        if (stringValuesOnly) {
            if (value.IsHolding<std::string>()) {
                WriteQuotedString(out, multiLine ? indent+1 : 0, *(i->first));
                Write(out, 0, ": ");
                WriteQuotedString(out, 0, value.Get<string>());
                if (counter > 0) {
                    Puts(out, 0, ", ");
                }
                if (multiLine) {
                    Puts(out, 0, "\n");
                }
            } else {
                // CODE_COVERAGE_OFF
                // This is not possible to hit with the current public API.
                TF_RUNTIME_ERROR("Dictionary has a non-string value under key "
                                 "\"%s\"; skipping", i->first->c_str());
                // CODE_COVERAGE_ON
            }
        } else {
            // Put quotes around the keyName if it is not a valid identifier
            string keyName = *(i->first);
            if (not TfIsValidIdentifier(keyName)) {
                keyName = "\"" + keyName + "\"";
            }
            if (value.IsHolding<VtDictionary>()) {
                Write(out, multiLine ? indent+1 : 0, "dictionary %s = ",
                      keyName.c_str());
                const VtDictionary &nestedDictionary =
                    value.Get<VtDictionary>();
                Sdf_FileIOUtility::_OrderedDictionary newDictionary;
                TF_FOR_ALL(it, nestedDictionary) {
                    newDictionary[&it->first] = &it->second;
                }
                _WriteDictionary(out, indent+1, multiLine, newDictionary,
                                 /* stringValuesOnly = */ false );
            } else {
                const TfToken& typeName =
                    SdfValueTypeNames->GetSerializationName(value);
                Write(out, multiLine ? indent+1 : 0, "%s %s = ",
                      typeName.GetText(), keyName.c_str());

                // XXX: The logic here is very similar to that in
                //      WriteDefaultValue. WBN to refactor.
                string str;
                if (_StringFromVtStringValue<string>(&str, value) or
                    _StringFromVtStringValue<TfToken>(&str, value)) {
                    Puts(out, 0, str);
                } else {
                    Puts(out, 0, TfStringify(value));
                }
                if (multiLine) {
                    Puts(out, 0, "\n");
                }
            }
        }
        if (not multiLine and counter > 0) {
            // CODE_COVERAGE_OFF
            // See multiLine comment below.
            Puts(out, 0, "; ");
            // CODE_COVERAGE_ON
        }
    }
    if (multiLine) {
        Puts(out, indent, "}\n");
    } else {
        // CODE_COVERAGE_OFF
        // Not currently hittable from public API.
        Puts(out, 0, " }");
        // CODE_COVERAGE_ON
    }
}

void 
Sdf_FileIOUtility::WriteDictionary(ostream &out, 
            size_t indent, bool multiLine,
            const VtDictionary &dictionary,
            bool stringValuesOnly)
{
    // Make sure the dictionary keys are written out in order.
    _OrderedDictionary newDictionary;
    TF_FOR_ALL(it, dictionary) {
        newDictionary[&it->first] = &it->second;
    }
    _WriteDictionary(out, indent, multiLine, newDictionary, stringValuesOnly);
}

template <class T>
void 
Sdf_FileIOUtility::WriteListOp(std::ostream &out,
                               size_t indent,
                               const TfToken& fieldName,
                               const SdfListOp<T>& listOp)
{
    _WriteListOp(out, indent, fieldName, listOp);
}

template void
Sdf_FileIOUtility::WriteListOp(std::ostream &, size_t, const TfToken&, 
                               const SdfPathListOp&);
template void
Sdf_FileIOUtility::WriteListOp(std::ostream &, size_t, const TfToken&, 
                               const SdfReferenceListOp&);
template void
Sdf_FileIOUtility::WriteListOp(std::ostream &, size_t, const TfToken&, 
                               const SdfIntListOp&);
template void
Sdf_FileIOUtility::WriteListOp(std::ostream &, size_t, const TfToken&, 
                               const SdfInt64ListOp&);
template void
Sdf_FileIOUtility::WriteListOp(std::ostream &, size_t, const TfToken&, 
                               const SdfUIntListOp&);
template void
Sdf_FileIOUtility::WriteListOp(std::ostream &, size_t, const TfToken&, 
                               const SdfUInt64ListOp&);
template void
Sdf_FileIOUtility::WriteListOp(std::ostream &, size_t, const TfToken&, 
                               const SdfStringListOp&);
template void
Sdf_FileIOUtility::WriteListOp(std::ostream &, size_t, const TfToken&, 
                               const SdfTokenListOp&);
template void
Sdf_FileIOUtility::WriteListOp(std::ostream &, size_t, const TfToken&, 
                               const SdfUnregisteredValueListOp&);

void 
Sdf_FileIOUtility::WriteLayerOffset(ostream &out,
            size_t indent, bool multiLine,
            const SdfLayerOffset& layerOffset)
{
    // If there's anything interesting to write, write it.
    if (layerOffset != SdfLayerOffset()) {
        if (not multiLine) {
            Write(out, 0, " (");
        }
        double offset = layerOffset.GetOffset();
        double scale = layerOffset.GetScale();
        if (offset != 0.0) {
            Write(out, multiLine ? indent : 0, "offset = %s%s",
                  TfStringify(offset).c_str(), multiLine ? "\n" : "");
        }
        if (scale != 1.0) {
            if (not multiLine and offset != 0) {
                Write(out, 0, "; ");
            }
            Write(out, multiLine ? indent : 0, "scale = %s%s",
                  TfStringify(scale).c_str(), multiLine ? "\n" : "");
        }
        if (not multiLine) {
            Write(out, 0, ")");
        }
    }
}

string
Sdf_FileIOUtility::Quote(const string &str)
{
    static const char* hexdigit = "0123456789abcedf";
    static const bool allowTripleQuotes = true;

    string result;

    // Choose quotes, double quote preferred.
    char quote = '"';
    if (str.find('"') != string::npos and str.find('\'') == string::npos) {
        quote = '\'';
    }

    // Open quote.  Choose single or triple quotes.
    bool tripleQuotes = false;
    if (allowTripleQuotes) {
        if (str.find('\n') != string::npos) {
            tripleQuotes = true;
            result += quote;
            result += quote;
        }
    }
    result += quote;

    // Escape string.
    TF_FOR_ALL(i, str) {
        switch (*i) {
        case '\n':
            // Pass newline as-is if using triple quotes, otherwise escape.
            if (tripleQuotes) {
                result += *i;
            }
            else {
                result += "\\n";
            }
            break;

        case '\r':
            result += "\\r";
            break;

        case '\t':
            result += "\\t";
            break;

        case '\\':
            result += "\\\\";
            break;

        default:
            if (*i == quote) {
                // Always escape the character we're using for quoting.
                result += '\\';
                result += quote;
            }
            else if (not std::isprint(*i)) {
                // Non-printable;  use two digit hex form.
                result += "\\x";
                result += hexdigit[(*i >> 4) & 15];
                result += hexdigit[*i & 15];
            }
            else {
                // Printable, non-special.
                result += *i;
            }
            break;
        }
    }

    // End quote.
    result += quote;
    if (tripleQuotes) {
        result += quote;
        result += quote;
    }

    return result;
}

string 
Sdf_FileIOUtility::Quote(const TfToken &token)
{
    return Quote(token.GetString());
}

string
Sdf_FileIOUtility::StringFromVtValue(const VtValue &value)
{
    string s;
    if (_StringFromVtStringValue<string>(&s, value) or
        _StringFromVtStringValue<TfToken>(&s, value)) {
        return s;
    }
    
    if (value.IsHolding<char>()) {
        return TfStringify(static_cast<int>(value.UncheckedGet<char>()));
    } else if (value.IsHolding<unsigned char>()) {
        return TfStringify(
            static_cast<unsigned int>(value.UncheckedGet<unsigned char>()));
    } else if (value.IsHolding<signed char>()) {
        return TfStringify(
            static_cast<int>(value.UncheckedGet<signed char>()));
    }

    return TfStringify(value);
}

const char* Sdf_FileIOUtility::Stringify( SdfPermission val )
{
    switch(val) {
    case SdfPermissionPublic:
        return "public";
    case SdfPermissionPrivate:
        return "private";
    default:
        TF_CODING_ERROR("unknown value");
        return "";
    }
}

const char* Sdf_FileIOUtility::Stringify( SdfSpecifier val )
{
    switch(val) {
    case SdfSpecifierDef:
        return "def";
    case SdfSpecifierOver:
        return "over";
    case SdfSpecifierClass:
        return "class";
    default:
        TF_CODING_ERROR("unknown value");
        return "";
    }
}

const char* Sdf_FileIOUtility::Stringify( SdfVariability val )
{
    switch(val) {
    case SdfVariabilityVarying:
        // Empty string implies SdfVariabilityVarying
        return "";
    case SdfVariabilityUniform:
        return "uniform";
    case SdfVariabilityConfig:
        return "config";
    default:
        TF_CODING_ERROR("unknown value");
        return "";
    }
}
