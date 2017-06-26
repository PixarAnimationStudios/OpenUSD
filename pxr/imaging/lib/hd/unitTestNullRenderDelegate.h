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
#ifndef HD_UNIT_TEST_NULL_RENDER_DELEGATE
#define HD_UNIT_TEST_NULL_RENDER_DELEGATE

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/instancer.h"

PXR_NAMESPACE_OPEN_SCOPE

class Hd_UnitTestNullRenderDelegate final : public HdRenderDelegate
{
public:
    Hd_UnitTestNullRenderDelegate() = default;
    virtual ~Hd_UnitTestNullRenderDelegate() = default;

    virtual const TfTokenVector &GetSupportedRprimTypes() const override;
    virtual const TfTokenVector &GetSupportedSprimTypes() const override;
    virtual const TfTokenVector &GetSupportedBprimTypes() const override;
    virtual HdRenderParam *GetRenderParam() const override;

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Renderpass factory
    ///
    ////////////////////////////////////////////////////////////////////////////

    virtual HdRenderPassSharedPtr CreateRenderPass(HdRenderIndex *index,
                HdRprimCollection const& collection) override;

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Instancer Factory
    ///
    ////////////////////////////////////////////////////////////////////////////

    virtual HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                         SdfPath const& id,
                                         SdfPath const& instancerId) override {
        return nullptr;
    }

    virtual void DestroyInstancer(HdInstancer *instancer) override {
        delete instancer;
    }

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Prim Factories
    ///
    ////////////////////////////////////////////////////////////////////////////

    virtual HdRprim *CreateRprim(TfToken const& typeId,
                                 SdfPath const& rprimId,
                                 SdfPath const& instancerId) override;

    virtual void DestroyRprim(HdRprim *rPrim) override;

    virtual HdSprim *CreateSprim(TfToken const& typeId,
                                 SdfPath const& sprimId) override;

    virtual HdSprim *CreateFallbackSprim(TfToken const& typeId) override;
    virtual void DestroySprim(HdSprim *sprim) override;

    virtual HdBprim *CreateBprim(TfToken const& typeId,
                                 SdfPath const& bprimId) override;

    virtual HdBprim *CreateFallbackBprim(TfToken const& typeId) override;

    virtual void DestroyBprim(HdBprim *bprim) override;

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Sync, Execute & Dispatch Hooks
    ///
    ////////////////////////////////////////////////////////////////////////////

    virtual void CommitResources(HdChangeTracker *tracker) override;



private:
    static const TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const TfTokenVector SUPPORTED_BPRIM_TYPES;

    Hd_UnitTestNullRenderDelegate(
                                const Hd_UnitTestNullRenderDelegate &) = delete;
    Hd_UnitTestNullRenderDelegate &operator =(
                                const Hd_UnitTestNullRenderDelegate &) = delete;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_UNIT_TEST_NULL_RENDER_DELEGATE
