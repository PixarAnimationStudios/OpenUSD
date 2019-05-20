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

#include "usdMaya/meshUtil.h"
#include "usdMaya/util.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"

#include <maya/MFnMesh.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>

#include <boost/python/class.hpp>
#include <boost/python.hpp>

#include <string>


using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE


namespace {

static
tuple
_GetMeshNormals(const std::string& meshDagPath)
{
    VtArray<GfVec3f> normalsArray;
    TfToken interpolation;

    MObject meshObj;
    MStatus status = UsdMayaUtil::GetMObjectByName(meshDagPath, meshObj);
    if (status != MS::kSuccess) {
        TF_CODING_ERROR("Could not get MObject for dagPath: %s",
                        meshDagPath.c_str());
        return make_tuple(normalsArray, interpolation);
    }

    MFnMesh meshFn(meshObj, &status);
    if (status != MS::kSuccess) {
        TF_CODING_ERROR("MFnMesh() failed for object at dagPath: %s",
                        meshDagPath.c_str());
        return make_tuple(normalsArray, interpolation);
    }

    UsdMayaMeshUtil::GetMeshNormals(meshFn, &normalsArray, &interpolation);

    return make_tuple(normalsArray, interpolation);
}

// Dummy class for putting UsdMayaMeshUtil namespace functions in a Python
// MeshUtil namespace.
class DummyScopeClass{};

} // anonymous namespace 


void wrapMeshUtil()
{
    scope s = class_<DummyScopeClass>("MeshUtil", no_init)

        .def("GetMeshNormals", &_GetMeshNormals)
            .staticmethod("GetMeshNormals")

        ;
}
