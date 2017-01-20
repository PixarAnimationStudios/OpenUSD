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
#ifndef HDSTREAM_RENDER_DELEGATE_H
#define HDSTREAM_RENDER_DELEGATE_H

#include "pxr/imaging/hd/renderDelegate.h"

///
/// HdStreamRenderDelegate
///
/// The Stream Render Delegate provides a Hydra render that uses a
/// streaming graphics implementation (abstracted by the Gal) to draw the scene.
///
class HdStreamRenderDelegate final : public HdRenderDelegate {
public:
    HdStreamRenderDelegate();
    virtual ~HdStreamRenderDelegate() = default;

    virtual TfToken GetDefaultGalId() const override;

    virtual HdRprim *CreateRprim(TfToken const& typeId,
                                 SdfPath const& rprimId,
                                 SdfPath const& instancerId) override;
    virtual void DestroyRprim(HdRprim *rPrim) override;

    virtual HdSprim *CreateSprim(TfToken const& typeId,
                                 SdfPath const& sprimId) override;
    virtual void DestroySprim(HdSprim *sPrim) override;

    virtual HdBprim *CreateBprim(TfToken const& typeId,
                                 SdfPath const& bprimId) override;
    virtual void DestroyBprim(HdBprim *bPrim) override;
private:
    static void _ConfigureReprs();

    HdStreamRenderDelegate(const HdStreamRenderDelegate &)             = delete;
    HdStreamRenderDelegate &operator =(const HdStreamRenderDelegate &) = delete;
};

#endif // HDSTREAM_RENDER_DELEGATE_H
