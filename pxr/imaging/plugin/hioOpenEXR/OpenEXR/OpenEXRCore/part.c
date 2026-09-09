/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_part.h"

#include "internal_attr.h"
#include "internal_constants.h"
#include "internal_structs.h"

#include <string.h>

/**************************************/

exr_result_t
exr_get_count (exr_const_context_t ctxt, int* count)
{
    int cnt;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;
    if (!count) return ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    if (ctxt->mode == EXR_CONTEXT_WRITE)
    {
        internal_exr_lock (ctxt);
        cnt = ctxt->num_parts;
        internal_exr_unlock (ctxt);
    }
    else
        cnt = ctxt->num_parts;

    *count = cnt;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_get_storage (exr_const_context_t ctxt, int part_index, exr_storage_t* out)
{
    exr_storage_t smode;
    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);
    smode = part->storage_mode;
    if (ctxt->mode == EXR_CONTEXT_WRITE) internal_exr_unlock (ctxt);

    if (!out) return ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    *out = smode;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_add_part (
    exr_context_t ctxt,
    const char*   partname,
    exr_storage_t type,
    int*          new_index)
{
    exr_result_t    rv;
    size_t          pnamelen;
    int32_t         attrsz  = -1;
    const char*     typestr = NULL;
    exr_priv_part_t part    = NULL;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;
    internal_exr_lock (ctxt);

    if (ctxt->mode != EXR_CONTEXT_WRITE && ctxt->mode != EXR_CONTEXT_TEMPORARY)
        return EXR_UNLOCK_AND_RETURN (
            ctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE));

    pnamelen = partname ? strlen (partname) : 0;
    if (pnamelen >= INT32_MAX)
    {
        return EXR_UNLOCK_AND_RETURN (ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ATTR,
            "Part name '%s': Invalid name length %" PRIu64,
            partname,
            (uint64_t) pnamelen));
    }

    if (ctxt->num_parts > 0)
    {
        // ensure multi part has at least some name?
        if (!partname) partname = "";

        for ( int pidx = 0; pidx < ctxt->num_parts; ++pidx )
        {
            const exr_attribute_t* pname = ctxt->parts[pidx]->name;
            if (!pname)
            {
                return EXR_UNLOCK_AND_RETURN (
                    ctxt->print_error (
                        ctxt,
                        EXR_ERR_INVALID_ARGUMENT,
                        "Part %d missing required attribute 'name' for multi-part file",
                        pidx));
            }
            if (!strcmp (partname, pname->string->str))
            {
                return EXR_UNLOCK_AND_RETURN (
                    ctxt->print_error (
                        ctxt,
                        EXR_ERR_INVALID_ARGUMENT,
                        "Each part should have a unique name, part %d and %d attempting to have same name '%s'",
                        pidx, ctxt->num_parts, partname));
            }
        }
    }

    rv = internal_exr_add_part (ctxt, &part, new_index);
    if (rv != EXR_ERR_SUCCESS) return EXR_UNLOCK_AND_RETURN (rv);

    part->storage_mode = type;
    switch (type)
    {
        case EXR_STORAGE_SCANLINE:
            typestr = "scanlineimage";
            attrsz  = 13;
            break;
        case EXR_STORAGE_TILED:
            typestr = "tiledimage";
            attrsz  = 10;
            break;
        case EXR_STORAGE_DEEP_SCANLINE:
            typestr = "deepscanline";
            attrsz  = 12;
            break;
        case EXR_STORAGE_DEEP_TILED:
            typestr = "deeptile";
            attrsz  = 8;
            break;
        case EXR_STORAGE_LAST_TYPE:
        default:
            internal_exr_revert_add_part (ctxt, &part, new_index);
            return EXR_UNLOCK_AND_RETURN (ctxt->print_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid storage type %d for new part",
                (int) type));
    }

    rv = exr_attr_list_add_static_name (
        ctxt,
        &(part->attributes),
        EXR_REQ_TYPE_STR,
        EXR_ATTR_STRING,
        0,
        NULL,
        &(part->type));

    if (rv != EXR_ERR_SUCCESS)
    {
        internal_exr_revert_add_part (ctxt, &part, new_index);
        return EXR_UNLOCK_AND_RETURN (rv);
    }

    rv = exr_attr_string_init_static_with_length (
        ctxt, part->type->string, typestr, attrsz);

    if (rv != EXR_ERR_SUCCESS)
    {
        internal_exr_revert_add_part (ctxt, &part, new_index);
        return EXR_UNLOCK_AND_RETURN (rv);
    }

    if (partname)
    {
        rv = exr_attr_list_add_static_name (
            ctxt,
            &(part->attributes),
            EXR_REQ_NAME_STR,
            EXR_ATTR_STRING,
            0,
            NULL,
            &(part->name));

        if (rv == EXR_ERR_SUCCESS)
            rv = exr_attr_string_create_with_length (
                ctxt, part->name->string, partname, (int32_t) pnamelen);
    }

    if (rv == EXR_ERR_SUCCESS &&
        (type == EXR_STORAGE_DEEP_TILED || type == EXR_STORAGE_DEEP_SCANLINE))
    {
        rv = exr_attr_list_add_static_name (
            ctxt,
            &(part->attributes),
            EXR_REQ_VERSION_STR,
            EXR_ATTR_INT,
            0,
            NULL,
            &(part->version));
        if (rv == EXR_ERR_SUCCESS) part->version->i = 1;
        ctxt->has_nonimage_data = 1;
    }

    if (rv == EXR_ERR_SUCCESS)
    {
        if (ctxt->num_parts > 1) ctxt->is_multipart = 1;

        if (!ctxt->has_nonimage_data && ctxt->num_parts == 1 &&
            type == EXR_STORAGE_TILED)
            ctxt->is_singlepart_tiled = 1;
        else
            ctxt->is_singlepart_tiled = 0;
    }
    else
        internal_exr_revert_add_part (ctxt, &part, new_index);

    return EXR_UNLOCK_AND_RETURN (rv);
}

/**************************************/

exr_result_t
exr_get_tile_levels (
    exr_const_context_t ctxt, int part_index, int* levelsx, int* levelsy)
{
    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);

    if (part->storage_mode == EXR_STORAGE_TILED ||
        part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        if (!part->tiles || part->num_tile_levels_x <= 0 ||
            part->num_tile_levels_y <= 0 || !part->tile_level_tile_count_x ||
            !part->tile_level_tile_count_y)
        {
            return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->print_error (
                ctxt,
                EXR_ERR_MISSING_REQ_ATTR,
                "Tile data missing or corrupt"));
        }

        if (levelsx) *levelsx = part->num_tile_levels_x;
        if (levelsy) *levelsy = part->num_tile_levels_y;
        return EXR_UNLOCK_WRITE_AND_RETURN (EXR_ERR_SUCCESS);
    }

    return EXR_UNLOCK_WRITE_AND_RETURN (
        ctxt->standard_error (ctxt, EXR_ERR_TILE_SCAN_MIXEDAPI));
}

/**************************************/

exr_result_t exr_get_tile_counts (
    exr_const_context_t ctxt,
    int                 part_index,
    int                 levelx,
    int                 levely,
    int32_t*            countx,
    int32_t*            county)
{
    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);

    if (part->storage_mode == EXR_STORAGE_TILED ||
        part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        if (!part->tiles || part->num_tile_levels_x <= 0 ||
            part->num_tile_levels_y <= 0 || !part->tile_level_tile_count_x ||
            !part->tile_level_tile_count_y)
        {
            return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->print_error (
                ctxt,
                EXR_ERR_MISSING_REQ_ATTR,
                "Tile data missing or corrupt"));
        }

        if (levelx < 0 || levely < 0 || levelx >= part->num_tile_levels_x ||
            levely >= part->num_tile_levels_y)
            return EXR_UNLOCK_WRITE_AND_RETURN (
                ctxt->standard_error (ctxt, EXR_ERR_ARGUMENT_OUT_OF_RANGE));

        if (countx) *countx = part->tile_level_tile_count_x[levelx];
        if (county) *county = part->tile_level_tile_count_y[levely];
        return EXR_UNLOCK_WRITE_AND_RETURN (EXR_ERR_SUCCESS);
    }

    return EXR_UNLOCK_WRITE_AND_RETURN (
        ctxt->standard_error (ctxt, EXR_ERR_TILE_SCAN_MIXEDAPI));
}

/**************************************/

exr_result_t
exr_get_tile_sizes (
    exr_const_context_t ctxt,
    int                 part_index,
    int                 levelx,
    int                 levely,
    int32_t*            tilew,
    int32_t*            tileh)
{
    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);

    if (part->storage_mode == EXR_STORAGE_TILED ||
        part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        const exr_attr_tiledesc_t* tiledesc;

        if (!part->tiles || part->num_tile_levels_x <= 0 ||
            part->num_tile_levels_y <= 0 || !part->tile_level_tile_count_x ||
            !part->tile_level_tile_count_y)
        {
            return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->print_error (
                ctxt,
                EXR_ERR_MISSING_REQ_ATTR,
                "Tile data missing or corrupt"));
        }

        if (levelx < 0 || levely < 0 || levelx >= part->num_tile_levels_x ||
            levely >= part->num_tile_levels_y)
            return EXR_UNLOCK_WRITE_AND_RETURN (
                ctxt->standard_error (ctxt, EXR_ERR_ARGUMENT_OUT_OF_RANGE));

        tiledesc = part->tiles->tiledesc;
        if (tilew)
        {
            int32_t levw = part->tile_level_tile_size_x[levelx];
            if (tiledesc->x_size < (uint32_t) levw)
                *tilew = (int32_t) tiledesc->x_size;
            else
                *tilew = levw;
        }
        if (tileh)
        {
            int32_t levh = part->tile_level_tile_size_y[levely];
            if (tiledesc->y_size < (uint32_t) levh)
                *tileh = (int32_t) tiledesc->y_size;
            else
                *tileh = levh;
        }
        return EXR_UNLOCK_WRITE_AND_RETURN (EXR_ERR_SUCCESS);
    }

    return EXR_UNLOCK_WRITE_AND_RETURN (
        ctxt->standard_error (ctxt, EXR_ERR_TILE_SCAN_MIXEDAPI));
}

/**************************************/

exr_result_t
exr_get_level_sizes (
    exr_const_context_t ctxt,
    int                 part_index,
    int                 levelx,
    int                 levely,
    int32_t*            levw,
    int32_t*            levh)
{
    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);

    if (part->storage_mode == EXR_STORAGE_TILED ||
        part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        if (!part->tiles || part->num_tile_levels_x <= 0 ||
            part->num_tile_levels_y <= 0 || !part->tile_level_tile_count_x ||
            !part->tile_level_tile_count_y)
        {
            return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->print_error (
                ctxt,
                EXR_ERR_MISSING_REQ_ATTR,
                "Tile data missing or corrupt"));
        }

        if (levelx < 0 || levely < 0 || levelx >= part->num_tile_levels_x ||
            levely >= part->num_tile_levels_y)
            return EXR_UNLOCK_WRITE_AND_RETURN (
                ctxt->standard_error (ctxt, EXR_ERR_ARGUMENT_OUT_OF_RANGE));

        if (levw) *levw = part->tile_level_tile_size_x[levelx];
        if (levh) *levh = part->tile_level_tile_size_y[levely];
        return EXR_UNLOCK_WRITE_AND_RETURN (EXR_ERR_SUCCESS);
    }

    return EXR_UNLOCK_WRITE_AND_RETURN (
        ctxt->standard_error (ctxt, EXR_ERR_TILE_SCAN_MIXEDAPI));
}

/**************************************/

exr_result_t
exr_get_chunk_count (exr_const_context_t ctxt, int part_index, int32_t* out)
{
    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);

    if (!out)
        return EXR_UNLOCK_WRITE_AND_RETURN (
            ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT));

    if (part->dataWindow)
    {
        if (part->storage_mode == EXR_STORAGE_TILED ||
            part->storage_mode == EXR_STORAGE_DEEP_TILED)
        {
            if (part->tiles)
            {
                *out = part->chunk_count;
                return EXR_UNLOCK_WRITE_AND_RETURN (EXR_ERR_SUCCESS);
            }
            return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->report_error (
                ctxt,
                EXR_ERR_MISSING_REQ_ATTR,
                "Tile data missing or corrupt"));
        }
        else if (
            part->storage_mode == EXR_STORAGE_SCANLINE ||
            part->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
        {
            if (part->compression)
            {
                *out = part->chunk_count;
                return EXR_UNLOCK_WRITE_AND_RETURN (EXR_ERR_SUCCESS);
            }
            return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->report_error (
                ctxt,
                EXR_ERR_MISSING_REQ_ATTR,
                "Missing scanline chunk compression information"));
        }
        else if (part->storage_mode == EXR_STORAGE_UNKNOWN)
        {
            *out = part->chunk_count;
            return EXR_UNLOCK_WRITE_AND_RETURN (EXR_ERR_SUCCESS);
        }
    }

    return EXR_UNLOCK_WRITE_AND_RETURN (ctxt->report_error (
        ctxt,
        EXR_ERR_MISSING_REQ_ATTR,
        "Missing data window for chunk information"));
}

/**************************************/

exr_result_t extract_chunk_table (
    exr_const_context_t   ctxt,
    exr_const_priv_part_t part,
    uint64_t**            chunktable,
    uint64_t*             chunkminoffset);

exr_result_t
exr_get_chunk_table (exr_const_context_t ctxt, int part_index, uint64_t **table, int32_t* count)
{
    exr_result_t rv;

    if (!table)
        return EXR_ERR_INVALID_ARGUMENT;

    rv = exr_get_chunk_count (ctxt, part_index, count);
    if (rv == EXR_ERR_SUCCESS)
    {
        uint64_t chunkmin;
        EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);

        /* need to read from the file to get the packed chunk size */
        rv = extract_chunk_table (ctxt, part, table, &chunkmin);

        if (rv != EXR_ERR_SUCCESS) return rv;
    }

    return rv;
}

/**************************************/

exr_result_t
exr_validate_chunk_table (exr_context_t ctxt, int part_index)
{
    exr_result_t rv;
    uint64_t     chunkmin, maxoff = ((uint64_t) -1);
    uint64_t*    ctable;
    int          complete;
    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);

    /* need to read from the file to get the packed chunk size */
    rv = extract_chunk_table (ctxt, part, &ctable, &chunkmin);

    if (rv != EXR_ERR_SUCCESS) return rv;

    if (ctxt->file_size > 0) maxoff = (uint64_t) ctxt->file_size;
    complete = 1;

    for (int ci = 0; ci < part->chunk_count; ++ci)
    {
        uint64_t cchunk = ctable[ci];
        if (cchunk < chunkmin || cchunk >= maxoff)
        {
            complete = 0;
            break;
        }
    }

    if (!complete) return EXR_ERR_INCOMPLETE_CHUNK_TABLE;

    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_get_scanlines_per_chunk (
    exr_const_context_t ctxt, int part_index, int32_t* out)
{
    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);

    if (!out) return EXR_UNLOCK_WRITE_AND_RETURN (EXR_ERR_INVALID_ARGUMENT);

    if (part->storage_mode == EXR_STORAGE_SCANLINE ||
        part->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
    {
        *out = part->lines_per_chunk;
        return EXR_UNLOCK_WRITE_AND_RETURN (EXR_ERR_SUCCESS);
    }
    *out = -1;
    return EXR_UNLOCK_WRITE_AND_RETURN (
        ctxt->standard_error (ctxt, EXR_ERR_SCAN_TILE_MIXEDAPI));
}

/**************************************/

exr_result_t
exr_get_chunk_unpacked_size (
    exr_const_context_t ctxt, int part_index, uint64_t* out)
{
    uint64_t sz;
    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);
    sz = part->unpacked_size_per_chunk;
    if (ctxt->mode == EXR_CONTEXT_WRITE) internal_exr_unlock (ctxt);

    if (!out) return ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    *out = sz;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_get_zip_compression_level (
    exr_const_context_t ctxt, int part_index, int* level)
{
    int l;
    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);
    l = part->zip_compression_level;
    if (ctxt->mode == EXR_CONTEXT_WRITE) internal_exr_unlock (ctxt);

    if (!level) return ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);
    *level = l;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_set_zip_compression_level (exr_context_t ctxt, int part_index, int level)
{
    exr_result_t rv;
    EXR_LOCK_AND_DEFINE_PART (part_index);

    if (ctxt->mode != EXR_CONTEXT_WRITE && ctxt->mode != EXR_CONTEXT_TEMPORARY)
        return EXR_UNLOCK_AND_RETURN (
            ctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE));

    if (level >= -1 && level < 10)
    {
        part->zip_compression_level = level;
        rv                          = EXR_ERR_SUCCESS;
    }
    else
    {
        return EXR_UNLOCK_AND_RETURN (ctxt->report_error (
            ctxt, EXR_ERR_INVALID_ARGUMENT, "Invalid zip level specified"));
    }

    return EXR_UNLOCK_AND_RETURN (rv);
}

/**************************************/

exr_result_t
exr_get_dwa_compression_level (
    exr_const_context_t ctxt, int part_index, float* level)
{
    float l;
    EXR_LOCK_WRITE_AND_DEFINE_PART (part_index);
    l = part->dwa_compression_level;
    if (ctxt->mode == EXR_CONTEXT_WRITE) internal_exr_unlock (ctxt);

    if (!level) return ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);
    *level = l;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_set_dwa_compression_level (exr_context_t ctxt, int part_index, float level)
{
    exr_result_t rv;
    EXR_LOCK_AND_DEFINE_PART (part_index);

    if (ctxt->mode != EXR_CONTEXT_WRITE && ctxt->mode != EXR_CONTEXT_TEMPORARY)
        return EXR_UNLOCK_AND_RETURN (
            ctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE));

    // avoid bad math (fp exceptions or whatever) by clamping here
    // there has always been a clamp to 0, but on the upper end, there
    // is a limit too, where you only get black images anyway, so that
    // is not particularly useful, not that any large value will
    // really be crushing the image
    if (level >= 0.f && level <= (65504.f*100000.f))
    {
        part->dwa_compression_level = level;
        rv                          = EXR_ERR_SUCCESS;
    }
    else
    {
        return EXR_UNLOCK_AND_RETURN (ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid dwa quality level specified"));
    }

    return EXR_UNLOCK_AND_RETURN (rv);
}
