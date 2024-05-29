//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hio/dictionary.h"

#include "pxr/base/js/converter.h"
#include "pxr/base/js/json.h"
#include "pxr/base/tf/debug.h"

PXR_NAMESPACE_OPEN_SCOPE


using namespace std;


TF_DEBUG_CODES(

    HIO_DEBUG_DICTIONARY
    
);


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HIO_DEBUG_DICTIONARY,
        "glslfx dictionary parsing");
}

static VtDictionary
_Hio_GetDictionaryFromJSON(
    const string &input,
    string *errorStr )
{
    if (input.empty())
    {
        const char *errorMsg = "Cannot create VtDictionary from empty string";
        TF_DEBUG(HIO_DEBUG_DICTIONARY).Msg("%s", errorMsg);

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
        if (errorStr || TfDebug::IsEnabled(HIO_DEBUG_DICTIONARY)) {
            std::string errorMessageStr = TfStringPrintf(
                "Failed to extract dictionary from input (line %d, col %d): %s",
                error.line, error.column, error.reason.c_str());
            if (errorStr) {
                *errorStr = errorMessageStr;
            }
            TF_DEBUG(HIO_DEBUG_DICTIONARY).Msg("%s", errorMessageStr.c_str());
        }
        return VtDictionary();
    }

    if (!jsdict.IsObject()) {
        if (errorStr || TfDebug::IsEnabled(HIO_DEBUG_DICTIONARY)) {
            std::string errorMessageStr = TfStringPrintf(
                "Input string did not evaluate to a JSON dictionary:\n%s\n",
                input.c_str());
            if (errorStr) {
                *errorStr = errorMessageStr;
            }
            TF_DEBUG(HIO_DEBUG_DICTIONARY).Msg("%s", errorMessageStr.c_str());
        }
        return VtDictionary();
    }

    const VtValue vtdict = 
        JsValueTypeConverter<VtValue, VtDictionary, /*UseInt64*/false>::Convert(jsdict);
    return vtdict.IsHolding<VtDictionary>() ?
        vtdict.UncheckedGet<VtDictionary>() : VtDictionary();
}


VtDictionary
Hio_GetDictionaryFromInput(
    const string &input,
    const string &filename,
    string *errorStr )
{
    std::string jsError;
    VtDictionary ret = _Hio_GetDictionaryFromJSON(input, &jsError);

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

PXR_NAMESPACE_CLOSE_SCOPE

