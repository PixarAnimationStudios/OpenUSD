//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Verifies that the safety-over-speed checks in the .usdc reader trip on
// malformed array sizes.  The body is compiled out for all cases but
// baseline.usdc when PXR_PREFER_SAFETY_OVER_SPEED is not defined.
//
// Test files:
//   baseline.usdc                 -- well-formed, opens and reads cleanly
//   bad_oversize_zerocopy.usdc    -- half2[] count = 2^32
//   bad_overflow_size.usdc        -- half2[] count = 2^64-1
//   bad_moderate_oversize.usdc    -- half2[] count = 5000
//   bad_str_oversize.usdc         -- string[] count = 100000
//   bad_obsolete_version.usdc     -- 96 bytes: a bootstrap with version 0.0.0
//                                    and tocOffset 88, followed by a TOC
//                                    section count of 2^64-1
//

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/setenv.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"

#include <cstdio>
#include <optional>

PXR_NAMESPACE_USING_DIRECTIVE

// Open `layerPath` (which should succeed) and read the default value from
// `attrPath`, which should trip a safety check and emit at least one TfError if
// `expectError` is true, and no errors otherwise.
static void
_Test(char const *layerPath, SdfPath const &attrPath, bool expectError)
{
    SdfLayerRefPtr layer = SdfLayer::FindOrOpen(layerPath);
    if (!TF_VERIFY(layer, "failed to open %s", layerPath)) {
        return;
    }
    SdfAttributeSpecHandle attr = layer->GetAttributeAtPath(attrPath);
    if (!TF_VERIFY(attr, "no spec at %s in %s",
                   attrPath.GetText(), layerPath)) {
        return;
    }
    // Read the possibly malformed default -- this is where the safety check
    // will fire.
    std::optional<TfErrorMark> mark;
    mark.emplace();
    VtValue val = attr->GetDefaultValue();
    if (!mark->IsClean() && !expectError) {
        fprintf(stderr, "Failed due to unexpected errors:\n");
        mark.reset(); // destroy mark to report errors.
        TF_FATAL_ERROR("Failed");
    }
    if (mark->IsClean() && expectError) {
        fprintf(stderr, "Failed -- expected errors but none issued\n");
        TF_FATAL_ERROR("Failed");
    }
    mark->Clear();
}

// Open `layerPath`, which has an obsolete version and a malformed table of
// contents.  The open should fail with exactly one error, the obsolete version
// error, showing that nothing past the bootstrap was read.
static void
_TestObsolete(char const *layerPath)
{
    std::optional<TfErrorMark> mark;
    mark.emplace();
    SdfLayerRefPtr layer = SdfLayer::FindOrOpen(layerPath);
    size_t numErrors = 0;
    bool foundObsolete = false;
    for (TfError const &err: *mark) {
        ++numErrors;
        foundObsolete |= TfStringContains(
            err.GetCommentary(), "obsolete version");
    }
    if (layer || numErrors != 1 || !foundObsolete) {
        mark.reset(); // destroy mark to report errors.
        TF_FATAL_ERROR(
            "Failed -- expected only an obsolete version error "
            "opening %s, got layer=%s and %zu error(s):\n", layerPath,
            layer ? "valid" : "null", numErrors);
    }
    mark->Clear();
}

int
main()
{    
    _Test("baseline.usdc",
          SdfPath("/Root.zcPoints"), /*error=*/false);

    _TestObsolete("bad_obsolete_version.usdc");

#ifdef PXR_PREFER_SAFETY_OVER_SPEED
    // These tests rely on safety checks that only exist when
    // PXR_PREFER_SAFETY_OVER_SPEED is defined.
    _Test("bad_oversize_zerocopy.usdc",
          SdfPath("/Root.zcPoints"), /*error=*/true);
    _Test("bad_overflow_size.usdc",
          SdfPath("/Root.zcPoints"), /*error=*/true);
    _Test("bad_moderate_oversize.usdc",
          SdfPath("/Root.zcPoints"), /*error=*/true);
    _Test("bad_str_oversize.usdc",
          SdfPath("/Root.strs"), /*error=*/true);
#endif
    printf("PASSED\n");
    return 0;
}
