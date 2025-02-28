//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/changeManager.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/notice.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/schema.h"

#include <map>
#include <sstream>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

static std::pair<SdfLayerRefPtr, SdfLayerRefPtr>
_CreateLayerDiffTestLayers() {
    SdfLayerRefPtr actualLayer = SdfLayer::CreateAnonymous();
    actualLayer->ImportFromString(    
        R"(#sdf 1.4.32
            over "a"{}
            def "b"{}
            over "c"{
                int propC = 1
            }
            def "r" {
                int propR = 1
            }
            def "p" {
                int propP = 1
            }
            )"
    );

    SdfLayerRefPtr diffLayer = SdfLayer::CreateAnonymous();
    diffLayer->ImportFromString(
        R"(#sdf 1.4.32
            def "z"{}
            def "b"{}
            over "c"{
                int propC = 2
            }
            def "n" {
                int propN = 1
            }
            def "p" {}
    )");

    return std::make_pair(actualLayer, diffLayer);
}

static void _CompareChangeLists(
    const SdfChangeList &expected, 
    const SdfChangeList &actual)
{
    std::ostringstream expectedClStr, actualClStr;
    expectedClStr << expected;
    actualClStr << actual;

    TF_AXIOM(actualClStr.str() == expectedClStr.str());
}

static void
_TestSdfLayerCreateDiffChangeListWithoutValues()
{
    const auto [actualLayer, diffLayer] = _CreateLayerDiffTestLayers();

    SdfChangeList expectedCl;
    expectedCl.DidRemoveProperty(SdfPath("/r.propR"), false);
    expectedCl.DidRemovePrim(SdfPath("/r"), false);
    expectedCl.DidRemoveProperty(SdfPath("/p.propP"), false);
    expectedCl.DidRemovePrim(SdfPath("/a"), true);
    expectedCl.DidAddPrim(SdfPath("/n"), false);
    expectedCl.DidChangeInfo(SdfPath("/n"), SdfFieldKeys->Specifier, 
        std::move(VtValue()), VtValue(SdfSpecifierDef));
    expectedCl.DidAddProperty(SdfPath("/n.propN"), false);
    expectedCl.DidAddPrim(SdfPath("/z"), false);
    expectedCl.DidChangeInfo(SdfPath("/z"), SdfFieldKeys->Specifier, 
        std::move(VtValue()), VtValue(SdfSpecifierDef));
    expectedCl.DidRemoveProperty(SdfPath("/c.propC"), false);
    expectedCl.DidAddProperty(SdfPath("/c.propC"), false);

    // Copy the layer so we can verify it does not change during the operation
    SdfLayerRefPtr expectedLayer = SdfLayer::CreateAnonymous();
    expectedLayer->TransferContent(actualLayer);

    // Ensure that the layer remains unchanged during the process
    std::string actualLayerStr, expectedLayerStr;
    TF_AXIOM(expectedLayer->ExportToString(&expectedLayerStr));

    SdfChangeList actualCl = actualLayer->CreateDiff(diffLayer, 
            /*compareFieldValues*/ false);

    TF_AXIOM(expectedLayer->ExportToString(&actualLayerStr));
    TF_AXIOM(actualLayerStr == expectedLayerStr);

    _CompareChangeLists(expectedCl, actualCl);
}

static void
_TestSdfLayerCreateDiffChangeListWithValues()
{
    const auto [actualLayer, diffLayer] = _CreateLayerDiffTestLayers();

    SdfChangeList expectedCl;
    expectedCl.DidRemoveProperty(SdfPath("/r.propR"), false);
    expectedCl.DidRemovePrim(SdfPath("/r"), false);
    expectedCl.DidRemoveProperty(SdfPath("/p.propP"), false);
    expectedCl.DidRemovePrim(SdfPath("/a"), true);
    expectedCl.DidAddPrim(SdfPath("/n"), false);
    expectedCl.DidChangeInfo(SdfPath("/n"), SdfFieldKeys->Specifier, 
        std::move(VtValue()), VtValue(SdfSpecifierDef));
    expectedCl.DidChangeInfo(SdfPath("/r.propR"), SdfFieldKeys->Default, 
        std::move(VtValue(1)), VtValue());
    expectedCl.DidAddProperty(SdfPath("/n.propN"), true);
    expectedCl.DidChangeInfo(SdfPath("/n.propN"), 
        SdfFieldKeys->TypeName, std::move(VtValue()), VtValue("int"));
    expectedCl.DidChangeInfo(SdfPath("/n.propN"), SdfFieldKeys->Default, 
        std::move(VtValue()), VtValue(1));
    expectedCl.DidChangeInfo(SdfPath("/n.propN"), SdfFieldKeys->Custom, 
        std::move(VtValue()), VtValue(0));
    expectedCl.DidChangeInfo(SdfPath("/n.propN"), 
        SdfFieldKeys->Variability, std::move(VtValue()), 
        VtValue(SdfVariabilityVarying));
    expectedCl.DidChangeInfo(SdfPath("/p.propP"), SdfFieldKeys->Default, 
        std::move(VtValue(1)), VtValue());
    expectedCl.DidAddPrim(SdfPath("/z"), false);
    expectedCl.DidChangeInfo(SdfPath("/z"), SdfFieldKeys->Specifier, 
        std::move(VtValue()), VtValue(SdfSpecifierDef));
    expectedCl.DidChangeInfo(SdfPath("/c.propC"), SdfFieldKeys->Default, 
        std::move(VtValue(1)), VtValue(2));

    // Copy the layer so we can verify it does not change during the operation
    SdfLayerRefPtr expectedLayer = SdfLayer::CreateAnonymous();
    expectedLayer->TransferContent(actualLayer);

    // Ensure that the layer remains unchanged during the process
    std::string actualLayerStr, expectedLayerStr;
    TF_AXIOM(expectedLayer->ExportToString(&expectedLayerStr));

    SdfChangeList actualCl = actualLayer->CreateDiff(diffLayer, 
            /*compareFieldValues*/ true);

    TF_AXIOM(expectedLayer->ExportToString(&actualLayerStr));
    TF_AXIOM(actualLayerStr == expectedLayerStr);

    _CompareChangeLists(expectedCl, actualCl);

}

static std::pair<SdfLayerRefPtr, SdfLayerRefPtr>
_CreateTimeSampleLayerDiffTestLayers()
{
    SdfLayerRefPtr layerA = SdfLayer::CreateAnonymous();
    layerA->ImportFromString(    
        R"(#sdf 1.4.32
            def Sphere "PixarBall"
            {
                double radius = 100
                double radius.timeSamples = {
                    1: 100,
                    24: 500,
                }
            }
        )"
    );

    SdfLayerRefPtr layerB = SdfLayer::CreateAnonymous();
    layerB->ImportFromString(    
        R"(#sdf 1.4.32
            def Sphere "PixarBall"
            {
                double radius = 100
                double radius.timeSamples = {
                    1: 50,
                    48: 1000,
                }
            }
        )"
    );

    return std::make_pair(layerA, layerB);
}

static void 
_testSdfLayerCreateDiffTimeSamplesWithoutValues()
{
    const auto [layerA, layerB] = _CreateTimeSampleLayerDiffTestLayers();

    SdfChangeList expectedCl;
    expectedCl.DidRemoveProperty(SdfPath("/PixarBall.radius"), false);
    expectedCl.DidAddProperty(SdfPath("/PixarBall.radius"), false);

    SdfChangeList actualCl = layerA->CreateDiff(layerB, 
            /*compareFieldValues*/ false);

    _CompareChangeLists(expectedCl, actualCl);
}

static void 
_testSdfLayerCreateDiffTimeSamplesWithValues()
{
    const auto [layerA, layerB] = _CreateTimeSampleLayerDiffTestLayers();

    SdfChangeList expectedCl;
    SdfTimeSampleMap samplesA = {{1, VtValue(100)}, {24, VtValue(500)}};
    SdfTimeSampleMap samplesB = {{1, VtValue(50)}, {48, VtValue(1000)}};
    expectedCl.DidChangeInfo(SdfPath("/PixarBall.radius"), 
        SdfFieldKeys->TimeSamples, std::move(VtValue(samplesA)), 
        VtValue(samplesB));
    expectedCl.DidChangeAttributeTimeSamples(SdfPath("/PixarBall.radius"));

    SdfChangeList actualCl = layerA->CreateDiff(
        layerB, /*compareFieldValues*/ true);

    _CompareChangeLists(expectedCl, actualCl);
}

static void 
_TestSdfChangeManagerExtractLocalChanges()
{
    struct Listener : public TfWeakBase
    {
        void LayersDidChange(const SdfNotice::LayersDidChange& change)
        {
            invocations += 1;
        }

        Listener()
        {
            _key = TfNotice::Register(
                TfCreateWeakPtr(this), &Listener::LayersDidChange );
        }

        ~Listener()
        {
            TfNotice::Revoke(_key);
        }

        TfNotice::Key _key;
        size_t invocations = 0;
    };

    SdfLayerRefPtr testLayer = SdfLayer::CreateAnonymous();
    Listener listener;

    // This block should trigger an invocation of the listener
    {
        SdfChangeBlock block;
        SdfCreatePrimInLayer(testLayer, SdfPath("/test1"));
    }

    TF_AXIOM(listener.invocations == 1);

    // There should be no additional invocation of the the listener once the
    // the block goes out of scope because the changes for the layer have been
    // extracted.
    {
        SdfChangeBlock block;
        SdfCreatePrimInLayer(testLayer, SdfPath("/test2"));
        SdfChangeList changes = 
            Sdf_ChangeManager::Get().ExtractLocalChanges(testLayer);
        TF_AXIOM(!changes.GetEntryList().empty());
    }

    TF_AXIOM(listener.invocations == 1);
}

static void
_TestSdfLayerDictKeyOps()
{
    std::string txt;

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    SdfPath fooPath("/foo");
    SdfPrimSpecHandle foo = SdfCreatePrimInLayer(layer, fooPath);

    // Set a key in a nested dict.
    layer->SetFieldDictValueByKey(fooPath, SdfFieldKeys->CustomData,
                                  TfToken("hello:world"), std::string("value"));

    // Obtain the whole dict and check the key was set correctly.
    VtValue dictVal = layer->GetField(fooPath, SdfFieldKeys->CustomData);
    TF_AXIOM(dictVal.IsHolding<VtDictionary>());
    VtDictionary dict = dictVal.Get<VtDictionary>();
    TF_AXIOM(dict.GetValueAtPath("hello:world"));
    TF_AXIOM(*dict.GetValueAtPath("hello:world") == VtValue("value"));

    // Get the one value through Sdf API.
    TF_AXIOM(layer->GetFieldDictValueByKey(
                 fooPath, SdfFieldKeys->CustomData, TfToken("hello:world")) ==
             VtValue("value"));

    // Erase the key through the Sdf API.
    layer->EraseFieldDictValueByKey(
        fooPath, SdfFieldKeys->CustomData, TfToken("hello:world"));

    TF_AXIOM(layer->GetField(fooPath, SdfFieldKeys->CustomData).IsEmpty());
}

static void
_TestSdfLayerTimeSampleValueType()
{
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    SdfPrimSpecHandle foo = SdfCreatePrimInLayer(layer, SdfPath("/foo"));
    SdfAttributeSpecHandle attr =
        SdfAttributeSpec::New(foo, "attr", SdfValueTypeNames->Double);

    double value = 0.0;
    VtValue vtValue;

    // Set a double time sample into the double-valued attribute and
    // ensure that we get the same value back and that it maintains its
    // type.
    layer->SetTimeSample<double>(attr->GetPath(), 0.0, 1.0);
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 0.0, &value));
    TF_AXIOM(value == 1.0);
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 0.0, &vtValue));
    TF_AXIOM(vtValue.IsHolding<double>());
    TF_AXIOM(vtValue.UncheckedGet<double>() == 1.0);
    
    layer->SetTimeSample(attr->GetPath(), 1.0, VtValue(double(2.0)));
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 1.0, &value));
    TF_AXIOM(value == 2.0);
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 1.0, &vtValue));
    TF_AXIOM(vtValue.IsHolding<double>());
    TF_AXIOM(vtValue.UncheckedGet<double>() == 2.0);

    // Now try setting a float into the double-valued attribute.
    // The value should be converted to a double, and that's how
    // we should get it back.
    layer->SetTimeSample<float>(attr->GetPath(), 3.0, 3.0);
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 3.0, &value));
    TF_AXIOM(value == 3.0);
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 3.0, &vtValue));
    TF_AXIOM(vtValue.IsHolding<double>());
    TF_AXIOM(vtValue.UncheckedGet<double>() == 3.0);
    
    layer->SetTimeSample(attr->GetPath(), 4.0, VtValue(float(4.0)));
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 4.0, &value));
    TF_AXIOM(value == 4.0);
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 4.0, &vtValue));
    TF_AXIOM(vtValue.IsHolding<double>());
    TF_AXIOM(vtValue.UncheckedGet<double>() == 4.0);

    // Ensure time samples can be set and retrieved directly on
    // attributes themselves.
    attr->SetTimeSample(5.0, 5.0);
    TF_AXIOM(attr->QueryTimeSample(5.0, &value));
    TF_AXIOM(value == 5.0);
    TF_AXIOM(attr->GetNumTimeSamples() == 5);
    TF_AXIOM(attr->QueryTimeSample(4.0, &value));
    TF_AXIOM(value == 4.0);
}

static void
_TestSdfLayerTransferContentsEmptyLayer()
{
    // Tests that setting data on non empty layers properly cleans up all
    // specs in that layer without the use of SdfLayer::_IsInertSubtree
    const char* layerStr = 
    R"(#sdf 1.4.32
    def "Root"{
        def "Node1" (
            prepend variantSets = "testVariants"
            variants = { string testVariants = "option1" }
        )
        {
            variantSet "testVariants" = {
                "option1" {
                    def "VariantChild" {}
                }
            }
            def "Node1Child" {}
        }
    })";

    SdfLayerRefPtr srcLayer = SdfLayer::CreateAnonymous();
    srcLayer->ImportFromString(layerStr);
    TF_AXIOM(!srcLayer->IsEmpty());
    
    srcLayer->TransferContent(SdfLayer::CreateAnonymous());
    TF_AXIOM(srcLayer->IsEmpty());
}

static void
_TestSdfLayerTransferContents()
{
    // Test for bug where transferring an empty over (an inert spec)
    // from a layer would be registered as the addition of a non-inert 
    // spec.
    struct _ChangeListener : public TfWeakBase
    {
    public:
        _ChangeListener()
        {
            TfNotice::Register(
                TfCreateWeakPtr(this), &_ChangeListener::OnChangeNotice);
        }

        void OnChangeNotice(const SdfNotice::LayersDidChange& n)
        {
            changeListVec = n.GetChangeListVec();
        }

        SdfLayerChangeListVec changeListVec;
    };

    const SdfPath fooPath("/Foo");
    SdfLayerRefPtr srcLayer = SdfLayer::CreateAnonymous();
    SdfCreatePrimInLayer(srcLayer, fooPath);

    _ChangeListener l;
    SdfLayerRefPtr dstLayer = SdfLayer::CreateAnonymous();
    dstLayer->TransferContent(srcLayer);

    auto iter = std::find_if(
        l.changeListVec.begin(),
        l.changeListVec.end(),
        [&dstLayer](SdfLayerChangeListVec::value_type const &p) {
            return p.first == dstLayer;
        });
    TF_AXIOM(iter != l.changeListVec.end());
   
    auto entryIter = iter->second.FindEntry(fooPath);
    TF_AXIOM(entryIter != iter->second.end());
    TF_AXIOM(entryIter->second.flags.didAddInertPrim);
}

static void
_TestSdfRelationshipTargetSpecEdits()
{
    // Test for a subtle bug where relationship target specs were not
    // being properly created when using the prepended/appended form.
    // (Testingin C++ because rel target specs are not accessible from python.)
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    SdfPrimSpecHandle prim = SdfCreatePrimInLayer(layer, SdfPath("/Foo"));
    SdfRelationshipSpecHandle rel = SdfRelationshipSpec::New(prim, "rel");
    rel->GetTargetPathList().Prepend(SdfPath("/Target"));
    TF_AXIOM(layer->GetObjectAtPath(SdfPath("/Foo.rel[/Target]")));

    // XXX Unfortunately, there is another bug where if you add the same
    // target path via multiple lists, then remove it from only one,
    // Sdf_ConnectionListEditor will remove the associated spec even
    // though it should arguably still exist.  See bug 153466.
    // We demonstrate this busted behavior here.
    rel->GetTargetPathList().Append(SdfPath("/Target"));
    TF_AXIOM(layer->GetObjectAtPath(SdfPath("/Foo.rel[/Target]")));
    rel->GetTargetPathList().GetAppendedItems().clear();
    // The target spec should still exist, because it is still in the
    // prepended list, but the appended list proxy has removed it.
    TF_AXIOM(!layer->GetObjectAtPath(SdfPath("/Foo.rel[/Target]")));
}

static void
_TestSdfPathFindLongestPrefix()
{
    std::vector<SdfPath> paths = {
        SdfPath("/"),
        SdfPath("/foo"),
        SdfPath("/foo/bar/baz"),
        SdfPath("/bar/foo"),
        SdfPath("/bar/baz"),
        SdfPath("/qux")
    };

    std::sort(paths.begin(), paths.end());
    
    TF_AXIOM(*SdfPathFindLongestPrefix(
                 paths.begin(), paths.end(),
                 SdfPath("/foo/bar/baz/qux")) == SdfPath("/foo/bar/baz"));
    TF_AXIOM(*SdfPathFindLongestPrefix(
                 paths.begin(), paths.end(),
                 SdfPath("/foo/baz/baz/qux")) == SdfPath("/foo"));
    TF_AXIOM(*SdfPathFindLongestPrefix(
                 paths.begin(), paths.end(),
                 SdfPath("/bar/foo")) == SdfPath("/bar/foo"));
    TF_AXIOM(*SdfPathFindLongestPrefix(
                 paths.begin(), paths.end(),
                 SdfPath("/qux/foo/bar")) == SdfPath("/qux"));
    TF_AXIOM(*SdfPathFindLongestPrefix(
                 paths.begin(), paths.end(),
                 SdfPath("/qix")) == SdfPath("/"));

    TF_AXIOM(*SdfPathFindLongestStrictPrefix(
                 paths.begin(), paths.end(),
                 SdfPath("/foo/bar/baz/qux")) == SdfPath("/foo/bar/baz"));
    TF_AXIOM(*SdfPathFindLongestStrictPrefix(
                 paths.begin(), paths.end(),
                 SdfPath("/foo/baz/baz/qux")) == SdfPath("/foo"));
    TF_AXIOM(*SdfPathFindLongestStrictPrefix(
                 paths.begin(), paths.end(),
                 SdfPath("/bar/foo")) == SdfPath("/"));
    TF_AXIOM(*SdfPathFindLongestStrictPrefix(
                 paths.begin(), paths.end(),
                 SdfPath("/qux/foo/bar")) == SdfPath("/qux"));
    TF_AXIOM(*SdfPathFindLongestStrictPrefix(
                 paths.begin(), paths.end(),
                 SdfPath("/qix")) == SdfPath("/"));

    std::set<SdfPath> pathSet;
    for (SdfPath const &p: paths) {
        pathSet.insert(p);
    }

    TF_AXIOM(*SdfPathFindLongestPrefix(
                 pathSet, SdfPath("/foo/bar/baz/qux")) ==
             SdfPath("/foo/bar/baz"));
    TF_AXIOM(*SdfPathFindLongestPrefix(
                 pathSet, SdfPath("/foo/baz/baz/qux")) ==
             SdfPath("/foo"));
    TF_AXIOM(*SdfPathFindLongestPrefix(
                 pathSet, SdfPath("/bar/foo")) == SdfPath("/bar/foo"));
    TF_AXIOM(*SdfPathFindLongestPrefix(
                 pathSet, SdfPath("/qux/foo/bar")) == SdfPath("/qux"));
    TF_AXIOM(*SdfPathFindLongestPrefix(
                 pathSet, SdfPath("/qix")) == SdfPath("/"));
    
    TF_AXIOM(*SdfPathFindLongestStrictPrefix(
                 pathSet, SdfPath("/foo/bar/baz/qux")) ==
             SdfPath("/foo/bar/baz"));
    TF_AXIOM(*SdfPathFindLongestStrictPrefix(
                 pathSet, SdfPath("/foo/baz/baz/qux")) ==
             SdfPath("/foo"));
    TF_AXIOM(*SdfPathFindLongestStrictPrefix(
                 pathSet, SdfPath("/bar/foo")) == SdfPath("/"));
    TF_AXIOM(*SdfPathFindLongestStrictPrefix(
                 pathSet, SdfPath("/qux/foo/bar")) == SdfPath("/qux"));
    TF_AXIOM(*SdfPathFindLongestStrictPrefix(
                 pathSet, SdfPath("/qix")) == SdfPath("/"));

    std::map<SdfPath, int> pathMap;
    for (SdfPath const &p: paths) {
        pathMap.emplace(p, 0);
    }

    TF_AXIOM(SdfPathFindLongestPrefix(
                 pathMap, SdfPath("/foo/bar/baz/qux"))->first ==
             SdfPath("/foo/bar/baz"));
    TF_AXIOM(SdfPathFindLongestPrefix(
                 pathMap, SdfPath("/foo/baz/baz/qux"))->first ==
             SdfPath("/foo"));
    TF_AXIOM(SdfPathFindLongestPrefix(
                 pathMap, SdfPath("/bar/foo"))->first == SdfPath("/bar/foo"));
    TF_AXIOM(SdfPathFindLongestPrefix(
                 pathMap, SdfPath("/qux/foo/bar"))->first == SdfPath("/qux"));
    TF_AXIOM(SdfPathFindLongestPrefix(
                 pathMap, SdfPath("/qix"))->first == SdfPath("/"));
    
    TF_AXIOM(SdfPathFindLongestStrictPrefix(
                 pathMap, SdfPath("/foo/bar/baz/qux"))->first ==
             SdfPath("/foo/bar/baz"));
    TF_AXIOM(SdfPathFindLongestStrictPrefix(
                 pathMap, SdfPath("/foo/baz/baz/qux"))->first ==
             SdfPath("/foo"));
    TF_AXIOM(SdfPathFindLongestStrictPrefix(
                 pathMap, SdfPath("/bar/foo"))->first == SdfPath("/"));
    TF_AXIOM(SdfPathFindLongestStrictPrefix(
                 pathMap, SdfPath("/qux/foo/bar"))->first == SdfPath("/qux"));
    TF_AXIOM(SdfPathFindLongestStrictPrefix(
                 pathMap, SdfPath("/qix"))->first == SdfPath("/"));
}

static void
_TestSdfFpsAndTcps()
{
    // Test the interplay between framesPerSecond and timeCodesPerSecond, as
    // documented for SdfLayer::GetTimeCodesPerSecond.

    // Listener that records change notices we receive.
    class _ChangeListener : public TfWeakBase
    {
    public:
        _ChangeListener(const SdfLayerHandle &layer)
            : _layer(layer)
        {
            TfNotice::Register(
                TfCreateWeakPtr(this), &_ChangeListener::OnInfoChange, layer);
            TfNotice::Register(
                TfCreateWeakPtr(this), &_ChangeListener::OnLayerChange, layer);
        }

        void OnInfoChange(const SdfNotice::LayerInfoDidChange &n)
        {
            _changedFields.push_back(n.key());
        }

        void OnLayerChange(const SdfNotice::LayersDidChangeSentPerLayer &n)
        {
            _changeListVec = n.GetChangeListVec();
        }

        void ValidateAndClear(
            const VtValue &oldFps,
            const VtValue &newFps,
            const VtValue &oldTcps,
            const VtValue &newTcps)
        {
            // Verify fields have expected new values.
            TF_AXIOM(_layer->GetFramesPerSecond()
                == newFps.GetWithDefault(24.0));
            TF_AXIOM(_layer->GetTimeCodesPerSecond()
                == newTcps.GetWithDefault(24.0));

            // Verify we received expected LayerInfoDidChange notices.
            // These come in a deterministic order if both fields change.
            TfTokenVector expectedFields;
            if (newFps != oldFps) {
                expectedFields.push_back(SdfFieldKeys->FramesPerSecond);
            }
            if (newTcps != oldTcps) {
                expectedFields.push_back(SdfFieldKeys->TimeCodesPerSecond);
            }
            TF_AXIOM(_changedFields == expectedFields);

            // Verify we received a LayersDidChangeSentPerLayer containing
            // changes for the psuedo-root.
            TF_AXIOM(_changeListVec.size() == 1);
            const auto &entryList = _changeListVec[0].second.GetEntryList();
            TF_AXIOM(entryList.size() == 1);
            TF_AXIOM(entryList[0].first == SdfPath::AbsoluteRootPath());
            const auto &entry = entryList[0].second;

            // Verify we did or did not receive change notification for FPS,
            // with expected old and new values.
            const auto &fpsIt =
                entry.FindInfoChange(SdfFieldKeys->FramesPerSecond);
            if (newFps != oldFps) {
                TF_AXIOM(fpsIt != entry.infoChanged.end()
                    && fpsIt->second.first == oldFps
                    && fpsIt->second.second == newFps);
            } else {
                TF_AXIOM(fpsIt == entry.infoChanged.end());
            }

            // Verify we did or did not receive change notification for TCPS,
            // with expected old and new values.
            const auto &tcpsIt =
                entry.FindInfoChange(SdfFieldKeys->TimeCodesPerSecond);
            if (newTcps != oldTcps) {
                TF_AXIOM(tcpsIt != entry.infoChanged.end()
                    && tcpsIt->second.first == oldTcps
                    && tcpsIt->second.second == newTcps);
            } else {
                TF_AXIOM(tcpsIt == entry.infoChanged.end());
            }

            // Clear accumulated notice data.
            _changedFields.clear();
            _changeListVec.clear();
        }

    private:
        const SdfLayerHandle _layer;

        TfTokenVector _changedFields;
        SdfLayerChangeListVec _changeListVec;
    };

    // Create layer and listener.
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    _ChangeListener listener(layer);

    // Verify initial state.
    TF_AXIOM(layer->GetFramesPerSecond() == 24);
    TF_AXIOM(layer->GetTimeCodesPerSecond() == 24);

    // Add FPS, verify both fields change.
    layer->SetFramesPerSecond(30);
    listener.ValidateAndClear(
        VtValue(), VtValue(30.0),
        VtValue(), VtValue(30.0));

    // Add TCPS, verify fields take on separate values.
    layer->SetTimeCodesPerSecond(1000);
    listener.ValidateAndClear(
        VtValue(30.0), VtValue(30.0),
        VtValue(30.0), VtValue(1000.0));

    // Change FPS, verify only FPS changes.
    layer->SetFramesPerSecond(48);
    listener.ValidateAndClear(
        VtValue(30.0), VtValue(48.0),
        VtValue(1000.0), VtValue(1000.0));

    // Remove TCPS, verify return to dynamic fallback from FPS.
    layer->ClearTimeCodesPerSecond();
    listener.ValidateAndClear(
        VtValue(48.0), VtValue(48.0),
        VtValue(1000.0), VtValue(48.0));

    // Remove FPS, verify return to initial state.
    layer->ClearFramesPerSecond();
    listener.ValidateAndClear(
        VtValue(48.0), VtValue(),
        VtValue(48.0), VtValue());
}

static void
_TestSdfSchemaPathValidation()
{
    const SdfSchema &schema = SdfSchema::GetInstance();

    // Exercise path restrictions on fields used for composition arcs.

    TF_AXIOM(schema.IsValidInheritPath(SdfPath("/A")));
    TF_AXIOM(!schema.IsValidInheritPath(SdfPath()));
    TF_AXIOM(!schema.IsValidInheritPath(SdfPath("/A.a")));
    TF_AXIOM(!schema.IsValidInheritPath(SdfPath("A")));
    TF_AXIOM(!schema.IsValidInheritPath(SdfPath("/A{x=y}")));
    TF_AXIOM(!schema.IsValidInheritPath(SdfPath("/A{x=y}B")));

    TF_AXIOM(schema.IsValidSpecializesPath(SdfPath("/A")));
    TF_AXIOM(!schema.IsValidSpecializesPath(SdfPath()));
    TF_AXIOM(!schema.IsValidSpecializesPath(SdfPath("/A.a")));
    TF_AXIOM(!schema.IsValidSpecializesPath(SdfPath("A")));
    TF_AXIOM(!schema.IsValidSpecializesPath(SdfPath("/A{x=y}")));
    TF_AXIOM(!schema.IsValidSpecializesPath(SdfPath("/A{x=y}B")));

    TF_AXIOM(schema.IsValidPayload(SdfPayload("a.sdf", SdfPath())));
    TF_AXIOM(schema.IsValidPayload(SdfPayload("a.sdf", SdfPath("/A"))));
    TF_AXIOM(schema.IsValidPayload(SdfPayload("", SdfPath("/A"))));
    TF_AXIOM(!schema.IsValidPayload(SdfPayload("a.sdf", SdfPath("/A.a"))));
    TF_AXIOM(!schema.IsValidPayload(SdfPayload("a.sdf", SdfPath("A"))));
    TF_AXIOM(!schema.IsValidPayload(SdfPayload("a.sdf", SdfPath("/A{x=y}"))));
    TF_AXIOM(!schema.IsValidPayload(SdfPayload("a.sdf", SdfPath("/A{x=y}B"))));

    TF_AXIOM(schema.IsValidReference(SdfReference("a.sdf", SdfPath())));
    TF_AXIOM(schema.IsValidReference(SdfReference("a.sdf", SdfPath("/A"))));
    TF_AXIOM(schema.IsValidReference(SdfReference("", SdfPath("/A"))));
    TF_AXIOM(!schema.IsValidReference(SdfReference("a.sdf", SdfPath("/A.a"))));
    TF_AXIOM(!schema.IsValidReference(SdfReference("a.sdf", SdfPath("A"))));
    TF_AXIOM(!schema.IsValidReference(SdfReference("a.sdf",
                                                   SdfPath("/A{x=y}"))));
    TF_AXIOM(!schema.IsValidReference(SdfReference("a.sdf", 
                                                   SdfPath("/A{x=y}B"))));

    TF_AXIOM(schema.IsValidRelocatesSourcePath(SdfPath("A")));
    TF_AXIOM(schema.IsValidRelocatesSourcePath(SdfPath("/A")));
    TF_AXIOM(schema.IsValidRelocatesSourcePath(SdfPath("/A/B")));
    TF_AXIOM(!schema.IsValidRelocatesSourcePath(SdfPath()));
    TF_AXIOM(!schema.IsValidRelocatesSourcePath(SdfPath("/A.a")));
    TF_AXIOM(!schema.IsValidRelocatesSourcePath(SdfPath("/A{x=y}")));
    TF_AXIOM(!schema.IsValidRelocatesSourcePath(SdfPath("/A{x=y}B")));

    // IsValidRelocatesTargetPath is the same as IsValidRelocatesSourcePath
    // except that the empty path is allowed for target paths.
    TF_AXIOM(schema.IsValidRelocatesTargetPath(SdfPath("A")));
    TF_AXIOM(schema.IsValidRelocatesTargetPath(SdfPath("/A")));
    TF_AXIOM(schema.IsValidRelocatesTargetPath(SdfPath("/A/B")));
    TF_AXIOM(schema.IsValidRelocatesTargetPath(SdfPath()));
    TF_AXIOM(!schema.IsValidRelocatesTargetPath(SdfPath("/A.a")));
    TF_AXIOM(!schema.IsValidRelocatesTargetPath(SdfPath("/A{x=y}")));
    TF_AXIOM(!schema.IsValidRelocatesTargetPath(SdfPath("/A{x=y}B")));
}

static void 
_TestSdfMapEditorProxyOperators()
{
    // SdfVariantSelectionProxy is used as a test here since it is
    // typedef'd to std::map<std::string, std::string> which has
    // the required operators defined for comparison

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    SdfPrimSpecHandle prim = SdfCreatePrimInLayer(layer, SdfPath("/Test"));

    SdfVariantSelectionProxy validProxy = prim->GetVariantSelections();
    TF_AXIOM(validProxy);

    // Two invalid SdfMapEditProxy objects should always compare equal.
    SdfVariantSelectionProxy invalidProxyA, invalidProxyB;
    TF_AXIOM((!invalidProxyA) && (!invalidProxyB));

    TF_AXIOM(invalidProxyA == invalidProxyB);
    TF_AXIOM(!(invalidProxyA != invalidProxyA));

    // Invalid proxy should not compare equal to a valid one
    TF_AXIOM(invalidProxyA != validProxy);
    TF_AXIOM(!(invalidProxyA == validProxy));

    // Invalid SdfMapEditProxy objects should always compare less to an object
    // of their map type
    std::map<std::string, std::string> testMap = {{"key", "value"}};

    TF_AXIOM(!(invalidProxyA == testMap));
    TF_AXIOM(invalidProxyA != testMap);
    TF_AXIOM(invalidProxyA < testMap);
    TF_AXIOM(invalidProxyA <= testMap);
    TF_AXIOM(!(invalidProxyA > testMap));
    TF_AXIOM(!(invalidProxyA >= testMap));

    TF_AXIOM(!(testMap == invalidProxyA));
    TF_AXIOM(testMap != invalidProxyA);
    TF_AXIOM(!(testMap < invalidProxyA));
    TF_AXIOM(!(testMap <= invalidProxyA));
    TF_AXIOM(testMap > invalidProxyA);
    TF_AXIOM(testMap >= invalidProxyA);
}

static void 
_TestSdfAbstractDataValue()
{
    int i = 123;

    SdfAbstractDataTypedValue<int> a(&i);

    TF_AXIOM(a.valueType == typeid(int));
    TF_AXIOM(!a.isValueBlock);
    TF_AXIOM(!a.typeMismatch);

    // Store a different value of the correct type.
    a.StoreValue(234);
    TF_AXIOM(i == 234);
    TF_AXIOM(!a.isValueBlock);
    TF_AXIOM(!a.typeMismatch);

    // Store via VtValue.
    a.StoreValue(VtValue { 345 });
    TF_AXIOM(i == 345);
    TF_AXIOM(!a.isValueBlock);
    TF_AXIOM(!a.typeMismatch);

    // Store an incorrect type.
    a.StoreValue(1.234);
    TF_AXIOM(i == 345);
    TF_AXIOM(!a.isValueBlock);
    TF_AXIOM(a.typeMismatch);

    // Store the correct type again, this should clear the `typeMismatch` flag.
    a.StoreValue(456);
    TF_AXIOM(i == 456);
    TF_AXIOM(!a.isValueBlock);
    TF_AXIOM(!a.typeMismatch);

    // Store an incorrect type via VtValue.
    a.StoreValue(VtValue { 1.234 });
    TF_AXIOM(i == 456);
    TF_AXIOM(!a.isValueBlock);
    TF_AXIOM(a.typeMismatch);
    
    // Store the correct type via VtValue.
    a.StoreValue(VtValue { 567 });
    TF_AXIOM(i == 567);
    TF_AXIOM(!a.isValueBlock);
    TF_AXIOM(!a.typeMismatch);

    // Store a value block.
    a.StoreValue(SdfValueBlock {});
    TF_AXIOM(!a.typeMismatch);
    TF_AXIOM(a.isValueBlock);

    // Store a non-block, then store a block via VtValue.
    a.StoreValue(678);
    TF_AXIOM(i == 678);
    TF_AXIOM(!a.isValueBlock);
    a.StoreValue(VtValue { SdfValueBlock {} });
    TF_AXIOM(!a.typeMismatch);
    TF_AXIOM(a.isValueBlock);
}

int
main(int argc, char **argv)
{
    _TestSdfChangeManagerExtractLocalChanges();
    _TestSdfLayerCreateDiffChangeListWithoutValues();
    _TestSdfLayerCreateDiffChangeListWithValues();
    _testSdfLayerCreateDiffTimeSamplesWithValues();
    _testSdfLayerCreateDiffTimeSamplesWithoutValues();
    _TestSdfLayerDictKeyOps();
    _TestSdfLayerTimeSampleValueType();
    _TestSdfLayerTransferContents();
    _TestSdfLayerTransferContentsEmptyLayer();
    _TestSdfRelationshipTargetSpecEdits();
    _TestSdfPathFindLongestPrefix();
    _TestSdfFpsAndTcps();
    _TestSdfSchemaPathValidation();
    _TestSdfMapEditorProxyOperators();
    _TestSdfAbstractDataValue();

    return 0;
}
