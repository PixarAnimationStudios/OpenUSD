//
// Copyright 2021 Pixar
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
#include "pxr/pxr.h"

#include "TestUsdResolverChangedResolver.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/notice.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/staticInterface.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

// We don't have the infrastructure set up to have test executable
// link to test-only libraries. So, we use this PlugStaticInterface
// to get access to the functions on the _TestResolver that we need
// to call.
static PlugStaticInterface<_TestResolverPluginInterface> _ResolverInterface;

static void
SetupPlugins()
{
    ArSetPreferredResolver("_TestResolver");

    // Register _TestResolver plugin. We assume the build system will
    // install it to the UsdPlugins subdirectory in the same location as
    // this test.
    const std::string pluginPath =
        TfStringCatPaths(
            TfGetPathName(ArchGetExecutablePath()),
            "UsdPlugins/lib/TestUsdResolverChangedResolver*/Resources/") + "/";

    PlugPluginPtrVector plugins =
        PlugRegistry::GetInstance().RegisterPlugins(pluginPath);
    
    TF_AXIOM(plugins.size() == 1);
    TF_AXIOM(plugins[0]->GetName() == "TestUsdResolverChangedResolver");
}

void
ValidateValue(
    const UsdStageRefPtr& stage, const std::string& attrPath,
    const std::string& expectedValue)
{
    std::string value;
    UsdAttribute attr = stage->GetAttributeAtPath(SdfPath(attrPath));
    TF_AXIOM(attr);
    TF_AXIOM(attr.Get(&value));
    TF_AXIOM(value == expectedValue);
}

void
ValidatePaths(
    const UsdNotice::ObjectsChanged::PathRange range,
    const std::vector<std::string>& expectedPaths)
{
    TF_AXIOM(
        SdfPathVector(range) == 
        SdfPathVector(expectedPaths.begin(), expectedPaths.end()));
}

class NoticeTester 
    : public TfWeakBase
{
public:
    NoticeTester(const UsdStageRefPtr& stage)
        : noticeCount(0)
    {
        TfNotice::Register(TfCreateWeakPtr(this), 
            &NoticeTester::_HandleNotice, UsdStagePtr(stage));
    }

    size_t noticeCount;
    std::function<void(const UsdNotice::ObjectsChanged&)> test;

private:
    void _HandleNotice(const UsdNotice::ObjectsChanged& n)
    {
        ++noticeCount;
        test(n);
    }
};

int
main(int argc, char** argv)
{
    SetupPlugins();

    // The "shots" in this test use asset paths with two different forms
    // to exercise UsdStage's change processing:
    //
    //  - {version}/Model.usda
    //    The {version} string is replaced in _TestResolver::CreateIdentifier
    //    using the version specified via SetVersionForConfig.
    //
    //  - Model
    //    The model name is looked up in the asset paths table set with
    //    SetAssetPathsForConfig during _TestResolver::Resolve. 
    //    
    std::unordered_map<std::string, std::string> assetPaths = {
        { "Woody", "ts1/Woody.usda" },
        { "Buzz", "ts1/Buzz.usda" } 
    };

    _ResolverInterface->SetAssetPathsForConfig("toy_story", assetPaths);
    _ResolverInterface->SetVersionForConfig("toy_story", "ts1");

    UsdStageRefPtr shotA = UsdStage::Open(
        "shotA.usda", _TestResolverContext("toy_story"));
    TF_AXIOM(shotA);

    UsdStageRefPtr shotB = UsdStage::Open(
        "shotB.usda", _TestResolverContext("toy_story"));
    TF_AXIOM(shotB);

    UsdStageRefPtr shotC = UsdStage::Open(
        "shotC.usda", _TestResolverContext("toy_story"));
    TF_AXIOM(shotC);

    UsdStageRefPtr woody = UsdStage::Open(
        "Woody.usda", _TestResolverContext("toy_story"));
    TF_AXIOM(woody);

    UsdStageRefPtr unrelatedShot = UsdStage::CreateInMemory(
        "unrelated", _TestResolverContext("unrelated"));
    TF_AXIOM(unrelatedShot);

    NoticeTester shotAListener(shotA), shotBListener(shotB),
        shotCListener(shotC), woodyListener(woody),
        unrelatedListener(unrelatedShot);

    // Change notifications should never come from unrelatedShot.
    unrelatedListener.test = 
        [](const UsdNotice::ObjectsChanged& n) {
            TF_AXIOM(false);
        };

    ValidateValue(shotA, "/AndysRoom/Woody.movie", "toy_story_1");
    ValidateValue(shotA, "/AndysRoom/Buzz.movie", "toy_story_1");
    ValidateValue(shotB, "/BonniesRoom/Woody.movie", "toy_story_1");
    ValidateValue(shotB, "/BonniesRoom/Buzz.movie", "toy_story_1");
    ValidateValue(shotC, "/AntiquesRoom/Woody.movie", "toy_story_1");
    ValidateValue(shotC, "/AntiquesRoom/Buzz.movie", "toy_story_1");
    ValidateValue(woody, "/Woody.movie", "toy_story_1");

    // Change the asset path associated with Buzz and reload. This
    // should cause the _TestResolver to emit a ResolverChanged notice,
    // which should cause all other stages using equivalent contexts
    // to update and resync Buzz. We also expect resolved asset path
    // resyncs at the pseudo-root, since a resolver change may affect
    // asset paths throughout the entire stage.
    shotAListener.test = [&](const UsdNotice::ObjectsChanged& n) {
        ValidatePaths(n.GetResyncedPaths(), {"/AndysRoom/Buzz"});
        ValidatePaths(n.GetChangedInfoOnlyPaths(), {});
        ValidatePaths(n.GetResolvedAssetPathsResyncedPaths(), {"/"});
    };
    shotBListener.test = [&](const UsdNotice::ObjectsChanged& n) {
        ValidatePaths(n.GetResyncedPaths(), {"/BonniesRoom/Buzz"});
        ValidatePaths(n.GetChangedInfoOnlyPaths(), {});
        ValidatePaths(n.GetResolvedAssetPathsResyncedPaths(), {"/"});
    };
    shotCListener.test = [&](const UsdNotice::ObjectsChanged& n) {
        ValidatePaths(n.GetResyncedPaths(), {"/AntiquesRoom/Buzz"});
        ValidatePaths(n.GetChangedInfoOnlyPaths(), {});
        ValidatePaths(n.GetResolvedAssetPathsResyncedPaths(), {"/"});
    };
    woodyListener.test = [&](const UsdNotice::ObjectsChanged& n) {
        ValidatePaths(n.GetResyncedPaths(), {});
        ValidatePaths(n.GetChangedInfoOnlyPaths(), {});
        ValidatePaths(n.GetResolvedAssetPathsResyncedPaths(), {"/"});
    };

    assetPaths["Buzz"] = "ts2/Buzz.usda";
    _ResolverInterface->SetAssetPathsForConfig("toy_story", assetPaths);

    printf("Reloading stage...\n");
    shotA->Reload();

    TF_AXIOM(shotAListener.noticeCount == 1);
    TF_AXIOM(shotBListener.noticeCount == 1);
    TF_AXIOM(shotCListener.noticeCount == 1);
    TF_AXIOM(woodyListener.noticeCount == 1);
    TF_AXIOM(unrelatedListener.noticeCount == 0);
    
    ValidateValue(shotA, "/AndysRoom/Woody.movie", "toy_story_1");
    ValidateValue(shotA, "/AndysRoom/Buzz.movie", "toy_story_2");
    ValidateValue(shotB, "/BonniesRoom/Woody.movie", "toy_story_1");
    ValidateValue(shotB, "/BonniesRoom/Buzz.movie", "toy_story_2");
    ValidateValue(shotC, "/AntiquesRoom/Woody.movie", "toy_story_1");
    ValidateValue(shotC, "/AntiquesRoom/Buzz.movie", "toy_story_2");
    ValidateValue(woody, "/Woody.movie", "toy_story_1");

    // Change the version associated with Woody and reload. Same
    // thing should happen as above.
    shotAListener.test = [&](const UsdNotice::ObjectsChanged& n) {
        ValidatePaths(n.GetResyncedPaths(), {"/AndysRoom/Woody"});
        ValidatePaths(n.GetChangedInfoOnlyPaths(), {});
        ValidatePaths(n.GetResolvedAssetPathsResyncedPaths(), {"/"});
    };
    shotBListener.test = [&](const UsdNotice::ObjectsChanged& n) {
        ValidatePaths(n.GetResyncedPaths(), {"/BonniesRoom/Woody"});
        ValidatePaths(n.GetChangedInfoOnlyPaths(), {});
        ValidatePaths(n.GetResolvedAssetPathsResyncedPaths(), {"/"});
    };
    shotCListener.test = [&](const UsdNotice::ObjectsChanged& n) {
        ValidatePaths(n.GetResyncedPaths(), {"/AntiquesRoom/Woody"});
        ValidatePaths(n.GetChangedInfoOnlyPaths(), {});
        ValidatePaths(n.GetResolvedAssetPathsResyncedPaths(), {"/"});
    };
    woodyListener.test = [&](const UsdNotice::ObjectsChanged& n) {
        // In this case, the version change affects the resolution of
        // a local sublayer, which currently translates to a full resync
        // of the stage. The resolved asset path resync of the pseudo-root
        // is subsumed by the stage resync.
        ValidatePaths(n.GetResyncedPaths(), {"/"});
        ValidatePaths(n.GetChangedInfoOnlyPaths(), {});
        ValidatePaths(n.GetResolvedAssetPathsResyncedPaths(), {});
    };

    _ResolverInterface->SetVersionForConfig("toy_story", "ts2");

    printf("Reloading stage...\n");
    shotA->Reload();
    
    TF_AXIOM(shotAListener.noticeCount == 2);
    TF_AXIOM(shotBListener.noticeCount == 2);
    TF_AXIOM(shotCListener.noticeCount == 2);
    TF_AXIOM(woodyListener.noticeCount == 2);
    TF_AXIOM(unrelatedListener.noticeCount == 0);

    ValidateValue(shotA, "/AndysRoom/Woody.movie", "toy_story_2");
    ValidateValue(shotA, "/AndysRoom/Buzz.movie", "toy_story_2");
    ValidateValue(shotB, "/BonniesRoom/Woody.movie", "toy_story_2");
    ValidateValue(shotB, "/BonniesRoom/Buzz.movie", "toy_story_2");
    ValidateValue(shotC, "/AntiquesRoom/Woody.movie", "toy_story_2");
    ValidateValue(shotC, "/AntiquesRoom/Buzz.movie", "toy_story_2");
    ValidateValue(woody, "/Woody.movie", "toy_story_2");

    printf("PASSED!\n");

    return 0;
}

