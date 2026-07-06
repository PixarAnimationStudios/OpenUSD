//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/diagnosticTrap.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

int
main(int argc, char const *argv[])
{
    {
        // Ensure that we emit a deprecation warning when we open a crate file
        // with a deprecated version.  Versions older than 0.8.0 are deprecated.
        TfDiagnosticTrap trap;
        SdfLayerRefPtr layer = SdfLayer::FindOrOpen("deprecated_0_7_0.usd");
        TF_AXIOM(layer);
        TF_AXIOM(trap.EraseMatching([&](TfWarning const &w) {
            return TfStringContains(w.GetCommentary(), "deprecated version");
        }) == 1);
    }

    {
        // Ensure that Export()ing to the layer's resolved path upgrades the
        // file version.
        {
            TF_AXIOM(!SdfLayer::Find("deprecated_0_7_0.usd"));
            TfDiagnosticTrap trap;
            SdfLayerRefPtr layer = SdfLayer::FindOrOpen("deprecated_0_7_0.usd");
            TF_AXIOM(layer);
            TF_AXIOM(layer->Export(layer->GetResolvedPath().GetPathString()));
        }
        // Now that we Export()ed to upgrade, we should see no warnings.
        TF_AXIOM(!SdfLayer::Find("deprecated_0_7_0.usd"));
        TfDiagnosticTrap trap;
        SdfLayerRefPtr layer = SdfLayer::FindOrOpen("deprecated_0_7_0.usd");
        TF_AXIOM(layer);
        TF_AXIOM(trap.EraseMatching([&](TfWarning const &w) {
            return TfStringContains(w.GetCommentary(), "deprecated version");
        }) == 0);
    }
    
    // Ensure no warnings are emitted reading a current version.
    SdfLayerRefPtr newLayer = SdfLayer::CreateNew("current.usdc");
    TF_AXIOM(newLayer);
    TF_AXIOM(newLayer->Save());
    newLayer.Reset();
    TF_AXIOM(!SdfLayer::Find("current.usdc"));
    {
        TfDiagnosticTrap trap;
        SdfLayerRefPtr layer = SdfLayer::FindOrOpen("current.usdc");
        TF_AXIOM(layer);
        TF_AXIOM(!trap.HasWarnings());
    }
}
