//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/quatd.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/arrayEdit.h"
#include "pxr/base/vt/value.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

// Each pair below holds two values that compare equal with operator== but
// differ in the sign of a zero.  The usdc writer deduplicates values, so it
// must not let one of them share the other's storage.
static const char *_layerText = R"(#usda 1.2
def "p"
{
    float[] a = [0, 1]
    float[] b = [-0, 1]
    double[] da = [0]
    double[] db = [-0]
    float3[] pa = [(0, 0, 0)]
    float3[] pb = [(-0, 0, 0)]
    quatd[] qa = [(1, 0, 0, 0)]
    quatd[] qb = [(1, -0, 0, 0)]
    double3 va = (0, 0.1, 0)
    double3 vb = (-0, 0.1, 0)
    matrix2d ma = ( (1.5, 0), (0, 1) )
    matrix2d mb = ( (1.5, -0), (0, 1) )
    float[] ea = edit [ append 0 ]
    float[] eb = edit [ append -0 ]
}
)";

template <class T>
static bool
_BitwiseEqual(T const &x, T const &y)
{
    return memcmp(&x, &y, sizeof(T)) == 0;
}

template <class T>
static bool
_BitwiseEqual(VtArray<T> const &x, VtArray<T> const &y)
{
    return x.size() == y.size() &&
        (x.empty() || memcmp(x.cdata(), y.cdata(), sizeof(T) * x.size()) == 0);
}

template <class T>
static bool
_BitwiseEqual(VtArrayEdit<T> const &x, VtArrayEdit<T> const &y)
{
    return _BitwiseEqual(x.ComposeOver(VtArray<T>()),
                         y.ComposeOver(VtArray<T>()));
}

static VtValue
_GetDefault(SdfLayerHandle const &layer, std::string const &name)
{
    return layer->GetField(SdfPath("/p." + name), SdfFieldKeys->Default);
}

// Check that attribute `name` has a value of type T in `src` and the same
// bits in `dst`.
template <class T>
static int
_CheckRoundTrip(SdfLayerHandle const &src, SdfLayerHandle const &dst,
                std::string const &name)
{
    VtValue const srcVal = _GetDefault(src, name);
    VtValue const dstVal = _GetDefault(dst, name);
    TF_AXIOM(srcVal.IsHolding<T>());
    if (!dstVal.IsHolding<T>()) {
        printf("FAILED: %s has type %s after round trip\n",
               name.c_str(), dstVal.GetTypeName().c_str());
        return 1;
    }
    if (!_BitwiseEqual(srcVal.UncheckedGet<T>(), dstVal.UncheckedGet<T>())) {
        printf("FAILED: %s changed from %s to %s after round trip\n",
               name.c_str(), TfStringify(srcVal).c_str(),
               TfStringify(dstVal).c_str());
        return 1;
    }
    printf("ok: %s\n", name.c_str());
    return 0;
}

int
main(int argc, char const *argv[])
{
    SdfLayerRefPtr src = SdfLayer::CreateAnonymous(".usda");
    TF_AXIOM(src->ImportFromString(_layerText));

    // Make sure the text parser kept the negative zeros, so that a failure
    // below can only come from the usdc round trip.
    TF_AXIOM(std::signbit(
        _GetDefault(src, "b").UncheckedGet<VtFloatArray>()[0]));
    TF_AXIOM(std::signbit(
        _GetDefault(src, "vb").UncheckedGet<GfVec3d>()[0]));

    TF_AXIOM(src->Export("signedZero.usdc"));
    SdfLayerRefPtr dst = SdfLayer::FindOrOpen("signedZero.usdc");
    TF_AXIOM(dst);

    int failures = 0;
    for (char const *name: {"a", "b"}) {
        failures += _CheckRoundTrip<VtFloatArray>(src, dst, name);
    }
    for (char const *name: {"da", "db"}) {
        failures += _CheckRoundTrip<VtDoubleArray>(src, dst, name);
    }
    for (char const *name: {"pa", "pb"}) {
        failures += _CheckRoundTrip<VtVec3fArray>(src, dst, name);
    }
    for (char const *name: {"qa", "qb"}) {
        failures += _CheckRoundTrip<VtQuatdArray>(src, dst, name);
    }
    for (char const *name: {"va", "vb"}) {
        failures += _CheckRoundTrip<GfVec3d>(src, dst, name);
    }
    for (char const *name: {"ma", "mb"}) {
        failures += _CheckRoundTrip<GfMatrix2d>(src, dst, name);
    }
    for (char const *name: {"ea", "eb"}) {
        failures += _CheckRoundTrip<VtArrayEdit<float>>(src, dst, name);
    }

    printf("%d failure(s)\n", failures);
    fflush(stdout);
    TF_AXIOM(failures == 0);
    return 0;
}
