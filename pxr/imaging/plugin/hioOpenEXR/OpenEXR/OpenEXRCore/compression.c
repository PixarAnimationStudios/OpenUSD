/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_compression.h"
#include "openexr_base.h"
#include "internal_memory.h"
#include "internal_structs.h"
#include "internal_compress.h"
#include "internal_decompress.h"
#include "internal_coding.h"
#include "internal_coding.h"
#include "internal_file.h"
#include "internal_huf.h"

#include "OpenEXRConfigInternal.h"

#if OPENEXR_USE_INTERNAL_DEFLATE
#    include "../deflate/lib/lib_common.h"
#    include "../deflate/lib/utils.c"
#    include "../deflate/lib/arm/cpu_features.c"
#    include "../deflate/lib/x86/cpu_features.c"
#    include "../deflate/lib/deflate_compress.c"
#    undef BITBUF_NBITS
#    include "../deflate/lib/deflate_decompress.c"
#    undef BITBUF_NBITS
#    include "../deflate/lib/adler32.c"
#    include "../deflate/lib/zlib_compress.c"
#    include "../deflate/lib/zlib_decompress.c"
#else
// pxr #include <libdeflate.h>
#include "../deflate/libdeflate.h"
#endif
#include <string.h>

#if (                                                                          \
    LIBDEFLATE_VERSION_MAJOR > 1 ||                                            \
    (LIBDEFLATE_VERSION_MAJOR == 1 && LIBDEFLATE_VERSION_MINOR > 18))
#    define EXR_USE_CONFIG_DEFLATE_STRUCT 1
#endif

/* value Aras found to be better trade off of speed vs size */
#define EXR_DEFAULT_ZLIB_COMPRESS_LEVEL 4

/**************************************/

size_t
exr_compress_max_buffer_size (size_t in_bytes)
{
    size_t r, extra;

    r = libdeflate_zlib_compress_bound (NULL, in_bytes);
    /*
     * lib deflate has a message about needing a 9 byte boundary
     * but is unclear if it actually adds that or not
     * (see the comment on libdeflate_deflate_compress)
     */
    if (r > (SIZE_MAX - 9)) return (size_t) (SIZE_MAX);
    r += 9;

    /*
     * old library had uiAdd( uiAdd( in, ceil(in * 0.01) ), 100 )
     */
    extra = (r * (size_t) 130);
    if (extra < r) return (size_t) (SIZE_MAX);
    extra /= (size_t) 128;

    if (extra > (SIZE_MAX - 100)) return (size_t) (SIZE_MAX);
    if (extra > r) r = extra;

    /*
     * in case huf is larger than zlib
     */
    extra = in_bytes + internal_exr_huf_compress_spare_bytes ();
    if (r < extra) r = extra;

    extra = in_bytes + internal_exr_huf_decompress_spare_bytes ();
    if (r < extra) r = extra;

    /* make sure there is some small 2 pages worth of buffer */
    if (8192 > r) r = 8192;

    return r;
}

/**************************************/

exr_result_t
exr_compress_buffer (
    exr_const_context_t ctxt,
    int                 level,
    const void*         in,
    size_t              in_bytes,
    void*               out,
    size_t              out_bytes_avail,
    size_t*             actual_out)
{
    struct libdeflate_compressor* comp;

#ifdef EXR_USE_CONFIG_DEFLATE_STRUCT
    struct libdeflate_options opt = {
        .sizeof_options = sizeof (struct libdeflate_options),
        .malloc_func    = ctxt ? ctxt->alloc_fn : internal_exr_alloc,
        .free_func      = ctxt ? ctxt->free_fn : internal_exr_free};

#else
    libdeflate_set_memory_allocator (
        ctxt ? ctxt->alloc_fn : internal_exr_alloc,
        ctxt ? ctxt->free_fn : internal_exr_free);
#endif

    if (level < 0)
    {
        exr_get_default_zip_compression_level (&level);
        /* truly unset anywhere */
        if (level < 0) level = EXR_DEFAULT_ZLIB_COMPRESS_LEVEL;
    }

#ifdef EXR_USE_CONFIG_DEFLATE_STRUCT
    comp = libdeflate_alloc_compressor_ex (level, &opt);
#else
    comp = libdeflate_alloc_compressor (level);
#endif
    if (comp)
    {
        size_t outsz;
        outsz =
            libdeflate_zlib_compress (comp, in, in_bytes, out, out_bytes_avail);

        libdeflate_free_compressor (comp);

        if (outsz != 0)
        {
            if (actual_out) *actual_out = outsz;
            return EXR_ERR_SUCCESS;
        }
        return EXR_ERR_OUT_OF_MEMORY;
    }
    return EXR_ERR_OUT_OF_MEMORY;
}

/**************************************/

exr_result_t
exr_uncompress_buffer (
    exr_const_context_t ctxt,
    const void*         in,
    size_t              in_bytes,
    void*               out,
    size_t              out_bytes_avail,
    size_t*             actual_out)
{
    struct libdeflate_decompressor* decomp;
    enum libdeflate_result          res;
    size_t                          actual_in_bytes;
#ifdef EXR_USE_CONFIG_DEFLATE_STRUCT
    struct libdeflate_options opt = {
        .sizeof_options = sizeof (struct libdeflate_options),
        .malloc_func    = ctxt ? ctxt->alloc_fn : internal_exr_alloc,
        .free_func      = ctxt ? ctxt->free_fn : internal_exr_free};
#endif

//    if (in_bytes == out_bytes_avail)
//    {
//        if (actual_out) *actual_out = in_bytes;
//        if (in != out)
//            memcpy(out, in, in_bytes);
//
//        return EXR_ERR_SUCCESS;
//    }

#ifdef EXR_USE_CONFIG_DEFLATE_STRUCT
    decomp = libdeflate_alloc_decompressor_ex (&opt);
#else
    libdeflate_set_memory_allocator (
        ctxt ? ctxt->alloc_fn : internal_exr_alloc,
        ctxt ? ctxt->free_fn : internal_exr_free);
    decomp = libdeflate_alloc_decompressor ();
#endif
    if (decomp)
    {
        res = libdeflate_zlib_decompress_ex (
            decomp,
            in,
            in_bytes,
            out,
            out_bytes_avail,
            &actual_in_bytes,
            actual_out);

        libdeflate_free_decompressor (decomp);

        if (res == LIBDEFLATE_SUCCESS)
        {
            if (in_bytes == actual_in_bytes) return EXR_ERR_SUCCESS;
            /* it's an error to not consume the full buffer, right? */
        }
        else if (res == LIBDEFLATE_INSUFFICIENT_SPACE)
        {
            return EXR_ERR_OUT_OF_MEMORY;
        }
        else if (res == LIBDEFLATE_SHORT_OUTPUT)
        {
            /* Decompression succeeded; *actual_out is the byte count. This is
             * not an error when out_bytes_avail exceeds the true uncompressed
             * size (e.g. PXR24/ZIP use padded scratch buffers). Callers that
             * need an exact payload size must compare *actual_out (see e.g.
             * undo_pxr24_impl). */
            return EXR_ERR_SUCCESS;
        }
        return EXR_ERR_CORRUPT_CHUNK;
    }
    return EXR_ERR_OUT_OF_MEMORY;
}

/**************************************/
/**************************************/

size_t
exr_rle_compress_buffer (size_t in_bytes, const void* in, void* out, size_t out_avail)
{
    return internal_rle_compress (out, out_avail, in, in_bytes);
}

size_t
exr_rle_uncompress_buffer (size_t in_bytes, size_t max_len, const void* in, void* out)
{
    return internal_rle_decompress (out, max_len, in, in_bytes);
}

/**************************************/
/**************************************/

int exr_compression_lines_per_chunk (exr_compression_t comptype)
{
    int linePerChunk = -1;

    switch (comptype)
    {
        case EXR_COMPRESSION_NONE:
        case EXR_COMPRESSION_RLE:
        case EXR_COMPRESSION_ZIPS: linePerChunk = 1; break;
        case EXR_COMPRESSION_ZIP:
        case EXR_COMPRESSION_PXR24: linePerChunk = 16; break;
        case EXR_COMPRESSION_PIZ:
        case EXR_COMPRESSION_B44:
        case EXR_COMPRESSION_B44A:
        case EXR_COMPRESSION_HTJ2K32:
        case EXR_COMPRESSION_DWAA: linePerChunk = 32; break;
        case EXR_COMPRESSION_DWAB: linePerChunk = 256; break;
        case EXR_COMPRESSION_HTJ2K256: linePerChunk = 256; break;
        case EXR_COMPRESSION_LAST_TYPE:
        default:
            /* ERROR CONDITION */
            break;
    }
    return linePerChunk;
}

/**************************************/

exr_result_t
exr_compress_chunk (exr_encode_pipeline_t* encode)
{
    exr_result_t    rv;
    exr_context_t   ctxt;
    exr_priv_part_t part;
    size_t          maxbytes;

    if (!encode) return EXR_ERR_MISSING_CONTEXT_ARG;
    ctxt = (exr_context_t) encode->context;
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    /* TODO: Double check need for a lock? */
    if (encode->part_index < 0 || encode->part_index >= ctxt->num_parts)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_ARGUMENT_OUT_OF_RANGE,
            "Part index (%d) out of range",
            encode->part_index);

    part = ctxt->parts[encode->part_index];

    maxbytes = encode->chunk.unpacked_size;
    if (encode->packed_bytes > maxbytes)
        maxbytes = encode->packed_bytes;

    rv = internal_encode_alloc_buffer (
        encode,
        EXR_TRANSCODE_BUFFER_COMPRESSED,
        &(encode->compressed_buffer),
        &(encode->compressed_alloc_size),
        exr_compress_max_buffer_size (maxbytes));
    if (rv != EXR_ERR_SUCCESS)
        return ctxt->print_error (
            ctxt,
            rv,
            "error allocating buffer %zu",
            exr_compress_max_buffer_size (maxbytes));
    //return rv;

    if (encode->sample_count_table)
    {
        uint64_t sampsize =
            (((uint64_t) encode->chunk.width) *
             ((uint64_t) encode->chunk.height));

        sampsize *= sizeof (int32_t);

        if (part->comp_type == EXR_COMPRESSION_NONE)
        {
            internal_encode_free_buffer (
                encode,
                EXR_TRANSCODE_BUFFER_PACKED_SAMPLES,
                &(encode->packed_sample_count_table),
                &(encode->packed_sample_count_alloc_size));

            encode->packed_sample_count_table      = encode->sample_count_table;
            encode->packed_sample_count_alloc_size = 0;
            encode->packed_sample_count_bytes = sampsize;
        }
        else
        {
            void *pb;
            size_t pbb, pas;

            pb = encode->packed_buffer;
            pbb = encode->packed_bytes;
            pas = encode->packed_alloc_size;

            rv = internal_encode_alloc_buffer (
                encode,
                EXR_TRANSCODE_BUFFER_PACKED_SAMPLES,
                &(encode->packed_sample_count_table),
                &(encode->packed_sample_count_alloc_size),
                exr_compress_max_buffer_size (sampsize));
            if (rv != EXR_ERR_SUCCESS)
                return rv;

            encode->packed_buffer = encode->packed_sample_count_table;
            encode->packed_bytes = sampsize;
            encode->packed_alloc_size = encode->packed_sample_count_alloc_size;
            switch (part->comp_type)
            {
                case EXR_COMPRESSION_NONE: rv = EXR_ERR_INVALID_ARGUMENT; break;
                case EXR_COMPRESSION_RLE: rv = internal_exr_apply_rle (encode); break;
                case EXR_COMPRESSION_ZIP:
                case EXR_COMPRESSION_ZIPS: rv = internal_exr_apply_zip (encode); break;

                default:
                    rv = EXR_ERR_INVALID_ARGUMENT;
                    break;
            }
            encode->packed_buffer = pb;
            encode->packed_bytes = pbb;
            encode->packed_alloc_size = pas;

            if (rv != EXR_ERR_SUCCESS)
                return ctxt->print_error (
                    ctxt,
                    rv,
                    "Unable to compress sample table");
        }
    }

    switch (part->comp_type)
    {
        case EXR_COMPRESSION_NONE:
            return ctxt->report_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "no compression set but still trying to compress");

        case EXR_COMPRESSION_RLE: rv = internal_exr_apply_rle (encode); break;
        case EXR_COMPRESSION_ZIP:
        case EXR_COMPRESSION_ZIPS: rv = internal_exr_apply_zip (encode); break;
        case EXR_COMPRESSION_PIZ: rv = internal_exr_apply_piz (encode); break;
        case EXR_COMPRESSION_PXR24:
            rv = internal_exr_apply_pxr24 (encode);
            break;
        case EXR_COMPRESSION_B44: rv = internal_exr_apply_b44 (encode); break;
        case EXR_COMPRESSION_B44A: rv = internal_exr_apply_b44a (encode); break;
        case EXR_COMPRESSION_DWAA: rv = internal_exr_apply_dwaa (encode); break;
        case EXR_COMPRESSION_DWAB: rv = internal_exr_apply_dwab (encode); break;
        case EXR_COMPRESSION_HTJ2K32:
        case EXR_COMPRESSION_HTJ2K256:
            rv = internal_exr_apply_ht (encode); break;
        case EXR_COMPRESSION_LAST_TYPE:
        default:
            return ctxt->print_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Compression technique 0x%02X invalid",
                (int) part->comp_type);
    }
    return rv;
}

/**************************************/
/**************************************/

static exr_result_t
decompress_data (
    exr_const_context_t     ctxt,
    const exr_compression_t ctype,
    exr_decode_pipeline_t*  decode,
    void*                   packbufptr,
    size_t                  packsz,
    void*                   unpackbufptr,
    size_t                  unpacksz)
{
    exr_result_t rv;

    if (packsz == 0) return EXR_ERR_SUCCESS;

    if (packsz == unpacksz)
    {
        if (unpackbufptr != packbufptr)
            memcpy (unpackbufptr, packbufptr, unpacksz);
        return EXR_ERR_SUCCESS;
    }

    switch (ctype)
    {
        case EXR_COMPRESSION_NONE:
            return ctxt->report_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "no compression set but still trying to decompress");

        case EXR_COMPRESSION_RLE:
            rv = internal_exr_undo_rle (
                decode, packbufptr, packsz, unpackbufptr, unpacksz);
            break;
        case EXR_COMPRESSION_ZIP:
        case EXR_COMPRESSION_ZIPS:
            rv = internal_exr_undo_zip (
                decode, packbufptr, packsz, unpackbufptr, unpacksz);
            break;
        case EXR_COMPRESSION_PIZ:
            rv = internal_exr_undo_piz (
                decode, packbufptr, packsz, unpackbufptr, unpacksz);
            break;
        case EXR_COMPRESSION_PXR24:
            rv = internal_exr_undo_pxr24 (
                decode, packbufptr, packsz, unpackbufptr, unpacksz);
            break;
        case EXR_COMPRESSION_B44:
            rv = internal_exr_undo_b44 (
                decode, packbufptr, packsz, unpackbufptr, unpacksz);
            break;
        case EXR_COMPRESSION_B44A:
            rv = internal_exr_undo_b44a (
                decode, packbufptr, packsz, unpackbufptr, unpacksz);
            break;
        case EXR_COMPRESSION_DWAA:
            rv = internal_exr_undo_dwaa (
                decode, packbufptr, packsz, unpackbufptr, unpacksz);
            break;
        case EXR_COMPRESSION_DWAB:
            rv = internal_exr_undo_dwab (
                decode, packbufptr, packsz, unpackbufptr, unpacksz);
            break;
        case EXR_COMPRESSION_HTJ2K256:
        case EXR_COMPRESSION_HTJ2K32:
            rv = internal_exr_undo_ht (
                decode, packbufptr, packsz, unpackbufptr, unpacksz);
            break;
        case EXR_COMPRESSION_LAST_TYPE:
        default:
            return ctxt->print_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Compression technique 0x%02X invalid",
                ctype);
    }

    return rv;
}

exr_result_t
exr_uncompress_chunk (exr_decode_pipeline_t* decode)
{
    exr_result_t    rv   = EXR_ERR_SUCCESS;
    exr_context_t   ctxt;
    exr_priv_part_t part;

    if (!decode) return EXR_ERR_MISSING_CONTEXT_ARG;

    decode->bytes_decompressed = 0;

    ctxt = (exr_context_t)decode->context;
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    /* TODO: Double check need for a lock? */
    if (decode->part_index < 0 || decode->part_index >= ctxt->num_parts)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_ARGUMENT_OUT_OF_RANGE,
            "Part index (%d) out of range",
            decode->part_index);

    part = ctxt->parts[decode->part_index];

//    if (decode->chunk.unpacked_size != part->unpacked_size_per_chunk)
//        return ctxt->print_error (
//            ctxt,
//            EXR_ERR_ARGUMENT_OUT_OF_RANGE,
//            "Memory not sufficient to unpack a full part, expect %" PRIu64 ", given %" PRIu64,
//            part->unpacked_size_per_chunk,
//            decode->chunk.unpacked_size);

    if (decode->packed_sample_count_table)
    {
        uint64_t sampsize =
            (((uint64_t) decode->chunk.width) *
             ((uint64_t) decode->chunk.height));

        sampsize *= sizeof (int32_t);

        rv = decompress_data (
            ctxt,
            part->comp_type,
            decode,
            decode->packed_sample_count_table,
            decode->chunk.sample_count_table_size,
            decode->sample_count_table,
            sampsize);

        if (rv != EXR_ERR_SUCCESS)
        {
            return ctxt->print_error (
                ctxt,
                rv,
                "Unable to decompress sample table %" PRIu64 " -> %" PRIu64,
                decode->chunk.sample_count_table_size,
                (uint64_t) sampsize);
        }
    }

    if ((decode->decode_flags & EXR_DECODE_SAMPLE_DATA_ONLY)) return rv;

    if (rv == EXR_ERR_SUCCESS &&
        decode->chunk.packed_size > 0 &&
        decode->chunk.unpacked_size > 0)
        rv = decompress_data (
            ctxt,
            part->comp_type,
            decode,
            decode->packed_buffer,
            decode->chunk.packed_size,
            decode->unpacked_buffer,
            decode->chunk.unpacked_size);

    if (rv != EXR_ERR_SUCCESS)
    {
        return ctxt->print_error (
            ctxt,
            rv,
            "Unable to decompress w %d image data %" PRIu64 " -> %" PRIu64 ", got %" PRIu64,
            (int)part->comp_type,
            decode->chunk.packed_size,
            decode->chunk.unpacked_size,
            decode->bytes_decompressed);
    }
    return rv;
}
