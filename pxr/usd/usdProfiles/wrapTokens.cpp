//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/usdProfiles/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

#define _ADD_TOKEN(cls, name) \
    cls.def_prop_ro_static(#name, +[](nb::object) { return UsdProfilesTokens->name.GetString(); });

void wrapUsdProfilesTokens(nb::module_ &m)
{
    nb::class_<UsdProfilesTokensType> cls(m, "Tokens");
    _ADD_TOKEN(cls, enhancement);
    _ADD_TOKEN(cls, hard);
    _ADD_TOKEN(cls, soft);
    _ADD_TOKEN(cls, ClaimsAPI);
}
