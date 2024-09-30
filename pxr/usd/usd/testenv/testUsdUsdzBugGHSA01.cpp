//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/usd/usd/stage.h"

#include <string>
#include <cstring>

PXR_NAMESPACE_USING_DIRECTIVE

// This test checks for the security issue detailed in Github security 
// advisory ghsa-4j7j-gm3f-m63w.
static void
TestUsdzFile()
{
    // This test relies on range checks that are only enabled when
    // PXR_PREFER_SAFETY_OVER_SPEED is enabled.
#ifdef PXR_PREFER_SAFETY_OVER_SPEED

    TfErrorMark m;

    auto stage = UsdStage::Open("root.usdz");
    TF_AXIOM(stage != nullptr);

    // a runtime error should have been posted
    TF_AXIOM(!m.IsClean());

    // Look for the specific runtime error for the invalid spec type
    auto invalidSpecType = [](const TfError& e) -> bool {
        return e.GetCommentary() == "Invalid spec type -32702198";
    };
    TF_AXIOM(std::any_of(m.begin(), m.end(), invalidSpecType));

    // Make sure that a corrupt asset error was also posted
    auto corruptAsset = [](const TfError& e) -> bool {
        return TfStringStartsWith(e.GetCommentary(), "Corrupt asset <") && 
               TfStringEndsWith(e.GetCommentary(),
                                "root.usdz[scene.usdc]>: "
                                "exception thrown unpacking a value, "
                                "returning an empty VtValue");
    };
    TF_AXIOM(std::any_of(m.begin(), m.end(), corruptAsset));
#endif // PXR_PREFER_SAFETY_OVER_SPEED
}

int main(int argc, char** argv)
{
    TestUsdzFile();

    return 0;
}
