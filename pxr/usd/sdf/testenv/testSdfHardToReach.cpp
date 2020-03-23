//
// Copyright 2017 Pixar
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
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/notice.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/schema.h"

#include <map>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

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

int
main(int argc, char **argv)
{
    _TestSdfLayerDictKeyOps();
    _TestSdfLayerTimeSampleValueType();
    _TestSdfLayerTransferContents();
    _TestSdfRelationshipTargetSpecEdits();
    _TestSdfPathFindLongestPrefix();
    _TestSdfFpsAndTcps();

    return 0;
}
