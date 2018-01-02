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
#ifndef HDST_DRAW_ITEM_H
#define HDST_DRAW_ITEM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/drawItem.h"

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class HdSt_GeometricShader> HdSt_GeometricShaderSharedPtr;
typedef boost::shared_ptr<class HdShaderCode> HdShaderCodeSharedPtr;

class HdStDrawItem : public HdDrawItem {
public:
    HF_MALLOC_TAG_NEW("new HdStDrawItem");

    HDST_API
    HdStDrawItem(HdRprimSharedData const *sharedData);

    HDST_API
    ~HdStDrawItem();

    void SetGeometricShader(HdSt_GeometricShaderSharedPtr const &shader) {
        _geometricShader = shader;
    }

    HdSt_GeometricShaderSharedPtr const &GetGeometricShader() const {
        return _geometricShader;
    }

private:
    HdSt_GeometricShaderSharedPtr _geometricShader;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDST_DRAW_ITEM_H
