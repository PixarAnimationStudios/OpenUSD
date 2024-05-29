//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/pcp/cache.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/reference.h"

#include <iostream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE;

static void
TestBug160419()
{
    std::cout << "TestBug160419..." << std::endl;

    SdfLayerRefPtr payloadLayer = SdfLayer::CreateAnonymous();
    SdfPrimSpecHandle payloadPrim = 
        SdfCreatePrimInLayer(payloadLayer, SdfPath("/Payload"));

    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous();
    SdfPrimSpecHandle refPrim = 
        SdfCreatePrimInLayer(rootLayer, SdfPath("/Ref/Child"));
    refPrim->GetPayloadList().Prepend(
        SdfPayload(payloadLayer->GetIdentifier(), payloadPrim->GetPath()));

    SdfPrimSpecHandle rootPrim = 
        SdfCreatePrimInLayer(rootLayer, SdfPath("/Root"));
    rootPrim->GetReferenceList().Prepend(
        SdfReference(std::string(), refPrim->GetPath()));
    
    PcpCache cache{PcpLayerStackIdentifier(rootLayer), std::string(), true};
    TF_AXIOM(cache.GetIncludedPayloads().empty());

    PcpErrorVector errors;
    cache.ComputePrimIndexesInParallel(
        SdfPath("/"), &errors, 
        [](const PcpPrimIndex&, TfTokenVector*) { return true; },
        [](const SdfPath&) { return true; });

    TF_AXIOM(errors.empty());
    TF_AXIOM((cache.GetIncludedPayloads() == 
              PcpCache::PayloadSet {
                  SdfPath("/Ref/Child"), SdfPath("/Root") }));
}

int main(int argc, char** argv)
{
    TestBug160419();

    std::cout << "Passed!" << std::endl;

    return EXIT_SUCCESS;
}
