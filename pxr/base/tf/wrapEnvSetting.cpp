//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include <locale>

#include <boost/python/def.hpp>
#include <boost/python/object.hpp>

#include <string>
#include <variant>

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

extern std::variant<int, bool, std::string> const *
Tf_GetEnvSettingByName(std::string const&);

static boost::python::object
_GetEnvSettingByName(std::string const& name) {
    std::variant<int, bool, std::string> const *
        variantValue = Tf_GetEnvSettingByName(name);

    if (!variantValue) {
        return boost::python::object();
    } 

    if (std::string const *value = std::get_if<std::string>(variantValue)) {
        return boost::python::object(*value);
    } else if (bool const *value = std::get_if<bool>(variantValue)) {
        return boost::python::object(*value);
    } else if (int const *value = std::get_if<int>(variantValue)) {
        return boost::python::object(*value); 
    } 
            
    return boost::python::object();
}

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

void wrapEnvSetting() {
    def("GetEnvSetting", &_GetEnvSettingByName);
}
