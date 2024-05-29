//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerHints.h"
#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestSdfLayerHintsMaybeHasRelocates()
{
    // Test empty layer hints
    {
        SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
        SdfLayerHints emptyLayerHints = layer->GetHints();
        TF_AXIOM(!emptyLayerHints.mightHaveRelocates);
    }

    // Open layer without relocates
    {
        SdfLayerRefPtr layer = SdfLayer::FindOrOpen(
            "testSdfLayerHints.testenv/without_relocates.sdf");
        TF_AXIOM(!layer->GetHints().mightHaveRelocates);
    }

    // Open layer with relocates
    {
        SdfLayerRefPtr layer = SdfLayer::FindOrOpen(
            "testSdfLayerHints.testenv/with_relocates.sdf");
        TF_AXIOM(layer->GetHints().mightHaveRelocates);
    }

    // Author relocates
    {
        SdfLayerRefPtr layer = SdfLayer::FindOrOpen(
            "testSdfLayerHints.testenv/without_relocates.sdf");
        TF_AXIOM(!layer->GetHints().mightHaveRelocates);

        SdfPrimSpecHandle prim = layer->GetPrimAtPath(SdfPath("/Prim"));
        prim->SetRelocates({{ SdfPath("Prim"), SdfPath("Prim") }});
        TF_AXIOM(layer->GetHints().mightHaveRelocates);
    }

    // Author something that is not relocates
    {
        SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
        bool createdPrim = SdfJustCreatePrimInLayer(layer, SdfPath("Prim"));
        TF_AXIOM(createdPrim);
        TF_AXIOM(layer->GetHints().mightHaveRelocates);
    }

    // Transfer content without relocates
    {
        SdfLayerRefPtr srcLayer = SdfLayer::FindOrOpen(
            "testSdfLayerHints.testenv/without_relocates.sdf");
        SdfLayerRefPtr dstLayer = SdfLayer::CreateAnonymous();
        dstLayer->TransferContent(srcLayer);
        // Ideally, this would not hint maybe-has-relocates because the source
        // layer does not have relocates but TransferContent dirties the
        // layer.
        TF_AXIOM(dstLayer->GetHints().mightHaveRelocates);
    }

    // Transfer content with relocates
    {
        SdfLayerRefPtr srcLayer = SdfLayer::FindOrOpen(
            "testSdfLayerHints.testenv/with_relocates.sdf");
        SdfLayerRefPtr dstLayer = SdfLayer::CreateAnonymous();
        dstLayer->TransferContent(srcLayer);
        TF_AXIOM(dstLayer->GetHints().mightHaveRelocates);
    }

    // Import without relocates
    {
        SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
        layer->Import("testSdfLayerHints.testenv/without_relocates.sdf");
        // Similar to TransferContent, importing layer contents dirties the
        // layer and, therefore, hints indicate that there may be relocates.
        TF_AXIOM(layer->GetHints().mightHaveRelocates);
    }

    // Import with relocates
    {
        SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
        layer->Import("testSdfLayerHints.testenv/with_relocates.sdf");
        TF_AXIOM(layer->GetHints().mightHaveRelocates);
    }

    // Save layer without relocates
    {
        SdfLayerRefPtr layer = SdfLayer::CreateNew(
            ArchMakeTmpFileName("testSdfLayerHints_", ".sdf"));
        SdfPrimSpecHandle prim = SdfCreatePrimInLayer(layer, SdfPath("Prim"));
        TF_AXIOM(prim);
        layer->Save();
        TF_AXIOM(layer->GetHints().mightHaveRelocates);
    }

    // Save layer with relocates
    {
        SdfLayerRefPtr layer = SdfLayer::CreateNew(
            ArchMakeTmpFileName("testSdfLayerHints_", ".sdf"));
        TF_AXIOM(!layer->GetHints().mightHaveRelocates);
        SdfPrimSpecHandle prim = SdfCreatePrimInLayer(layer, SdfPath("Prim"));
        TF_AXIOM(prim);
        prim->SetRelocates({{ SdfPath("Prim"), SdfPath("Prim") }});
        TF_AXIOM(layer->GetHints().mightHaveRelocates);
        layer->Save();
        TF_AXIOM(layer->GetHints().mightHaveRelocates);
    }

    // Attempt to save a layer that cannot be saved and ensure that the
    // relocates hint is still correct after the failure.
    {
        SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
        TF_AXIOM(!layer->GetHints().mightHaveRelocates);
        SdfPrimSpecHandle prim = SdfCreatePrimInLayer(layer, SdfPath("Prim"));
        TF_AXIOM(prim);
        prim->SetRelocates({{ SdfPath("Prim"), SdfPath("Prim") }});
        TF_AXIOM(layer->GetHints().mightHaveRelocates);
        {
            TfErrorMark m;
            bool success = layer->Save();
            m.Clear();
            TF_AXIOM(!success);
        }
        TF_AXIOM(layer->GetHints().mightHaveRelocates);
    }

    // Export without relocates
    {
        SdfLayerRefPtr layer = SdfLayer::FindOrOpen(
            "testSdfLayerHints.testenv/without_relocates.sdf");
        layer->Export(ArchMakeTmpFileName("testSdfLayerHints_", ".sdf"));
        TF_AXIOM(!layer->GetHints().mightHaveRelocates);
    }

    // Export with relocates
    {
        SdfLayerRefPtr layer = SdfLayer::FindOrOpen(
            "testSdfLayerHints.testenv/with_relocates.sdf");
        layer->Export(ArchMakeTmpFileName("testSdfLayerHints_", ".sdf"));
        TF_AXIOM(layer->GetHints().mightHaveRelocates);
    }

    // Clear without relocates
    {
        SdfLayerRefPtr layer = SdfLayer::FindOrOpen(
            "testSdfLayerHints.testenv/without_relocates.sdf");
        layer->Clear();
        TF_AXIOM(layer->GetHints().mightHaveRelocates);
    }

    // Clear with relocates
    {
        SdfLayerRefPtr layer = SdfLayer::FindOrOpen(
            "testSdfLayerHints.testenv/with_relocates.sdf");
        layer->Clear();
        TF_AXIOM(layer->GetHints().mightHaveRelocates);
    }
}

int
main(int argc, char **argv)
{
    TestSdfLayerHintsMaybeHasRelocates();

    return 0;
}
