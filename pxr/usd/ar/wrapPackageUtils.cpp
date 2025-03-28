//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/ar/packageUtils.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/return_value_policy.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void
wrapPackageUtils()
{
    def("IsPackageRelativePath", 
        &ArIsPackageRelativePath, arg("path"));

    def("JoinPackageRelativePath", 
        (std::string(*)(const std::vector<std::string>&))
            &ArJoinPackageRelativePath, 
        arg("paths"));

    def("JoinPackageRelativePath", 
        (std::string(*)(const std::pair<std::string, std::string>&))
            &ArJoinPackageRelativePath, 
        arg("paths"));

    def("JoinPackageRelativePath", 
        (std::string(*)(const std::string&, const std::string&))
            &ArJoinPackageRelativePath, 
        (arg("packagePath"), arg("packagedPath")));

    def("SplitPackageRelativePathOuter", 
        &ArSplitPackageRelativePathOuter, arg("path"));

    def("SplitPackageRelativePathInner", 
        &ArSplitPackageRelativePathInner, arg("path"));
}
