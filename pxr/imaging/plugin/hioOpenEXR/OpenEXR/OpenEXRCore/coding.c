/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_coding.h"
#include "internal_util.h"

#include <string.h>

exr_result_t
internal_coding_fill_channel_info (
    exr_coding_channel_info_t** channels,
    int16_t*                    num_chans,
    exr_coding_channel_info_t*  builtinextras,
    const exr_chunk_info_t*     cinfo,
    exr_const_context_t         ctxt,
    exr_const_priv_part_t       part)
{
    int                        chans;
    exr_attr_chlist_t*         chanlist;
    exr_coding_channel_info_t* chanfill;

    chanlist = part->channels->chlist;
    chans    = chanlist->num_channels;
    if (chans <= 5) { chanfill = builtinextras; }
    else
    {
        chanfill = ctxt->alloc_fn (
            (size_t) (chans) * sizeof (exr_coding_channel_info_t));
        if (chanfill == NULL)
            return ctxt->standard_error (ctxt, EXR_ERR_OUT_OF_MEMORY);
        memset (
            chanfill, 0, (size_t) (chans) * sizeof (exr_coding_channel_info_t));
    }

    for (int c = 0; c < chans; ++c)
    {
        const exr_attr_chlist_entry_t* curc = (chanlist->entries + c);
        exr_coding_channel_info_t*     decc = (chanfill + c);

        decc->channel_name = curc->name.str;

        decc->height = compute_sampled_height (
            cinfo->height, curc->y_sampling, cinfo->start_y);
        decc->width = compute_sampled_width (
            cinfo->width, curc->x_sampling, cinfo->start_x);

        decc->x_samples         = curc->x_sampling;
        decc->y_samples         = curc->y_sampling;
        decc->p_linear          = curc->p_linear;
        decc->bytes_per_element = (curc->pixel_type == EXR_PIXEL_HALF) ? 2 : 4;
        decc->data_type         = (uint16_t) (curc->pixel_type);

        /* initialize these so they don't trip us up during decoding
         * when the user also chooses to skip a channel */
        decc->user_bytes_per_element = decc->bytes_per_element;
        decc->user_data_type         = decc->data_type;
        /* but leave the rest as zero for the user to fill in */
    }

    *channels  = chanfill;
    *num_chans = (int16_t) chans;

    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
internal_coding_update_channel_info (
    exr_coding_channel_info_t* channels,
    int16_t                    num_chans,
    const exr_chunk_info_t*    cinfo,
    exr_const_context_t        ctxt,
    exr_const_priv_part_t      part)
{
    int                chans;
    exr_attr_chlist_t* chanlist;

    chanlist = part->channels->chlist;
    chans    = chanlist->num_channels;

    if (num_chans != chans)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Mismatch in channel counts: stored %d, incoming %d",
            num_chans,
            chans);

    for (int c = 0; c < chans; ++c)
    {
        const exr_attr_chlist_entry_t* curc = (chanlist->entries + c);
        exr_coding_channel_info_t*     ccic = (channels + c);

        ccic->channel_name = curc->name.str;

        ccic->height = compute_sampled_height (
            cinfo->height, curc->y_sampling, cinfo->start_y);
        ccic->width = compute_sampled_width (
            cinfo->width, curc->x_sampling, cinfo->start_x);

        ccic->x_samples = curc->x_sampling;
        ccic->y_samples = curc->y_sampling;

        ccic->p_linear          = curc->p_linear;
        ccic->bytes_per_element = (curc->pixel_type == EXR_PIXEL_HALF) ? 2 : 4;
        ccic->data_type         = (uint16_t) (curc->pixel_type);
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
internal_encode_free_buffer (
    exr_encode_pipeline_t*               encode,
    exr_transcoding_pipeline_buffer_id_t bufid,
    void**                               buf,
    size_t*                              sz)
{
    void*  curbuf = *buf;
    size_t cursz  = *sz;
    if (curbuf)
    {
        if (cursz > 0)
        {
            if (encode->free_fn)
                encode->free_fn (bufid, curbuf);
            else
            {
                exr_const_context_t ctxt = encode->context;
                EXR_CHECK_CONTEXT_AND_PART (encode->part_index);

                ctxt->free_fn (curbuf);
            }
        }
        *buf = NULL;
    }
    *sz = 0;
    return EXR_ERR_SUCCESS;
}

exr_result_t
internal_encode_alloc_buffer (
    exr_encode_pipeline_t*               encode,
    exr_transcoding_pipeline_buffer_id_t bufid,
    void**                               buf,
    size_t*                              cursz,
    size_t                               newsz)
{
    void* curbuf = *buf;
    if (newsz == 0)
    {
        exr_const_context_t ctxt = encode->context;
        EXR_CHECK_CONTEXT_AND_PART (encode->part_index);

        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Attempt to allocate 0 byte buffer for transcode buffer %d",
            (int) bufid);
    }

    if (!curbuf || *cursz < newsz)
    {
        internal_encode_free_buffer (encode, bufid, buf, cursz);

        if (encode->alloc_fn)
            curbuf = encode->alloc_fn (bufid, newsz);
        else
        {
            exr_const_context_t ctxt = encode->context;
            EXR_CHECK_CONTEXT_AND_PART (encode->part_index);

            curbuf = ctxt->alloc_fn (newsz);
        }

        if (curbuf == NULL)
        {
            exr_const_context_t ctxt = encode->context;
            EXR_CHECK_CONTEXT_AND_PART (encode->part_index);

            return ctxt->print_error (
                ctxt,
                EXR_ERR_OUT_OF_MEMORY,
                "Unable to allocate %" PRIu64 " bytes",
                (uint64_t) newsz);
        }

        *buf   = curbuf;
        *cursz = newsz;
    }
    return EXR_ERR_SUCCESS;
}

exr_result_t
internal_decode_free_buffer (
    exr_decode_pipeline_t*               decode,
    exr_transcoding_pipeline_buffer_id_t bufid,
    void**                               buf,
    size_t*                              sz)
{
    void*  curbuf = *buf;
    size_t cursz  = *sz;
    if (curbuf)
    {
        if (cursz > 0)
        {
            if (decode->free_fn)
                decode->free_fn (bufid, curbuf);
            else
            {
                exr_const_context_t ctxt = decode->context;
                EXR_CHECK_CONTEXT_AND_PART (decode->part_index);

                ctxt->free_fn (curbuf);
            }
        }
        *buf = NULL;
    }
    *sz = 0;
    return EXR_ERR_SUCCESS;
}

exr_result_t
internal_decode_alloc_buffer (
    exr_decode_pipeline_t*               decode,
    exr_transcoding_pipeline_buffer_id_t bufid,
    void**                               buf,
    size_t*                              cursz,
    size_t                               newsz)
{
    void* curbuf = *buf;

    /* We might have a zero size here due to y sampling on a scanline
     * image where there is an attempt to read that portion of the
     * image. Just shortcut here and handle at a higher level where
     * there is more context
     */
    if (newsz == 0) return EXR_ERR_SUCCESS;

    if (!curbuf || *cursz < newsz)
    {
        internal_decode_free_buffer (decode, bufid, buf, cursz);

        if (decode->alloc_fn)
            curbuf = decode->alloc_fn (bufid, newsz);
        else
        {
            exr_const_context_t ctxt = decode->context;
            EXR_CHECK_CONTEXT_AND_PART (decode->part_index);

            curbuf = ctxt->alloc_fn (newsz);
        }

        if (curbuf == NULL)
        {
            exr_const_context_t ctxt = decode->context;
            EXR_CHECK_CONTEXT_AND_PART (decode->part_index);

            return ctxt->print_error (
                ctxt,
                EXR_ERR_OUT_OF_MEMORY,
                "Unable to allocate %" PRIu64 " bytes",
                (uint64_t) newsz);
        }

        *buf   = curbuf;
        *cursz = newsz;
    }
    return EXR_ERR_SUCCESS;
}
