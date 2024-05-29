//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include <boost/python/def.hpp>
#include <boost/python/tuple.hpp>

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;

static int 
_GetMajorVersion() {
    return PXR_MAJOR_VERSION;
}

static int 
_GetMinorVersion() {
    return PXR_MINOR_VERSION;
}

static int 
_GetPatchVersion() {
    return PXR_PATCH_VERSION;
}

static boost::python::tuple
_GetVersion() {
    return make_tuple(PXR_MAJOR_VERSION, 
                      PXR_MINOR_VERSION,
                      PXR_PATCH_VERSION);
}

// Create a doc string corresponding to the particular
// section of the version description (i.e. major/minor/patch/complete)
// and the type returned.
static std::string 
_MakeVersionFuncDocstring(const std::string& section,
                          const std::string& type)
{
    return "Get the " + section + " version number for this build of USD.\n" +
           "Returns a value of type " + type + ".\n" +
           "USD versions are described as (major,minor,patch)\n";
}

void wrapVersion()
{
    def("GetMajorVersion", _GetMajorVersion,
        _MakeVersionFuncDocstring("major", "int").c_str()); 
    def("GetMinorVersion", _GetMinorVersion,
        _MakeVersionFuncDocstring("minor", "int").c_str());
    def("GetPatchVersion", _GetPatchVersion,
        _MakeVersionFuncDocstring("patch", "int").c_str());
    def("GetVersion", _GetVersion,
        _MakeVersionFuncDocstring("complete", "tuple(int,int,int)").c_str());
}
