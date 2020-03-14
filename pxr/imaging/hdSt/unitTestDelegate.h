//
// Copyright 2018 Pixar
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
#ifndef PXR_IMAGING_HD_ST_UNIT_TEST_DELEGATE_H
#define PXR_IMAGING_HD_ST_UNIT_TEST_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/unitTestDelegate.h"
#include "pxr/imaging/glf/texture.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdSt_UnitTestDelegate
///
/// A simple delegate class for unit test driver.
///
class HdSt_UnitTestDelegate : public HdUnitTestDelegate {
public:
    HDST_API
    HdSt_UnitTestDelegate(HdRenderIndex *parentIndex,
                          SdfPath const& delegateID);

    virtual ~HdSt_UnitTestDelegate();

    void AddTexture(SdfPath const& id, 
                    GlfTextureRefPtr const& texture);

    virtual HdTextureResourceSharedPtr
    GetTextureResource(SdfPath const& textureId) override;

private:
    struct _Texture {
        _Texture() {}
        _Texture(GlfTextureRefPtr const &tex)
            : texture(tex) {
        }
        GlfTextureRefPtr texture;
    };

    std::map<SdfPath, _Texture> _textures;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_UNIT_TEST_DELEGATE_H
