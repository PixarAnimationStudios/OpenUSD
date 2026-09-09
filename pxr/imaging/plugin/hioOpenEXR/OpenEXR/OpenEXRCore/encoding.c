/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_encode.h"
#include "openexr_compression.h"

#include "internal_coding.h"
#include "internal_structs.h"
#include "internal_xdr.h"

#include "openexr_compression.h"

/**************************************/

static exr_result_t
default_yield (exr_encode_pipeline_t* encode)
{
    exr_result_t        rv;
    exr_const_context_t ctxt = encode->context;
    EXR_LOCK_WRITE_AND_DEFINE_PART (encode->part_index);

    rv = internal_validate_next_chunk (encode, ctxt, part);

    return EXR_UNLOCK_WRITE_AND_RETURN (rv);
}

/**************************************/

static exr_result_t
default_write_chunk (exr_encode_pipeline_t* encode)
{
    exr_result_t rv;

    if (!encode) return EXR_ERR_INVALID_ARGUMENT;

    switch (encode->chunk.type)
    {
        case EXR_STORAGE_SCANLINE:
            rv = exr_write_scanline_chunk (
                EXR_CONST_CAST (exr_context_t, encode->context),
                encode->part_index,
                encode->chunk.start_y,
                encode->compressed_buffer,
                encode->compressed_bytes);
            break;
        case EXR_STORAGE_TILED:
            rv = exr_write_tile_chunk (
                EXR_CONST_CAST (exr_context_t, encode->context),
                encode->part_index,
                encode->chunk.start_x,
                encode->chunk.start_y,
                encode->chunk.level_x,
                encode->chunk.level_y,
                encode->compressed_buffer,
                encode->compressed_bytes);
            break;
        case EXR_STORAGE_DEEP_SCANLINE:
            if (!encode->packed_sample_count_table ||
                encode->packed_sample_count_bytes == 0)
                return EXR_ERR_INVALID_ARGUMENT;
            rv = exr_write_deep_scanline_chunk (
                EXR_CONST_CAST (exr_context_t, encode->context),
                encode->part_index,
                encode->chunk.start_y,
                encode->compressed_buffer,
                encode->compressed_bytes,
                encode->packed_bytes,
                encode->packed_sample_count_table,
                encode->packed_sample_count_bytes);
            break;
        case EXR_STORAGE_DEEP_TILED:
            if (!encode->packed_sample_count_table ||
                encode->packed_sample_count_bytes == 0)
                return EXR_ERR_INVALID_ARGUMENT;
            rv = exr_write_deep_tile_chunk (
                EXR_CONST_CAST (exr_context_t, encode->context),
                encode->part_index,
                encode->chunk.start_x,
                encode->chunk.start_y,
                encode->chunk.level_x,
                encode->chunk.level_y,
                encode->compressed_buffer,
                encode->compressed_bytes,
                encode->packed_bytes,
                encode->packed_sample_count_table,
                encode->packed_sample_count_bytes);
            break;
        case EXR_STORAGE_LAST_TYPE:
        default: rv = EXR_ERR_INVALID_ARGUMENT; break;
    }
    return rv;
}

/**************************************/

exr_result_t
exr_encoding_initialize (
    exr_const_context_t     ctxt,
    int                     part_index,
    const exr_chunk_info_t* cinfo,
    exr_encode_pipeline_t*  encode)
{
    exr_result_t          rv;
    exr_encode_pipeline_t nil = {0};

    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);
    if (!cinfo || !encode)
        return EXR_UNLOCK_WRITE_AND_RETURN (
            ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT));

    if (ctxt->mode != EXR_CONTEXT_WRITING_DATA &&
        ctxt->mode != EXR_CONTEXT_TEMPORARY)
    {
        if (ctxt->mode == EXR_CONTEXT_WRITE)
            return EXR_UNLOCK_WRITE_AND_RETURN (
                ctxt->standard_error (ctxt, EXR_ERR_HEADER_NOT_WRITTEN));
        return EXR_UNLOCK_WRITE_AND_RETURN (
            ctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE));
    }

    *encode = nil;

    rv = internal_coding_fill_channel_info (
        &(encode->channels),
        &(encode->channel_count),
        encode->_quick_chan_store,
        cinfo,
        ctxt,
        part);

    if (rv == EXR_ERR_SUCCESS)
    {
        encode->part_index = part_index;
        encode->context    = ctxt;
        encode->chunk      = *cinfo;
    }
    return EXR_UNLOCK_WRITE_AND_RETURN (rv);
}

/**************************************/

exr_result_t
exr_encoding_choose_default_routines (
    exr_const_context_t ctxt, int part_index, exr_encode_pipeline_t* encode)
{
    int32_t isdeep = 0;
    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);
    if (!encode)
        return EXR_UNLOCK_WRITE_AND_RETURN (
            ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT));

    if (encode->context != ctxt || encode->part_index != part_index)
        return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Cross-wired request for default routines from different context / part"));

    isdeep = (part->storage_mode == EXR_STORAGE_DEEP_SCANLINE ||
              part->storage_mode == EXR_STORAGE_DEEP_TILED)
                 ? 1
                 : 0;

    encode->convert_and_pack_fn = internal_exr_match_encode (encode, isdeep);
    if (part->comp_type != EXR_COMPRESSION_NONE)
        encode->compress_fn = &exr_compress_chunk;
    encode->yield_until_ready_fn = &default_yield;
    encode->write_fn             = &default_write_chunk;

    return EXR_UNLOCK_WRITE_AND_RETURN (EXR_ERR_SUCCESS);
}

/**************************************/

exr_result_t
exr_encoding_update (
    exr_const_context_t     ctxt,
    int                     part_index,
    const exr_chunk_info_t* cinfo,
    exr_encode_pipeline_t*  encode)
{
    exr_result_t rv;

    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);
    if (!cinfo || !encode)
        return EXR_UNLOCK_WRITE_AND_RETURN (
            ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT));

    if (encode->context != ctxt || encode->part_index != part_index)
        return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Cross-wired request for default routines from different context / part"));

    if (encode->packed_buffer == encode->compressed_buffer)
        encode->compressed_buffer = NULL;

    encode->packed_bytes              = 0;
    encode->packed_sample_count_bytes = 0;
    encode->compressed_bytes          = 0;

    rv = internal_coding_update_channel_info (
        encode->channels, encode->channel_count, cinfo, ctxt, part);

    if (rv == EXR_ERR_SUCCESS) encode->chunk = *cinfo;
    return EXR_UNLOCK_WRITE_AND_RETURN (rv);
}

/**************************************/

exr_result_t
exr_encoding_run (
    exr_const_context_t ctxt, int part_index, exr_encode_pipeline_t* encode)
{
    exr_result_t rv           = EXR_ERR_SUCCESS;
    uint64_t     packed_bytes = 0;
    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);

    if (!encode)
        return EXR_UNLOCK_WRITE_AND_RETURN (
            ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT));
    if (encode->context != ctxt || encode->part_index != part_index)
        return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid request for encoding update from different context / part"));

    if (part->storage_mode == EXR_STORAGE_DEEP_SCANLINE ||
        part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        if (encode->sample_count_table == NULL ||
            encode->sample_count_alloc_size !=
                (((size_t) encode->chunk.width) *
                 ((size_t) encode->chunk.height) * sizeof (int32_t)))
        {
            return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->report_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid / missing sample count table for deep data"));
        }
    }

    for (int c = 0; c < encode->channel_count; ++c)
    {
        const exr_coding_channel_info_t* encc = (encode->channels + c);

        if (encc->height == 0) continue;

        if (encc->width == 0)
            return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->print_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Unexpected 0-width chunk to encode"));
        if (!encc->encode_from_ptr)
            return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->print_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Missing channel data pointer - must encode all channels"));

        /*
         * if a user specifies a bad pixel stride / line stride
         * we can't know this realistically, and they may want to
         * use 0 to cause things to collapse for testing purposes
         * so only test the values we can
         */
        if (encc->user_bytes_per_element != 2 &&
            encc->user_bytes_per_element != 4)
            return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->print_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid / unsupported output bytes per element (%d) for channel %c (%s)",
                (int) encc->user_bytes_per_element,
                c,
                encc->channel_name));

        if (encc->user_data_type != (uint16_t) (EXR_PIXEL_HALF) &&
            encc->user_data_type != (uint16_t) (EXR_PIXEL_FLOAT) &&
            encc->user_data_type != (uint16_t) (EXR_PIXEL_UINT))
            return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->print_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid / unsupported output data type (%d) for channel %c (%s)",
                (int) encc->user_data_type,
                c,
                encc->channel_name));

        packed_bytes +=
            ((uint64_t) (encc->height) * (uint64_t) (encc->width) *
             (uint64_t) (encc->bytes_per_element));
    }

    encode->packed_bytes = 0;
    if (encode->convert_and_pack_fn)
    {
        if (packed_bytes > 0)
        {
            rv = internal_encode_alloc_buffer (
                encode,
                EXR_TRANSCODE_BUFFER_PACKED,
                &(encode->packed_buffer),
                &(encode->packed_alloc_size),
                packed_bytes);

            if (rv == EXR_ERR_SUCCESS)
                rv = encode->convert_and_pack_fn (encode);
        }
    }
    else if (!encode->packed_buffer || packed_bytes != encode->compressed_bytes)
    {
        return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Encode pipeline has no packing function declared and packed buffer is null or appears to need packing"));
    }
    if (ctxt->mode == EXR_CONTEXT_WRITE) internal_exr_unlock (ctxt);

    if ((part->storage_mode == EXR_STORAGE_DEEP_SCANLINE ||
         part->storage_mode == EXR_STORAGE_DEEP_TILED) &&
        encode->sample_count_table != NULL)
    {
        /* Process the sample count table line by line to avoid integer overflows */
        int32_t* current_line = encode->sample_count_table;
        for (int y = 0; y < encode->chunk.height; ++y)
        {
            priv_from_native32 (current_line, encode->chunk.width);
            current_line += encode->chunk.width;
        }
    }

    if (rv == EXR_ERR_SUCCESS)
    {
        if (encode->compress_fn && encode->packed_bytes > 0)
        {
            rv = encode->compress_fn (encode);
        }
        else
        {
            internal_encode_free_buffer (
                encode,
                EXR_TRANSCODE_BUFFER_COMPRESSED,
                &(encode->compressed_buffer),
                &(encode->compressed_alloc_size));

            internal_encode_free_buffer (
                encode,
                EXR_TRANSCODE_BUFFER_PACKED_SAMPLES,
                &(encode->packed_sample_count_table),
                &(encode->packed_sample_count_alloc_size));

            encode->compressed_buffer     = encode->packed_buffer;
            encode->compressed_bytes      = encode->packed_bytes;
            encode->compressed_alloc_size = 0;

            encode->packed_sample_count_table      = encode->sample_count_table;
            encode->packed_sample_count_alloc_size = 0;
            encode->packed_sample_count_bytes =
                (((size_t) encode->chunk.width) *
                 ((size_t) encode->chunk.height) * sizeof (int32_t));
        }
    }

    if (rv == EXR_ERR_SUCCESS && encode->yield_until_ready_fn)
        rv = encode->yield_until_ready_fn (encode);

    if (rv == EXR_ERR_SUCCESS && encode->write_fn)
        rv = encode->write_fn (encode);

    if ((part->storage_mode == EXR_STORAGE_DEEP_SCANLINE ||
         part->storage_mode == EXR_STORAGE_DEEP_TILED) &&
        encode->sample_count_table != NULL)
    {
        /* Process the sample count table line by line to avoid integer overflows */
        int32_t* current_line = encode->sample_count_table;
        for (int y = 0; y < encode->chunk.height; ++y)
        {
            priv_to_native32 (current_line, encode->chunk.width);
            current_line += encode->chunk.width;
        }
    }

    return rv;
}

/**************************************/

exr_result_t
exr_encoding_destroy (exr_const_context_t ctxt, exr_encode_pipeline_t* encode)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (encode)
    {
        exr_encode_pipeline_t nil = {0};
        if (encode->channels != encode->_quick_chan_store)
            ctxt->free_fn (encode->channels);

        internal_encode_free_buffer (
            encode,
            EXR_TRANSCODE_BUFFER_PACKED,
            &(encode->packed_buffer),
            &(encode->packed_alloc_size));
        internal_encode_free_buffer (
            encode,
            EXR_TRANSCODE_BUFFER_COMPRESSED,
            &(encode->compressed_buffer),
            &(encode->compressed_alloc_size));
        internal_encode_free_buffer (
            encode,
            EXR_TRANSCODE_BUFFER_SCRATCH1,
            &(encode->scratch_buffer_1),
            &(encode->scratch_alloc_size_1));
        internal_encode_free_buffer (
            encode,
            EXR_TRANSCODE_BUFFER_SCRATCH2,
            &(encode->scratch_buffer_2),
            &(encode->scratch_alloc_size_2));
        internal_encode_free_buffer (
            encode,
            EXR_TRANSCODE_BUFFER_PACKED_SAMPLES,
            &(encode->packed_sample_count_table),
            &(encode->packed_sample_count_alloc_size));
        *encode = nil;
    }
    return EXR_ERR_SUCCESS;
}
