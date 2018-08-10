//
// Copyright 2018 Pixar
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
    refPrim->SetPayload(
        SdfPayload(payloadLayer->GetIdentifier(), payloadPrim->GetPath()));

    SdfPrimSpecHandle rootPrim = 
        SdfCreatePrimInLayer(rootLayer, SdfPath("/Root"));
    rootPrim->GetReferenceList().Prepend(
        SdfReference(std::string(), refPrim->GetPath()));
    
    PcpCache cache{PcpLayerStackIdentifier(rootLayer), std::string(), true};
    TF_AXIOM(cache.GetIncludedPayloads() == SdfPathSet());

    PcpErrorVector errors;
    cache.ComputePrimIndexesInParallel(
        SdfPath("/"), &errors, 
        [](const PcpPrimIndex&, TfTokenVector*) { return true; },
        [](const SdfPath&) { return true; });

    TF_AXIOM(errors.empty());
    TF_AXIOM((cache.GetIncludedPayloads() == 
              SdfPathSet{SdfPath("/Ref/Child"), SdfPath("/Root")}));
}

int main(int argc, char** argv)
{
    TestBug160419();

    std::cout << "Passed!" << std::endl;

    return EXIT_SUCCESS;
}
