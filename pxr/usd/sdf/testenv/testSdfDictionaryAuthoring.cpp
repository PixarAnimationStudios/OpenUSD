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

static VtDictionary
_GetInvalidNestedDictionary()
{
    const VtDictionary inner = {{"invalid", VtValue()}};
    VtDictionary outer;
    outer.SetValueAtPath("invalidNested", VtValue(inner));
    return outer;
}

static void
_VerifyWriterSkipsEmptyValues()
{
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->SetCustomLayerData(_GetInvalidDictionary());

    std::string layerText;
    TfErrorMark mark;
    TF_AXIOM(layer->ExportToString(&layerText));
    TF_AXIOM(mark.IsClean());
    mark.Clear();

    TF_AXIOM(!TfStringContains(layerText, "invalid ="));
    TF_AXIOM(TfStringContains(layerText, "string valid = \"value\""));

    // Check that a dictionary with a single invalid nested item is emitted
    // as an empty dictionary
    layer->SetField(
        SdfPath::AbsoluteRootPath(),
        SdfFieldKeys->CustomLayerData,
        VtValue(_GetInvalidNestedDictionary()));

    TF_AXIOM(layer->ExportToString(&layerText));
    TF_AXIOM(mark.IsClean());
    mark.Clear();

    TF_AXIOM(TfStringContains(layerText, "dictionary invalidNested ="));
    TF_AXIOM(!TfStringContains(layerText, "dictionary invalid ="));
}

int
main()
{
    _VerifyWriterSkipsEmptyValues();
}
