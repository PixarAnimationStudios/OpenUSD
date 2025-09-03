//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/exec/systemDiagnostics.h"

#include "pxr/exec/vdf/context.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/trace/collector.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/xform.h"

#include <fstream>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

static const std::string layerContents =
    R"usda(#usda 1.0
(
    defaultPrim = "Root"
    metersPerUnit = 0.01
    upAxis = "Z"
)
def Xform "Root" (
    kind = "component"
)
{
    def Xform "A1"
    {
        matrix4d xf = ( (2, 0, 0, 0), (0, 2, 0, 0), (0, 0, 2, 0), (0, 0, 0, 1) )
        def Xform "B"
        {
            matrix4d xf = ( (3, 0, 0, 0), (0, 3, 0, 0), (0, 0, 3, 0), (0, 0, 0, 1) )
        }
    }
    def Xform "A2"
    {
        matrix4d xf = ( (5, 0, 0, 0), (0, 5, 0, 0), (0, 0, 5, 0), (0, 0, 0, 1) )
    }
}
)usda";

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (computeXf)
    (parentXf)
    (xf)
);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(UsdGeomXform)
{
    self.PrimComputation(_tokens->computeXf)
    .Callback<GfMatrix4d>(+[](const VdfContext &ctx) {
        ctx.SetOutput(GfMatrix4d(1));
    })
    .Inputs(
        AttributeValue<GfMatrix4d>(_tokens->xf),
        NamespaceAncestor<GfMatrix4d>(_tokens->computeXf)
            .InputName(_tokens->parentXf)
    );
}

static void
TestCompiler()
{
    TraceCollector::GetInstance().SetEnabled(true);

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(layerContents);
    UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    ExecUsdSystem execSystem(usdStage);

    std::vector<ExecUsdValueKey> valueKeys {
        {usdStage->GetPrimAtPath(SdfPath("/Root/A1/B")), _tokens->computeXf}
    };

    ExecUsdRequest request = execSystem.BuildRequest(std::move(valueKeys));
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    ExecUsdSystem::Diagnostics execSystemDiagnostics(&execSystem);
    execSystemDiagnostics.GraphNetwork("testCompiler.dot");

    TraceCollector::GetInstance().SetEnabled(false);
    
    std::ofstream traceFile("testCompiler.spy");
    TraceReporter::GetGlobalReporter()->UpdateTraceTrees();
    TraceReporter::GetGlobalReporter()->SerializeProcessedCollections(
        traceFile);
}

int main()
{
    TestCompiler();
}
