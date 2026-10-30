//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include <locale>


#include <string>
#include <variant>

#include "pxr/base/tf/envSetting.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/string.h>

PXR_NAMESPACE_OPEN_SCOPE

extern std::variant<int, bool, std::string> const *
Tf_GetEnvSettingByName(std::string const&);

static nanobind::object
_GetEnvSettingByName(std::string const& name) {
    std::variant<int, bool, std::string> const *
        variantValue = Tf_GetEnvSettingByName(name);

    if (!variantValue) {
        return nanobind::none();
    } 

    if (std::string const *value = std::get_if<std::string>(variantValue)) {
        return nanobind::cast(*value);
    } else if (bool const *value = std::get_if<bool>(variantValue)) {
        return nanobind::cast(*value);
    } else if (int const *value = std::get_if<int>(variantValue)) {
        return nanobind::cast(*value); 
    } 
            
    return nanobind::none();
}

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

void wrapEnvSetting(nanobind::module_& m) {
    m.def("GetEnvSetting", &_GetEnvSettingByName);
}
