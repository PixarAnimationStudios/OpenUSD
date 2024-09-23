//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/textParserContext.h"

PXR_NAMESPACE_OPEN_SCOPE

Sdf_TextParserContext::Sdf_TextParserContext() :
    parsingContext(std::vector<Sdf_TextParserCurrentParsingContext>()),
    listOpType(SdfListOpTypeExplicit),
    currentDictionaries(std::vector<VtDictionary>(1)),
    path(SdfPath::AbsoluteRootPath()),
    // This parser supports the maybe-has-relocates hint.  The parser will set
    // it to true if it encounters a relocates field.
    layerHints{/*.mightHaveRelocates =*/ false}
{
}

PXR_NAMESPACE_CLOSE_SCOPE
