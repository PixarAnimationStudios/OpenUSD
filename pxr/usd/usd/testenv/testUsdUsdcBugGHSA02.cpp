//
// Copyright 2025 Pixar
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
// advisory GHSA-58p5-r2f6-g2cj.
static void
TestUsdcFile()
{
    // This test relies on range checks that are only enabled when
    // PXR_PREFER_SAFETY_OVER_SPEED is enabled.
#ifdef PXR_PREFER_SAFETY_OVER_SPEED

    TfErrorMark m;
    auto stage = UsdStage::Open("root.usdc");

    // a runtime error should have been posted
    TF_AXIOM(!m.IsClean());

    // Look for the specific runtime error for the invalid spec type
    auto decompressError = [](const TfError& e) -> bool {
        return TfStringEndsWith(e.GetCommentary(),
                                "Failed to decompress data, "
                                "possibly corrupt? LZ4 error code: -596");
    };
    TF_AXIOM(std::any_of(m.begin(), m.end(), decompressError));

    // Make sure that a corrupt asset error was also posted
    auto corruptPathIndex = [](const TfError& e) -> bool {
        return e.GetCommentary() == "Corrupt path index in crate file "
                                    "(0 repeated)";
    };
    TF_AXIOM(std::any_of(m.begin(), m.end(), corruptPathIndex));
#endif // PXR_PREFER_SAFETY_OVER_SPEED
}

int main(int argc, char** argv)
{
    TestUsdcFile();

    return 0;
}
