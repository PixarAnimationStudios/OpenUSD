//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/tf/diagnostic.h"

#include <iostream>
#include <string>
#include <vector>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE;

static void
TestSubLayersAndOffsetsMismatch()
{
    std::cout << "TestSubLayersAndOffsetsMismatch..." << std::endl;

    SdfLayerRefPtr sublayer = SdfLayer::CreateAnonymous(".usda");
    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous(".usda");

    // Author sublayers directly, bypassing the sublayer list editor, so that
    // the sublayerOffsets field is left unauthored.
    rootLayer->SetField(
        SdfPath::AbsoluteRootPath(), SdfFieldKeys->SubLayers,
        std::vector<std::string>{sublayer->GetIdentifier()});

    TF_AXIOM(rootLayer->GetSubLayerPaths().size() == 1);
    TF_AXIOM(rootLayer->GetSubLayerOffsets().empty());

    const PcpLayerStackIdentifier id(rootLayer);
    PcpCache cache(id, std::string(), true);
    PcpErrorVector errors;
    PcpLayerStackRefPtr layerStack = cache.ComputeLayerStack(id, &errors);
    TF_AXIOM(layerStack);
    TF_AXIOM(errors.size() == 1);
    PcpErrorInvalidSublayerAndOffsetCountPtr error = 
        std::dynamic_pointer_cast<PcpErrorInvalidSublayerAndOffsetCount>(
            errors[0]);
    TF_AXIOM(error);
    TF_AXIOM(error->layer == rootLayer);
    TF_AXIOM(error->sublayerCount == 1);
    TF_AXIOM(error->offsetCount == 0);

    // We should still have a sublayer with identity offset, so that the layer
    // stack is valid.
    const SdfLayerRefPtrVector& layers = layerStack->GetLayers();
    TF_AXIOM(layers.size() == 2);
    TF_AXIOM(layers[0] == rootLayer);
    TF_AXIOM(layers[1] == sublayer);
    TF_AXIOM(layerStack->GetLayerOffsetForLayer(sublayer) == nullptr);
}

int main(int argc, char** argv)
{
    TestSubLayersAndOffsetsMismatch();
    std::cout << "TestSubLayersAndOffsetsMismatch passed." << std::endl;
    return EXIT_SUCCESS;
}
