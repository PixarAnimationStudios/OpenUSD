//
// Copyright 2022 Pixar
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
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/primCompositionQuery.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/token.h"

#include <iostream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

// This just for stringifying UsdResolveTarget to help debug failures in this
// test.
std::ostream& operator<<(std::ostream& os, const UsdResolveTarget& rt)
{
    os << "Resolve target:";
    os << "\n  start node: " << rt.GetStartNode().GetSite();
    if (rt.GetStartLayer()) {
        os << "\n  start layer: " << rt.GetStartLayer()->GetIdentifier();
    }
    if (rt.GetStopNode()) {
        os << "\n  stop node: " << rt.GetStopNode().GetSite();
        if (rt.GetStartLayer()) {
            os << "\n  stop layer: " << rt.GetStopLayer()->GetIdentifier();
        }
    }
    return os;
}

// Helper for _VerifyResolveTarget
static void _VerifyResolveTargetSite(
    const PcpNodeRef &node, 
    const PcpSite &expectedSite) 
{
    PcpSite site(node.GetSite());
    TF_VERIFY(site == expectedSite,
        "Site '%s' does not match expected '%s'",
        TfStringify(site).c_str(), 
        TfStringify(expectedSite).c_str());
}

// Helper for _VerifyResolveTarget
static void _VerifyResolverTargetLayer(
    const SdfLayerHandle &layer, 
    const std::string &expectedLayerName)
{
    std::string layerName = TfGetBaseName(layer->GetIdentifier());
    // Special case for expecting the session layer as the session layer created
    // in this test will be an anonymous layer without a consistent identifier
    // between runs. But it will always end in "root-session.usda"
    if (expectedLayerName == "session") {
        TF_VERIFY(TfStringEndsWith(layerName, "root-session.usda"),
            "Layer name '%s' does not end with expected session layer suffix "
            "'root-session.usda'",
            layerName.c_str());
    } else {
        TF_VERIFY(layerName == expectedLayerName,
            "Layer name '%s' does not match expected layer name '%s'",
            layerName.c_str(), 
            expectedLayerName.c_str());
    }
}

// Helper for verifying a resolve target matches expected expect values.
// All resolve targets will have a start node and start layer but do not always
// have to have a stop node or stop layer
static void _VerifyResolveTarget(
    const UsdResolveTarget &resolveTarget,
    const std::pair<PcpSite, std::string> &expectedStart,
    const std::pair<PcpSite, std::string> *expectedStop = nullptr)
{
    _VerifyResolveTargetSite(
        resolveTarget.GetStartNode(), expectedStart.first);
    _VerifyResolverTargetLayer(
        resolveTarget.GetStartLayer(), expectedStart.second);

    if (expectedStop) {
        _VerifyResolveTargetSite(
            resolveTarget.GetStopNode(), expectedStop->first);
        _VerifyResolverTargetLayer(
            resolveTarget.GetStopLayer(), expectedStop->second);
    } else {
        TF_VERIFY(!resolveTarget.GetStopNode());
        TF_VERIFY(!resolveTarget.GetStopLayer());
    }
}

// Helper for verifying the returned value from UsdAttributeQuery::Get. This
// tests both the templated explicit type overload and the type erased (VtValue)
// overload.
template <class T>
static void
_GetAndVerifyAttributeValue(
    const UsdAttributeQuery &attrQuery, 
    const UsdTimeCode &time, 
    const T* expected)
{
    T value;
    VtValue vtValue; 
    if (expected) {
        TF_VERIFY(attrQuery.Get(&value, time), 
            "Failed to get value from attribute query at time %s", 
            TfStringify(time).c_str());
        TF_VERIFY(value == *expected, 
            "Returned value %s != expected value %s", 
            TfStringify(value).c_str(), 
            TfStringify(*expected).c_str());

        TF_VERIFY(attrQuery.Get(&vtValue, time),
            "Failed to get value from attribute query at time %s", 
            TfStringify(time).c_str());
        TF_VERIFY(vtValue.UncheckedGet<T>() == *expected,
            "Returned value %s != expected value %s", 
            TfStringify(vtValue).c_str(), 
            TfStringify(*expected).c_str());
    } else {
        TF_VERIFY(!attrQuery.Get(&value, time),
            "Attribute query Get was expected to fail at time %s",
            TfStringify(time).c_str());
        TF_VERIFY(!attrQuery.Get(&vtValue, time),
            "Attribute query Get was expected to fail at time %s",
            TfStringify(time).c_str());
    }
}

// Format for expected time sample values.
template <class T>
using _ExpectedTimeSamples = std::vector<std::pair<double, T>>;

// Format for all expected values: a pair of expected time sample values and
// a VtValue holding the expected default value (if any).
template <class T>
using _ExpectedAttrGetValues = std::pair<_ExpectedTimeSamples<T>, VtValue>;

// Verifies that the results of calling the various API functions on the given 
// UsdAttributeQuery match the expected results of those queries.
template <class T>
static void
_VerifyQuery(
    const UsdAttributeQuery &attrQuery,
    const _ExpectedAttrGetValues<T> &expectedValues)
{
    // Extract the list of expected time sample values.
    const _ExpectedTimeSamples<T> &expectedTimeSampleValues = 
        expectedValues.first;

    // Extract the typed expected default value from expected values, which
    // may be null if a default value is not expected.
    const VtValue &expectedDefaultVtValue = expectedValues.second;
    const T *expectedAuthoredDefaultValue = nullptr;
    if (!expectedDefaultVtValue.IsEmpty()) {
        TF_VERIFY(expectedDefaultVtValue.IsHolding<T>(), 
            "Non-empty expected default VtValue must be holding a value of the "
            "templated type.");
        expectedAuthoredDefaultValue = 
            &(expectedDefaultVtValue.UncheckedGet<T>());
    }

    // We expect HasAuthoredValue() to return true if we expect either time
    // samples or a default value.
    const bool expectedHasAuthoredValue = 
        !expectedTimeSampleValues.empty() || 
        expectedAuthoredDefaultValue;
    TF_VERIFY(attrQuery.HasAuthoredValue() == expectedHasAuthoredValue,
        "expected HasAuthoredValue() == %s", 
        TfStringify(expectedHasAuthoredValue).c_str());

    // We expect HasValue to return true if we expect an authored value.
    // Note that HasValue would return true if an attribute has a fallback value
    // but this whole test doesn't use attributes with fallbacks.
    const bool expectedHasValue = expectedHasAuthoredValue;
    TF_VERIFY(attrQuery.HasValue() == expectedHasValue,
        "expected HasValue() == %s", 
        TfStringify(expectedHasValue).c_str());

    // Verify that GetTimeSamples returns the expected time sample times.
    std::vector<double> expectedTimeSampleTimes;
    for (const auto &timeAndVal : expectedTimeSampleValues) {
        expectedTimeSampleTimes.push_back(timeAndVal.first);
    }
    std::vector<double> timeSampleTimes;
    TF_VERIFY(attrQuery.GetTimeSamples(&timeSampleTimes));
    TF_VERIFY(timeSampleTimes == expectedTimeSampleTimes,
        "Returned time sample times %s do not match expected time sample times "
        "%s.", 
        TfStringify(timeSampleTimes).c_str(), 
        TfStringify(expectedTimeSampleTimes).c_str());

    // Since this test currently doesn't involve clips, we expect 
    // ValueMightBeTimeVarying to be true iff we expect more than one time 
    // sample.
    if (expectedTimeSampleValues.size() > 1) {
        TF_VERIFY(attrQuery.ValueMightBeTimeVarying());
    } else {
        TF_VERIFY(!attrQuery.ValueMightBeTimeVarying());
    }

    // Verify that calling Get at default time returns the expected default 
    // value.
    _GetAndVerifyAttributeValue(
        attrQuery, UsdTimeCode::Default(), expectedAuthoredDefaultValue);
    // Verify that calling Get at each expected time sample time returns the 
    // expect time sample value.
    for (const auto &timeAndVal : expectedTimeSampleValues) {
        _GetAndVerifyAttributeValue(
            attrQuery, timeAndVal.first, &(timeAndVal.second));
    } 
    // If we expect no time samples, verify that calling Get with a numeric time
    // code returns the expected default value.
    if (expectedTimeSampleValues.empty()) {
        _GetAndVerifyAttributeValue(
            attrQuery, 1.0, expectedAuthoredDefaultValue);
    }
}

// Makes a UsdAttributeQuery for the attribute using each of the given resolve
// targets and verifies for each that it produces the expected values 
template <class T>
static void
_MakeAndVerifyQueries(
    const UsdAttribute &attr,
    const std::vector<UsdResolveTarget> &resolveTargets,
    const std::vector<_ExpectedAttrGetValues<T>> expectedValues)
{
    std::cout << "\n** Start: Make and verify queries for attribute " << 
        attr.GetPath() << " **" << std::endl;

    TF_VERIFY(expectedValues.size() == resolveTargets.size(), 
        "Number or resolve targets %lu doesn't match the number of expected "
        "values %lu.",
        resolveTargets.size(), expectedValues.size());

    for (size_t i = 0; i < resolveTargets.size(); ++i) {
        std::cout << "Verifying query at " << resolveTargets[i] << std::endl;

        UsdAttributeQuery attrQuery(attr, resolveTargets[i]);
        _VerifyQuery<T>(attrQuery, expectedValues[i]);
    }

    std::cout << "** SUCCESS: Make and verify queries for attribute " << 
        attr.GetPath() << " **" << std::endl;
}

// Get all the possible resolve targets for the prim that can be created to 
// resolve up to and to resolve stronger than the possible nodes and layers in 
// its prim index. 
static void
_GetAllResolveTargetsForPrim(
    const UsdPrim &prim,
    std::vector<UsdResolveTarget> *upToResolveTargets,
    std::vector<UsdResolveTarget> *strongThanResolveTargets) 
{
    // The prim composition query gets us every arc that could contribute specs
    // to the prim (even if the arc would be culled normally) so we use it to 
    // create all resolve targets.
    UsdPrimCompositionQuery query(prim);
    std::vector<UsdPrimCompositionQueryArc> arcs = query.GetCompositionArcs();

    // Loop through every layer in each composition arc creating both the 
    // "up to" and "stronger than" resolve targets for each.
    for (const UsdPrimCompositionQueryArc &arc : arcs) {
        const SdfLayerRefPtrVector &layers = 
            arc.GetTargetNode().GetLayerStack()->GetLayers();
        for (const SdfLayerHandle &layer : layers) {

            upToResolveTargets->push_back(
                arc.MakeResolveTargetUpTo(layer));
            strongThanResolveTargets->push_back(
                arc.MakeResolveTargetStrongerThan(layer));
        }
    }
}

static void _TestGetAttrValueWithResolveTargets()
{
    UsdStageRefPtr stage = UsdStage::Open("./resolveTarget/root.usda");
    TF_AXIOM(stage);

    // Parent unculled prim stack is:
    //   /Parent : session.usda -> root.usda -> sub1.usda -> sub2.usda
    //      |
    //     ref
    //      v
    //   /InternalRef : session.usda -> root.usda -> sub1.usda -> sub2.usda
    //      |
    //     ref
    //      v
    //   /RefParent : ref.usda -> ref_sub1.usda -> ref_sub2.usda
    UsdPrim parentPrim = stage->GetPrimAtPath(SdfPath("/Parent"));
    TF_AXIOM(parentPrim);
    // /Parent/RefChild is just a namespace child of /Parent with no additional
    // composition arcs of its own outside of its ancestral composition.
    UsdPrim childPrim = stage->GetPrimAtPath(SdfPath("/Parent/RefChild"));
    TF_AXIOM(childPrim);

    // Get the root layer stack ID to use for verification purposes.
    PcpLayerStackIdentifier rootLayerStackId(
        stage->GetRootLayer(), 
        stage->GetSessionLayer(), 
        stage->GetPathResolverContext());
    
    // Get the ref.usda layer stack ID to also use for verification purposes.
    SdfLayerHandle refLayer = SdfLayer::Find("./resolveTarget/ref.usda");
    PcpLayerStackIdentifier 
        refLayerStackId(refLayer, nullptr, stage->GetPathResolverContext());

    // Get all the possible resolve targets for the child prim.
    std::vector<UsdResolveTarget> upToResolveTargets;
    std::vector<UsdResolveTarget> strongThanResolveTargets;
    _GetAllResolveTargetsForPrim(
        childPrim,
        &upToResolveTargets,
        &strongThanResolveTargets);

    // This is the expected list of all node sites and sublayers we expect 
    // resolve targets to have been created for from the child prim.
    std::vector<std::pair<PcpSite, std::string> > expectedTargets {
        // Node: /Parent/RefChild
        {PcpSite(rootLayerStackId, SdfPath("/Parent/RefChild")), "session"},
        {PcpSite(rootLayerStackId, SdfPath("/Parent/RefChild")), "root.usda"},
        {PcpSite(rootLayerStackId, SdfPath("/Parent/RefChild")), "sub1.usda"},
        {PcpSite(rootLayerStackId, SdfPath("/Parent/RefChild")), "sub2.usda"},

        // Node: /Internal/RefChild
        {PcpSite(rootLayerStackId, SdfPath("/InternalRef/RefChild")), "session"},
        {PcpSite(rootLayerStackId, SdfPath("/InternalRef/RefChild")), "root.usda"},
        {PcpSite(rootLayerStackId, SdfPath("/InternalRef/RefChild")), "sub1.usda"},
        {PcpSite(rootLayerStackId, SdfPath("/InternalRef/RefChild")), "sub2.usda"},

        // Node: /RefParent/RefChild
        {PcpSite(refLayerStackId, SdfPath("/RefParent/RefChild")), "ref.usda"},
        {PcpSite(refLayerStackId, SdfPath("/RefParent/RefChild")), "ref_sub1.usda"},
        {PcpSite(refLayerStackId, SdfPath("/RefParent/RefChild")), "ref_sub2.usda"}
        };

    TF_AXIOM(upToResolveTargets.size() == 11);
    TF_AXIOM(strongThanResolveTargets.size() == 11);
    for (size_t i = 0; i < expectedTargets.size(); ++i) {
        // Verify that each "up to" resolve target starts at the expected node
        // and layer.
        _VerifyResolveTarget(upToResolveTargets[i], expectedTargets[i]);

        // Verify that each "stronger than" resolve target starts at the root
        // node and session layer (strongest layer in the root node layer stack)
        // and stops at the expected node and layer. 
        _VerifyResolveTarget(strongThanResolveTargets[i], 
            expectedTargets[0], &expectedTargets[i]);
    }

    // Verify expected values from attribute queries made on attributes of 
    // childPrim using each resolve target.

    // /Parent/RefChild.foo
    // Has only default values authored:
    //    root.usda: /Parent/RefChild -> 6.0
    //    sub1.usda: /Parent/RefChild -> 5.0
    //    sub2.usda: /Parent/RefChild -> 4.0
    //    ref.usda: /RefParent/RefChild -> 3.0
    //    ref_sub1.usda: /RefParent/RefChild -> 2.0
    //    ref_sub2.usda: /RefParent/RefChild -> 1.0
    UsdAttribute fooAttr = childPrim.GetAttribute(TfToken("foo"));
    TF_AXIOM(fooAttr);
    _MakeAndVerifyQueries<float>(fooAttr, upToResolveTargets,
        {
            // Node: /Parent/RefChild
            {{}, VtValue(6.0f)}, 
            {{}, VtValue(6.0f)},
            {{}, VtValue(5.0f)},
            {{}, VtValue(4.0f)},

            // Node: /Internal/RefChild
            {{}, VtValue(3.0f)},
            {{}, VtValue(3.0f)},
            {{}, VtValue(3.0f)},
            {{}, VtValue(3.0f)},

            // Node: /RefParent/RefChild
            {{}, VtValue(3.0f)},
            {{}, VtValue(2.0f)},
            {{}, VtValue(1.0f)}
        });

    _MakeAndVerifyQueries<float>(fooAttr, strongThanResolveTargets,
        {
            // Node: /Parent/RefChild
            {{}, VtValue()},
            {{}, VtValue()},
            {{}, VtValue(6.0f)},
            {{}, VtValue(6.0f)},

            // Node: /Internal/RefChild
            {{}, VtValue(6.0f)},
            {{}, VtValue(6.0f)},
            {{}, VtValue(6.0f)},
            {{}, VtValue(6.0f)},

            // Node: /RefParent/RefChild
            {{}, VtValue(6.0f)},
            {{}, VtValue(6.0f)},
            {{}, VtValue(6.0f)}
        });

    // /Parent/RefChild.var
    // Has only time sample values authored:
    //    root.usda: /Parent/RefChild -> {1.0: 6, 6.0: 1}
    //    sub1.usda: /Parent/RefChild -> {1.0: 5, 5.0: 1}
    //    sub2.usda: /Parent/RefChild -> {1.0: 4, 4.0: 1}
    //    ref.usda: /RefParent/RefChild -> {1.0: 3, 3.0: 1}
    //    ref_sub1.usda: /RefParent/RefChild -> {1.0: 2, 2.0: 1}
    //    ref_sub2.usda: /RefParent/RefChild -> {1.0: 1}
    UsdAttribute varAttr = childPrim.GetAttribute(TfToken("var"));
    TF_AXIOM(varAttr);
    _MakeAndVerifyQueries<int>(varAttr, upToResolveTargets,
        {
            // Node: /Parent/RefChild
            {{{1.0, 6}, {6.0, 1}}, VtValue()},
            {{{1.0, 6}, {6.0, 1}}, VtValue()},
            {{{1.0, 5}, {5.0, 1}}, VtValue()},
            {{{1.0, 4}, {4.0, 1}}, VtValue()},

            // Node: /Internal/RefChild
            {{{1.0, 3}, {3.0, 1}}, VtValue()},
            {{{1.0, 3}, {3.0, 1}}, VtValue()},
            {{{1.0, 3}, {3.0, 1}}, VtValue()},
            {{{1.0, 3}, {3.0, 1}}, VtValue()},

            // Node: /RefParent/RefChild
            {{{1.0, 3}, {3.0, 1}}, VtValue()},
            {{{1.0, 2}, {2.0, 1}}, VtValue()},
            {{{1.0, 1}}, VtValue()}
        });

    _MakeAndVerifyQueries<int>(varAttr, strongThanResolveTargets,
        {
            // Node: /Parent/RefChild
            {{}, VtValue()},
            {{}, VtValue()},
            {{{1.0, 6}, {6.0, 1}}, VtValue()},
            {{{1.0, 6}, {6.0, 1}}, VtValue()},

            // Node: /Internal/RefChild
            {{{1.0, 6}, {6.0, 1}}, VtValue()},
            {{{1.0, 6}, {6.0, 1}}, VtValue()},
            {{{1.0, 6}, {6.0, 1}}, VtValue()},
            {{{1.0, 6}, {6.0, 1}}, VtValue()},

            // Node: /RefParent/RefChild
            {{{1.0, 6}, {6.0, 1}}, VtValue()},
            {{{1.0, 6}, {6.0, 1}}, VtValue()},
            {{{1.0, 6}, {6.0, 1}}, VtValue()}
        });

    // /Parent/RefChild.bar
    // Has alternating time samples and default values authored:
    //    root.usda: /Parent/RefChild -> {1.0: 6, 6.0: 1}
    //    sub1.usda: /Parent/RefChild -> 5
    //    sub2.usda: /Parent/RefChild -> {1.0: 4, 4.0: 1}
    //    ref.usda: /RefParent/RefChild -> 3
    //    ref_sub1.usda: /RefParent/RefChild -> {1.0: 2, 2.0: 1}
    //    ref_sub2.usda: /RefParent/RefChild -> 1
    UsdAttribute barAttr = childPrim.GetAttribute(TfToken("bar"));
    TF_AXIOM(barAttr);
    _MakeAndVerifyQueries<int>(barAttr, upToResolveTargets,
        {
            // Node: /Parent/RefChild
            {{{1.0, 6}, {6.0, 1}}, VtValue(5)},
            {{{1.0, 6}, {6.0, 1}}, VtValue(5)},
            {{}, VtValue(5)},
            {{{1.0, 4}, {4.0, 1}}, VtValue(3)},

            // Node: /Internal/RefChild
            {{}, VtValue(3)},
            {{}, VtValue(3)},
            {{}, VtValue(3)},
            {{}, VtValue(3)},

            // Node: /RefParent/RefChild
            {{}, VtValue(3)},
            {{{1.0, 2}, {2.0, 1}}, VtValue(1)},
            {{}, VtValue(1)}
        });

    _MakeAndVerifyQueries<int>(barAttr, strongThanResolveTargets,
        {
            // Node: /Parent/RefChild
            {{}, VtValue()},
            {{}, VtValue()},
            {{{1.0, 6}, {6.0, 1}}, VtValue()},
            {{{1.0, 6}, {6.0, 1}}, VtValue(5)},

            // Node: /Internal/RefChild
            {{{1.0, 6}, {6.0, 1}}, VtValue(5)},
            {{{1.0, 6}, {6.0, 1}}, VtValue(5)},
            {{{1.0, 6}, {6.0, 1}}, VtValue(5)},
            {{{1.0, 6}, {6.0, 1}}, VtValue(5)},

            // Node: /RefParent/RefChild
            {{{1.0, 6}, {6.0, 1}}, VtValue(5)},
            {{{1.0, 6}, {6.0, 1}}, VtValue(5)},
            {{{1.0, 6}, {6.0, 1}}, VtValue(5)}
        });

    // /Parent/RefChild.sub1
    // Has default and time samples authored only on the sub1 layer of the
    // root node:
    //    sub1.usda: /Parent/RefChild -> {1.0: "sub1_1", 5.0: "sub1_5"}
    //                                   "sub1_def"
    UsdAttribute sub1Attr = childPrim.GetAttribute(TfToken("sub1"));
    TF_AXIOM(sub1Attr);
    _MakeAndVerifyQueries<TfToken>(sub1Attr, upToResolveTargets,
        {
            // Node: /Parent/RefChild
            {{{1.0, TfToken("sub1_1")}, {5.0, TfToken("sub1_5")}}, 
                VtValue(TfToken("sub1_def"))},
            {{{1.0, TfToken("sub1_1")}, {5.0, TfToken("sub1_5")}}, 
                VtValue(TfToken("sub1_def"))},
            {{{1.0, TfToken("sub1_1")}, {5.0, TfToken("sub1_5")}}, 
                VtValue(TfToken("sub1_def"))},
            {{}, VtValue()},

            // Node: /Internal/RefChild
            {{}, VtValue()},
            {{}, VtValue()},
            {{}, VtValue()},
            {{}, VtValue()},

            // Node: /RefParent/RefChild
            {{}, VtValue()},
            {{}, VtValue()},
            {{}, VtValue()}
        });

    _MakeAndVerifyQueries<TfToken>(sub1Attr, strongThanResolveTargets,
        {
            // Node: /Parent/RefChild
            {{}, VtValue()},
            {{}, VtValue()},
            {{}, VtValue()},
            {{{1.0, TfToken("sub1_1")}, {5.0, TfToken("sub1_5")}}, 
                VtValue(TfToken("sub1_def"))},

            // Node: /Internal/RefChild
            {{{1.0, TfToken("sub1_1")}, {5.0, TfToken("sub1_5")}}, 
                VtValue(TfToken("sub1_def"))},
            {{{1.0, TfToken("sub1_1")}, {5.0, TfToken("sub1_5")}}, 
                VtValue(TfToken("sub1_def"))},
            {{{1.0, TfToken("sub1_1")}, {5.0, TfToken("sub1_5")}}, 
                VtValue(TfToken("sub1_def"))},
            {{{1.0, TfToken("sub1_1")}, {5.0, TfToken("sub1_5")}}, 
                VtValue(TfToken("sub1_def"))},

            // Node: /RefParent/RefChild
            {{{1.0, TfToken("sub1_1")}, {5.0, TfToken("sub1_5")}}, 
                VtValue(TfToken("sub1_def"))},
            {{{1.0, TfToken("sub1_1")}, {5.0, TfToken("sub1_5")}}, 
                VtValue(TfToken("sub1_def"))},
            {{{1.0, TfToken("sub1_1")}, {5.0, TfToken("sub1_5")}}, 
                VtValue(TfToken("sub1_def"))}
        });

    // /Parent/RefChild.ref_sub1
    // Has default and time samples authored only on the ref_sub1 layer of the
    // reference node:
    //    sub1.usda: /Parent/RefChild -> {1.0: "ref_sub1_1", 2.0: "ref_sub1_2"}
    //                                   "ref_sub1_def"
    UsdAttribute refSub1Attr = childPrim.GetAttribute(TfToken("ref_sub1"));
    TF_AXIOM(refSub1Attr);
    _MakeAndVerifyQueries<TfToken>(refSub1Attr, upToResolveTargets,
        {
            // Node: /Parent/RefChild
            {{{1.0, TfToken("ref_sub1_1")}, {2.0, TfToken("ref_sub1_2")}}, 
                VtValue(TfToken("ref_sub1_def"))},
            {{{1.0, TfToken("ref_sub1_1")}, {2.0, TfToken("ref_sub1_2")}}, 
                VtValue(TfToken("ref_sub1_def"))},
            {{{1.0, TfToken("ref_sub1_1")}, {2.0, TfToken("ref_sub1_2")}}, 
                VtValue(TfToken("ref_sub1_def"))},
            {{{1.0, TfToken("ref_sub1_1")}, {2.0, TfToken("ref_sub1_2")}}, 
                VtValue(TfToken("ref_sub1_def"))},

            // Node: /Internal/RefChild
            {{{1.0, TfToken("ref_sub1_1")}, {2.0, TfToken("ref_sub1_2")}}, 
                VtValue(TfToken("ref_sub1_def"))},
            {{{1.0, TfToken("ref_sub1_1")}, {2.0, TfToken("ref_sub1_2")}}, 
                VtValue(TfToken("ref_sub1_def"))},
            {{{1.0, TfToken("ref_sub1_1")}, {2.0, TfToken("ref_sub1_2")}}, 
                VtValue(TfToken("ref_sub1_def"))},
            {{{1.0, TfToken("ref_sub1_1")}, {2.0, TfToken("ref_sub1_2")}}, 
                VtValue(TfToken("ref_sub1_def"))},

            // Node: /RefParent/RefChild
            {{{1.0, TfToken("ref_sub1_1")}, {2.0, TfToken("ref_sub1_2")}}, 
                VtValue(TfToken("ref_sub1_def"))},
            {{{1.0, TfToken("ref_sub1_1")}, {2.0, TfToken("ref_sub1_2")}}, 
                VtValue(TfToken("ref_sub1_def"))},
            {{}, VtValue()}
        });

    _MakeAndVerifyQueries<TfToken>(refSub1Attr, strongThanResolveTargets,
        {
            // Node: /Parent/RefChild
            {{}, VtValue()},
            {{}, VtValue()},
            {{}, VtValue()},
            {{}, VtValue()},

            // Node: /Internal/RefChild
            {{}, VtValue()},
            {{}, VtValue()},
            {{}, VtValue()},
            {{}, VtValue()},

            // Node: /RefParent/RefChild
            {{}, VtValue()},
            {{}, VtValue()},
            {{{1.0, TfToken("ref_sub1_1")}, {2.0, TfToken("ref_sub1_2")}}, 
                VtValue(TfToken("ref_sub1_def"))}
        });

    // Test creating resolve targets from edit targets
    {
        // Create an edit target that targets the sub2 layer with no PcpMapping
        UsdEditTarget editTarget(SdfLayer::Find("./resolveTarget/sub2.usda"));

        // Make both an "up to" and "stronger than" resolve target for 
        // /Parent/RefChild from this edit target.
        UsdResolveTarget upToEditTarget = 
            childPrim.MakeResolveTargetUpToEditTarget(editTarget);
        UsdResolveTarget strongerThanEditTarget = 
            childPrim.MakeResolveTargetStrongerThanEditTarget(editTarget);

        // Verify the resolve targets created from edit targets against the
        // expected targets established above.
        _VerifyResolveTarget(upToEditTarget, expectedTargets[3]);
        _VerifyResolveTarget(strongerThanEditTarget,
            expectedTargets[0], &expectedTargets[3]);

        // Using /Parent/RefChild.foo verify the attribute value resolves 
        // correctly based on "up to" and "stronger than" the edit target spec:
        //  root.usda: /Parent/RefChild -> 6.0
        //  sub1.usda: /Parent/RefChild -> 5.0
        //  sub2.usda: /Parent/RefChild -> 4.0 (edit target spec)
        //  ...
        _VerifyQuery<float>(UsdAttributeQuery(fooAttr, upToEditTarget), 
            {{}, VtValue(4.0f)});
        _VerifyQuery<float>(UsdAttributeQuery(fooAttr, strongerThanEditTarget), 
            {{}, VtValue(6.0f)});

        // Now set /Parent/RefChild.foo to 10.0 with the edit target.
        UsdEditContext context(stage, editTarget);
        fooAttr.Set(10.0f);

        // Like UsdPrimCompositionQuery and UsdAttributeQuery, resolve targets
        // do not listen to change notification and must be recreated if a 
        // change potentially affecting the composed scene occurs. In this case
        // authoring fooAttr's default on a layer that already has a spec for
        // it does cause recomposition, but we recreate the resolve targets 
        // anyway. 
        upToEditTarget = 
            childPrim.MakeResolveTargetUpToEditTarget(editTarget);
        strongerThanEditTarget = 
            childPrim.MakeResolveTargetStrongerThanEditTarget(editTarget);

        _VerifyResolveTarget(upToEditTarget, expectedTargets[3]);
        _VerifyResolveTarget(strongerThanEditTarget,
            expectedTargets[0], &expectedTargets[3]);

        // Verify the attribute value resolves correctly based on "up to" and 
        // "stronger than" the edit target spec's new value in sub2.usda:
        //  root.usda: /Parent/RefChild -> 6.0
        //  sub1.usda: /Parent/RefChild -> 5.0
        //  sub2.usda: /Parent/RefChild -> 10.0 (edit target spec)
        //  ...
        _VerifyQuery<float>(UsdAttributeQuery(fooAttr, upToEditTarget), 
            {{}, VtValue(10.0f)});
        _VerifyQuery<float>(UsdAttributeQuery(fooAttr, strongerThanEditTarget), 
            {{}, VtValue(6.0f)});
    }

    {
        // Create an edit target that targets the sub2 layer but maps across
        // the /Parent's internal reference to /InternalRef.
        PcpNodeRef internalRefNode = 
            parentPrim.GetPrimIndex().GetNodeProvidingSpec(
                stage->GetRootLayer(), SdfPath("/InternalRef"));
        UsdEditTarget editTarget(
            SdfLayer::Find("./resolveTarget/sub2.usda"), internalRefNode);

        // Make both an "up to" and "stronger than" resolve target for 
        // /Parent/RefChild from this edit target.
        UsdResolveTarget upToEditTarget = 
            childPrim.MakeResolveTargetUpToEditTarget(editTarget);
        UsdResolveTarget strongerThanEditTarget = 
            childPrim.MakeResolveTargetStrongerThanEditTarget(editTarget);

        // Verify the resolve targets created from edit targets against the
        // expected targets established above.
        _VerifyResolveTarget(upToEditTarget, expectedTargets[7]);
        _VerifyResolveTarget(strongerThanEditTarget,
            expectedTargets[0], &expectedTargets[7]);

        // Using /Parent/RefChild.foo verify the attribute value resolves 
        // correctly based on "up to" and "stronger than" the edit target spec:
        //  root.usda: /Parent/RefChild -> 6.0
        //  sub1.usda: /Parent/RefChild -> 5.0
        //  sub2.usda: /Parent/RefChild -> 4.0
        //  ...
        //  sub2.usda: /InternalRef/RefChild -> no spec (edit target spec)
        //  ...
        //  ref.usda: /RefParent/RefChild -> 3.0
        //  ref_sub1.usda: /RefParent/RefChild -> 2.0
        //  ref_sub2.usda: /RefParent/RefChild -> 1.0
        _VerifyQuery<float>(UsdAttributeQuery(fooAttr, upToEditTarget), 
            {{}, VtValue(3.0f)});
        _VerifyQuery<float>(UsdAttributeQuery(fooAttr, strongerThanEditTarget), 
            {{}, VtValue(6.0f)});

        // Now set /Parent/RefChild.foo to 20.0 with the edit target.
        UsdEditContext context(stage, editTarget);
        fooAttr.Set(20.0f);

        // Like mentioned above, resolve targets do not listen to change
        // notification and must be recreated if a change potentially affecting
        // the composed scene occurs. In this case authoring fooAttr's default 
        // introduces a new spec that causes a node to have specs when it didn't
        // before. We MUST recreate the resolve targets due to this change.
        upToEditTarget = 
            childPrim.MakeResolveTargetUpToEditTarget(editTarget);
        strongerThanEditTarget = 
            childPrim.MakeResolveTargetStrongerThanEditTarget(editTarget);

        _VerifyResolveTarget(upToEditTarget, expectedTargets[7]);
        _VerifyResolveTarget(strongerThanEditTarget,
            expectedTargets[0], &expectedTargets[7]);

        // Verify the attribute value resolves correctly based on "up to" and 
        // "stronger than" the edit target spec's new value in sub2.usda:
        //  root.usda: /Parent/RefChild -> 6.0
        //  sub1.usda: /Parent/RefChild -> 5.0
        //  sub2.usda: /Parent/RefChild -> 4.0
        //  ...
        //  sub2.usda: /InternalRef/RefChild -> 20.0 (edit target spec)
        //  ...
        //  ref.usda: /RefParent/RefChild -> 3.0
        //  ref_sub1.usda: /RefParent/RefChild -> 2.0
        //  ref_sub2.usda: /RefParent/RefChild -> 1.0
        _VerifyQuery<float>(UsdAttributeQuery(fooAttr, upToEditTarget), 
            {{}, VtValue(20.0f)});
        _VerifyQuery<float>(UsdAttributeQuery(fooAttr, strongerThanEditTarget), 
            {{}, VtValue(6.0f)});

    }
}

int main() {
    _TestGetAttrValueWithResolveTargets();

    printf("\n\n>>> Test SUCCEEDED\n");
}
