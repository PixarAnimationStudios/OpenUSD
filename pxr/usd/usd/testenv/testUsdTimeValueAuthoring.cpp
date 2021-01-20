//
// Copyright 2019 Pixar
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
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

// Tests to ensure the following templated APIs are checked for time based 
// values:
//   UsdObject::GetMetadata / SetMetadata
//   UsdAttribute::Get / Set
//   UsdAttributeQuery::Get
//
// These tests verify that all time based values are resolved across layers by
// layer offsets both when getting the resolved value as well as when setting 
// values on specific layers using edit targets. This test should be almost 
// identical to testUsdTimeValueAuthoring.py except this uses the templated C++ 
// APIs that are inaccessible through python. testUsdTimeValueAuthoring.py 
// handles all testing of the type erased (i.e. VtValue) version of this API.

using SdfTimeCodeArray = VtArray<SdfTimeCode>;
using _EditTargets = std::array < UsdEditTarget, 4>;

template <class T>
static void
_GetAndVerifyMetadata(const UsdObject &obj, const TfToken &field, const T& expected)
{
    T value;
    TF_AXIOM(obj.GetMetadata(field, &value));
    TF_AXIOM(value == expected);
}

template <class T>
static void
_GetAndVerifyMetadata(const UsdStagePtr &stage, const TfToken &field, const T& expected)
{
    T value;
    TF_AXIOM(stage->GetMetadata(field, &value));
    TF_AXIOM(value == expected);
}

template <class T>
static void
_GetAndVerifyMetadataByDictKey(const UsdObject &obj, const TfToken &field, 
                               const TfToken &key, const T& expected)
{
    T value;
    TF_AXIOM(obj.GetMetadataByDictKey(field, key, &value));
    TF_AXIOM(value == expected);
}

// Creates the edit target for each layer composed into the root stage with
// its correct composition map function.
static _EditTargets 
_GetEditTargets(const UsdPrim &prim)
{
    _EditTargets editTargets;
    const PcpPrimIndex &primIndex = prim.GetPrimIndex();
    const PcpNodeRef rootNode = primIndex.GetRootNode();
    const PcpNodeRef refNode = 
        *(primIndex.GetNodeRange(PcpRangeTypeReference).first);

    SdfLayerHandle rootLayer = 
        SdfLayer::Find("timeCodes/root.usda");
    SdfLayerHandle rootSubLayer = 
        SdfLayer::Find("timeCodes/root_sub.usda");
    SdfLayerHandle refLayer = 
        SdfLayer::Find("timeCodes/ref.usda");
    SdfLayerHandle refSubLayer = 
        SdfLayer::Find("timeCodes/ref_sub.usda");
    TF_AXIOM(rootLayer);
    TF_AXIOM(rootSubLayer);
    TF_AXIOM(refLayer);
    TF_AXIOM(refSubLayer);

    // Edit targets are returned in order from weakest layer to strongest layer
    // as that is the order in which we want use them when setting values.

    // Composed layer offset: scale = 2, offset = +3.0
    editTargets[0] = UsdEditTarget(refSubLayer, refNode);
    // Composed layer offset: scale = 2, offset = -3.0
    editTargets[1] = UsdEditTarget(refLayer, refNode);
    // Composed layer offset: scale = 0.5
    editTargets[2] = UsdEditTarget(rootSubLayer, rootNode);
    // No mapping
    editTargets[3] = UsdEditTarget(rootLayer);
    TF_AXIOM(editTargets[0].GetLayer() == refSubLayer);
    TF_AXIOM(editTargets[1].GetLayer() == refLayer);
    TF_AXIOM(editTargets[2].GetLayer() == rootSubLayer);
    TF_AXIOM(editTargets[3].GetLayer() == rootLayer);

    return editTargets;
}

// Verify that a value authored to the edit target exists on the correct
// spec on the target's layer and matches the expected value.
template <class T> 
static void _VerifyAuthoredValue(const UsdEditTarget &editTarget,
                                 const SdfPath &objPath, 
                                 const TfToken &fieldName, 
                                 const T &expectedValue)
{
    SdfSpecHandle spec = editTarget.GetSpecForScenePath(objPath);
    TF_AXIOM(spec->GetLayer() == editTarget.GetLayer());
    T authoredValue;
    TF_AXIOM(spec->HasField(fieldName, &authoredValue));
    TF_AXIOM(authoredValue == expectedValue);
}


// Sets the value for a metadata field of a prim or attribute using the
// given edit target and verifies the resolved and authored values.
template <class T>
static void _SetMetadataWithEditTarget(const UsdStagePtr &stage,
                                       const UsdEditTarget &editTarget, 
                                       const UsdObject &obj, 
                                       const TfToken &fieldName, 
                                       const T &resolvedValue,
                                       const T &expectedAuthoredValue)
{
    // Set the edit target on the stage.
    stage->SetEditTarget(editTarget);
    // Set the metadata field to the resolved value and verify that
    // GetMetadata returns the resolved value.
    obj.SetMetadata(fieldName, resolvedValue);
    _GetAndVerifyMetadata(obj, fieldName, resolvedValue);
    // Verify that the value authored on the edit target's layer matches
    // the expected authored value.
    _VerifyAuthoredValue(editTarget, obj.GetPath(), fieldName,
                         expectedAuthoredValue);
}

// Sets the value for a particular key in a dictionary metadata field of
// a prim or attribute using the given edit target and verifies the
// resolved and authored values.
template <class T>
static void _SetMetadataByKeyWithEditTarget(
    const UsdStagePtr &stage,
    const UsdEditTarget &editTarget, 
    const UsdObject &obj, 
    const TfToken &fieldName,
    const TfToken &key,
    const T &resolvedValue,
    const VtDictionary &expectedAuthoredValue)
{
    // Set the edit target on the stage.
    stage->SetEditTarget(editTarget);
    // Set the value at key for the metadata field to the resolved value
    // and verify that GetMetadataByDictKey returns the resolved value.
    obj.SetMetadataByDictKey(fieldName, key, resolvedValue);
    _GetAndVerifyMetadataByDictKey(obj, fieldName, key, resolvedValue);
    // Verify that the value authored on the edit target's layer matches
    // the expected authored value.
    _VerifyAuthoredValue(editTarget, obj.GetPath(), fieldName,
                         expectedAuthoredValue);
}

template <class T>
static void
_GetAndVerifyAttributeValue(const UsdAttribute &attr, const UsdTimeCode &time, 
                            const T& expected)
{
    T value, queryValue;
    TF_AXIOM(attr.Get(&value, time));
    TF_AXIOM(value == expected);

    // Note that we create the attribute query in this function for the
    // same attribute because we're making changes that affect value
    // resolution
    UsdAttributeQuery attrQuery(attr);
    TF_AXIOM(attrQuery.Get(&queryValue, time));
    TF_AXIOM(queryValue == expected);
}


// Sets the value at time for an attribute using the given edit target
// and verifies the resolved and authored values.
template <class T>
static void _SetTimeSampleWithEditTarget(
    const UsdStagePtr &stage,
    const UsdEditTarget &editTarget, 
    const UsdAttribute &attr, 
    double time, 
    const T &resolvedValue,
    const SdfTimeSampleMap &expectedAuthoredValue)
{
    // Set the edit target on the stage.
    stage->SetEditTarget(editTarget);
    // Set the value at time to the resolved value and verify we get the
    // same resolved value back from both the attr and a
    // UsdAttributeQuery
    attr.Set(resolvedValue, time);
    _GetAndVerifyAttributeValue(attr, time, resolvedValue);
    // Verify that timeSamples authored on the edit target's layer
    // matches the expected authored value.
    _VerifyAuthoredValue(editTarget, attr.GetPath(), SdfFieldKeys->TimeSamples,
                         expectedAuthoredValue);
}

// Sets the default value for an attribute using the given edit target
// and verifies the resolved and authored values.
template <class T>
static void _SetDefaultWithEditTarget(
    const UsdStagePtr &stage,
    const UsdEditTarget &editTarget, 
    const UsdAttribute &attr, 
    const T &resolvedValue,
    const T &expectedAuthoredValue)
{
    // Set the edit target on the stage.
    stage->SetEditTarget(editTarget);
    // Set the value at time to the resolved value and verify we get the
    // same resolved value back from both the attr and a
    // UsdAttributeQuery
    attr.Set(resolvedValue);
    _GetAndVerifyAttributeValue(attr, UsdTimeCode::Default(), 
                                resolvedValue);
    // Verify that the default value authored on the edit target's layer
    // matches the expected authored value.
    _VerifyAuthoredValue(editTarget, attr.GetPath(), SdfFieldKeys->Default,
                         expectedAuthoredValue);
}

// Sets the value for a metadata field of a prim or attribute using to
// the same resolved value using each of the available edit targets
// in turn and verifies the resolved and authored values.
template <class T>
static void _SetMetadataWithEachEditTarget(
    const UsdStagePtr &stage,
    const _EditTargets &editTargets, 
    const UsdObject &obj, 
    const TfToken &fieldName, 
    const T &resolvedValue,
    const std::vector<T> &expectedAuthoredValues)
{
    // We set the value using each edit target in order from weakest
    // layer to strongest layer.
    TF_AXIOM(expectedAuthoredValues.size() == editTargets.size());
    for (size_t i = 0; i < editTargets.size(); ++ i) {
        _SetMetadataWithEditTarget(
            stage, editTargets[i], obj, fieldName, resolvedValue,
            expectedAuthoredValues[i]);
    }
}

// Sets the value for a particular key in a dictionary metadata field of
// a prim or attribute using each of the available edit targets in turn
// and verifies the resolved and authored values.
template <class T>
static void _SetMetadataByKeyWithEachEditTarget(
    const UsdStagePtr &stage,
    const _EditTargets &editTargets, 
    const UsdObject &obj, 
    const TfToken &fieldName, 
    const TfToken &key,
    const T &resolvedValue,
    const std::vector<VtDictionary> &expectedAuthoredValues)
{
    // We set the value using each edit target in order from weakest
    // layer to strongest layer.
    TF_AXIOM(expectedAuthoredValues.size() == editTargets.size());
    for (size_t i = 0; i < editTargets.size(); ++ i) {
        _SetMetadataByKeyWithEditTarget(
            stage, editTargets[i], obj, fieldName, key, resolvedValue,
            expectedAuthoredValues[i]);
    }
}

// Sets the value at time for an attribute using the same resolved value
// using each of the available edit targets in turn and verifies the
// resolved and authored values.
template <class T>
static void _SetTimeSampleWithEachEditTarget(
    const UsdStagePtr &stage,
    const _EditTargets &editTargets, 
    const UsdAttribute &attr, 
    double time, 
    const T &resolvedValue,
    const std::vector<SdfTimeSampleMap> &expectedAuthoredValues)
{
    // We set the value using each edit target in order from weakest
    // layer to strongest layer.
    TF_AXIOM(expectedAuthoredValues.size() == editTargets.size());
    for (size_t i = 0; i < editTargets.size(); ++ i) {
        _SetTimeSampleWithEditTarget(
            stage, editTargets[i], attr, time, resolvedValue,
            expectedAuthoredValues[i]);
    }
}

// Sets the default value for an attribute using the same resolved value
// using each of the available edit targets in turn and verifies the
// resolved and authored values.
template <class T>
static void _SetDefaultWithEachEditTarget(
    const UsdStagePtr &stage,
    const _EditTargets &editTargets, 
    const UsdAttribute &attr, 
    const T &resolvedValue,
    const std::vector<T> &expectedAuthoredValues)
{
    // We set the value using each edit target in order from weakest
    // layer to strongest layer.
    TF_AXIOM(expectedAuthoredValues.size() == editTargets.size());
    for (size_t i = 0; i < editTargets.size(); ++ i) {
        _SetDefaultWithEditTarget(
            stage, editTargets[i], attr, resolvedValue,
            expectedAuthoredValues[i]);
    }
}

// Tests GetMetadata functions on time code valued fields when there
// are no layer offsets present.
static void _TestGetMetadataNoOffsets()
{
    // Create a stage from the ref_sub layer. All opinions are authored on
    // this layer so we can get all resolved values without the affect of
    // layer offsets. Metadata fields will all be returned as authored in
    // this test case.
    UsdStageRefPtr s = UsdStage::Open("./timeCodes/ref_sub.usda");
    TF_AXIOM(s);
    UsdPrim prim = s->GetPrimAtPath(SdfPath("/TimeCodeTest"));

    // Test attribute metadata resolution
    UsdAttribute timeAttr = prim.GetAttribute(TfToken("TimeCode"));
    UsdAttribute arrayAttr = prim.GetAttribute(TfToken("TimeCodeArray"));
    UsdAttribute doubleAttr = prim.GetAttribute(TfToken("Double"));

    const TfToken timeCodeTest("timeCodeTest");
    const TfToken timeCodeArrayTest("timeCodeArrayTest");
    const TfToken doubleTest("doubleTest");

    // Attribute timeSamples metadata.
    _GetAndVerifyMetadata(timeAttr, SdfFieldKeys->TimeSamples, 
        SdfTimeSampleMap({{0.0, VtValue(SdfTimeCode(10.0))},
                          {1.0, VtValue(SdfTimeCode(20.0))}}));
    _GetAndVerifyMetadata(arrayAttr, SdfFieldKeys->TimeSamples, 
        SdfTimeSampleMap({{0.0, VtValue(SdfTimeCodeArray({10.0, 30.0}))},
                          {1.0, VtValue(SdfTimeCodeArray({20.0, 40.0}))}}));
    _GetAndVerifyMetadata(doubleAttr, SdfFieldKeys->TimeSamples, 
        SdfTimeSampleMap({{0.0, VtValue(10.0)},
                          {1.0, VtValue(20.0)}}));

    // Attribute default metadata.
    _GetAndVerifyMetadata(timeAttr, SdfFieldKeys->Default, SdfTimeCode(10.0));
    _GetAndVerifyMetadata(arrayAttr, SdfFieldKeys->Default, 
                          SdfTimeCodeArray({10.0, 20.0}));
    _GetAndVerifyMetadata(doubleAttr, SdfFieldKeys->Default, 10.0);

    // Test prim metadata resolution
    _GetAndVerifyMetadata(prim, timeCodeTest, SdfTimeCode(10.0));
    _GetAndVerifyMetadata(prim, timeCodeArrayTest, 
                          SdfTimeCodeArray({10.0, 20.0}));
    _GetAndVerifyMetadata(prim, doubleTest, 10.0);

    // Prim customData retrieved as the full dictionary
    VtDictionary expectedCustomData({
        {"timeCode", VtValue(SdfTimeCode(10.0))},
        {"timeCodeArray", VtValue(SdfTimeCodeArray({10.0, 20.0}))},
        {"doubleVal", VtValue(10.0)},
        {"subDict", VtValue(VtDictionary({
            {"timeCode", VtValue(SdfTimeCode(10.0))},
            {"timeCodeArray", VtValue(SdfTimeCodeArray({10.0, 20.0}))},
            {"doubleVal", VtValue(10.0)}}))}});

    _GetAndVerifyMetadata(prim, SdfFieldKeys->CustomData, expectedCustomData);

    // Also test getting customData values by dict key.
    _GetAndVerifyMetadataByDictKey(prim, SdfFieldKeys->CustomData, 
        TfToken("timeCode"), SdfTimeCode(10.0));
    _GetAndVerifyMetadataByDictKey(prim, SdfFieldKeys->CustomData, 
        TfToken("timeCodeArray"), SdfTimeCodeArray({10.0, 20.0}));
    _GetAndVerifyMetadataByDictKey(prim, SdfFieldKeys->CustomData, 
        TfToken("doubleVal"), 10.0);

    _GetAndVerifyMetadataByDictKey(prim, SdfFieldKeys->CustomData, 
        TfToken("subDict:timeCode"), SdfTimeCode(10.0));
    _GetAndVerifyMetadataByDictKey(prim, SdfFieldKeys->CustomData, 
        TfToken("subDict:timeCodeArray"), SdfTimeCodeArray({10.0, 20.0}));
    _GetAndVerifyMetadataByDictKey(prim, SdfFieldKeys->CustomData, 
        TfToken("subDict:doubleVal"), 10.0);
}

// Tests GetMetadata functions on time code valued fields resolved
// through layers with layer offsets.'''
static void _TestGetMetaDataWithLayerOffsets()
{
    // Create a stage from root.usda which has a sublayer, root_sub.usda,
    // which defines a prim with a reference to ref.usda, which itself has
    // a sublayer ref_sub.usda. All the prim metadata and attributes are
    // authored in ref_sub.usda and layer offsets exist across each sublayer
    // and reference. Time code metadata values will be resolved by these
    // offsets.
    UsdStageRefPtr s = UsdStage::Open("./timeCodes/root.usda");
    TF_AXIOM(s);
    UsdPrim prim = s->GetPrimAtPath(SdfPath("/TimeCodeTest"));

    // Test attribute metadata resolution
    UsdAttribute timeAttr = prim.GetAttribute(TfToken("TimeCode"));
    UsdAttribute arrayAttr = prim.GetAttribute(TfToken("TimeCodeArray"));
    UsdAttribute doubleAttr = prim.GetAttribute(TfToken("Double"));

    const TfToken timeCodeTest("timeCodeTest");
    const TfToken timeCodeArrayTest("timeCodeArrayTest");
    const TfToken doubleTest("doubleTest");

    // Attribute timeSamples metadata. The SdfTimeCode valued attribute
    // has offsets applied to both the time sample keys and the value itself.
    // The double value attribute is only offset by the time sample keys, the
    // values remains as authored.
    _GetAndVerifyMetadata(timeAttr, SdfFieldKeys->TimeSamples, 
        SdfTimeSampleMap({{3.0, VtValue(SdfTimeCode(23.0))},
                          {5.0, VtValue(SdfTimeCode(43.0))}}));
    _GetAndVerifyMetadata(arrayAttr, SdfFieldKeys->TimeSamples, 
        SdfTimeSampleMap({{3.0, VtValue(SdfTimeCodeArray({23.0, 63.0}))},
                          {5.0, VtValue(SdfTimeCodeArray({43.0, 83.0}))}}));
    _GetAndVerifyMetadata(doubleAttr, SdfFieldKeys->TimeSamples, 
        SdfTimeSampleMap({{3.0, VtValue(10.0)},
                          {5.0, VtValue(20.0)}}));

    // Attribute default metadata. Time code values are resolved by layer
    // offsets, double values are not.
    _GetAndVerifyMetadata(timeAttr, SdfFieldKeys->Default, SdfTimeCode(23.0));
    _GetAndVerifyMetadata(arrayAttr, SdfFieldKeys->Default, 
                          SdfTimeCodeArray({23.0, 43.0}));
    _GetAndVerifyMetadata(doubleAttr, SdfFieldKeys->Default, 10.0);

    // Test prim metadata resolution. All time code values are offset,
    // doubles are not. This applies even when the values are contained
    // within dictionaries.
    _GetAndVerifyMetadata(prim, timeCodeTest, SdfTimeCode(23.0));
    _GetAndVerifyMetadata(prim, timeCodeArrayTest, 
                          SdfTimeCodeArray({23.0, 43.0}));
    _GetAndVerifyMetadata(prim, doubleTest, 10.0);

    // Prim customData retrieved as the full dictionary
    VtDictionary expectedCustomData({
        {"timeCode", VtValue(SdfTimeCode(23.0))},
        {"timeCodeArray", VtValue(SdfTimeCodeArray({23.0, 43.0}))},
        {"doubleVal", VtValue(10.0)},
        {"subDict", VtValue(VtDictionary({
            {"timeCode", VtValue(SdfTimeCode(23.0))},
            {"timeCodeArray", VtValue(SdfTimeCodeArray({23.0, 43.0}))},
            {"doubleVal", VtValue(10.0)}}))}});

    _GetAndVerifyMetadata(prim, SdfFieldKeys->CustomData, expectedCustomData);

    // Also test getting customData values by dict key.
    _GetAndVerifyMetadataByDictKey(prim, SdfFieldKeys->CustomData, 
        TfToken("timeCode"), SdfTimeCode(23.0));
    _GetAndVerifyMetadataByDictKey(prim, SdfFieldKeys->CustomData, 
        TfToken("timeCodeArray"), SdfTimeCodeArray({23.0, 43.0}));
    _GetAndVerifyMetadataByDictKey(prim, SdfFieldKeys->CustomData, 
        TfToken("doubleVal"), 10.0);

    _GetAndVerifyMetadataByDictKey(prim, SdfFieldKeys->CustomData, 
        TfToken("subDict:timeCode"), SdfTimeCode(23.0));
    _GetAndVerifyMetadataByDictKey(prim, SdfFieldKeys->CustomData, 
        TfToken("subDict:timeCodeArray"), SdfTimeCodeArray({23.0, 43.0}));
    _GetAndVerifyMetadataByDictKey(prim, SdfFieldKeys->CustomData, 
        TfToken("subDict:doubleVal"), 10.0);

    // Test stage level metadata resolution. Stage metadata is always defined
    // on the root layer so there are never any layer offsets to apply to
    // this metadata.
    VtDictionary expectedCustomLayerData({
        {"timeCode", VtValue(SdfTimeCode(10.0))},
        {"timeCodeArray", VtValue(SdfTimeCodeArray({10.0, 20.0}))},
        {"subDict", VtValue(VtDictionary({
            {"timeCode", VtValue(SdfTimeCode(10.0))},
            {"timeCodeArray", VtValue(SdfTimeCodeArray({10.0, 20.0}))}}))}});

    _GetAndVerifyMetadata(s, SdfFieldKeys->CustomLayerData, 
                          expectedCustomLayerData);
}

// Test the SetMetadata API on UsdObjects for time code valued metadata
// when using edit targets that author across layers with layer offsets.

static void _TestSetMetaDataWithEditTarget()
{
    // Create a stage from root.usda which has a sublayer, root_sub.usda,
    // which defines a prim with a reference to ref.usda, which itself has
    // a sublayer ref_sub.usda. All the prim metadata and attributes are
    // authored in ref_sub.usda and layer offsets exist across each sublayer
    // and reference. Time code metadata values will be resolved by these
    // offsets.
    UsdStageRefPtr stage = UsdStage::Open("./timeCodes/root.usda");
    TF_AXIOM(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/TimeCodeTest"));
    UsdAttribute timeAttr = prim.GetAttribute(TfToken("TimeCode"));
    UsdAttribute arrayAttr = prim.GetAttribute(TfToken("TimeCodeArray"));
    UsdAttribute doubleAttr = prim.GetAttribute(TfToken("Double"));

    const TfToken timeCodeTest("timeCodeTest");
    const TfToken timeCodeArrayTest("timeCodeArrayTest");
    const TfToken doubleTest("doubleTest");

    // Get an edit target for each of our layers. These will each have a
    // different layer offset.
    _EditTargets editTargets = _GetEditTargets(prim);

    // Set SdfTimeCode and SdfTimeCodeArray metadata on prim. Each edit
    // target resolves against a different composed layer offset.
    _SetMetadataWithEachEditTarget(stage, editTargets, prim, timeCodeTest,
                                   SdfTimeCode(25.0),
                                   {SdfTimeCode(11.0),
                                    SdfTimeCode(14.0),
                                    SdfTimeCode(50.0),
                                    SdfTimeCode(25.0)});
    _SetMetadataWithEachEditTarget(stage, editTargets, prim, timeCodeArrayTest,
                                   SdfTimeCodeArray({25.0, 45.0}),
                                   {SdfTimeCodeArray({11.0, 21.0}),
                                    SdfTimeCodeArray({14.0, 24.0}),
                                    SdfTimeCodeArray({50.0, 90.0}),
                                    SdfTimeCodeArray({25.0, 45.0})});

    // Set double value metadata on prim. Values are not resolved over
    // edit target offsets.
    _SetMetadataWithEachEditTarget(stage, editTargets, prim, doubleTest, 25.0,
                                   {25.0, 25.0, 25.0, 25.0});

    // For the customData dictionary tests, the weakest layer has the
    // original authored value dictionary. We'll need to compare the
    // expected authored value for that layer against the whole dictionary
    // for that edit target.
    VtDictionary authoredCustomData({
        {"timeCode", VtValue(SdfTimeCode(10.0))},
        {"timeCodeArray", VtValue(SdfTimeCodeArray({10.0, 20.0}))},
        {"doubleVal", VtValue(10.0)},
        {"subDict", VtValue(VtDictionary({
            {"timeCode", VtValue(SdfTimeCode(10.0))},
            {"timeCodeArray", VtValue(SdfTimeCodeArray({10.0, 20.0}))},
            {"doubleVal", VtValue(10.0)}}))}});

    // Set SdfTimeCode and SdfTimeCodeArray metadata by key in the prim's
    // customData metadata. Each edit target resolves against a different
    // composed layer offset.
    authoredCustomData["timeCode"] = VtValue(SdfTimeCode(1.5));
    _SetMetadataByKeyWithEachEditTarget(
        stage, editTargets, prim, SdfFieldKeys->CustomData,
        TfToken("timeCode"), SdfTimeCode(6.0),
        {authoredCustomData,
         {{"timeCode", VtValue(SdfTimeCode(4.5))}},
         {{"timeCode", VtValue(SdfTimeCode(12.0))}},
         {{"timeCode", VtValue(SdfTimeCode(6.0))}}});

    authoredCustomData.SetValueAtPath("subDict:timeCode", VtValue(SdfTimeCode(4)));
    _SetMetadataByKeyWithEachEditTarget(
        stage, editTargets, prim, SdfFieldKeys->CustomData,
        TfToken("subDict:timeCode"), SdfTimeCode(11.0),
        {authoredCustomData,
         {{"timeCode", VtValue(SdfTimeCode(4.5))},
          {"subDict", VtValue(VtDictionary({
             {"timeCode", VtValue(SdfTimeCode(7))}}))}},
         {{"timeCode", VtValue(SdfTimeCode(12.0))},
          {"subDict", VtValue(VtDictionary({
             {"timeCode", VtValue(SdfTimeCode(22))}}))}},
         {{"timeCode", VtValue(SdfTimeCode(6.0))},
          {"subDict", VtValue(VtDictionary({
             {"timeCode", VtValue(SdfTimeCode(11))}}))}}});

    // Set double value metadata by key in the prim's customData metadata.
    // The double values are not resolved over edit target offsets.
    authoredCustomData.SetValueAtPath("subDict:doubleVal", VtValue(11.0));
    _SetMetadataByKeyWithEachEditTarget(
        stage, editTargets, prim, SdfFieldKeys->CustomData,
        TfToken("subDict:doubleVal"), 11.0,
        {authoredCustomData,
         {{"timeCode", VtValue(SdfTimeCode(4.5))},
          {"subDict", VtValue(VtDictionary({
             {"timeCode", VtValue(SdfTimeCode(7))},
             {"doubleVal", VtValue(11.0)}}))}},
         {{"timeCode", VtValue(SdfTimeCode(12.0))},
          {"subDict", VtValue(VtDictionary({
             {"timeCode", VtValue(SdfTimeCode(22))},
             {"doubleVal", VtValue(11.0)}}))}},
         {{"timeCode", VtValue(SdfTimeCode(6.0))},
          {"subDict", VtValue(VtDictionary({
             {"timeCode", VtValue(SdfTimeCode(11))},
             {"doubleVal", VtValue(11.0)}}))}}});

    // Note that with this testing setup, we MUST set and test the "timeSamples"
    // metadata before setting the "default" metadata. This because of special
    // value resolution of timeSamples where default values in a stronger layer
    // supercede time samples in a weaker layer. We won't get the results we're
    // testing for if we set the default values first. 

    // Set an SdfTimeSampleMap of SdfTimeCode and SdfTimeCodeArray for the 
    // timeSample metadata of the timeCode attributes. Both the time keys and 
    // values are resolved for each edit target's composed layer offset.
    _SetMetadataWithEachEditTarget(
        stage, editTargets, timeAttr, SdfFieldKeys->TimeSamples,
        SdfTimeSampleMap({{11.0, VtValue(SdfTimeCode(30.0))},
                          {21.0, VtValue(SdfTimeCode(40.0))}}),
        {SdfTimeSampleMap({{4.0, VtValue(SdfTimeCode(13.5))},
                           {9.0, VtValue(SdfTimeCode(18.5))}}),
         SdfTimeSampleMap({{7.0, VtValue(SdfTimeCode(16.5))},
                           {12.0, VtValue(SdfTimeCode(21.5))}}),
         SdfTimeSampleMap({{22.0, VtValue(SdfTimeCode(60.0))},
                           {42.0, VtValue(SdfTimeCode(80.0))}}),
         SdfTimeSampleMap({{11.0, VtValue(SdfTimeCode(30.0))},
                           {21.0, VtValue(SdfTimeCode(40.0))}})});
    _SetMetadataWithEachEditTarget(
        stage, editTargets, arrayAttr, SdfFieldKeys->TimeSamples,
        SdfTimeSampleMap({{11.0, VtValue(SdfTimeCodeArray({30.0, 50.0}))},
                          {21.0, VtValue(SdfTimeCodeArray({40.0, 60.0}))}}),
        {SdfTimeSampleMap({{4.0, VtValue(SdfTimeCodeArray({13.5, 23.5}))},
                           {9.0, VtValue(SdfTimeCodeArray({18.5, 28.5}))}}),
         SdfTimeSampleMap({{7.0, VtValue(SdfTimeCodeArray({16.5, 26.5}))},
                           {12.0, VtValue(SdfTimeCodeArray({21.5, 31.5}))}}),
         SdfTimeSampleMap({{22.0, VtValue(SdfTimeCodeArray({60.0, 100.0}))},
                           {42.0, VtValue(SdfTimeCodeArray({80.0, 120.0}))}}),
         SdfTimeSampleMap({{11.0, VtValue(SdfTimeCodeArray({30.0, 50.0}))},
                           {21.0, VtValue(SdfTimeCodeArray({40.0, 60.0}))}})});


    // Set an SdfTimeSampleMap of doubles for the timeSample metadata of
    // the double valued attribute. The values themselves are not resolved, but
    // The time keys are still resolved for each edit target's composed layer 
    // offset.
    _SetMetadataWithEachEditTarget(
        stage, editTargets, doubleAttr, SdfFieldKeys->TimeSamples,
        SdfTimeSampleMap({{11.0, VtValue(30.0)},
                          {21.0, VtValue(40.0)}}),
        {SdfTimeSampleMap({{4.0, VtValue(30.0)},
                           {9.0, VtValue(40.0)}}),
         SdfTimeSampleMap({{7.0, VtValue(30.0)},
                           {12.0, VtValue(40.0)}}),
         SdfTimeSampleMap({{22.0, VtValue(30.0)},
                           {42.0, VtValue(40.0)}}),
         SdfTimeSampleMap({{11.0, VtValue(30.0)},
                           {21.0, VtValue(40.0)}})});


    // Set SdfTimeCode and SdfTimeCodeArray default value metadata on the
    // time valued attributes. Each edit target resolves against a different
    // composed layer offset.
    _SetMetadataWithEachEditTarget(
        stage, editTargets, timeAttr, SdfFieldKeys->Default,
        SdfTimeCode(19.0),
        {SdfTimeCode(8.0),
         SdfTimeCode(11.0),
         SdfTimeCode(38.0),
         SdfTimeCode(19.0)});
    _SetMetadataWithEachEditTarget(
        stage, editTargets, arrayAttr, SdfFieldKeys->Default,
        SdfTimeCodeArray({19.0, -11.0}),
        {SdfTimeCodeArray({8.0, -7.0}),
         SdfTimeCodeArray({11.0, -4.0}),
         SdfTimeCodeArray({38.0, -22.0}),
         SdfTimeCodeArray({19.0, -11.0})});

    // Set double value default metadata on the double valued attribute.
    // Values are not resolved over edit target offsets.
    _SetMetadataWithEachEditTarget(
        stage, editTargets, doubleAttr, SdfFieldKeys->Default, 19.0, 
        {19.0, 19.0, 19.0, 19.0});
}

static void _TestSetAttrValueWithEditTarget()
{
    // Create a stage from root.usda which has a sublayer, root_sub.usda,
    // which defines a prim with a reference to ref.usda, which itself has
    // a sublayer ref_sub.usda. All the prim metadata and attributes are
    // authored in ref_sub.usda and layer offsets exist across each sublayer
    // and reference. Time code metadata values will be resolved by these
    // offsets.

    UsdStageRefPtr stage = UsdStage::Open("./timeCodes/root.usda");
    TF_AXIOM(stage);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/TimeCodeTest"));
    UsdAttribute timeAttr = prim.GetAttribute(TfToken("TimeCode"));
    UsdAttribute arrayAttr = prim.GetAttribute(TfToken("TimeCodeArray"));
    UsdAttribute doubleAttr = prim.GetAttribute(TfToken("Double"));

    // Get an edit target for each of our layers. These will each have a
    // different layer offset.
    _EditTargets editTargets = _GetEditTargets(prim);

    // Set SdfTimeCode and SdfTimeCodeArray values at times and at default.
    // Each edit target resolves against a different composed layer offset.
    // Both the time sample keys and the time sample values are resolved
    // against offsets
    _SetTimeSampleWithEachEditTarget(stage, editTargets, timeAttr, 12.0, 
         SdfTimeCode(19.0),
         {
             SdfTimeSampleMap({
                {0.0, VtValue(SdfTimeCode(10.0))},
                {1.0, VtValue(SdfTimeCode(20.0))},
                {4.5, VtValue(SdfTimeCode(8.0))}
             }),
             SdfTimeSampleMap({
                {7.5, VtValue(SdfTimeCode(11.0))}
             }),
             SdfTimeSampleMap({
                {24.0, VtValue(SdfTimeCode(38.0))}
             }),
             SdfTimeSampleMap({
                {12.0, VtValue(SdfTimeCode(19.0))}
             })
         });

    _SetDefaultWithEachEditTarget(stage, editTargets, timeAttr, 
        SdfTimeCode(19.0),
        {SdfTimeCode(8.0),
         SdfTimeCode(11.0),
         SdfTimeCode(38.0),
         SdfTimeCode(19.0)});

    _SetTimeSampleWithEachEditTarget(stage, editTargets, arrayAttr, 12.0,
        SdfTimeCodeArray({19.0, 12.0}),
        {   
            SdfTimeSampleMap({
                {0.0, VtValue(SdfTimeCodeArray({10.0, 30.0}))},
                {1.0, VtValue(SdfTimeCodeArray({20.0, 40.0}))},
                {4.5, VtValue(SdfTimeCodeArray({8.0, 4.5}))}
            }),
            SdfTimeSampleMap({
                {7.5, VtValue(SdfTimeCodeArray({11.0, 7.5}))}
            }),
            SdfTimeSampleMap({
                {24.0, VtValue(SdfTimeCodeArray({38.0, 24.0}))}
            }),
            SdfTimeSampleMap({
                {12.0, VtValue(SdfTimeCodeArray({19.0, 12.0}))}
            })
        });

    _SetDefaultWithEachEditTarget(stage, editTargets, arrayAttr, 
        SdfTimeCodeArray({19.0, 12.0}),
        {SdfTimeCodeArray({8.0, 4.5}),
         SdfTimeCodeArray({11.0, 7.5}),
         SdfTimeCodeArray({38.0, 24.0}),
         SdfTimeCodeArray({19.0, 12.0})});

    // Set double values at times and at default. Time sample keys are
    // resolved against each edit target's offset, but none of the values
    // themselves are.
    _SetTimeSampleWithEachEditTarget(stage, editTargets, doubleAttr, 12.0, 
        19.0,
        {
            SdfTimeSampleMap({
                {0.0, VtValue(10.0)},
                {1.0, VtValue(20.0)},
                {4.5, VtValue(19.0)}
            }),
            SdfTimeSampleMap({
                {7.5, VtValue(19.0)}
            }),
            SdfTimeSampleMap({
                {24.0, VtValue(19.0)}
            }),
            SdfTimeSampleMap({
                {12.0, VtValue(19.0)}
            })
        });

    _SetDefaultWithEachEditTarget(stage, editTargets, doubleAttr, 19.0,
        {19.0, 19.0, 19.0, 19.0});
}

int main() {
    const std::string testDir = ArchGetCwd();
    TF_AXIOM(!PlugRegistry::GetInstance().RegisterPlugins(testDir).empty());

    _TestGetMetadataNoOffsets();
    _TestGetMetaDataWithLayerOffsets();
    _TestSetMetaDataWithEditTarget();
    _TestSetAttrValueWithEditTarget();

    printf("\n\n>>> Test SUCCEEDED\n");
}
