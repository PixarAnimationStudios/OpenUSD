//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/debugCodes.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdrVersionFilter>();
}

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(SdrVersionFilterDefaultOnly, "DefaultOnly");
    TF_ADD_ENUM_NAME(SdrVersionFilterAllVersions, "AllVersions");
}

namespace {

SdrVersion
_ParseVersionString(const std::string& x)
{
    try {
        std::size_t i;
        auto major = std::stoi(x, &i);
        if (i == x.size()) {
            return SdrVersion(major);
        }
        if (i < x.size() && x[i] == '.') {
            std::size_t j;
            auto minor = std::stoi(x.substr(i + 1), &j);
            if (i + j + 1 == x.size()) {
                return SdrVersion(major, minor);
            }
        }
    }
    catch (std::invalid_argument&) {
    }
    catch (std::out_of_range&) {
    }
    auto result = SdrVersion();
    TF_CODING_ERROR("Invalid version string '%s'", x.c_str());
    return result;
}

} // anonymous namespace

SdrVersion::SdrVersion(int major, int minor)
    : _major(major), _minor(minor)
{
    if (_major < 0 || _minor < 0 || (_major == 0 && _minor == 0)) {
        *this = SdrVersion();
        TF_CODING_ERROR("Invalid version %d.%d: both components must be " 
                        "non-negative and at least one non-zero",
                        major,  minor);
    }
}

SdrVersion::SdrVersion(const std::string& x)
    : SdrVersion(_ParseVersionString(x))
{
    // Do nothing
}

std::string
SdrVersion::GetString() const
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
SdrVersion::GetStringSuffix() const
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

NdrVersionFilter SdrVersionFilterToNdr(SdrVersionFilter filter) {
    if (filter == SdrVersionFilterDefaultOnly) {
        return NdrVersionFilterDefaultOnly;
    } else if (filter == SdrVersionFilterAllVersions) {
        return NdrVersionFilterAllVersions;
    } else if (filter == SdrNumVersionFilters) {
        return NdrNumVersionFilters;
    } else {
        TF_CODING_ERROR("Invalid enumerated SdrVersionFilter value %s",
                        TfEnum::GetName(filter).c_str());
        return NdrNumVersionFilters;
    }
}

NdrVersion SdrToNdrVersion(SdrVersion version) {
    // If the SdrVersion is invalid, explicitly construct
    // a default NdrVersion so as to not invoke a CodingError
    // with the major-minor or version string constructors.
    NdrVersion dest;
    if (version) {
        dest = NdrVersion(version.GetMajor(), version.GetMinor());
    }

    if (version.IsDefault()) {
        return dest.GetAsDefault();
    } else {
        return dest;
    }
}

SdrVersion NdrToSdrVersion(NdrVersion version) {
    // If the NdrVersion is invalid, explicitly construct
    // a default SdrVersion so as to not invoke a CodingError
    // with the major-minor or version string constructors.
    SdrVersion dest;
    if (version) {
        dest = SdrVersion(version.GetMajor(), version.GetMinor());
    }

    if (version.IsDefault()) {
        return dest.GetAsDefault();
    } else {
        return dest;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
