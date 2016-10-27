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
#include "pxr/base/js/converter.h"
#include "pxr/base/js/json.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/vt/dictionary.h"

using namespace std;


TF_DEBUG_CODES(

    GLF_DEBUG_DICTIONARY
    
);


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(GLF_DEBUG_DICTIONARY,
        "glslfx dictionary parsing");
}

static VtDictionary
_Glf_GetDictionaryFromJSON(
    const string &input,
    string *errorStr )
{
    if (input.empty())
    {
        const char *errorMsg = "Cannot create VtDictionary from empty string";
        TF_DEBUG(GLF_DEBUG_DICTIONARY).Msg(errorMsg);

        if( errorStr ) {
            *errorStr = errorMsg;
        }
        return VtDictionary();
    }

    // Note Js library doesn't allow comments.  Strip comments as we do
    // for plugInfo files.
    //
    // XXX: This may not be worth the cost.
    //
    std::vector<std::string> filtered = TfStringSplit(input, "\n");
    for (auto & line : filtered) {
        // Clear comment lines but keep them to maintain line numbers for errors
        if (line.find('#') < line.find_first_not_of(" \t#"))
            line.clear();
    }
    
    JsParseError error;
    JsValue jsdict = JsParseString(TfStringJoin(filtered, "\n"), &error);

    if (jsdict.IsNull()) {
        if (errorStr or TfDebug::IsEnabled(GLF_DEBUG_DICTIONARY)) {
            std::string errorMessageStr = TfStringPrintf(
                "Failed to extract dictionary from input (line %d, col %d): %s",
                error.line, error.column, error.reason.c_str());
            if (errorStr) {
                *errorStr = errorMessageStr;
            }
            TF_DEBUG(GLF_DEBUG_DICTIONARY).Msg(errorMessageStr.c_str());
        }
        return VtDictionary();
    }

    if (not jsdict.IsObject()) {
        if (errorStr or TfDebug::IsEnabled(GLF_DEBUG_DICTIONARY)) {
            std::string errorMessageStr = TfStringPrintf(
                "Input string did not evaluate to a JSON dictionary:\n%s\n",
                input.c_str());
            if (errorStr) {
                *errorStr = errorMessageStr;
            }
            TF_DEBUG(GLF_DEBUG_DICTIONARY).Msg(errorMessageStr.c_str());
        }
        return VtDictionary();
    }

    const VtValue vtdict = 
        JsValueTypeConverter<VtValue, VtDictionary, /*UseInt64*/false>::Convert(jsdict);
    return vtdict.IsHolding<VtDictionary>() ?
        vtdict.UncheckedGet<VtDictionary>() : VtDictionary();
}


VtDictionary
Glf_GetDictionaryFromInput(
    const string &input,
    const string &filename,
    string *errorStr )
{
    std::string jsError;
    VtDictionary ret = _Glf_GetDictionaryFromJSON(input, &jsError);

    if (jsError.empty()) {
        // JSON succeeded, great, we're done.
        return ret;
    }

    // If the file has errors, report the errors from JSON as that is the new
    // format that we're expected to conform to.
    if (errorStr) {
        *errorStr = jsError;
    }
    return VtDictionary();
}
