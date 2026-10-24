//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdProfiles/profilesDocUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <map>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

std::string
_MarkdownNoArgs()
{
    return UsdProfilesMarkdown();
}

std::string
_MarkdownWithStyles(const std::map<std::string, std::string>& styles)
{
    return UsdProfilesMarkdown(styles);
}

std::string
_Mermaid(const std::map<std::string, std::string>& styles)
{
    return UsdProfilesMermaid(styles);
}

std::string
_Dot()
{
    return UsdProfilesDot();
}

} // anonymous namespace

void wrapUsdProfilesDocUtils(nb::module_ &m)
{
    m.def("ProfilesMarkdown", &_MarkdownNoArgs);
    m.def("ProfilesMarkdown", &_MarkdownWithStyles, nb::arg("styles"));
    m.def("ProfilesMermaid",  &_Mermaid,            nb::arg("styles"));
    m.def("ProfilesDot",      &_Dot);
}
