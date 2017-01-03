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
#include "pxr/imaging/hdStream/renderDelegate.h"
#include "pxr/imaging/hd/renderDelegateRegistry.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/points.h"

TF_REGISTRY_FUNCTION(TfType)
{
    HdRenderDelegateRegistry::Define<HdStreamRenderDelegate>();
}


TfToken
HdStreamRenderDelegate::GetDefaultGalId() const
{
    return TfToken();
}

HdRprim *
HdStreamRenderDelegate::CreateRprim(TfToken const& typeId,
                                    HdSceneDelegate* delegate,
                                    SdfPath const& rprimId,
                                    SdfPath const& instancerId)
{
    if (typeId == HdPrimTypeTokens->mesh) {
        return new HdMesh(delegate, rprimId, instancerId);
    } else if (typeId == HdPrimTypeTokens->basisCurves) {
        return new HdBasisCurves(delegate, rprimId, instancerId);
    } else  if (typeId == HdPrimTypeTokens->points) {
        return new HdPoints(delegate, rprimId, instancerId);
    } else {
        TF_CODING_ERROR("Unknown Rprim Type %s", typeId.GetText());
    }

    return nullptr;
}


void
HdStreamRenderDelegate::DestroyRprim(HdRprim *rPrim)
{
    delete rPrim;
}

