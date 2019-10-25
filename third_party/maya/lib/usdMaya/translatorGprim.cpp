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
#include "usdMaya/translatorGprim.h"

#include "usdMaya/util.h"

#include <maya/MFnDagNode.h>

PXR_NAMESPACE_OPEN_SCOPE


void
UsdMayaTranslatorGprim::Read(
        const UsdGeomGprim& gprim,
        MObject mayaNode,
        UsdMayaPrimReaderContext* )
{
    MFnDagNode fnGprim(mayaNode);

    TfToken orientation;
    if (gprim.GetOrientationAttr().Get(&orientation)){
        UsdMayaUtil::setPlugValue(fnGprim, "opposite", (orientation ==
                                                 UsdGeomTokens->leftHanded));
    }

    bool doubleSided;
    if (gprim.GetDoubleSidedAttr().Get(&doubleSided)){
        UsdMayaUtil::setPlugValue(fnGprim, "doubleSided", doubleSided);
    }
}

void
UsdMayaTranslatorGprim::Write(
        const MObject& mayaNode,
        const UsdGeomGprim& gprim, 
        UsdMayaPrimWriterContext*)
{
    MFnDependencyNode depFn(mayaNode);

    bool doubleSided = false;
    if (UsdMayaUtil::getPlugValue(depFn, "doubleSided", &doubleSided)){
        gprim.CreateDoubleSidedAttr(VtValue(doubleSided), true);
    }

    bool opposite = false;
    // Gprim properties always authored on the shape
    if (UsdMayaUtil::getPlugValue(depFn, "opposite", &opposite)){
        // If mesh is double sided in maya, opposite is disregarded
        TfToken orientation = (opposite && !doubleSided ? UsdGeomTokens->leftHanded :
                                                          UsdGeomTokens->rightHanded);
        gprim.CreateOrientationAttr(VtValue(orientation), true);
    }

}


PXR_NAMESPACE_CLOSE_SCOPE

