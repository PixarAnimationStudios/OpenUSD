//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/staticTokens.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _testTokens,
    (TestHdSip_GrindBeans)
    (TestHdSip_BoilWater)
    (TestHdSip_PourOver)
    (TestHdSip_DrinkCoffee)

    (TestHdSip_BreakEggs)
    (TestHdSip_CutVeggies)
    (TestHdSip_HeatPan)
    (TestHdSip_MakeOmelette)
    (TestHdSip_EatOmelette)

    (TestHdSip_TidyUp)
);

namespace
{

class _TestBaseSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    _TestBaseSceneIndexPlugin() = default;
    ~_TestBaseSceneIndexPlugin() override = default;
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override
    {
        // Not actually 
        return inputScene;
    }
};

}

// -----------------------------------------------------------------------------
// Define and register the test scene index plugins.
//
class TestHdSip_GrindBeans final : public _TestBaseSceneIndexPlugin { };
class TestHdSip_BoilWater final : public _TestBaseSceneIndexPlugin { };
class TestHdSip_PourOver final : public _TestBaseSceneIndexPlugin { };
class TestHdSip_DrinkCoffee final : public _TestBaseSceneIndexPlugin { };

class TestHdSip_BreakEggs final : public _TestBaseSceneIndexPlugin { };
class TestHdSip_CutVeggies final : public _TestBaseSceneIndexPlugin { };
class TestHdSip_HeatPan final : public _TestBaseSceneIndexPlugin { };
class TestHdSip_MakeOmelette final : public _TestBaseSceneIndexPlugin { };
class TestHdSip_EatOmelette final : public _TestBaseSceneIndexPlugin { };

class TestHdSip_TidyUp final : public _TestBaseSceneIndexPlugin { };

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<TestHdSip_GrindBeans>();
    HdSceneIndexPluginRegistry::Define<TestHdSip_BoilWater>();
    HdSceneIndexPluginRegistry::Define<TestHdSip_PourOver>();
    HdSceneIndexPluginRegistry::Define<TestHdSip_DrinkCoffee>();
    
    HdSceneIndexPluginRegistry::Define<TestHdSip_BreakEggs>();
    HdSceneIndexPluginRegistry::Define<TestHdSip_CutVeggies>();
    HdSceneIndexPluginRegistry::Define<TestHdSip_HeatPan>();
    HdSceneIndexPluginRegistry::Define<TestHdSip_MakeOmelette>();
    HdSceneIndexPluginRegistry::Define<TestHdSip_EatOmelette>();

    HdSceneIndexPluginRegistry::Define<TestHdSip_TidyUp>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    constexpr HdSceneIndexPluginRegistry::InsertionPhase prepPhase = 0;
    constexpr HdSceneIndexPluginRegistry::InsertionPhase makePhase = 2;
    constexpr HdSceneIndexPluginRegistry::InsertionPhase enjoyPhase = 4;
    constexpr HdSceneIndexPluginRegistry::InsertionPhase tidyUpPhase = 8;

    const auto &orderAtStart =
        HdSceneIndexPluginRegistry::InsertionOrderAtStart;
    const auto &orderAtEnd =
        HdSceneIndexPluginRegistry::InsertionOrderAtEnd;

    using _TokenPhaseOrder =
        std::tuple<TfToken, HdSceneIndexPluginRegistry::InsertionPhase, HdSceneIndexPluginRegistry::InsertionOrder>;
    
    // Register plugins with insertion/phase based ordering.
    //
    const std::vector<_TokenPhaseOrder> idAndPhases = {
        // "coffee" plugins
        { _testTokens->TestHdSip_GrindBeans, prepPhase, orderAtStart },
        { _testTokens->TestHdSip_BoilWater, prepPhase, orderAtStart },
        { _testTokens->TestHdSip_PourOver, makePhase, orderAtStart },
        { _testTokens->TestHdSip_DrinkCoffee, enjoyPhase, orderAtStart },

        // "omelette" plugins
        { _testTokens->TestHdSip_BreakEggs, prepPhase, orderAtStart },
        { _testTokens->TestHdSip_CutVeggies, prepPhase, orderAtStart },
        // "HeatPan" could arguably be in the "prep" phase, but we don't want to
        // overheat the pan while prepping...
        { _testTokens->TestHdSip_HeatPan, makePhase, orderAtStart },
        // "MakeOmelette" needs to be after "HeatPan".
        { _testTokens->TestHdSip_MakeOmelette, makePhase, orderAtEnd },
        { _testTokens->TestHdSip_EatOmelette, enjoyPhase, orderAtStart },

        // "clean" plugins
        { _testTokens->TestHdSip_TidyUp, tidyUpPhase, orderAtEnd },
    };

    for (const auto& [id, phase, order] : idAndPhases) {
         HdSceneIndexPluginRegistry::GetInstance()
         .RegisterSceneIndexForRenderer(
            HdSceneIndexPluginRegistryTokens->allRenderers,
            id,
            nullptr,
            phase,
            order);
    }
}

// -----------------------------------------------------------------------------

static std::ostream &
operator<<(std::ostream &out, const TfTokenVector &v)
{
    out << "{" << std::endl;
    for (const auto &t : v) {
        out << t << std::endl;
    }
    out << "}" << std::endl;
    return out;
}

static bool 
_CompareValue(
    const char *msg, const TfTokenVector &value, const TfTokenVector &expected)
{
    if (value == expected) {
        std::cout << msg << " matches." << std::endl;
    } else {
        std::cerr << msg << " doesn't match. Expecting " << expected
                  << " got " << value << std::endl;
        return false;
    }
    return true;
}

static
bool
BasicTest()
{
   HdSceneIndexPluginRegistry &registry =
        HdSceneIndexPluginRegistry::GetInstance();
    
    std::vector<TfToken> pluginIds = registry.LoadAndGetSceneIndexPluginIds(
        HdSceneIndexPluginRegistryTokens->allRenderers.GetString(),
        {/*empty app name*/});
    
    const TfTokenVector expectedPluginOrder = {
        // Plugins with the same (phase, order) are sorted lexicographically.
        // "prep" phase plugins:
        _testTokens->TestHdSip_BoilWater,
        _testTokens->TestHdSip_BreakEggs,
        _testTokens->TestHdSip_CutVeggies,
        _testTokens->TestHdSip_GrindBeans,

        // "make" phase plugins:
        _testTokens->TestHdSip_HeatPan,
        _testTokens->TestHdSip_PourOver,
        _testTokens->TestHdSip_MakeOmelette,

        // "enjoy" phase plugins:
        _testTokens->TestHdSip_DrinkCoffee,
        _testTokens->TestHdSip_EatOmelette,

        // "tidy up" phase plugins:
        _testTokens->TestHdSip_TidyUp
    };

    return _CompareValue("ORDERED SCENE INDEX PLUGINS (phase/order based)",
        pluginIds, expectedPluginOrder);
}

int main()
{
    TfErrorMark mark;

    const std::string testDirPath = ArchGetCwd();
    // Simulate the scenario where scene index plugins are registered in
    // different libraries (directories).
    const std::vector<std::string> expectedPlugInfoPaths = {
        testDirPath + "/coffee/plugInfo.json",
        testDirPath + "/omelette/plugInfo.json",
        testDirPath + "/clean/plugInfo.json"
    };
    for (const auto &path : expectedPlugInfoPaths) {
        TF_AXIOM(TfPathExists(path));
        TF_AXIOM(!PlugRegistry::GetInstance().RegisterPlugins(path).empty());
    }
    
    bool success = BasicTest();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
