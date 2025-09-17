//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/shaderMetadataHelpers.h"

PXR_NAMESPACE_USING_DIRECTIVE

void
TestSdrParseValue()
{
    // Check parse behavior given a basic int property
    SdrShaderProperty propInt = SdrShaderProperty(
        TfToken("name"),
        SdrPropertyTypes->Int, // sdrType
        VtValue(5),
        false,
        0, // arraysize
        {}, {}, {}
    );
    std::string err;
    VtValue val;
    val = ShaderMetadataHelpers::ParseSdfValue("3", &propInt, &err);
    TF_VERIFY(val.IsHolding<int>());
    TF_VERIFY(val == VtValue(3));
    TF_VERIFY(err.empty());
    val = ShaderMetadataHelpers::ParseSdfValue("huh 3,", &propInt, &err);
    TF_VERIFY(val.IsEmpty());
    TF_VERIFY(!err.empty());
    err.clear();

    // Check parse behavior given an array type property
    SdrShaderProperty propIntArr = SdrShaderProperty(
        TfToken("name"),
        SdrPropertyTypes->Int, // sdrType
        VtValue(VtArray<int>(2)),
        false,
        2, // arraysize
        {}, {}, {}
    );
    val = ShaderMetadataHelpers::ParseSdfValue("3, 2", &propIntArr, &err);
    TF_VERIFY(val.IsHolding<GfVec2i>());
    TF_VERIFY((val == GfVec2i{3, 2}));
    TF_VERIFY(err.empty());
    // Fixed length arrays and tuples are unconditionally surrounded by ().
    val = ShaderMetadataHelpers::ParseSdfValue("(3, 2)", &propIntArr, &err);
    TF_VERIFY(val.IsEmpty());
    TF_VERIFY(!err.empty());
    err.clear();
    // Too many values for fixed array of length 2.
    val = ShaderMetadataHelpers::ParseSdfValue("3, 2, 1", &propIntArr, &err);
    TF_VERIFY(val.IsEmpty());
    TF_VERIFY(!err.empty());
    err.clear();
    // Unrecognized brace style for int tuple.
    val = ShaderMetadataHelpers::ParseSdfValue("{3, 2}", &propIntArr, &err);
    TF_VERIFY(val.IsEmpty());
    TF_VERIFY(!err.empty());
    err.clear();
    // Not enough values; this call will error
    val = ShaderMetadataHelpers::ParseSdfValue("3", &propIntArr, &err);
    TF_VERIFY(val.IsEmpty());
    TF_VERIFY(!err.empty());
    err.clear();

    // Check parse behavior given a special type property that has an Sdf
    // type equivalent of SdfValueTypeNames->Token
    SdrShaderProperty propTerm = SdrShaderProperty(
        TfToken("name"),
        SdrPropertyTypes->Terminal, // sdrType
        VtValue("foo"),
        false,
        0, // arraysize
        {}, {}, {}
    );
    val = ShaderMetadataHelpers::ParseSdfValue("3, 2", &propTerm, &err);
    TF_VERIFY(val.IsHolding<TfToken>());
    TF_VERIFY((val == TfToken("3, 2")));
    TF_VERIFY(err.empty());

    // Check parse behavior given a string property
    SdrShaderProperty propString = SdrShaderProperty(
        TfToken("name"),
        SdrPropertyTypes->String, // sdrType
        VtValue("foo"),
        false,
        0, // arraysize
        {}, {}, {}
    );
    val = ShaderMetadataHelpers::ParseSdfValue("", &propString, &err);
    TF_VERIFY(val.IsHolding<std::string>());
    TF_VERIFY((val == std::string("")));
    TF_VERIFY(err.empty());
    val = ShaderMetadataHelpers::ParseSdfValue("\tsome \"string\" stuff",
                                               &propString, &err);
    TF_VERIFY(val.IsHolding<std::string>());
    TF_VERIFY((val == "\tsome \"string\" stuff"));
    TF_VERIFY(err.empty());
    val = ShaderMetadataHelpers::ParseSdfValue("foo", &propString, &err);
    TF_VERIFY(val.IsHolding<std::string>());
    TF_VERIFY((val == "foo"));
    TF_VERIFY(err.empty());
    // Eol characters are not supported in ParseSdfValue. We could escape
    // them, but there should be a good use case for them before we add
    // such support.
    val = ShaderMetadataHelpers::ParseSdfValue("foo\nfoo", &propString, &err);
    TF_VERIFY(val.IsHolding<std::string>());
    TF_VERIFY((val == "foo\nfoo"));
    TF_VERIFY(err.empty());

    // Check parse behavior given an asset property
    SdrShaderProperty propAsset = SdrShaderProperty(
        TfToken("name"),
        SdrPropertyTypes->String, // sdrType
        VtValue("foo"),
        false,
        0, // arraysize
        {{SdrPropertyMetadata->IsAssetIdentifier, "true"}}, // metadata
        {}, {}
    );
    val = ShaderMetadataHelpers::ParseSdfValue("@/some/path@",
                                               &propAsset, &err);
    TF_VERIFY(val.IsHolding<SdfAssetPath>());
    TF_VERIFY((val == SdfAssetPath("@/some/path@")));
    TF_VERIFY(err.empty());
    val = ShaderMetadataHelpers::ParseSdfValue("how/about/relative/paths",
                                               &propAsset, &err);
    TF_VERIFY(val.IsHolding<SdfAssetPath>());
    TF_VERIFY((val == SdfAssetPath("how/about/relative/paths")));
    TF_VERIFY(err.empty());
    val = ShaderMetadataHelpers::ParseSdfValue("/@some/@path",
                                               &propAsset, &err);
    TF_VERIFY(val.IsHolding<SdfAssetPath>());
    TF_VERIFY((val == SdfAssetPath("/@some/@path")));
    TF_VERIFY(err.empty());
    val = ShaderMetadataHelpers::ParseSdfValue("", &propAsset, &err);
    TF_VERIFY(val.IsHolding<SdfAssetPath>());
    TF_VERIFY((val == SdfAssetPath()));
    TF_VERIFY(err.empty());
}

int main()
{
    TestSdrParseValue();
}
