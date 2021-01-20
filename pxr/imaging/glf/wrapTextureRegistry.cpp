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
#include "pxr/imaging/glf/textureRegistry.h"

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyEnum.h"

#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static void _RequiresGarbageCollection()
{
    GlfTextureRegistry::GetInstance().RequiresGarbageCollection();
}
static void _GarbageCollectIfNeeded()
{
    GlfTextureRegistry::GetInstance().GarbageCollectIfNeeded();
}

static std::vector<VtDictionary> _GetTextureInfos()
{
    return GlfTextureRegistry::GetInstance().GetTextureInfos();
}

static void _Reset()
{
    GlfTextureRegistry::GetInstance().Reset();
}

} // anonymous namespace 

void wrapTextureRegistry()
{    
    typedef GlfTextureRegistry This;

    class_<This, boost::noncopyable>("TextureRegistry", no_init)

        .def("RequiresGarbageCollection", _RequiresGarbageCollection)
        .staticmethod("RequiresGarbageCollection")

        .def("GarbageCollectIfNeeded", _GarbageCollectIfNeeded)
        .staticmethod("GarbageCollectIfNeeded")

        .def("GetTextureInfo",
             make_function(_GetTextureInfos,
                           return_value_policy<TfPySequenceToList>()))
        .staticmethod("GetTextureInfo")

        .def("Reset", _Reset)
        .staticmethod("Reset")
        ;
}
