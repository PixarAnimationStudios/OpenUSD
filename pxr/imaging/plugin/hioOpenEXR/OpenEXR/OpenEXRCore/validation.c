/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_file.h"

#include "internal_constants.h"

#include "openexr_part.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/**************************************/

static exr_result_t
validate_req_attr (exr_context_t f, exr_priv_part_t curpart, int adddefault)
{
    exr_result_t rv = EXR_ERR_SUCCESS;

    if (!curpart->compression)
    {
        if (adddefault)
        {
            rv = exr_attr_list_add_static_name (
                (exr_context_t) f,
                &(curpart->attributes),
                "compression",
                EXR_ATTR_COMPRESSION,
                0,
                NULL,
                &(curpart->compression));
            if (rv != EXR_ERR_SUCCESS) return rv;
            curpart->compression->uc = (uint8_t) EXR_COMPRESSION_ZIP;
            curpart->comp_type       = EXR_COMPRESSION_ZIP;
        }
        else
        {
            return f->print_error (
                f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'compression' attribute not found");
        }
    }
    else if (curpart->compression->type != EXR_ATTR_COMPRESSION)
        return f->print_error (
            f,
            EXR_ERR_ATTR_TYPE_MISMATCH,
            "'compression' attribute has wrong data type");

    if (!curpart->dataWindow)
    {
        if (adddefault)
        {
            exr_attr_box2i_t defdw = {{.x = 0, .y = 0}, {.x = 63, .y = 63}};
            rv                     = exr_attr_list_add_static_name (
                (exr_context_t) f,
                &(curpart->attributes),
                "dataWindow",
                EXR_ATTR_BOX2I,
                0,
                NULL,
                &(curpart->dataWindow));
            if (rv != EXR_ERR_SUCCESS) return rv;
            *(curpart->dataWindow->box2i) = defdw;
            curpart->data_window          = defdw;

            rv = internal_exr_compute_tile_information (f, curpart, 1);
        }
        else
        {
            return f->print_error (
                f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'dataWindow' attribute not found");
        }
    }
    else if (curpart->dataWindow->type != EXR_ATTR_BOX2I)
        return f->print_error (
            f,
            EXR_ERR_ATTR_TYPE_MISMATCH,
            "'dataWindow' attribute has wrong data type");

    if (!curpart->displayWindow)
    {
        if (adddefault)
        {
            exr_attr_box2i_t defdw = {{.x = 0, .y = 0}, {.x = 63, .y = 63}};
            rv                     = exr_attr_list_add_static_name (
                (exr_context_t) f,
                &(curpart->attributes),
                "displayWindow",
                EXR_ATTR_BOX2I,
                0,
                NULL,
                &(curpart->displayWindow));
            if (rv != EXR_ERR_SUCCESS) return rv;
            *(curpart->displayWindow->box2i) = defdw;
            curpart->display_window          = defdw;
        }
        else
        {
            return f->print_error (
                f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'displayWindow' attribute not found");
        }
    }
    else if (curpart->displayWindow->type != EXR_ATTR_BOX2I)
        return f->print_error (
            f,
            EXR_ERR_ATTR_TYPE_MISMATCH,
            "'displayWindow' attribute has wrong data type");

    if (!curpart->lineOrder)
    {
        if (adddefault)
        {
            rv = exr_attr_list_add_static_name (
                (exr_context_t) f,
                &(curpart->attributes),
                "lineOrder",
                EXR_ATTR_LINEORDER,
                0,
                NULL,
                &(curpart->lineOrder));
            if (rv != EXR_ERR_SUCCESS) return rv;
            curpart->lineOrder->uc = (uint8_t) EXR_LINEORDER_INCREASING_Y;
            curpart->lineorder     = EXR_LINEORDER_INCREASING_Y;
        }
        else
        {
            return f->print_error (
                f, EXR_ERR_MISSING_REQ_ATTR, "'lineOrder' attribute not found");
        }
    }
    else if (curpart->lineOrder->type != EXR_ATTR_LINEORDER)
        return f->print_error (
            f,
            EXR_ERR_ATTR_TYPE_MISMATCH,
            "'lineOrder' attribute has wrong data type");

    if (!curpart->pixelAspectRatio)
    {
        if (adddefault)
        {
            rv = exr_attr_list_add_static_name (
                (exr_context_t) f,
                &(curpart->attributes),
                "pixelAspectRatio",
                EXR_ATTR_FLOAT,
                0,
                NULL,
                &(curpart->pixelAspectRatio));
            if (rv != EXR_ERR_SUCCESS) return rv;
            curpart->pixelAspectRatio->f = 1.f;
        }
        else
        {
            return f->print_error (
                f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'pixelAspectRatio' attribute not found");
        }
    }
    else if (curpart->pixelAspectRatio->type != EXR_ATTR_FLOAT)
        return f->print_error (
            f,
            EXR_ERR_ATTR_TYPE_MISMATCH,
            "'pixelAspectRatio' attribute has wrong data type");

    if (!curpart->screenWindowCenter)
    {
        if (adddefault)
        {
            exr_attr_v2f_t defswc = {.x = 0.f, .y = 0.f};
            rv                    = exr_attr_list_add_static_name (
                (exr_context_t) f,
                &(curpart->attributes),
                "screenWindowCenter",
                EXR_ATTR_V2F,
                0,
                NULL,
                &(curpart->screenWindowCenter));
            if (rv != EXR_ERR_SUCCESS) return rv;
            *(curpart->screenWindowCenter->v2f) = defswc;
        }
        else
        {
            return f->print_error (
                f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'screenWindowCenter' attribute not found");
        }
    }
    else if (curpart->screenWindowCenter->type != EXR_ATTR_V2F)
        return f->print_error (
            f,
            EXR_ERR_ATTR_TYPE_MISMATCH,
            "'screenWindowCenter' attribute has wrong data type");

    if (!curpart->screenWindowWidth)
    {
        if (adddefault)
        {
            rv = exr_attr_list_add_static_name (
                (exr_context_t) f,
                &(curpart->attributes),
                "screenWindowWidth",
                EXR_ATTR_FLOAT,
                0,
                NULL,
                &(curpart->screenWindowWidth));
            if (rv != EXR_ERR_SUCCESS) return rv;
            curpart->screenWindowWidth->f = 1.f;
        }
        else
        {
            return f->print_error (
                f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'screenWindowWidth' attribute not found");
        }
    }
    else if (curpart->screenWindowWidth->type != EXR_ATTR_FLOAT)
        return f->print_error (
            f,
            EXR_ERR_ATTR_TYPE_MISMATCH,
            "'screenWindowWidth' attribute has wrong data type, expect float");

    if (f->is_multipart || f->has_nonimage_data)
    {
        if (f->is_multipart)
        {
            if (!curpart->name)
                return f->print_error (
                    f,
                    EXR_ERR_MISSING_REQ_ATTR,
                    "'name' attribute for multipart file not found");
            else if (curpart->name->type != EXR_ATTR_STRING)
                return f->print_error (
                    f,
                    EXR_ERR_ATTR_TYPE_MISMATCH,
                    "'name' attribute has wrong data type, expect string");
        }
        if (!curpart->type)
        {
            return f->print_error (
                f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'type' attribute for v2+ file not found");
        }
        else if (curpart->type->type != EXR_ATTR_STRING)
            return f->print_error (
                f,
                EXR_ERR_ATTR_TYPE_MISMATCH,
                "'type' attribute has wrong data type, expect string");
        if (f->has_nonimage_data && !curpart->version)
        {
            if (adddefault)
            {
                rv = exr_attr_list_add_static_name (
                    f,
                    &(curpart->attributes),
                    EXR_REQ_VERSION_STR,
                    EXR_ATTR_INT,
                    0,
                    NULL,
                    &(curpart->version));
                curpart->version->i = 1;
            }
            else
            {
                return f->print_error (
                    f,
                    EXR_ERR_MISSING_REQ_ATTR,
                    "'version' attribute for deep file not found");
            }
        }
        if (f->strict_header && !curpart->chunkCount)
            return f->print_error (
                f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'chunkCount' attribute for multipart / deep file not found");
    }

    return rv;
}

/**************************************/

static exr_result_t
validate_image_dimensions (exr_context_t f, exr_priv_part_t curpart)
{
    // sanity check the various parts...
    const int64_t          kLargeVal = (int64_t) (INT32_MAX / 2);
    const exr_attr_box2i_t dw        = curpart->data_window;
    const exr_attr_box2i_t dspw      = curpart->display_window;
    int64_t                w, h;
    float                  par, sww;
    int                    maxw = f->max_image_w;
    int                    maxh = f->max_image_h;

    par = curpart->pixelAspectRatio->f;
    sww = curpart->screenWindowWidth->f;

    w = (int64_t) dw.max.x - (int64_t) dw.min.x + 1;
    h = (int64_t) dw.max.y - (int64_t) dw.min.y + 1;

    if (dspw.min.x > dspw.max.x || dspw.min.y > dspw.max.y ||
        dspw.min.x <= -kLargeVal || dspw.min.y <= -kLargeVal ||
        dspw.max.x >= kLargeVal || dspw.max.y >= kLargeVal)
        return f->print_error (
            f,
            EXR_ERR_INVALID_ATTR,
            "Invalid display window (%d, %d - %d, %d)",
            dspw.min.x,
            dspw.min.y,
            dspw.max.x,
            dspw.max.y);

    if (dw.min.x > dw.max.x || dw.min.y > dw.max.y || dw.min.x <= -kLargeVal ||
        dw.min.y <= -kLargeVal || dw.max.x >= kLargeVal ||
        dw.max.y >= kLargeVal)
        return f->print_error (
            f,
            EXR_ERR_INVALID_ATTR,
            "Invalid data window (%d, %d - %d, %d)",
            dw.min.x,
            dw.min.y,
            dw.max.x,
            dw.max.y);

    if (maxw > 0 && maxw < w)
        return f->print_error (
            f,
            EXR_ERR_INVALID_ATTR,
            "Invalid width (%" PRId64 ") too large (max %d)",
            w,
            maxw);

    if (maxh > 0 && maxh < h)
        return f->print_error (
            f,
            EXR_ERR_INVALID_ATTR,
            "Invalid height (%" PRId64 ") too large (max %d)",
            h,
            maxh);

    if (maxw > 0 && maxh > 0)
    {
        int64_t maxNum = (int64_t) maxw * (int64_t) maxh;
        int64_t ccount = 0;
        if (curpart->chunkCount) ccount = (int64_t) curpart->chunk_count;
        if (ccount > maxNum)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "Invalid chunkCount (%" PRId64
                ") exceeds maximum area of %" PRId64 "",
                ccount,
                maxNum);
    }

    /* isnormal will return true when par is 0, which should also be disallowed */
    if (!isnormal (par) || par < 1e-6f || par > 1e+6f)
        return f->print_error (
            f,
            EXR_ERR_INVALID_ATTR,
            "Invalid pixel aspect ratio %g",
            (double) par);

    if (sww < 0.f)
        return f->print_error (
            f,
            EXR_ERR_INVALID_ATTR,
            "Invalid screen window width %g",
            (double) sww);

    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
validate_channels (
    exr_context_t f, exr_priv_part_t curpart)
{
    exr_attr_box2i_t         dw;
    int64_t                  w, h;
    const exr_attr_chlist_t* channels;

    if (!curpart->channels)
        return f->print_error (
            f, EXR_ERR_MISSING_REQ_ATTR, "'channels' attribute not found");
    else if (curpart->channels->type != EXR_ATTR_CHLIST)
        return f->print_error (
            f,
            EXR_ERR_ATTR_TYPE_MISMATCH,
            "'channels' attribute has wrong data type, expect chlist");

    channels = curpart->channels->chlist;

    if (!curpart->dataWindow)
        return f->report_error (
            f,
            EXR_ERR_NO_ATTR_BY_NAME,
            "request to validate channel list, but data window not set to validate against");

    if (channels->num_channels <= 0)
        return f->report_error (
            f, EXR_ERR_FILE_BAD_HEADER, "At least one channel required");

    dw = curpart->data_window;
    w  = (int64_t) dw.max.x - (int64_t) dw.min.x + 1;
    h  = (int64_t) dw.max.y - (int64_t) dw.min.y + 1;

    for (int c = 0; c < channels->num_channels; ++c)
    {
        int32_t xsamp = channels->entries[c].x_sampling;
        int32_t ysamp = channels->entries[c].y_sampling;

        if (xsamp < 1)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': x subsampling factor is invalid (%d)",
                channels->entries[c].name.str,
                xsamp);
        if (ysamp < 1)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': y subsampling factor is invalid (%d)",
                channels->entries[c].name.str,
                ysamp);
        if (dw.min.x % xsamp)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': minimum x coordinate (%d) of the data window is not a multiple of the x subsampling factor (%d)",
                channels->entries[c].name.str,
                dw.min.x,
                xsamp);
        if (dw.min.y % ysamp)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': minimum y coordinate (%d) of the data window is not a multiple of the y subsampling factor (%d)",
                channels->entries[c].name.str,
                dw.min.y,
                ysamp);
        if (w % xsamp)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': row width (%" PRId64
                ") of the data window is not a multiple of the x subsampling factor (%d)",
                channels->entries[c].name.str,
                w,
                xsamp);
        if (h % ysamp)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': column height (%" PRId64
                ") of the data window is not a multiple of the y subsampling factor (%d)",
                channels->entries[c].name.str,
                h,
                ysamp);
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
validate_part_type (exr_context_t f, exr_priv_part_t curpart)
{
    // TODO: there are probably more tests to add here...
    if (curpart->type)
    {
        const char *expectedtype = NULL;
        exr_result_t rv;

        // see if the type overwrote the storage mode
        if (f->is_singlepart_tiled)
        {
            if (f->is_multipart || f->num_parts > 1)
                return f->print_error (
                    f,
                    EXR_ERR_INVALID_ATTR,
                    "Multipart files cannot have the tiled bit set");

            if (curpart->storage_mode != EXR_STORAGE_TILED)
            {
                curpart->storage_mode = EXR_STORAGE_TILED;

                if (f->strict_header)
                {
                    return f->print_error (
                        f,
                        EXR_ERR_INVALID_ATTR,
                        "attribute 'type': Single part tiled flag set but not marked as tiled storage type");
                }
            }
        }

        if (curpart->storage_mode == EXR_STORAGE_SCANLINE)
            expectedtype = "scanlineimage";
        else if (curpart->storage_mode == EXR_STORAGE_TILED)
            expectedtype = "tiledimage";
        else if (curpart->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
            expectedtype = "deepscanline";
        else if (curpart->storage_mode == EXR_STORAGE_DEEP_TILED)
            expectedtype = "deeptile";

        if (expectedtype && 0 != strcmp (curpart->type->string->str, expectedtype))
        {
            if (f->mode == EXR_CONTEXT_WRITE) return EXR_ERR_INVALID_ATTR;

            if (f->strict_header)
            {
                return f->print_error (
                    f,
                    EXR_ERR_INVALID_ATTR,
                    "attribute 'type': Type should be '%s' but set to '%s', believing file flags",
                    expectedtype,
                    curpart->type->string->str);
            }
            else
            {
                /* C++ silently changed this */
                rv = exr_attr_string_set (
                    f, curpart->type->string, expectedtype);

                if (rv != EXR_ERR_SUCCESS)
                    return f->print_error (
                        f,
                        EXR_ERR_INVALID_ATTR,
                        "attribute 'type': Mismatch between file flags and type attribute, unable to fix");
            }
        }
    }

    /* NB: we allow an 'unknown' storage type of EXR_STORAGE_UNKNOWN
     * for future proofing */
    if (curpart->storage_mode == EXR_STORAGE_LAST_TYPE)
    {
        return f->print_error (
            f,
            EXR_ERR_INVALID_ATTR,
            "Unable to determine data storage type for part");
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
validate_tile_data (exr_context_t f, exr_priv_part_t curpart)
{
    if (curpart->storage_mode == EXR_STORAGE_TILED ||
        curpart->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        const exr_attr_tiledesc_t* desc;
        const int                  maxtilew = f->max_tile_w;
        const int                  maxtileh = f->max_tile_h;
        const exr_attr_chlist_t*   channels = curpart->channels->chlist;
        exr_tile_level_mode_t      levmode;
        exr_tile_round_mode_t      rndmode;

        if (!curpart->tiles)
            return f->print_error (
                f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'tiles' attribute for tiled file not found");
        else if (curpart->tiles->type != EXR_ATTR_TILEDESC)
            return f->print_error (
                f,
                EXR_ERR_ATTR_TYPE_MISMATCH,
                "'tiles' attribute has wrong data type, expect tile description");

        desc    = curpart->tiles->tiledesc;
        levmode = EXR_GET_TILE_LEVEL_MODE (*desc);
        rndmode = EXR_GET_TILE_ROUND_MODE (*desc);

        if (desc->x_size == 0 || desc->y_size == 0 ||
            desc->x_size > (uint32_t) (INT_MAX / 4) ||
            desc->y_size > (uint32_t) (INT_MAX / 4))
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "Invalid tile description size (%u x %u)",
                desc->x_size,
                desc->y_size);
        if (maxtilew > 0 && maxtilew < (int) (desc->x_size))
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "Width of tile exceeds max size (%d vs max %d)",
                (int) desc->x_size,
                maxtilew);
        if (maxtileh > 0 && maxtileh < (int) (desc->y_size))
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "Width of tile exceeds max size (%d vs max %d)",
                (int) desc->y_size,
                maxtileh);

        if ((int) levmode < EXR_TILE_ONE_LEVEL ||
            (int) levmode >= EXR_TILE_LAST_TYPE)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "Invalid level mode (%d) in tile description header",
                (int) levmode);

        if ((int) rndmode < EXR_TILE_ROUND_DOWN ||
            (int) rndmode >= EXR_TILE_ROUND_LAST_TYPE)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "Invalid rounding mode (%d) in tile description header",
                (int) rndmode);

        for (int c = 0; c < channels->num_channels; ++c)
        {
            if (channels->entries[c].x_sampling != 1)
                return f->print_error (
                    f,
                    EXR_ERR_INVALID_ATTR,
                    "channel '%s': x subsampling factor is not 1 (%d) for a tiled image",
                    channels->entries[c].name.str,
                    channels->entries[c].x_sampling);
            if (channels->entries[c].y_sampling != 1)
                return f->print_error (
                    f,
                    EXR_ERR_INVALID_ATTR,
                    "channel '%s': y subsampling factor is not 1 (%d) for a tiled image",
                    channels->entries[c].name.str,
                    channels->entries[c].y_sampling);
        }
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
validate_deep_data (exr_context_t f, exr_priv_part_t curpart)
{
    if (curpart->storage_mode == EXR_STORAGE_DEEP_SCANLINE ||
        curpart->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        const exr_attr_chlist_t* channels = curpart->channels->chlist;

        // none, rle, zips
        if (curpart->comp_type != EXR_COMPRESSION_NONE &&
            curpart->comp_type != EXR_COMPRESSION_RLE &&
            curpart->comp_type != EXR_COMPRESSION_ZIPS)
            return f->report_error (
                f, EXR_ERR_INVALID_ATTR, "Invalid compression for deep data");

        for (int c = 0; c < channels->num_channels; ++c)
        {
            if (channels->entries[c].x_sampling != 1)
                return f->print_error (
                    f,
                    EXR_ERR_INVALID_ATTR,
                    "channel '%s': x subsampling factor is not 1 (%d) for a deep image",
                    channels->entries[c].name.str,
                    channels->entries[c].x_sampling);
            if (channels->entries[c].y_sampling != 1)
                return f->print_error (
                    f,
                    EXR_ERR_INVALID_ATTR,
                    "channel '%s': y subsampling factor is not 1 (%d) for a deep image",
                    channels->entries[c].name.str,
                    channels->entries[c].y_sampling);
        }
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
internal_exr_validate_read_part (exr_context_t f, exr_priv_part_t curpart)
{
    exr_result_t rv;

    rv = validate_req_attr (f, curpart, !f->strict_header);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_image_dimensions (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_channels (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_part_type (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_tile_data (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_deep_data (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
internal_exr_validate_shared_attrs (exr_context_t ctxt,
                                    exr_priv_part_t basepart,
                                    exr_priv_part_t curpart,
                                    int curpartidx,
                                    const char **mismatchattr,
                                    int *mismatchcount)
{
    exr_result_t rv, rv1;
    const exr_attribute_t *battr, *cattr;
    int misidx = 0;

    rv = EXR_ERR_SUCCESS;
    if (basepart->displayWindow)
    {
        if (curpart->displayWindow)
        {
            if (basepart->displayWindow->type != EXR_ATTR_BOX2I ||
                basepart->displayWindow->type !=
                curpart->displayWindow->type)
            {
                rv = EXR_ERR_ATTR_TYPE_MISMATCH;
            }
            else if (memcmp (basepart->displayWindow->box2i,
                             curpart->displayWindow->box2i,
                             sizeof(exr_attr_box2i_t)))
                rv = EXR_ERR_ATTR_TYPE_MISMATCH;
        }
        else
            rv = EXR_ERR_ATTR_TYPE_MISMATCH;
    }
    else if (curpart->displayWindow)
        rv = EXR_ERR_ATTR_TYPE_MISMATCH;

    if (rv != EXR_ERR_SUCCESS)
        mismatchattr[misidx++] = EXR_REQ_DISP_STR;

    rv = EXR_ERR_SUCCESS;
    if (basepart->pixelAspectRatio)
    {
        if (curpart->pixelAspectRatio)
        {
            if (basepart->pixelAspectRatio->type != EXR_ATTR_FLOAT ||
                basepart->pixelAspectRatio->type !=
                curpart->pixelAspectRatio->type)
            {
                rv = EXR_ERR_ATTR_TYPE_MISMATCH;
            }
            else if (memcmp (&(basepart->pixelAspectRatio->f),
                             &(curpart->pixelAspectRatio->f),
                             sizeof(float)))
                rv = EXR_ERR_ATTR_TYPE_MISMATCH;
        }
        else
            rv = EXR_ERR_ATTR_TYPE_MISMATCH;
    }
    else if (curpart->pixelAspectRatio)
        rv = EXR_ERR_ATTR_TYPE_MISMATCH;
    if (rv != EXR_ERR_SUCCESS)
        mismatchattr[misidx++] = EXR_REQ_PAR_STR;

    rv = exr_get_attribute_by_name (ctxt, 0, "timecode", &battr);
    rv1 = exr_get_attribute_by_name (ctxt, curpartidx, "timecode", &cattr);
    if (EXR_ERR_SUCCESS == rv && rv == rv1)
    {
        if (battr->type != EXR_ATTR_TIMECODE ||
            battr->type != cattr->type)
        {
            rv = EXR_ERR_ATTR_TYPE_MISMATCH;
        }
        else if (memcmp (battr->timecode,
                         cattr->timecode,
                         sizeof(exr_attr_timecode_t)))
        {
            rv = EXR_ERR_ATTR_TYPE_MISMATCH;
        }
        else
            rv = EXR_ERR_SUCCESS;
    }
    else if (EXR_ERR_SUCCESS == rv1)
        rv = EXR_ERR_ATTR_TYPE_MISMATCH;
    else
        rv = EXR_ERR_SUCCESS; // both missing, ok
    if (rv != EXR_ERR_SUCCESS)
        mismatchattr[misidx++] = "timecode";

    rv = exr_get_attribute_by_name (ctxt, 0, "chromaticities", &battr);
    rv1 = exr_get_attribute_by_name (ctxt, curpartidx, "chromaticities", &cattr);
    if (EXR_ERR_SUCCESS == rv && rv == rv1)
    {
        if (battr->type != EXR_ATTR_CHROMATICITIES ||
            battr->type != cattr->type)
        {
            rv = EXR_ERR_ATTR_TYPE_MISMATCH;
        }
        else if (memcmp (battr->chromaticities,
                         cattr->chromaticities,
                         sizeof(exr_attr_chromaticities_t)))
            rv = EXR_ERR_ATTR_TYPE_MISMATCH;
        else
            rv = EXR_ERR_SUCCESS;
    }
    else if (EXR_ERR_SUCCESS == rv1)
        rv = EXR_ERR_ATTR_TYPE_MISMATCH;
    else
        rv = EXR_ERR_SUCCESS; // both missing, ok
    if (rv != EXR_ERR_SUCCESS)
        mismatchattr[misidx++] = "chromaticities";

    *mismatchcount = misidx;
    return misidx == 0 ? EXR_ERR_SUCCESS : EXR_ERR_ATTR_TYPE_MISMATCH;
}

/**************************************/

exr_result_t
internal_exr_validate_write_part (exr_context_t f, exr_priv_part_t curpart)
{
    exr_result_t rv;

    rv = validate_req_attr (f, curpart, 0);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_image_dimensions (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_channels (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_part_type (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_tile_data (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_deep_data (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    return EXR_ERR_SUCCESS;
}
