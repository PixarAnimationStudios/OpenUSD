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
// The test also writes bad_tokens_chunk.usdc, a copy of baseline.usdc whose
// compressed tokens claim one chunk but are too short to hold its size, and
// bad_path_jumps_*.usdc, copies of baseline.usdc whose compressed paths jumps
// tables are corrupt.  It writes lut.usdc, holding a float[] stored as a
// lookup table and indexes, and bad_lut_index.usdc, a copy whose last index is
// past the end of the table.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/setenv.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/crateInfo.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/types.h"
#include "../integerCoding.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <vector>

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

// Write a copy of `srcPath` to `dstPath` whose TOKENS section holds a
// one-byte compressed payload: a chunk count of 1 with no room for the chunk's
// size.  Then expect opening it to fail with errors.
static void
_TestTruncatedTokensChunk(char const *srcPath, char const *dstPath)
{
    SdfCrateInfo info = SdfCrateInfo::Open(srcPath);
    TF_AXIOM(info);
    int64_t tokensStart = -1;
    for (SdfCrateInfo::Section const &sec: info.GetSections()) {
        if (sec.name == "TOKENS") {
            tokensStart = sec.start;
        }
    }
    TF_AXIOM(tokensStart >= 0);

    FILE *in = fopen(srcPath, "rb");
    TF_AXIOM(in);
    std::vector<char> bytes;
    char buf[4096];
    for (size_t n; (n = fread(buf, 1, sizeof(buf), in)) != 0; ) {
        bytes.insert(bytes.end(), buf, buf + n);
    }
    fclose(in);

    // TOKENS: numTokens, uncompressedSize, compressedSize (uint64_t each),
    // then the compressed payload.
    uint64_t const compressedSize = 1;
    char const nChunks = 1;
    TF_AXIOM(tokensStart + 25 <= static_cast<int64_t>(bytes.size()));
    memcpy(&bytes[tokensStart + 16], &compressedSize, sizeof(compressedSize));
    bytes[tokensStart + 24] = nChunks;

    FILE *out = fopen(dstPath, "wb");
    TF_AXIOM(out);
    TF_AXIOM(fwrite(bytes.data(), 1, bytes.size(), out) == bytes.size());
    fclose(out);

    TfErrorMark mark;
    SdfLayerRefPtr layer = SdfLayer::FindOrOpen(dstPath);
    TF_AXIOM(!layer);
    TF_AXIOM(!mark.IsClean());
    mark.Clear();
}

// Write `srcPath` holding /Root.f, a float[] stored as a lookup table and
// indexes, then write a copy of it to `dstPath` whose last index is far past
// the end of the table.  Then expect reading /Root.f from the copy to fail with
// errors.
static void
_TestLookupTableIndex(char const *srcPath, char const *dstPath)
{
    // 32 elements cycling through 8 non-integral values, so the writer uses a
    // lookup table of size 8 with indexes 0..7.
    float const lut[8] = { 0.25, 1.25, 2.25, 3.25, 4.25, 5.25, 6.25, 7.25 };
    VtFloatArray values(32);
    for (size_t i = 0; i != values.size(); ++i) {
        values[i] = lut[i % 8];
    }
    {
        SdfLayerRefPtr layer = SdfLayer::CreateNew(srcPath);
        TF_AXIOM(layer);
        SdfPrimSpecHandle prim =
            SdfPrimSpec::New(layer, "Root", SdfSpecifierDef);
        SdfAttributeSpecHandle attr = SdfAttributeSpec::New(
            prim, "f", SdfValueTypeNames->FloatArray);
        TF_AXIOM(attr);
        attr->SetDefaultValue(VtValue(values));
        TF_AXIOM(layer->Save());
    }
    _Test(srcPath, SdfPath("/Root.f"), /*error=*/false);

    FILE *in = fopen(srcPath, "rb");
    TF_AXIOM(in);
    std::vector<char> bytes;
    char buf[4096];
    for (size_t n; (n = fread(buf, 1, sizeof(buf), in)) != 0; ) {
        bytes.insert(bytes.end(), buf, buf + n);
    }
    fclose(in);

    // The compressed array is the code 't', a uint32_t table size, the table,
    // then the indexes as a uint64_t compressed size followed by that many
    // bytes.
    uint32_t const lutSize = 8;
    std::vector<char> header(1 + sizeof(lutSize) + sizeof(lut));
    header[0] = 't';
    memcpy(&header[1], &lutSize, sizeof(lutSize));
    memcpy(&header[1 + sizeof(lutSize)], lut, sizeof(lut));
    auto iter = std::search(
        bytes.begin(), bytes.end(), header.begin(), header.end());
    TF_AXIOM(iter != bytes.end());
    TF_AXIOM(std::search(iter + 1, bytes.end(),
                         header.begin(), header.end()) == bytes.end());
    size_t const offset = (iter - bytes.begin()) + header.size();

    uint64_t indexesSize;
    TF_AXIOM(offset + sizeof(indexesSize) <= bytes.size());
    memcpy(&indexesSize, &bytes[offset], sizeof(indexesSize));
    TF_AXIOM(offset + sizeof(indexesSize) + indexesSize <= bytes.size());

    std::vector<uint32_t> indexes(values.size());
    TF_AXIOM(Sdf_IntegerCompression::DecompressFromBuffer(
                 &bytes[offset + sizeof(indexesSize)], indexesSize,
                 indexes.data(), indexes.size()) == indexes.size());
    for (size_t i = 0; i != indexes.size(); ++i) {
        TF_AXIOM(indexes[i] == i % 8);
    }
    // All zeros encode compactly, leaving room for one large index.
    std::fill(indexes.begin(), indexes.end(), 0);
    indexes.back() = 0x7fffffff;

    // The new encoding must fit in the old one's space.  Shorter is fine since
    // the reader takes its compressed size from the stored value.
    std::vector<char> compressed(
        Sdf_IntegerCompression::GetCompressedBufferSize(indexes.size()));
    uint64_t const newIndexesSize = Sdf_IntegerCompression::CompressToBuffer(
        indexes.data(), indexes.size(), compressed.data());
    TF_AXIOM(newIndexesSize <= indexesSize);
    memcpy(&bytes[offset], &newIndexesSize, sizeof(newIndexesSize));
    memcpy(&bytes[offset + sizeof(newIndexesSize)],
           compressed.data(), newIndexesSize);

    FILE *out = fopen(dstPath, "wb");
    TF_AXIOM(out);
    TF_AXIOM(fwrite(bytes.data(), 1, bytes.size(), out) == bytes.size());
    fclose(out);

    _Test(dstPath, SdfPath("/Root.f"), /*error=*/true);
}

#ifdef PXR_PREFER_SAFETY_OVER_SPEED
// Write a copy of `srcPath` to `dstPath` whose compressed PATHS section has its
// jumps table entry at `jumpIndex` replaced by `jump`.  Then expect opening it
// to fail with errors.
static void
_TestCorruptPathJumps(char const *srcPath, char const *dstPath,
                      size_t jumpIndex, int32_t jump)
{
    SdfCrateInfo info = SdfCrateInfo::Open(srcPath);
    TF_AXIOM(info);
    int64_t pathsStart = -1;
    for (SdfCrateInfo::Section const &sec: info.GetSections()) {
        if (sec.name == "PATHS") {
            pathsStart = sec.start;
        }
    }
    TF_AXIOM(pathsStart >= 0);

    FILE *in = fopen(srcPath, "rb");
    TF_AXIOM(in);
    std::vector<char> bytes;
    char buf[4096];
    for (size_t n; (n = fread(buf, 1, sizeof(buf), in)) != 0; ) {
        bytes.insert(bytes.end(), buf, buf + n);
    }
    fclose(in);

    // PATHS: numPaths, numEncodedPaths (uint64_t each), then three compressed
    // int arrays (pathIndexes, elementTokenIndexes, jumps), each a uint64_t
    // compressed size followed by that many bytes.
    auto readU64 = [&bytes](size_t offset) {
        uint64_t value;
        TF_AXIOM(offset + sizeof(value) <= bytes.size());
        memcpy(&value, &bytes[offset], sizeof(value));
        return value;
    };
    size_t const numEncoded = readU64(pathsStart + 8);
    size_t offset = pathsStart + 16;
    offset += 8 + readU64(offset); // pathIndexes
    offset += 8 + readU64(offset); // elementTokenIndexes
    size_t const jumpsSize = readU64(offset);
    TF_AXIOM(offset + 8 + jumpsSize <= bytes.size());

    std::vector<int32_t> jumps(numEncoded);
    TF_AXIOM(Sdf_IntegerCompression::DecompressFromBuffer(
                 &bytes[offset + 8], jumpsSize,
                 jumps.data(), jumps.size()) == numEncoded);
    // The cases in main() are written against this table.
    TF_AXIOM((jumps == std::vector<int32_t> { -1, -1, 0, -2 }));
    TF_AXIOM(jumpIndex < jumps.size());
    jumps[jumpIndex] = jump;

    // The new encoding must fit in the old one's space.  Shorter is fine since
    // the reader takes its compressed size from the stored value.
    std::vector<char> compressed(
        Sdf_IntegerCompression::GetCompressedBufferSize(jumps.size()));
    uint64_t const newJumpsSize = Sdf_IntegerCompression::CompressToBuffer(
        jumps.data(), jumps.size(), compressed.data());
    TF_AXIOM(newJumpsSize <= jumpsSize);
    memcpy(&bytes[offset], &newJumpsSize, sizeof(newJumpsSize));
    memcpy(&bytes[offset + 8], compressed.data(), newJumpsSize);

    FILE *out = fopen(dstPath, "wb");
    TF_AXIOM(out);
    TF_AXIOM(fwrite(bytes.data(), 1, bytes.size(), out) == bytes.size());
    fclose(out);

    TfErrorMark mark;
    SdfLayerRefPtr layer = SdfLayer::FindOrOpen(dstPath);
    TF_AXIOM(!layer);
    TF_AXIOM(!mark.IsClean());
    mark.Clear();
}
#endif // PXR_PREFER_SAFETY_OVER_SPEED

int
main()
{    
    _Test("baseline.usdc",
          SdfPath("/Root.zcPoints"), /*error=*/false);

    _TestObsolete("bad_obsolete_version.usdc");

    _TestTruncatedTokensChunk("baseline.usdc", "bad_tokens_chunk.usdc");

    _TestLookupTableIndex("lut.usdc", "bad_lut_index.usdc");

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

    // The baseline's paths are /, /Root, /Root.strs, /Root.zcPoints, with
    // jumps -1 (child only), -1, 0 (sibling only), -2 (leaf).  Giving /Root
    // both a child and a sibling at +1 makes the child and the sibling the
    // same index, visited twice.
    _TestCorruptPathJumps("baseline.usdc", "bad_path_jumps_repeat.usdc",
                          /*jumpIndex=*/1, /*jump=*/1);
    // A sibling jump past the end.
    _TestCorruptPathJumps("baseline.usdc", "bad_path_jumps_range.usdc",
                          /*jumpIndex=*/1, /*jump=*/100);
#endif
    printf("PASSED\n");
    return 0;
}
