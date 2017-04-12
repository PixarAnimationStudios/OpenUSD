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
#include "pxr/usd/pcp/mapExpression.h"
#include "pxr/usd/pcp/mapFunction.h"

PXR_NAMESPACE_USING_DIRECTIVE

static PcpMapFunction
_GetArcFunction(const std::string & source, const std::string & target)
{
    PcpMapFunction::PathMap pathMap;
    pathMap[SdfPath(source)] = SdfPath(target);
    return PcpMapFunction::Create(pathMap, SdfLayerOffset());
}

int 
main(int argc, char** argv)
{
    // Here we focus on testing the core PcpMapExpression API;
    // We don't bother testing convenience API that passes
    // queries onto the MapFunction value.

    // Accumulate unique test expressions.
    std::vector<PcpMapExpression> testExprs;

    // Null
    PcpMapExpression nullExpr;
    testExprs.push_back(nullExpr);
    assert(nullExpr.IsNull());
    assert(nullExpr.Evaluate() == PcpMapFunction());

    // Identity
    const PcpMapExpression identityExpr = PcpMapExpression::Identity();
    testExprs.push_back(identityExpr);
    assert(!identityExpr.IsNull());
    assert(identityExpr.Evaluate() == PcpMapFunction::Identity());

    // Swap
    PcpMapExpression a;
    PcpMapExpression b = PcpMapExpression::Identity();
    assert(a.IsNull());
    assert(!b.IsNull());
    a.Swap(b);
    assert(!a.IsNull());
    assert(b.IsNull());
    a.Swap(a);
    assert(!a.IsNull());
    
    // Constant (a typical model reference)
    const PcpMapFunction refFunc =
        _GetArcFunction("/Model", "/World/anim/Model_1");
    const PcpMapExpression refExpr = PcpMapExpression::Constant(refFunc);
    testExprs.push_back(refExpr);
    assert(refExpr.Evaluate() == refFunc);

    // Operation: Inverse
    const PcpMapExpression refExprInverse = refExpr.Inverse();
    testExprs.push_back(refExprInverse);
    assert(!refExprInverse.IsNull());
    assert(refExprInverse.Evaluate() == refFunc.GetInverse());

    // Operation: AddRootIdentity
    const PcpMapExpression rootIdentityExpr = refExpr.AddRootIdentity();
    testExprs.push_back(rootIdentityExpr);
    assert(refExpr.MapSourceToTarget(SdfPath("/Foo")) == SdfPath());
    assert(rootIdentityExpr.MapSourceToTarget(SdfPath("/Foo"))
           == SdfPath("/Foo"));

    // Operation: Compose
    const PcpMapExpression rigExpr = PcpMapExpression::Constant(
        _GetArcFunction("/Rig", "/Model/Rig"));
    const PcpMapExpression composedExpr = refExpr.Compose(rigExpr);
    testExprs.push_back(composedExpr);
    assert(composedExpr.Evaluate() ==
           _GetArcFunction("/Rig", "/World/anim/Model_1/Rig"));

    // Operation: Compose + Inverse
    assert(composedExpr.Inverse().Evaluate() ==
        _GetArcFunction("/World/anim/Model_1/Rig", "/Rig"));

    // Variable
    {
        // Variable will initial empty function
        PcpMapExpression::VariableRefPtr var =
            PcpMapExpression::NewVariable( PcpMapFunction() );
        const PcpMapExpression varExpr = var->GetExpression();
        testExprs.push_back(varExpr);
        assert(!varExpr.IsNull());
        assert(varExpr.Evaluate() == var->GetValue());
        assert(varExpr.Evaluate() == PcpMapFunction());

        // Test changing value
        const PcpMapFunction testValue =
            _GetArcFunction("/A", "/B");
        var->SetValue(testValue);
        assert(varExpr.Evaluate() == var->GetValue());
        assert(varExpr.Evaluate() == testValue);

        // Test using a variable in a derived expression
        const PcpMapExpression invVarExpr = var->GetExpression().Inverse();
        assert(invVarExpr.Evaluate() == testValue.GetInverse());

        // Test invalidation on changing a variable
        const PcpMapFunction testValue2 =
            _GetArcFunction("/A2", "/B2");
        var->SetValue(testValue2);
        assert(varExpr.Evaluate() == testValue2);
        assert(invVarExpr.Evaluate() == testValue2.GetInverse());

        // Test variable lifetime.
        // Change the variable value, discard it, and then
        // re-evaluate derived expressions.
        const PcpMapFunction testValue3 =
            _GetArcFunction("/A3", "/B3");
        var->SetValue(testValue3);
        var.reset();
        assert(varExpr.Evaluate() == testValue3);
        assert(invVarExpr.Evaluate() == testValue3.GetInverse());
    }

    // Semi-tricky AddRootIdentity scenario:
    //
    // Composing one expression over another expression with an
    // AddRootIdentity() component can cause there to not be a
    // root identity mapping in the result.
    {
        const PcpMapFunction a_to_b = _GetArcFunction("/A", "/B");
        const PcpMapFunction b_to_c = _GetArcFunction("/B", "/C");
        const PcpMapFunction a_to_c = _GetArcFunction("/A", "/C");
        const PcpMapExpression exp =
            PcpMapExpression::Constant(b_to_c)
            .Compose( PcpMapExpression::Constant(a_to_b).AddRootIdentity() );
        assert(exp.Evaluate() == a_to_c);


        const PcpMapExpression a_to_c_with_id = 
            PcpMapExpression::Constant(a_to_c).AddRootIdentity();
        const PcpMapExpression exp_with_id = 
            exp.AddRootIdentity();
        assert(exp_with_id.Evaluate() == a_to_c_with_id.Evaluate());
    }

    // TODO: test equality/inequality for testExprs
    // XXX do this after flyweighting is in place
    // also demonstrate that two exprs that evaluate two the
    // same value won't compare equal if their structure is different
}
