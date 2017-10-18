//
// Copyright 2017 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
