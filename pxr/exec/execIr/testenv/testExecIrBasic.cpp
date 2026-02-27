//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execIr/tokens.h"
#include "pxr/exec/execIr/types.h"

#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

PXR_NAMESPACE_USING_DIRECTIVE

static void
Test_IrForwardCompute()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(
        #usda 1.0

        def Scope "Root" (
            kind = "component"
        )
        {
            def IrFkController "FkController" {
                double In:Rx = 90.0
                double In:Ry = -90.0
                double In:Rz = 90.0
                double In:Tx = 1.0
                double In:Ty = 2.0
                double In:Tz = 3.0
            }
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/Root/FkController"));
    TF_AXIOM(prim);

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest request = execSystem.BuildRequest({
        ExecUsdValueKey{prim, ExecIrTokens->forwardCompute}
    });
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    {
        TfErrorMark mark;

        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(!value.IsEmpty());
        TF_AXIOM(GfIsClose(
                     value.Get<GfMatrix4d>(),
                     GfMatrix4d(0, 0, 1, 0,
                                0, -1, 0, 0,
                                1, 0, 0, 0,
                                1, 2, 3, 1),
                     1e-6));

        TF_AXIOM(mark.IsClean());
    }

    // Now set the parent space and compute again.
    {
        const UsdAttribute parentSpace =
            prim.GetAttribute(TfToken("ParentIn:Space"));
        TF_AXIOM(parentSpace);
        parentSpace.Set(GfMatrix4d(1, 0, 0, 0,
                                   0, 1, 0, 0,
                                   0, 0, 1, 0,
                                   1, 1, 1, 1));

        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(!value.IsEmpty());
        TF_AXIOM(GfIsClose(
                     value.Get<GfMatrix4d>(),
                     GfMatrix4d(0, 0, 1, 0,
                                0, -1, 0, 0,
                                1, 0, 0, 0,
                                2, 3, 4, 1),
                     1e-6));
    }
}

static void
Test_IrInverseCompute()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(
        #usda 1.0

        def Scope "Root" (
            kind = "component"
        )
        {
            def IrFkController "FkController" {
                matrix4d Out:Space = ((0,  0, 1, 0),
                                      (0, -1, 0, 0),
                                      (1,  0, 0, 0),
                                      (1,  2, 3, 1))
            }
        }
        )usda");

    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/Root/FkController"));
    TF_AXIOM(prim);

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest request = execSystem.BuildRequest({
        ExecUsdValueKey{prim, TfToken("inverseCompute")}
    });
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    {
        TfErrorMark mark;

        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(!value.IsEmpty());
        const ExecIrInversionResult valueMap =
            value.Get<ExecIrInversionResult>();

        const std::vector<std::pair<const char *, double>> expected{{
            {"In:Rx", 90.0},
            {"In:Ry", -90.0},
            {"In:Rz", 90.0},
            {"In:Tx", 1.0},
            {"In:Ty", 2.0},
            {"In:Tz", 3.0},
        }};
        for (const auto &entry : expected) {
            const auto it = valueMap.find(TfToken(entry.first));
            TF_AXIOM(it != valueMap.end());
            TF_AXIOM(GfIsClose(it->second.Get<double>(), entry.second, 1e-6));
        }

        TF_AXIOM(mark.IsClean());
    }

    // Now set the parent space and compute again.
    {
        const UsdAttribute parentSpace =
            prim.GetAttribute(TfToken("ParentIn:Space"));
        TF_AXIOM(parentSpace);
        parentSpace.Set(GfMatrix4d(1, 0, 0, 0,
                                   0, 1, 0, 0,
                                   0, 0, 1, 0,
                                   1, 1, 1, 1));

        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(!value.IsEmpty());
        const ExecIrInversionResult valueMap =
            value.Get<ExecIrInversionResult>();

        const std::vector<std::pair<const char *, double>> expected{{
            {"In:Rx", 90.0},
            {"In:Ry", -90.0},
            {"In:Rz", 90.0},
            {"In:Tx", 0.0},
            {"In:Ty", 1.0},
            {"In:Tz", 2.0},
        }};
        for (const auto &entry : expected) {
            const auto it = valueMap.find(TfToken(entry.first));
            TF_AXIOM(it != valueMap.end());
            TF_AXIOM(GfIsClose(it->second.Get<double>(), entry.second, 1e-6));
        }
    }
}

int main(int argc, char **argv)
{
    Test_IrForwardCompute();
    Test_IrInverseCompute();
}
