//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/unitTestHelper.h"

#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/renderIndex.h"

#include "pxr/usd/usd/stage.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class Hd_NullRprim final : public HdRprim
{
public:
    Hd_NullRprim(
        TfToken const & typeId,
        SdfPath const & id)
      : HdRprim(id)
      , _typeId(typeId)
    {}

    ~Hd_NullRprim() = default;

    TfTokenVector const & GetBuiltinPrimvarNames() const override {
        static const TfTokenVector primvarNames;
        return primvarNames;
    }

    void Sync(
        HdSceneDelegate * delegate,
        HdRenderParam * /*renderParam*/,
        HdDirtyBits * dirtyBits,
        TfToken const & /*reprToken*/) override
    {
        SdfPath const & id = GetId();

        // Normals is a primvar
        if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
            _SyncPrimvars(delegate, *dirtyBits);
        }
        *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
    }


    HdDirtyBits GetInitialDirtyBitsMask() const override
    {
        // Set all bits except the varying flag
        return  (HdChangeTracker::AllSceneDirtyBits) &
               (~HdChangeTracker::Varying);
    }

    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override
    {
        return bits;
    }


protected:
    void _InitRepr(
        TfToken const & reprToken,
        HdDirtyBits * /*dirtyBits*/) override
    {
        auto it = std::find_if(
            _reprs.begin(),
            _reprs.end(),
            _ReprComparator(reprToken));
        if (it == _reprs.end()) {
            _reprs.emplace_back(reprToken, HdReprSharedPtr());
        }
    }

private:
    TfToken _typeId;

    void _SyncPrimvars(
        HdSceneDelegate * delegate,
        HdDirtyBits dirtyBits)
    {
        SdfPath const & id = GetId();
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

                if (HdChangeTracker::IsPrimvarDirty(
                        dirtyBits,
                        id,
                        primvar.name)) {
                    GetPrimvar(delegate, primvar.name);
                }
            }
        }
    }

    Hd_NullRprim() = delete;
    Hd_NullRprim(const Hd_NullRprim &) = delete;
    Hd_NullRprim & operator=(const Hd_NullRprim &) = delete;
};

// Mock render delegate for testing - just handles the ExtComputation sprims.
class ExtCompTestRenderDelegate : public HdRenderDelegate
{
public:
    HdResourceRegistrySharedPtr GetResourceRegistry() const override {
        return nullptr;
    }

    HdRenderPassSharedPtr CreateRenderPass(
        HdRenderIndex * /*index*/,
        HdRprimCollection const & /*collection*/) override {
        return nullptr;
    }

    HdInstancer * CreateInstancer(
        HdSceneDelegate * /*delegate*/,
        SdfPath const & /*id*/) override {
        return nullptr;
    }

    void DestroyInstancer(HdInstancer * instancer) override {}

    HdRprim * CreateRprim(
        TfToken const & typeId,
        SdfPath const & rprimId) override {
        if (typeId == HdPrimTypeTokens->mesh) {
            return new Hd_NullRprim(typeId, rprimId);
        }

        return nullptr;
    }

    void DestroyRprim(HdRprim * rPrim) override {}

    HdSprim * CreateSprim(
        TfToken const & typeId,
        SdfPath const & sprimId) override {
        if (typeId == HdPrimTypeTokens->extComputation) {
            return new HdExtComputation(sprimId);
        }

        return nullptr;
    }

    HdSprim * CreateFallbackSprim(TfToken const & /*typeId*/) override {
        return new HdExtComputation(SdfPath::EmptyPath());
    }

    void DestroySprim(HdSprim * sprim) override {
        delete sprim;
    }

    HdBprim * CreateBprim(
        TfToken const & /*typeId*/,
        SdfPath const & /*bprimId*/) override {
        return nullptr;
    }

    HdBprim * CreateFallbackBprim(TfToken const& /*typeId*/) override {
        return nullptr;
    }

    void DestroyBprim(HdBprim * bPrim) override {}

    void CommitResources(HdChangeTracker * tracker) override {}

    const TfTokenVector & GetSupportedRprimTypes() const override {
        static const TfTokenVector rprimTypes = {
            HdPrimTypeTokens->mesh
        };
        return rprimTypes;
    }

    const TfTokenVector & GetSupportedSprimTypes() const override {
        static const TfTokenVector sprimTypes = {
            HdPrimTypeTokens->extComputation
        };
        return sprimTypes;
    }

    const TfTokenVector & GetSupportedBprimTypes() const override {
        static const TfTokenVector bprimTypes;
        return bprimTypes;
    }
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

    const std::string usdPath = "model.usda";
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
        const std::string& basePath =
            "/Root/" + skelRoot + "/Skinning/Box/";
        const std::string& pointsCompPath =
            basePath + "skinningPointsComputation";
        const std::string& normalsCompPath =
            basePath + "skinningNormalsComputation";
        const std::string& pointsAggregatorCompPath =
            basePath + "skinningPointsInputAggregatorComputation";
        const std::string& normalsAggregatorCompPath =
            basePath + "skinningNormalsInputAggregatorComputation";

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

            const auto& computationOutputs =
                normalsComp->GetComputationOutputs();
            TF_AXIOM(computationOutputs.size() == 1);
            TF_AXIOM(
                computationOutputs[0].name ==
                skelRootInfo.normalsComputationOutputName);

            if (skelRootInfo.hasNormalsPrimvar) {
                const auto& primvarDescs =
                    delegate->GetExtComputationPrimvarDescriptors(
                        SdfPath(normalsCompPath).GetParentPath(),
                        skelRootInfo.normalsInterpolation);
                size_t normalsPrimvarIndex = 0;
                // When it's vertex interpolation, there's an extra primvar for
                // points.
                if (skelRootInfo.normalsInterpolation ==
                        HdInterpolationVertex) {
                    normalsPrimvarIndex = 1;
                }
                TF_AXIOM(primvarDescs.size() == normalsPrimvarIndex + 1);
                TF_AXIOM(
                    primvarDescs[normalsPrimvarIndex].name ==
                    skelRootInfo.normalsComputationPrimvarName);
                TF_AXIOM(
                    primvarDescs[normalsPrimvarIndex].interpolation ==
                    skelRootInfo.normalsInterpolation);
                TF_AXIOM(
                    primvarDescs[normalsPrimvarIndex].sourceComputationId ==
                    SdfPath(normalsCompPath));
                TF_AXIOM(
                    primvarDescs[normalsPrimvarIndex].sourceComputationOutputName ==
                    skelRootInfo.sourceComputationOutputName);
            }
        }
    }
}

int main()
{
    TfErrorMark mark;

    TestSkinningComputations();

    if (TF_AXIOM(mark.IsClean())) {
        std::cout << "OK" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
    }
}