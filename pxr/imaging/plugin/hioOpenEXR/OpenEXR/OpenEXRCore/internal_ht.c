/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

// pxr: HT (HTJ2K / High-Throughput JPEG 2000) compression support is stubbed
// out in this vendored OpenEXRCore. OpenEXR implements it on top of OpenJPH
// which is a full JPEG 2000 codec we do not currently vendor.
//
// Both entry points return EXR_ERR_FEATURE_NOT_IMPLEMENTED, so an HT-compressed
// chunk fails cleanly on read and write. There is no DoS vulnerability.

#include "internal_compress.h"
#include "internal_decompress.h"

exr_result_t
internal_exr_undo_ht (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    (void) decode;
    (void) compressed_data;
    (void) comp_buf_size;
    (void) uncompressed_data;
    (void) uncompressed_size;
    return EXR_ERR_FEATURE_NOT_IMPLEMENTED;
}

exr_result_t
internal_exr_apply_ht (exr_encode_pipeline_t* encode)
{
    (void) encode;
    return EXR_ERR_FEATURE_NOT_IMPLEMENTED;
}
