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
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/shader.h"
#include "pxr/imaging/hd/texture.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/unitTestNullRenderPass.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////
// Null Prims

class Hd_NullRprim final : public HdRprim {
public:
    Hd_NullRprim(SdfPath const& id, SdfPath const& instancerId)
     : HdRprim(id, instancerId)
    {

    }

    virtual ~Hd_NullRprim() = default;

    virtual void Sync(HdSceneDelegate* delegate,
                      HdRenderParam*   renderParam,
                      HdDirtyBits*     dirtyBits,
                      TfToken const&   reprName,
                      bool             forcedRepr) override
    {
        *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
    }


    virtual HdDirtyBits _GetInitialDirtyBits() const override
    {
        // Set all bits except the varying flag
        return  (HdChangeTracker::AllSceneDirtyBits) &
               (~HdChangeTracker::Varying);
    }

    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override
    {
        return bits;
    }
    virtual void _InitRepr(TfToken const &reprName,
                           HdDirtyBits *dirtyBits) override
    {
    }

protected:
    virtual HdReprSharedPtr const &
        _GetRepr(HdSceneDelegate *sceneDelegate,
                 TfToken const &reprName,
                 HdDirtyBits *dirtyBits) override  {
        static HdReprSharedPtr result = boost::make_shared<HdRepr>();
        return result;
    };

private:
    Hd_NullRprim()                                 = delete;
    Hd_NullRprim(const Hd_NullRprim &)             = delete;
    Hd_NullRprim &operator =(const Hd_NullRprim &) = delete;
};

class Hd_NullShader final : public HdShader {
public:
    Hd_NullShader(SdfPath const& id) : HdShader(id) {}
    virtual ~Hd_NullShader() = default;

    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override
    {
        *dirtyBits = HdShader::Clean;
    };

    virtual VtValue Get(TfToken const &token) const override {
        return VtValue();
    }

    virtual HdDirtyBits GetInitialDirtyBitsMask() const override {
        return HdShader::AllDirty;
    }

    virtual void Reload() override {};

    virtual HdShaderCodeSharedPtr GetShaderCode() const override {
        static HdShaderCodeSharedPtr result;
        return result;
    }

private:
    Hd_NullShader()                                  = delete;
    Hd_NullShader(const Hd_NullShader &)             = delete;
    Hd_NullShader &operator =(const Hd_NullShader &) = delete;
};

const TfTokenVector Hd_UnitTestNullRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
    HdPrimTypeTokens->basisCurves,
    HdPrimTypeTokens->points
};

const TfTokenVector Hd_UnitTestNullRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
    HdPrimTypeTokens->shader
};

const TfTokenVector Hd_UnitTestNullRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
    HdPrimTypeTokens->texture
};

const TfTokenVector &
Hd_UnitTestNullRenderDelegate::GetSupportedRprimTypes() const
{
    return SUPPORTED_RPRIM_TYPES;
}

const TfTokenVector &
Hd_UnitTestNullRenderDelegate::GetSupportedSprimTypes() const
{
    return SUPPORTED_SPRIM_TYPES;
}

const TfTokenVector &
Hd_UnitTestNullRenderDelegate::GetSupportedBprimTypes() const
{
    return SUPPORTED_BPRIM_TYPES;
}

HdRenderParam *
Hd_UnitTestNullRenderDelegate::GetRenderParam() const
{
    return nullptr;
}

HdRenderPassSharedPtr
Hd_UnitTestNullRenderDelegate::CreateRenderPass(HdRenderIndex *index,
                                HdRprimCollection const& collection)
{
    return HdRenderPassSharedPtr(
        new Hd_UnitTestNullRenderPass(index, collection));
}

HdRprim *
Hd_UnitTestNullRenderDelegate::CreateRprim(TfToken const& typeId,
                                    SdfPath const& rprimId,
                                    SdfPath const& instancerId)
{
    return new Hd_NullRprim(rprimId, instancerId);
}

void
Hd_UnitTestNullRenderDelegate::DestroyRprim(HdRprim *rPrim)
{
    delete rPrim;
}

HdSprim *
Hd_UnitTestNullRenderDelegate::CreateSprim(TfToken const& typeId,
                                    SdfPath const& sprimId)
{
    if (typeId == HdPrimTypeTokens->shader) {
        return new Hd_NullShader(sprimId);
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
}

HdSprim *
Hd_UnitTestNullRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
    if (typeId == HdPrimTypeTokens->shader) {
        return new Hd_NullShader(SdfPath::EmptyPath());
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
}


void
Hd_UnitTestNullRenderDelegate::DestroySprim(HdSprim *sPrim)
{
    delete sPrim;
}

HdBprim *
Hd_UnitTestNullRenderDelegate::CreateBprim(TfToken const& typeId,
                                    SdfPath const& bprimId)
{
    if (typeId == HdPrimTypeTokens->texture) {
        return new HdTexture(bprimId);
    } else  {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }


    return nullptr;
}

HdBprim *
Hd_UnitTestNullRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    if (typeId == HdPrimTypeTokens->texture) {
        return new HdTexture(SdfPath::EmptyPath());
    } else {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }

    return nullptr;
}

void
Hd_UnitTestNullRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
    delete bPrim;
}

void
Hd_UnitTestNullRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
}

PXR_NAMESPACE_CLOSE_SCOPE
