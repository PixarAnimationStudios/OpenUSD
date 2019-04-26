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

#include "pxr/pxr.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/fileIO_Common.h"

#include <cctype>

using std::map;
using std::ostream;
using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

static const char *_IndentString = "    ";

// Helper for creating string representation of an asset path
static string
_StringFromAssetPath(const string& assetPath)
{
    // See Sdf_EvalAssetPath for the code that reads asset paths at parse time.

    // We want to avoid writing asset paths with escape sequences in them
    // so that it's easy for users to copy and paste these paths into other
    // apps without having to clean up those escape sequences.
    //
    // We use "@"s as delimiters so that asset paths are easily identifiable.
    // but use "@@@" if the path already has an "@" in it rather than escaping
    // it. If the path has a "@@@", then we'll escape that, but hopefully that's
    // a rarer case. We'll also strip out non-printable characters. so we don't
    // have to escape those.
    static const string singleDelim = "@";
    static const string tripleDelim = "@@@";
    const string* delim = (assetPath.find('@') == std::string::npos) ? 
        &singleDelim : &tripleDelim;

    string s = assetPath;
    s.erase(std::remove_if(s.begin(), s.end(), 
                           [](char s) { return !std::isprint(s); }),
            s.end());

    if (delim == &tripleDelim) {
        s = TfStringReplace(s, tripleDelim, "\\@@@");
    }

    return *delim + s + *delim;
}

static string
_StringFromValue(const string& s)
{
    return Sdf_FileIOUtility::Quote(s);
}

static string
_StringFromValue(const TfToken& s)
{
    return Sdf_FileIOUtility::Quote(s);
}

static string
_StringFromValue(const SdfAssetPath& assetPath)
{
    return _StringFromAssetPath(assetPath.GetAssetPath());
}

template <class T>
static void
_StringFromVtArray(
    string *valueStr,
    const VtArray<T> &valArray)
{
    valueStr->append("[");
    if (typename VtArray<T>::const_pointer d = valArray.cdata()) {
        if (const size_t n = valArray.size()) {
            valueStr->append(_StringFromValue(d[0]));
            for (size_t i = 1; i != n; ++i) {
                valueStr->append(", ");
                valueStr->append(_StringFromValue(d[i]));
            }
        }
    }
    valueStr->append("]");
}

// Helper for creating strings for VtValues holding certain types
// that can't use TfStringify, and arrays of those types.
template <class T>
static bool
_StringFromVtValueHelper(string* valueStr, const VtValue& value)
{
    if (value.IsHolding<T>()) {
        *valueStr = _StringFromValue(value.UncheckedGet<T>());
        return true;
    }
    else if (value.IsHolding<VtArray<T> >()) {
        const VtArray<T>& valArray = value.UncheckedGet<VtArray<T> >();
        _StringFromVtArray(valueStr,valArray);
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
        return !ref.GetCustomData().empty();
    }
    static void Write(ostream& out, size_t indent, const SdfReference& ref)
    {
        bool multiLineRefMetaData = !ref.GetCustomData().empty();
    
        Sdf_FileIOUtility::Write(out, indent, "");

        if (!ref.GetAssetPath().empty()) {
            Sdf_FileIOUtility::WriteAssetPath(out, 0, ref.GetAssetPath());
            if (!ref.GetPrimPath().IsEmpty())
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
        if (!ref.GetCustomData().empty()) {
            Sdf_FileIOUtility::Puts(out, indent+1, "customData = ");
            Sdf_FileIOUtility::WriteDictionary(
                out, indent+1, /* multiline = */ true, ref.GetCustomData());
        }
        if (multiLineRefMetaData) {
            Sdf_FileIOUtility::Puts(out, indent, ")");
        }
    }
};

template <>
struct _ListOpWriter<SdfPayload>
{
    static constexpr bool ItemPerLine = true;
    static bool SingleItemRequiresBrackets(const SdfPayload& payload)
    {
        return false;
    }
    static void Write(ostream& out, size_t indent, const SdfPayload& payload)
    {
        Sdf_FileIOUtility::Write(out, indent, "");

        if (!payload.GetAssetPath().empty()) {
            Sdf_FileIOUtility::WriteAssetPath(out, 0, payload.GetAssetPath());
            if (!payload.GetPrimPath().IsEmpty())
                Sdf_FileIOUtility::WriteSdfPath(out, 0, payload.GetPrimPath());
        }
        else {
            // If this is an internal payload, we always have to write
            // out a path, even if it's empty since that encodes a payload
            // to the default prim.
            Sdf_FileIOUtility::WriteSdfPath(out, 0, payload.GetPrimPath());
        }

        Sdf_FileIOUtility::WriteLayerOffset(
            out, indent+1, false /*multiLineMetaData*/, payload.GetLayerOffset());
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
    else if (listOpList.size() == 1 &&
             !_Writer::SingleItemRequiresBrackets(listOpList.front())) {
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
        if (!listOp.GetDeletedItems().empty()) {
            _WriteListOpList(out, indent, name, 
                             listOp.GetDeletedItems(), "delete");
        }
        if (!listOp.GetAddedItems().empty()) {
            _WriteListOpList(out, indent, name, 
                             listOp.GetAddedItems(), "add"); 
        }
        if (!listOp.GetPrependedItems().empty()) {
            _WriteListOpList(out, indent, name, 
                             listOp.GetPrependedItems(), "prepend"); 
        }
        if (!listOp.GetAppendedItems().empty()) {
            _WriteListOpList(out, indent, name, 
                             listOp.GetAppendedItems(), "append"); 
        }
        if (!listOp.GetOrderedItems().empty()) {
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
Sdf_FileIOUtility::WriteAssetPath(
    ostream &out, size_t indent, const string &assetPath) 
{
    Puts(out, indent, _StringFromAssetPath(assetPath));
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
            size_t indent, const SdfPath &path) 
{
    Write(out, indent, "<%s>", path.GetString().c_str());
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
                                    const SdfPropertySpec &prop)
{
    VtValue timeSamplesVal = prop.GetField(SdfFieldKeys->TimeSamples);
    if (timeSamplesVal.IsHolding<SdfTimeSampleMap>()) {
        SdfTimeSampleMap samples =
            timeSamplesVal.UncheckedGet<SdfTimeSampleMap>();
        TF_FOR_ALL(i, samples) {
            Write(out, indent+1, "%s: ", TfStringify(i->first).c_str());
            if (i->second.IsHolding<SdfPath>()) {
                WriteSdfPath(out, 0, i->second.Get<SdfPath>() );
            } else {
                Puts(out, 0, StringFromVtValue( i->second ));
            }
            out << ",\n";
        }
    }
    else if (timeSamplesVal.IsHolding<SdfHumanReadableValue>()) {
        Write(out, indent+1, "%s\n",
              TfStringify(timeSamplesVal.
                          UncheckedGet<SdfHumanReadableValue>()).c_str());
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
            if (!TfIsValidIdentifier(keyName)) {
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
                if (_StringFromVtValueHelper<string>(&str, value) || 
                    _StringFromVtValueHelper<TfToken>(&str, value) ||
                    _StringFromVtValueHelper<SdfAssetPath>(&str, value)) {
                    Puts(out, 0, str);
                } else {
                    Puts(out, 0, TfStringify(value));
                }
                if (multiLine) {
                    Puts(out, 0, "\n");
                }
            }
        }
        if (!multiLine && counter > 0) {
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
                               const SdfPayloadListOp&);
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
        if (!multiLine) {
            Write(out, 0, " (");
        }
        double offset = layerOffset.GetOffset();
        double scale = layerOffset.GetScale();
        if (offset != 0.0) {
            Write(out, multiLine ? indent : 0, "offset = %s%s",
                  TfStringify(offset).c_str(), multiLine ? "\n" : "");
        }
        if (scale != 1.0) {
            if (!multiLine && offset != 0) {
                Write(out, 0, "; ");
            }
            Write(out, multiLine ? indent : 0, "scale = %s%s",
                  TfStringify(scale).c_str(), multiLine ? "\n" : "");
        }
        if (!multiLine) {
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
    if (str.find('"') != string::npos && str.find('\'') == string::npos) {
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
            else if (!std::isprint(*i)) {
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
    if (_StringFromVtValueHelper<string>(&s, value) || 
        _StringFromVtValueHelper<TfToken>(&s, value) ||
        _StringFromVtValueHelper<SdfAssetPath>(&s, value)) {
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

PXR_NAMESPACE_CLOSE_SCOPE
