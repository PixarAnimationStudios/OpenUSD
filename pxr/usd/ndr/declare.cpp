//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/ndr/debugCodes.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<NdrVersionFilter>();
}

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(NdrVersionFilterDefaultOnly, "DefaultOnly");
    TF_ADD_ENUM_NAME(NdrVersionFilterAllVersions, "AllVersions");
}

namespace {

NdrVersion
_ParseVersionString(const std::string& x)
{
    try {
        std::size_t i;
        auto major = std::stoi(x, &i);
        if (i == x.size()) {
            return NdrVersion(major);
        }
        if (i < x.size() && x[i] == '.') {
            std::size_t j;
            auto minor = std::stoi(x.substr(i + 1), &j);
            if (i + j + 1 == x.size()) {
                return NdrVersion(major, minor);
            }
        }
    }
    catch (std::invalid_argument&) {
    }
    catch (std::out_of_range&) {
    }
    auto result = NdrVersion();
    TF_CODING_ERROR("Invalid version string '%s'", x.c_str());
    return result;
}

} // anonymous namespace

NdrVersion::NdrVersion(int major, int minor)
    : _major(major), _minor(minor)
{
    if (_major < 0 || _minor < 0 || (_major == 0 && _minor == 0)) {
        *this = NdrVersion();
        TF_CODING_ERROR("Invalid version %d.%d: both components must be " 
                        "non-negative and at least one non-zero",
                        major,  minor);
    }
}

NdrVersion::NdrVersion(const std::string& x)
    : NdrVersion(_ParseVersionString(x))
{
    // Do nothing
}

std::string
NdrVersion::GetString() const
{
    if (!*this) {
        return "<invalid version>";
    }
    else if (_minor) {
        return std::to_string(_major) + "." + std::to_string(_minor);
    }
    else {
        return std::to_string(_major);
    }
}

std::string
NdrVersion::GetStringSuffix() const
{
    if (IsDefault()) {
        return "";
    }
    else if (!*this) {
        // XXX -- It's not clear what to do here.  For now we return the
        //        same result as for a default version.
        return "";
    }
    else if (_minor) {
        return '_' + std::to_string(_major) + "." + std::to_string(_minor);
    }
    else {
        return '_' + std::to_string(_major);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
