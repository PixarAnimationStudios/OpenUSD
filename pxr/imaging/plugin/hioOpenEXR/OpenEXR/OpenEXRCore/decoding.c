/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_decode.h"

#include "openexr_compression.h"

#include "internal_coding.h"
#include "internal_decompress.h"
#include "internal_structs.h"
#include "internal_xdr.h"

#include <stdio.h>
#include <string.h>

/**************************************/

static exr_result_t
update_pack_unpack_ptrs (exr_decode_pipeline_t* decode)
{
    exr_result_t  rv;
    exr_storage_t stortype = ((exr_storage_t) decode->chunk.type);

    if (stortype == EXR_STORAGE_DEEP_SCANLINE ||
        stortype == EXR_STORAGE_DEEP_TILED)
    {
        uint64_t sampsize64 =
            ((uint64_t) decode->chunk.width) * ((uint64_t) decode->chunk.height);

        if ((decode->decode_flags & EXR_DECODE_SAMPLE_COUNTS_AS_INDIVIDUAL))
            sampsize64 += 1;
        sampsize64 *= sizeof (int32_t);
        if (sampsize64 != (size_t) sampsize64) return EXR_ERR_OUT_OF_MEMORY;
        size_t sampsize = (size_t) sampsize64;

        if (decode->chunk.sample_count_table_size == sampsize)
        {
            internal_decode_free_buffer (
                decode,
                EXR_TRANSCODE_BUFFER_SAMPLES,
                (void**) &(decode->sample_count_table),
                &(decode->sample_count_alloc_size));

            decode->sample_count_table = decode->packed_sample_count_table;
            rv                         = EXR_ERR_SUCCESS;
        }
        else
        {
            rv = internal_decode_alloc_buffer (
                decode,
                EXR_TRANSCODE_BUFFER_SAMPLES,
                (void**) &(decode->sample_count_table),
                &(decode->sample_count_alloc_size),
                sampsize);
        }

        if (rv != EXR_ERR_SUCCESS ||
            (decode->decode_flags & EXR_DECODE_SAMPLE_DATA_ONLY) != 0)
            return rv;
    }

    if (decode->chunk.packed_size == decode->chunk.unpacked_size)
    {
        internal_decode_free_buffer (
            decode,
            EXR_TRANSCODE_BUFFER_UNPACKED,
            &(decode->unpacked_buffer),
            &(decode->unpacked_alloc_size));

        decode->unpacked_buffer = decode->packed_buffer;
        rv                      = EXR_ERR_SUCCESS;
    }
    else
    {
        if (decode->chunk.unpacked_size != (size_t) decode->chunk.unpacked_size)
            return EXR_ERR_OUT_OF_MEMORY;
        rv = internal_decode_alloc_buffer (
            decode,
            EXR_TRANSCODE_BUFFER_UNPACKED,
            &(decode->unpacked_buffer),
            &(decode->unpacked_alloc_size),
            (size_t) decode->chunk.unpacked_size);
    }

    return rv;
}

static exr_result_t
read_uncompressed_direct (exr_decode_pipeline_t* decode)
{
    exr_result_t        rv;
    int                 height, start_y;
    uint64_t            dataoffset, toread;
    uint8_t*            cdata;
    exr_const_context_t ctxt = decode->context;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;
    if (ctxt->mode != EXR_CONTEXT_READ)
        return ctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_READ);
    if (decode->part_index < 0 || decode->part_index >= ctxt->num_parts)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_ARGUMENT_OUT_OF_RANGE,
            "Part index (%d) out of range",
            decode->part_index);

    dataoffset = decode->chunk.data_offset;

    height  = decode->chunk.height;
    start_y = decode->chunk.start_y;
    for (int y = 0; y < height; ++y)
    {
        for (int c = 0; c < decode->channel_count; ++c)
        {
            exr_coding_channel_info_t* decc = (decode->channels + c);

            cdata = decc->decode_to_ptr;
            toread =
                (uint64_t) decc->width * (uint64_t) decc->bytes_per_element;

            if (decc->height == 0) continue;

            if (decc->y_samples > 1)
            {
                if (((start_y + y) % decc->y_samples) != 0) continue;
                cdata +=
                    ((uint64_t) (y / decc->y_samples) *
                     (uint64_t) decc->user_line_stride);
            }
            else { cdata += (uint64_t) y * (uint64_t) decc->user_line_stride; }

            /* actual read into the output pointer */
            rv = ctxt->do_read (
                ctxt, cdata, toread, &dataoffset, NULL, EXR_MUST_READ_ALL);
            if (rv != EXR_ERR_SUCCESS) return rv;

            // need to swab them to native
            if (decc->bytes_per_element == 2)
                priv_to_native16 (cdata, decc->width);
            else
                priv_to_native32 (cdata, decc->width);
        }
    }

    return EXR_ERR_SUCCESS;
}

static exr_result_t
default_read_chunk (exr_decode_pipeline_t* decode)
{
    exr_result_t        rv;
    exr_const_context_t ctxt = decode->context;
    EXR_READONLY_AND_DEFINE_PART (decode->part_index);

    if (decode->unpacked_buffer == decode->packed_buffer &&
        decode->unpacked_alloc_size == 0)
        decode->unpacked_buffer = NULL;

    if ((part->storage_mode == EXR_STORAGE_DEEP_SCANLINE ||
         part->storage_mode == EXR_STORAGE_DEEP_TILED) &&
        decode->chunk.sample_count_table_size > 0)
    {
        rv = internal_decode_alloc_buffer (
            decode,
            EXR_TRANSCODE_BUFFER_PACKED_SAMPLES,
            &(decode->packed_sample_count_table),
            &(decode->packed_sample_count_alloc_size),
            decode->chunk.sample_count_table_size);
        if (rv != EXR_ERR_SUCCESS) return rv;

        if ((decode->decode_flags & EXR_DECODE_SAMPLE_DATA_ONLY))
        {
            rv = exr_read_deep_chunk (
                ctxt,
                decode->part_index,
                &(decode->chunk),
                NULL,
                decode->packed_sample_count_table);
        }
        else
        {
            rv = internal_decode_alloc_buffer (
                decode,
                EXR_TRANSCODE_BUFFER_PACKED,
                &(decode->packed_buffer),
                &(decode->packed_alloc_size),
                decode->chunk.packed_size);
            if (rv != EXR_ERR_SUCCESS) return rv;

            rv = exr_read_deep_chunk (
                ctxt,
                decode->part_index,
                &(decode->chunk),
                decode->packed_buffer,
                decode->packed_sample_count_table);
        }
    }
    else if (decode->chunk.packed_size > 0)
    {
        rv = internal_decode_alloc_buffer (
            decode,
            EXR_TRANSCODE_BUFFER_PACKED,
            &(decode->packed_buffer),
            &(decode->packed_alloc_size),
            decode->chunk.packed_size);
        if (rv != EXR_ERR_SUCCESS) return rv;
        rv = exr_read_chunk (
            ctxt, decode->part_index, &(decode->chunk), decode->packed_buffer);
    }
    else
        rv = EXR_ERR_SUCCESS;

    return rv;
}

static exr_result_t
unpack_sample_table (exr_const_context_t ctxt, exr_decode_pipeline_t* decode)
{
    exr_result_t rv           = EXR_ERR_SUCCESS;
    int64_t      w            = decode->chunk.width;
    int64_t      h            = decode->chunk.height;
    uint64_t     totsamp      = 0;
    int32_t*     samptable    = decode->sample_count_table;
    size_t       combSampSize = 0;

    for (int c = 0; c < decode->channel_count; ++c)
        combSampSize += ((size_t) decode->channels[c].bytes_per_element);

    if ((decode->decode_flags & EXR_DECODE_SAMPLE_COUNTS_AS_INDIVIDUAL))
    {
        for (int64_t y = 0; y < h; ++y)
        {
            int32_t* cursampline = samptable + y * w;
            int32_t  prevsamp    = 0;
            for (int32_t x = 0; x < w; ++x)
            {
                int32_t nsamps =
                    (int32_t) one_to_native32 ((uint32_t) cursampline[x]);
                // not monotonic, violation
                if (nsamps < prevsamp) return EXR_ERR_INVALID_SAMPLE_DATA;
                cursampline[x] = nsamps - prevsamp;
                prevsamp       = nsamps;
            }
            totsamp += (uint64_t) prevsamp;
        }
        if (totsamp >= (uint64_t) INT32_MAX) return EXR_ERR_INVALID_SAMPLE_DATA;
        samptable[w * h] = (int32_t) totsamp;
    }
    else
    {
        for (int64_t y = 0; y < h; ++y)
        {
            int32_t* cursampline = samptable + y * w;
            int32_t  prevsamp    = 0;
            for (int32_t x = 0; x < w; ++x)
            {
                int32_t nsamps =
                    (int32_t) one_to_native32 ((uint32_t) cursampline[x]);
                // not monotonic, violation
                if (nsamps < prevsamp) return EXR_ERR_INVALID_SAMPLE_DATA;

                cursampline[x] = nsamps;
                prevsamp       = nsamps;
            }

            totsamp += (uint64_t) prevsamp;
        }
        if (totsamp >= (uint64_t) INT32_MAX) return EXR_ERR_INVALID_SAMPLE_DATA;
    }

    if ((totsamp * combSampSize) > decode->chunk.unpacked_size)
    {
        rv = ctxt->report_error (
            ctxt, EXR_ERR_INVALID_SAMPLE_DATA, "Corrupt sample count table");
    }

    return rv;
}

/**************************************/

exr_result_t
exr_decoding_initialize (
    exr_const_context_t     ctxt,
    int                     part_index,
    const exr_chunk_info_t* cinfo,
    exr_decode_pipeline_t*  decode)
{
    exr_result_t          rv;
    exr_decode_pipeline_t nil = {0};
    exr_const_priv_part_t part;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (!cinfo || !decode)
        return ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    if (part_index < 0 || part_index >= ctxt->num_parts)
        return EXR_ERR_ARGUMENT_OUT_OF_RANGE;

    part = ctxt->parts[part_index];

    *decode = nil;

    if (part->storage_mode == EXR_STORAGE_DEEP_SCANLINE ||
        part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        if (part->version && part->version->i != 1)
        {
            return ctxt->print_error (
                ctxt,
                EXR_ERR_INVALID_ATTR,
                "Version %d not supported for deepscanline images in this version of the library",
                part->version->i);
        }
    }

    /* will have already been reported during header parse, but just
     * stop the file from being read
     */
    if (!part->channels || part->channels->type != EXR_ATTR_CHLIST ||
        part->channels->chlist->num_channels <= 0)
        return EXR_ERR_INVALID_ATTR;

    rv = internal_coding_fill_channel_info (
        &(decode->channels),
        &(decode->channel_count),
        decode->_quick_chan_store,
        cinfo,
        ctxt,
        part);

    if (rv == EXR_ERR_SUCCESS)
    {
        decode->part_index = part_index;
        decode->context    = ctxt;
        decode->chunk      = *cinfo;
    }
    return rv;
}

exr_result_t
exr_decoding_choose_default_routines (
    exr_const_context_t ctxt, int part_index, exr_decode_pipeline_t* decode)
{
    int32_t isdeep = 0, chanstofill = 0, chanstounpack = 0, sametype = -2,
            sameouttype = -2, samebpc = 0, sameoutbpc = 0, hassampling = 0,
            hastypechange = 0, simpinterleave = 0, simpinterleaverev = 0,
            simplineoff = 0, sameoutinc = 0;
    uint8_t* interleaveptr = NULL;
    EXR_READONLY_AND_DEFINE_PART (part_index);
    if (!decode) return ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    if (decode->context != ctxt || decode->part_index != part_index)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Cross-wired request for default routines from different context / part");

    isdeep = (part->storage_mode == EXR_STORAGE_DEEP_SCANLINE ||
              part->storage_mode == EXR_STORAGE_DEEP_TILED)
                 ? 1
                 : 0;

    for (int c = 0; c < decode->channel_count; ++c)
    {
        exr_coding_channel_info_t* decc = (decode->channels + c);

        if (decc->height == 0 || !decc->decode_to_ptr) continue;

        if (isdeep) continue;

        /*
         * if a user specifies a bad pixel stride / line stride
         * we can't know this realistically, and they may want to
         * use 0 to cause things to collapse for testing purposes
         * so only test the values we know we use for decisions
         */
        if (decc->user_bytes_per_element != 2 &&
            decc->user_bytes_per_element != 4)
            return ctxt->print_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid / unsupported output bytes per element (%d) for channel %c (%s)",
                (int) decc->user_bytes_per_element,
                c,
                decc->channel_name);

        if (decc->user_data_type != (uint16_t) (EXR_PIXEL_HALF) &&
            decc->user_data_type != (uint16_t) (EXR_PIXEL_FLOAT) &&
            decc->user_data_type != (uint16_t) (EXR_PIXEL_UINT))
            return ctxt->print_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid / unsupported output data type (%d) for channel %c (%s)",
                (int) decc->user_data_type,
                c,
                decc->channel_name);

        if (sametype == -2)
            sametype = (int32_t) decc->data_type;
        else if (sametype != (int32_t) decc->data_type)
            sametype = -1;

        if (sameouttype == -2)
            sameouttype = (int32_t) decc->user_data_type;
        else if (sameouttype != (int32_t) decc->user_data_type)
            sameouttype = -1;

        if (samebpc == 0)
            samebpc = decc->bytes_per_element;
        else if (samebpc != decc->bytes_per_element)
            samebpc = -1;

        if (sameoutbpc == 0)
            sameoutbpc = decc->user_bytes_per_element;
        else if (sameoutbpc != decc->user_bytes_per_element)
            sameoutbpc = -1;

        if (decc->x_samples != 1 || decc->y_samples != 1) hassampling = 1;

        ++chanstofill;
        if (decc->user_pixel_stride != decc->bytes_per_element) ++chanstounpack;
        if (decc->user_data_type != decc->data_type) ++hastypechange;

        if (simplineoff == 0)
            simplineoff = decc->user_line_stride;
        else if (simplineoff != decc->user_line_stride)
            simplineoff = -1;

        if (simpinterleave == 0)
        {
            interleaveptr     = decc->decode_to_ptr;
            simpinterleave    = decc->user_pixel_stride;
            simpinterleaverev = decc->user_pixel_stride;
        }
        else
        {
            if (simpinterleave > 0 &&
                decc->decode_to_ptr !=
                    (interleaveptr + c * decc->user_bytes_per_element))
            {
                simpinterleave = -1;
            }
            if (simpinterleaverev > 0 &&
                decc->decode_to_ptr !=
                    (interleaveptr - c * decc->user_bytes_per_element))
            {
                simpinterleaverev = -1;
            }
            if (simpinterleave < 0 && simpinterleaverev < 0)
                interleaveptr = NULL;
        }

        if (sameoutinc == 0)
            sameoutinc = decc->user_pixel_stride;
        else if (sameoutinc != decc->user_pixel_stride)
            sameoutinc = -1;
    }

    if (simpinterleave != sameoutbpc * decode->channel_count)
        simpinterleave = -1;
    if (simpinterleaverev != sameoutbpc * decode->channel_count)
        simpinterleaverev = -1;

    /* special case, uncompressed and reading planar data straight in
     * to all the channels */
    if (!isdeep && part->comp_type == EXR_COMPRESSION_NONE &&
        chanstounpack == 0 && hastypechange == 0 && chanstofill > 0 &&
        chanstofill == decode->channel_count)
    {
        decode->read_fn               = &read_uncompressed_direct;
        decode->decompress_fn         = NULL;
        decode->unpack_and_convert_fn = NULL;
        return EXR_ERR_SUCCESS;
    }
    decode->read_fn = &default_read_chunk;
    if (part->comp_type != EXR_COMPRESSION_NONE)
        decode->decompress_fn = &exr_uncompress_chunk;

    decode->unpack_and_convert_fn = internal_exr_match_decode (
        decode,
        isdeep,
        chanstofill,
        chanstounpack,
        sametype,
        sameouttype,
        samebpc,
        sameoutbpc,
        hassampling,
        hastypechange,
        sameoutinc,
        simpinterleave,
        simpinterleaverev,
        simplineoff);

    if (!decode->unpack_and_convert_fn)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_ARGUMENT_OUT_OF_RANGE,
            "Unable to choose valid unpack routine");

    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_decoding_update (
    exr_const_context_t     ctxt,
    int                     part_index,
    const exr_chunk_info_t* cinfo,
    exr_decode_pipeline_t*  decode)
{
    exr_result_t rv;

    exr_const_priv_part_t part;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;
    if (part_index < 0 || part_index >= ctxt->num_parts)
        return EXR_ERR_ARGUMENT_OUT_OF_RANGE;

    if (!cinfo || !decode)
        return ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    part = ctxt->parts[part_index];

    if (decode->context != ctxt || decode->part_index != part_index)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid request for decoding update from different context / part");

    rv = internal_coding_update_channel_info (
        decode->channels, decode->channel_count, cinfo, ctxt, part);
    decode->chunk = *cinfo;

    return rv;
}

/**************************************/

exr_result_t
exr_decoding_run (
    exr_const_context_t ctxt, int part_index, exr_decode_pipeline_t* decode)
{
    exr_result_t rv;
    exr_const_priv_part_t part;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;
    if (part_index < 0 || part_index >= ctxt->num_parts)
        return EXR_ERR_ARGUMENT_OUT_OF_RANGE;

    part = ctxt->parts[part_index];

    if (!decode) return ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    if (decode->context != ctxt || decode->part_index != part_index)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid request for decoding update from different context / part");

    if (!decode->read_fn)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Decode pipeline has no read_fn declared");
    rv = decode->read_fn (decode);
    if (rv != EXR_ERR_SUCCESS)
        return ctxt->report_error (
            ctxt, rv, "Unable to read pixel data block from context");

    if (rv == EXR_ERR_SUCCESS) rv = update_pack_unpack_ptrs (decode);
    if (rv != EXR_ERR_SUCCESS)
        return ctxt->report_error (
            ctxt,
            rv,
            "Decode pipeline unable to update pack / unpack pointers");

    if (rv == EXR_ERR_SUCCESS && decode->decompress_fn)
        rv = decode->decompress_fn (decode);
    if (rv != EXR_ERR_SUCCESS)
        return ctxt->report_error (
            ctxt, rv, "Decode pipeline unable to decompress data");

    if (rv == EXR_ERR_SUCCESS &&
        (part->storage_mode == EXR_STORAGE_DEEP_SCANLINE ||
         part->storage_mode == EXR_STORAGE_DEEP_TILED))
    {
        if (part->comp_type != EXR_COMPRESSION_NONE &&
            decode->packed_sample_count_table == NULL &&
            decode->sample_count_table != NULL)
        {
            /* A compressed deep chunk declared a zero-length (absent)
             * packed sample-count table, so decompress_data() above was
             * never invoked and decode->sample_count_table, which was
             * allocated to hold the full decompressed table, was left
             * uninitialized. Treat an absent table as all-zero sample
             * counts instead of reading uninitialized heap memory in
             * unpack_sample_table().
             */
            memset (
                decode->sample_count_table,
                0,
                decode->sample_count_alloc_size);
        }
        else if (
            part->comp_type == EXR_COMPRESSION_NONE &&
            decode->sample_count_table != decode->packed_sample_count_table)
        {
            /* Check that uncompressed data can be copied in safely.
             *
             * underflow potentially happens when we're requested to
             * pack to 'individual' mode
             *
             * also occurs when working with deep tiled images, and
             * the tile size is smaller than the defined base tile
             * width / height. The way the data was written is in a
             * packed form, but the old writers would write a full
             * tile-size block of data. (compressed would be fine)
             */
            size_t sampsize =
                (((size_t) decode->chunk.width) * ((size_t) decode->chunk.height));
            sampsize *= sizeof (int32_t);

            if (decode->sample_count_alloc_size <
                decode->chunk.sample_count_table_size &&
                decode->sample_count_alloc_size < sampsize)
            {
                return EXR_ERR_OUT_OF_MEMORY;
            }

            if (decode->chunk.sample_count_table_size > 0)
            {
                if (decode->chunk.sample_count_table_size < sampsize)
                {
                    memcpy (
                        decode->sample_count_table,
                        decode->packed_sample_count_table,
                        decode->chunk.sample_count_table_size);
                    memset (
                        decode->sample_count_table + (decode->chunk.sample_count_table_size / sizeof(int32_t)),
                        0,
                        sampsize - decode->chunk.sample_count_table_size);
                }
                else
                {
                    memcpy (
                        decode->sample_count_table,
                        decode->packed_sample_count_table,
                        sampsize);
                }
            }
            else
            {
                memset (
                    decode->sample_count_table,
                    0,
                    decode->sample_count_alloc_size);
            }
        }

        rv = unpack_sample_table (ctxt, decode);

        if ((decode->decode_flags & EXR_DECODE_SAMPLE_DATA_ONLY)) return rv;

        if (rv != EXR_ERR_SUCCESS)
            return ctxt->report_error (
                ctxt, rv, "Decode pipeline unable to unpack deep sample table");
    }

    if (rv == EXR_ERR_SUCCESS && decode->realloc_nonimage_data_fn)
        rv = decode->realloc_nonimage_data_fn (decode);
    if (rv != EXR_ERR_SUCCESS)
        return ctxt->report_error (
            ctxt,
            rv,
            "Decode pipeline unable to realloc deep sample table info");

    if (decode->chunk.unpacked_size > 0)
    {
        if (rv == EXR_ERR_SUCCESS && decode->unpack_and_convert_fn)
            rv = decode->unpack_and_convert_fn (decode);
        if (rv != EXR_ERR_SUCCESS)
            return ctxt->report_error (
                ctxt, rv, "Decode pipeline unable to unpack and convert data");
    }

    return rv;
}

/**************************************/

exr_result_t
exr_decoding_destroy (exr_const_context_t ctxt, exr_decode_pipeline_t* decode)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (decode)
    {
        exr_decode_pipeline_t nil = {0};
        if (decode->channels != decode->_quick_chan_store)
            ctxt->free_fn (decode->channels);

        if (decode->unpacked_buffer == decode->packed_buffer &&
            decode->unpacked_alloc_size == 0)
            decode->unpacked_buffer = NULL;

        if (decode->sample_count_table == decode->packed_sample_count_table &&
            decode->sample_count_alloc_size == 0)
            decode->sample_count_table = NULL;

        internal_decode_free_buffer (
            decode,
            EXR_TRANSCODE_BUFFER_PACKED,
            &(decode->packed_buffer),
            &(decode->packed_alloc_size));
        internal_decode_free_buffer (
            decode,
            EXR_TRANSCODE_BUFFER_UNPACKED,
            &(decode->unpacked_buffer),
            &(decode->unpacked_alloc_size));
        internal_decode_free_buffer (
            decode,
            EXR_TRANSCODE_BUFFER_SCRATCH1,
            &(decode->scratch_buffer_1),
            &(decode->scratch_alloc_size_1));
        internal_decode_free_buffer (
            decode,
            EXR_TRANSCODE_BUFFER_SCRATCH2,
            &(decode->scratch_buffer_2),
            &(decode->scratch_alloc_size_2));

        internal_decode_free_buffer (
            decode,
            EXR_TRANSCODE_BUFFER_SAMPLES,
            (void**) &(decode->sample_count_table),
            &(decode->sample_count_alloc_size));
        internal_decode_free_buffer (
            decode,
            EXR_TRANSCODE_BUFFER_PACKED_SAMPLES,
            &(decode->packed_sample_count_table),
            &(decode->packed_sample_count_alloc_size));
        *decode = nil;
    }
    return EXR_ERR_SUCCESS;
}
