//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include <locale>

#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/object.hpp"

#include <string>
#include <variant>

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

extern std::variant<int, bool, std::string> const *
Tf_GetEnvSettingByName(std::string const&);

static pxr_boost::python::object
_GetEnvSettingByName(std::string const& name) {
    std::variant<int, bool, std::string> const *
        variantValue = Tf_GetEnvSettingByName(name);

    if (!variantValue) {
        return pxr_boost::python::object();
    } 

    if (std::string const *value = std::get_if<std::string>(variantValue)) {
        return pxr_boost::python::object(*value);
    } else if (bool const *value = std::get_if<bool>(variantValue)) {
        return pxr_boost::python::object(*value);
    } else if (int const *value = std::get_if<int>(variantValue)) {
        return pxr_boost::python::object(*value); 
    } 
            
    return pxr_boost::python::object();
}

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

void wrapEnvSetting() {
    def("GetEnvSetting", &_GetEnvSettingByName);
}
