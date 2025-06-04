//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/unitTestHelper.h"

#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/root.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class Hd_NullRprim final : public HdRprim {
public:
    Hd_NullRprim(TfToken const& typeId,
                 SdfPath const& id)
     : HdRprim(id)
     , _typeId(typeId)
    {

    }

    virtual ~Hd_NullRprim() = default;

    TfTokenVector const & GetBuiltinPrimvarNames() const override {
        static const TfTokenVector primvarNames;
        return primvarNames;
    }

    virtual void Sync(HdSceneDelegate *delegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits,
                      TfToken const   &reprToken) override
    {
        SdfPath const& id = GetId();

        // Normals is a primvar
        if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
            _SyncPrimvars(delegate, *dirtyBits);
        }
        *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
    }


    virtual HdDirtyBits GetInitialDirtyBitsMask() const override
    {
        // Set all bits except the varying flag
        return  (HdChangeTracker::AllSceneDirtyBits) &
               (~HdChangeTracker::Varying);
    }

    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override
    {
        return bits;
    }


protected:
    virtual void _InitRepr(TfToken const &reprToken,
                           HdDirtyBits *dirtyBits) override
    {
        _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                                _ReprComparator(reprToken));
        if (it == _reprs.end()) {
            _reprs.emplace_back(reprToken, HdReprSharedPtr());
        }
    }

private:
    TfToken _typeId;

    void _SyncPrimvars(HdSceneDelegate *delegate,
                       HdDirtyBits      dirtyBits)
    {
        SdfPath const &id = GetId();
        for (size_t interpolation = HdInterpolationConstant;
                    interpolation < HdInterpolationCount;
                  ++interpolation) {
            HdPrimvarDescriptorVector primvars =
                    GetPrimvarDescriptors(delegate,
                            static_cast<HdInterpolation>(interpolation));

            size_t numPrimVars = primvars.size();
            for (size_t primVarNum = 0;
                        primVarNum < numPrimVars;
                      ++primVarNum) {
                HdPrimvarDescriptor const &primvar = primvars[primVarNum];

                if (HdChangeTracker::IsPrimvarDirty(dirtyBits,
                                                    id,
                                                    primvar.name)) {
                    GetPrimvar(delegate, primvar.name);
                }
            }
        }
    }

    Hd_NullRprim()                                 = delete;
    Hd_NullRprim(const Hd_NullRprim &)             = delete;
    Hd_NullRprim &operator =(const Hd_NullRprim &) = delete;
};

// Mock render delegate for testing - just handles the ExtComputation sprims.
class ExtCompTestRenderDelegate : public HdRenderDelegate {
private:
    static const TfTokenVector _emptyTypes;
    static const TfTokenVector _rprimTypes;
    static const TfTokenVector _sprimTypes;

public:
    virtual HdResourceRegistrySharedPtr GetResourceRegistry() const override {
        return nullptr;
    }

    virtual HdRenderPassSharedPtr CreateRenderPass(
                                HdRenderIndex *index,
                                HdRprimCollection const& collection) override {
        return nullptr;
    }

    virtual HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                         SdfPath const& id) override {
        return nullptr;
    }
    virtual void DestroyInstancer(HdInstancer *instancer) override {
    }

    virtual HdRprim *CreateRprim(TfToken const& typeId,
                                 SdfPath const& rprimId) override {
        if (typeId == HdPrimTypeTokens->mesh) {
            return new Hd_NullRprim(typeId, rprimId);
        }

        return nullptr;
    }
    virtual void DestroyRprim(HdRprim *rPrim) override {
    }

    virtual HdSprim *CreateSprim(TfToken const& typeId,
                                 SdfPath const& sprimId) override {
        if (typeId == HdPrimTypeTokens->extComputation) {
            return new HdExtComputation(sprimId);
        }

        return nullptr;
    }
    virtual HdSprim *CreateFallbackSprim(TfToken const& typeId) override {
        return new HdExtComputation(SdfPath::EmptyPath());
    }
    virtual void DestroySprim(HdSprim *sprim) override {
        delete sprim;
    }

    virtual HdBprim *CreateBprim(TfToken const& typeId,
                                 SdfPath const& bprimId) override {
        return nullptr;
    }
    virtual HdBprim *CreateFallbackBprim(TfToken const& typeId) override {
        return nullptr;
    }
    virtual void DestroyBprim(HdBprim *bPrim) override {
    }

    virtual void CommitResources(HdChangeTracker *tracker) override {
    }

    virtual const TfTokenVector &GetSupportedRprimTypes() const override {
        return _rprimTypes;
    }
    virtual const TfTokenVector &GetSupportedSprimTypes() const override {
        return _sprimTypes;
    }
    virtual const TfTokenVector &GetSupportedBprimTypes() const override {
        return _emptyTypes;
    }
};

const TfTokenVector ExtCompTestRenderDelegate::_emptyTypes;
const TfTokenVector ExtCompTestRenderDelegate::_rprimTypes = {
    HdPrimTypeTokens->mesh
};
const TfTokenVector ExtCompTestRenderDelegate::_sprimTypes = {
    HdPrimTypeTokens->extComputation
};

struct SkelRootInfo {
    bool hasNormalsComputation;
    bool hasNormalsPrimvar { false };
    HdInterpolation normalsInterpolation { HdInterpolationVertex };
    TfToken normalsComputationOutputName { TfToken("skinnedNormals") };
    TfToken normalsComputationPrimvarName { HdTokens->normals };
    TfToken sourceComputationOutputName { TfToken("skinnedNormals") };
};

static void
TestSkinningComputations()
{
    std::cout << "-------------------------------------------------------\n";
    std::cout << "TestSkinningComputations\n";
    std::cout << "-------------------------------------------------------\n";

    const std::string usdPath = "skinning/model.usda";
    UsdStageRefPtr stage = UsdStage::Open(usdPath);
    TF_AXIOM(stage);

    // Bring up Hydra
    ExtCompTestRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex>
        renderIndex(HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    auto delegate = std::make_unique<UsdImagingDelegate>(renderIndex.get(),
                               SdfPath::AbsoluteRootPath());
    delegate->Populate(stage->GetPseudoRoot());
    delegate->ApplyPendingUpdates();
    delegate->SyncAll(true);

    HdTaskContext _taskContext;
    HdTaskSharedPtrVector tasks;
    renderIndex->SyncAll(&tasks, &_taskContext);
    
    const std::unordered_map<std::string, SkelRootInfo> skelRootInfos = {
        {"SkinningWithoutNormals", {
            true,
        }},
        {"SkinningWithVertexVaryingNormals", {
            true,
            true,
            HdInterpolationVertex
        }},
        {"SkinningWithFaceVaryingNormals", {
            true,
            true,
            HdInterpolationFaceVarying
        }},
        {"SkinningWithVaryingNormals", {
            true,
            true,
            HdInterpolationVarying
        }},
        {"SkinningWithConstantNormals", {
            false
        }},
        {"SkinningWithUniformNormals", {
            false
        }},
        {"SkinningWithVertexVaryingPrimvarNormals", {
            true,
            true,
            HdInterpolationVertex
        }},
        {"SkinningWithFaceVaryingPrimvarNormals", {
            true,
            true,
            HdInterpolationFaceVarying
        }},
        {"SkinningWithVaryingPrimvarNormals", {
            true,
            true,
            HdInterpolationVarying
        }},
        {"SkinningWithConstantPrimvarNormals", {
            false,
        }},
        {"SkinningWithUniformPrimvarNormals", {
            false
        }}
    };

    for (const auto& [skelRoot, skelRootInfo] : skelRootInfos) {
        const std::string& basePath = "/Root/" + skelRoot + "/Skinning/Box/";
        const std::string& pointsCompPath = basePath + "skinningPointsComputation";
        const std::string& normalsCompPath = basePath + "skinningNormalsComputation";
        const std::string& pointsAggregatorCompPath = basePath + 
            "skinningPointsInputAggregatorComputation";
        const std::string& normalsAggregatorCompPath = basePath + 
            "skinningNormalsInputAggregatorComputation";

        // Convert to HdExtComputation*
        auto* pointsComp = static_cast<HdExtComputation*>(
            renderIndex->GetSprim(HdPrimTypeTokens->extComputation,
                                SdfPath(pointsCompPath)));
        auto* pointsAggregatorComp = static_cast<HdExtComputation*>(
            renderIndex->GetSprim(HdPrimTypeTokens->extComputation,
                                SdfPath(pointsAggregatorCompPath)));
        auto* normalsComp = static_cast<HdExtComputation*>(
            renderIndex->GetSprim(HdPrimTypeTokens->extComputation,
                                SdfPath(normalsCompPath)));
        auto* normalsAggregatorComp = static_cast<HdExtComputation*>(
            renderIndex->GetSprim(HdPrimTypeTokens->extComputation,
                                SdfPath(normalsAggregatorCompPath)));
        TF_AXIOM(pointsComp);
        TF_AXIOM(pointsAggregatorComp);

        // Constant and Uniform cases don't have normals computations.
        if (!skelRootInfo.hasNormalsComputation) {
            TF_AXIOM(!normalsComp);
            TF_AXIOM(!normalsAggregatorComp);
        } else {
            TF_AXIOM(normalsComp);
            TF_AXIOM(normalsAggregatorComp);

            const auto& computationOutputs = normalsComp->GetComputationOutputs();
            TF_AXIOM(computationOutputs.size() == 1);
            TF_AXIOM(computationOutputs[0].name == skelRootInfo.normalsComputationOutputName);

            if (skelRootInfo.hasNormalsPrimvar) {
                const auto& primvarDescriptors = delegate->GetExtComputationPrimvarDescriptors(
                    SdfPath(normalsCompPath).GetParentPath(), skelRootInfo.normalsInterpolation);
                int normalsPrimvarIndex = 0;
                // When it's vertex interpolation, there's an extra primvar for points.
                if(skelRootInfo.normalsInterpolation == HdInterpolationVertex) {
                    normalsPrimvarIndex = 1;
                }
                TF_AXIOM(primvarDescriptors.size() == normalsPrimvarIndex + 1);
                TF_AXIOM(primvarDescriptors[normalsPrimvarIndex].name == skelRootInfo.normalsComputationPrimvarName);
                TF_AXIOM(primvarDescriptors[normalsPrimvarIndex].interpolation == skelRootInfo.normalsInterpolation);
                TF_AXIOM(primvarDescriptors[normalsPrimvarIndex].sourceComputationId == SdfPath(normalsCompPath));
                TF_AXIOM(primvarDescriptors[normalsPrimvarIndex].sourceComputationOutputName == skelRootInfo.sourceComputationOutputName);
            }
        }
    }
}

int main()
{
    TestSkinningComputations();
}