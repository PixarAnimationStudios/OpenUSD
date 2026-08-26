/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_COMPRESSION_H
#define OPENEXR_CORE_COMPRESSION_H

#include "openexr_context.h"

#include "openexr_encode.h"
#include "openexr_decode.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @file */

/** Computes a buffer that will be large enough to hold the compressed
 * data. This may include some extra padding for headers / scratch */
EXR_EXPORT
size_t exr_compress_max_buffer_size (size_t in_bytes);

/** Compresses a buffer using a zlib style compression.
 *
 * If the level is -1, will use the default compression set to the library
 * \ref exr_set_default_zip_compression_level
 * data. This may include some extra padding for headers / scratch */
EXR_EXPORT
exr_result_t exr_compress_buffer (
    exr_const_context_t ctxt,
    int                 level,
    const void*         in,
    size_t              in_bytes,
    void*               out,
    size_t              out_bytes_avail,
    size_t*             actual_out);

/** Decompresses a buffer using a zlib style compression. */
EXR_EXPORT
exr_result_t exr_uncompress_buffer (
    exr_const_context_t ctxt,
    const void*         in,
    size_t              in_bytes,
    void*               out,
    size_t              out_bytes_avail,
    size_t*             actual_out);

/** Apply simple run length encoding and put in the output buffer. */
EXR_EXPORT
size_t exr_rle_compress_buffer (
    size_t in_bytes,
    const void* in,
    void* out,
    size_t out_bytes_avail);

/** Decode run length encoding and put in the output buffer. */
EXR_EXPORT
size_t exr_rle_uncompress_buffer (
    size_t in_bytes,
    size_t max_len,
    const void* in,
    void* out);


/** Routine to query the lines required per chunk to compress with the
 * specified method.
 *
 * This is only meaningful for scanline encodings, tiled
 * representations have a different interpretation of this.
 *
 * These are constant values, this function returns -1 if the compression
 * type is unknown.
 */
EXR_EXPORT
int exr_compression_lines_per_chunk (exr_compression_t comptype);

/** Exposes a method to apply compression to a chunk of data.
 *
 * This can be useful for inheriting default behavior of the
 * compression stage of an encoding pipeline, or other helper classes
 * to expose compression.
 *
 * NB: As implied, this function will be used during a normal encode
 * and write operation but can be used directly with a temporary
 * context (i.e. not running the full encode pipeline).
 */
EXR_EXPORT
exr_result_t exr_compress_chunk (exr_encode_pipeline_t *encode_state);

/** Exposes a method to decompress a chunk of data.
 *
 * This can be useful for inheriting default behavior of the
 * uncompression stage of an decoding pipeline, or other helper classes
 * to expose compress / uncompress operations.
 *
 * NB: This function will be used during a normal read and decode
 * operation but can be used directly with a temporary context (i.e.
 * not running the full decode pipeline).
 */
EXR_EXPORT
exr_result_t exr_uncompress_chunk (exr_decode_pipeline_t *decode_state);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_COMPRESSION_H */
