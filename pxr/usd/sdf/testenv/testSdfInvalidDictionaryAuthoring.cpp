//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

static VtDictionary
_GetInvalidDictionary()
{
    return {
        {"invalid", VtValue()},
        {"valid", VtValue(std::string("value"))}
    };
}

static void
_VerifyDictionaryConversionRejectsEmptyValues()
{
    VtDictionary dictionary = _GetInvalidDictionary();
    std::string errorMessage;
    TF_AXIOM(
        !SdfConvertToValidMetadataDictionary(&dictionary, &errorMessage));
    TF_AXIOM(TfStringContains(
        errorMessage,
        "empty value under key 'invalid' is not a valid dictionary entry"));
}

static void
_VerifyAuthoringRejectsEmptyValues()
{
    const VtDictionary invalidDictionary = _GetInvalidDictionary();

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    TfErrorMark mark;
    layer->SetCustomLayerData(invalidDictionary);
    TF_AXIOM(!mark.IsClean());
    mark.Clear();
    TF_AXIOM(layer->GetCustomLayerData().empty());
}

static void
_VerifyWriterSkipsEmptyValues()
{
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");

    // Use the raw field API to bypass the validated dictionary setters and
    // verify that the writer still cannot emit a typeless dictionary entry.
    layer->SetField(
        SdfPath::AbsoluteRootPath(),
        SdfFieldKeys->CustomLayerData,
        VtValue(_GetInvalidDictionary()));

    std::string layerText;
    TfErrorMark mark;
    TF_AXIOM(layer->ExportToString(&layerText));
    TF_AXIOM(!mark.IsClean());
    mark.Clear();

    TF_AXIOM(!TfStringContains(layerText, "invalid ="));
    TF_AXIOM(TfStringContains(layerText, "string valid = \"value\""));
}

int
main()
{
    _VerifyDictionaryConversionRejectsEmptyValues();
    _VerifyAuthoringRejectsEmptyValues();
    _VerifyWriterSkipsEmptyValues();
}
