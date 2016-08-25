//
// Copyright 2016 Pixar
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
#include "pxr/imaging/garch/glPlatformDebugContext.h"

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"

#include <boost/python/class.hpp>

using namespace boost::python;

static GarchGLPlatformDebugContextPtr
_New(int majorVersion, int minorVersion,
    bool coreProfile, bool directRendering)
{
    return TfCreateWeakPtr(new
        GarchGLPlatformDebugContext(majorVersion, minorVersion,
                                    coreProfile, directRendering));
}

void wrapPlatformDebugContext()
{    
    typedef GarchGLPlatformDebugContext This;

    class_<This, TfWeakPtr<This>,
           boost::noncopyable>("GLPlatformDebugContext", no_init)
        .def(TfMakePyConstructor(_New))
        .def("makeCurrent", &This::makeCurrent)
    ;
}
