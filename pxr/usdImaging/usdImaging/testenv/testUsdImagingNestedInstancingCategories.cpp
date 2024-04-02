//
// Copyright 2024 Pixar
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

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/unitTestHelper.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

struct Instance {
    Instance(const SdfPath& path_, const VtTokenArray& cats_)
        : path(path_), cats(cats_) {}
    Instance(std::string path_, std::vector<std::string> cats_)
        : path(SdfPath(path_)), cats(cats_.begin(), cats_.end()) {}
    static bool Diff(const Instance& lhs, const Instance& rhs, std::string& msg)
    {
        std::vector<TfToken> lhsCats(lhs.cats.begin(), lhs.cats.end());
        std::sort(lhsCats.begin(), lhsCats.end());
        std::vector<TfToken> rhsCats(rhs.cats.begin(), rhs.cats.end());
        std::sort(rhsCats.begin(), rhsCats.end());
        bool result = true;
        if (lhs.path != rhs.path) {
            msg += TfStringPrintf("\nPath mismatch: <%s> != <%s>\n", 
                lhs.path.GetText(), rhs.path.GetText());
            result = false;
        }
        if (lhsCats != rhsCats) {
            std::string strL, strR;
            const size_t max = std::max(lhsCats.size(), rhsCats.size());
            for (size_t i = 0; i < max; ++i) {
                if (i < lhsCats.size()) {
                    if (!strL.empty()) {
                        strL += ", ";
                    }
                    strL += lhsCats[i].GetString();
                }
                if (i < rhsCats.size()) {
                    if (!strR.empty()) {
                        strR += ", ";
                    }
                    strR += rhsCats[i].GetString();
                }
            }
            msg += TfStringPrintf(
                "\nCategories mismatch:\n  L: (%lu) [%s]\n  R: (%lu) [%s]\n",
                lhsCats.size(), strL.c_str(), rhsCats.size(), strR.c_str());
            result = false;
        }
        if (result) {
            msg += "OK\n";
        }
        return result;
    }
    const SdfPath path;
    const VtTokenArray cats;
};

static void
TestNestedInstancingCategories()
{
    UsdStageRefPtr stage = UsdStage::Open("./scene.usda");
    TF_VERIFY(stage);
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    TF_VERIFY(renderIndex);
    std::unique_ptr<UsdImagingDelegate> delegate(
        new UsdImagingDelegate(renderIndex.get(), SdfPath::AbsoluteRootPath()));
    delegate->Populate(stage->GetPseudoRoot());
    delegate->SetTime(0.0);
    delegate->SyncAll(true);

    static const std::vector<Instance> Expected = {
        Instance("/W/A/A/Sphere", { "/W/ShExBA.collection:shadowLink" ,
                                    "/W/LiExBB.collection:lightLink"  ,
                                    "/W/LiInAA.collection:lightLink"  } ),
        Instance("/W/B/A/Sphere", { "/W/LiExBB.collection:lightLink"  } ),
        Instance("/W/A/B/Sphere", { "/W/ShInAB.collection:shadowLink" ,
                                    "/W/ShExBA.collection:shadowLink" ,
                                    "/W/LiExBB.collection:lightLink"  } ),
        Instance("/W/B/B/Sphere", { "/W/ShExBA.collection:shadowLink" } ) };

    const SdfPath protoPath = SdfPath("/__Prototype_1/A/proto_Sphere_id0");
    const SdfPath& instancerId = delegate->GetInstancerId(protoPath);
    const std::vector<VtTokenArray> instanceCategories = 
        delegate->GetInstanceCategories(instancerId);
    const VtIntArray& instanceIndices = 
        delegate->GetInstanceIndices(instancerId, protoPath);
    const SdfPathVector& instancePaths = 
        delegate->GetScenePrimPaths(protoPath, 
            {instanceIndices.begin(), instanceIndices.end()});
    
    bool res = true;
    std::string msg;
    for (const int& instanceIndex : instanceIndices) {
        if (instanceIndex >= int(instanceCategories.size())) {
            TF_CODING_ERROR("InstanceIndex %i is out of range. "
                "GetInstanceCategories gave only %lu category lists", 
                instanceIndex, instanceCategories.size());
            break;
        }
        if (instanceIndex >= int(instancePaths.size())) {
            TF_CODING_ERROR("InstanceIndex %i is out of range. "
                "GetScenePrimPaths gave only %lu paths",
                instanceIndex, instancePaths.size());
            break;
        }
        const VtTokenArray& cats = instanceCategories[instanceIndex];
        const SdfPath& instancePath = instancePaths[instanceIndex];
        const Instance instance(instancePath, cats);
        
        msg += TfStringPrintf("\n%i <%s>: ", instanceIndex,
            instancePath.GetText());
        res = Instance::Diff(instance, Expected[instanceIndex], msg) && res;        
    }
    TF_VERIFY(res, "%s\n", msg.c_str());
}

int
main()
{
    TfErrorMark mark;
    TestNestedInstancingCategories();
    if (TF_VERIFY(mark.IsClean())) {
        std::cout << "OK" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
    }
}
