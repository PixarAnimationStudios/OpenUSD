//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/sdf/usdFileFormat.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyStaticTokens.h"

#include <nanobind/nanobind.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void
wrapUsdFileFormat(nanobind::module_& m)
{
    using This = SdfUsdFileFormat;

    nb::class_<This, SdfFileFormat> cls(
        m, "UsdFileFormat",
        nb::is_weak_referenceable(), nb::dynamic_attr());
    TfNbRefAndWeakPtr(cls);
    cls.def_static("GetUnderlyingFormatForLayer", 
                   &This::GetUnderlyingFormatForLayer)
        ;

    TF_NB_WRAP_PUBLIC_TOKENS(
        cls,
        "Tokens",
        SdfUsdFileFormatTokens,
        SDF_USD_FILE_FORMAT_TOKENS);
}
