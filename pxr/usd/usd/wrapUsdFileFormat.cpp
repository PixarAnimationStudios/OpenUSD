//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/sdf/usdFileFormat.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void
wrapUsdFileFormat()
{
    scope().attr("UsdFileFormat") =
        TfPyGetClassObject<SdfUsdFileFormat>();
    scope().attr("UsdFileFormat").attr("Tokens") =
        TfPyGetClassObject<SdfUsdFileFormatTokens_StaticTokenType>();
}
