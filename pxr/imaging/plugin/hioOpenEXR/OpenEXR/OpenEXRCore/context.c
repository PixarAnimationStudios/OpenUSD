/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef _LARGEFILE64_SOURCE
// define this if it hasn't been defined elsewhere
#    define _LARGEFILE64_SOURCE
#endif

#include "openexr_config.h"
#include "openexr_context.h"

#include "openexr_part.h"

#include "internal_constants.h"
#include "internal_file.h"
#include "backward_compatibility.h"

#if defined(_WIN32) || defined(_WIN64)
#    include "internal_win32_file_impl.h"
#else
#    include "internal_posix_file_impl.h"
#endif

/**************************************/

static exr_result_t
dispatch_read (
    exr_const_context_t          ctxt,
    void*                        buf,
    uint64_t                     sz,
    uint64_t*                    offsetp,
    int64_t*                     nread,
    enum _INTERNAL_EXR_READ_MODE rmode)
{
    int64_t      rval = -1;
    exr_result_t rv   = EXR_ERR_UNKNOWN;

    if (nread) *nread = rval;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (!offsetp)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "read requested with no output offset pointer");

    if (ctxt->read_fn)
        rval = ctxt->read_fn (
            ctxt, ctxt->user_data, buf, sz, *offsetp, ctxt->print_error);
    else
        return ctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_READ);

    if (nread) *nread = rval;
    if (rval > 0) *offsetp += (uint64_t) rval;

    if (rval == (int64_t) sz || (rmode == EXR_ALLOW_SHORT_READ && rval >= 0))
        rv = EXR_ERR_SUCCESS;
    else
        rv = EXR_ERR_READ_IO;

    return rv;
}

/**************************************/

static exr_result_t
dispatch_write (
    exr_context_t ctxt, const void* buf, uint64_t sz, uint64_t* offsetp)
{
    int64_t rval = -1;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (!offsetp)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "write requested with no output offset pointer");

    if (ctxt->write_fn)
        rval = ctxt->write_fn (
            ctxt, ctxt->user_data, buf, sz, *offsetp, ctxt->print_error);
    else
        return ctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE);

    if (rval > 0) *offsetp += (uint64_t) rval;

    return (rval == (int64_t) sz) ? EXR_ERR_SUCCESS : EXR_ERR_WRITE_IO;
}

/**************************************/

static exr_result_t
process_query_size (exr_context_t ctxt, exr_context_initializer_t* inits)
{
    if (inits->size_fn)
    {
        ctxt->file_size =
            (inits->size_fn) ((exr_const_context_t) ctxt, ctxt->user_data);
    }
    else { ctxt->file_size = -1; }
    return EXR_ERR_SUCCESS;
}

/**************************************/

static inline exr_context_initializer_t
fill_context_data (const exr_context_initializer_t* ctxtdata)
{
    exr_context_initializer_t inits = EXR_DEFAULT_CONTEXT_INITIALIZER;
    if (ctxtdata)
    {
        inits.error_handler_fn = ctxtdata->error_handler_fn;
        inits.alloc_fn         = ctxtdata->alloc_fn;
        inits.free_fn          = ctxtdata->free_fn;
        inits.user_data        = ctxtdata->user_data;
        inits.read_fn          = ctxtdata->read_fn;
        inits.size_fn          = ctxtdata->size_fn;
        inits.write_fn         = ctxtdata->write_fn;
        inits.destroy_fn       = ctxtdata->destroy_fn;
        inits.max_image_width  = ctxtdata->max_image_width;
        inits.max_image_height = ctxtdata->max_image_height;
        inits.max_tile_width   = ctxtdata->max_tile_width;
        inits.max_tile_height  = ctxtdata->max_tile_height;
        if (ctxtdata->size >= sizeof (struct _exr_context_initializer_v2))
        {
            inits.zip_level   = ctxtdata->zip_level;
            inits.dwa_quality = ctxtdata->dwa_quality;
        }
        if (ctxtdata->size >= sizeof (struct _exr_context_initializer_v3))
        {
            inits.flags = ctxtdata->flags;
        }
    }

    internal_exr_update_default_handlers (&inits);
    return inits;
}

/**************************************/

exr_result_t
exr_test_file_header (
    const char* filename, const exr_context_initializer_t* ctxtdata)
{
    exr_result_t              rv    = EXR_ERR_SUCCESS;
    exr_context_t             ret   = NULL;
    exr_context_initializer_t inits = fill_context_data (ctxtdata);

    if (filename)
    {
        rv = internal_exr_alloc_context (
            &ret,
            &inits,
            EXR_CONTEXT_READ,
            sizeof (struct _internal_exr_filehandle));
        if (rv == EXR_ERR_SUCCESS)
        {
            ret->do_read = &dispatch_read;

            rv = exr_attr_string_create (ret, &(ret->filename), filename);
            if (rv == EXR_ERR_SUCCESS)
            {
                if (!inits.read_fn)
                {
                    inits.size_fn = &default_query_size_func;
                    rv            = default_init_read_file (ret);
                }

                if (rv == EXR_ERR_SUCCESS)
                    rv = process_query_size (ret, &inits);
                if (rv == EXR_ERR_SUCCESS) rv = internal_exr_check_magic (ret);
            }

            exr_finish (&ret);
        }
        else
            rv = EXR_ERR_OUT_OF_MEMORY;
    }
    else
    {
        inits.error_handler_fn (
            NULL,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid filename passed to test file header function");
        rv = EXR_ERR_INVALID_ARGUMENT;
    }
    return rv;
}

/**************************************/

exr_result_t
exr_finish (exr_context_t* pctxt)
{
    exr_context_t ctxt;
    exr_result_t  rv = EXR_ERR_SUCCESS;

    if (!pctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    ctxt = *pctxt;
    if (ctxt)
    {
        int failed = 0;
        if (ctxt->mode == EXR_CONTEXT_WRITE ||
            ctxt->mode == EXR_CONTEXT_WRITING_DATA)
            failed = 1;

        if (ctxt->mode != EXR_CONTEXT_READ &&
            ctxt->mode != EXR_CONTEXT_TEMPORARY)
            rv = finalize_write (ctxt, failed);

        if (ctxt->destroy_fn) ctxt->destroy_fn (ctxt, ctxt->user_data, failed);

        internal_exr_destroy_context (ctxt);
    }
    *pctxt = NULL;

    return rv;
}

/**************************************/

exr_result_t
exr_start_read (
    exr_context_t*                   ctxt,
    const char*                      filename,
    const exr_context_initializer_t* ctxtdata)
{
    exr_result_t              rv    = EXR_ERR_UNKNOWN;
    exr_context_t             ret   = NULL;
    exr_context_initializer_t inits = fill_context_data (ctxtdata);

    if (!ctxt)
    {
        if (!(inits.flags & EXR_CONTEXT_FLAG_SILENT_HEADER_PARSE))
            inits.error_handler_fn (
                NULL,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid context handle passed to start_read function");
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if (filename)
    {
        rv = internal_exr_alloc_context (
            &ret,
            &inits,
            EXR_CONTEXT_READ,
            sizeof (struct _internal_exr_filehandle));
        if (rv == EXR_ERR_SUCCESS)
        {
            ret->do_read = &dispatch_read;

            rv = exr_attr_string_create (
                (exr_context_t) ret, &(ret->filename), filename);
            if (rv == EXR_ERR_SUCCESS)
            {
                if (!inits.read_fn)
                {
                    inits.size_fn = &default_query_size_func;
                    rv            = default_init_read_file (ret);
                }

                if (rv == EXR_ERR_SUCCESS)
                    rv = process_query_size (ret, &inits);
                if (rv == EXR_ERR_SUCCESS) rv = internal_exr_parse_header (ret);
            }

            if (rv != EXR_ERR_SUCCESS) exr_finish ((exr_context_t*) &ret);
        }
        else
            rv = EXR_ERR_OUT_OF_MEMORY;
    }
    else
    {
        if (!(inits.flags & EXR_CONTEXT_FLAG_SILENT_HEADER_PARSE))
            inits.error_handler_fn (
                NULL,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid filename passed to start_read function");
        rv = EXR_ERR_INVALID_ARGUMENT;
    }

    *ctxt = (exr_context_t) ret;
    return rv;
}

/**************************************/

exr_result_t
exr_start_write (
    exr_context_t*                   ctxt,
    const char*                      filename,
    exr_default_write_mode_t         default_mode,
    const exr_context_initializer_t* ctxtdata)
{
    int                       rv    = EXR_ERR_UNKNOWN;
    exr_context_t             ret   = NULL;
    exr_context_initializer_t inits = fill_context_data (ctxtdata);

    if (!ctxt)
    {
        inits.error_handler_fn (
            NULL,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid context handle passed to start_read function");
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if (filename)
    {
        rv = internal_exr_alloc_context (
            &ret,
            &inits,
            EXR_CONTEXT_WRITE,
            sizeof (struct _internal_exr_filehandle));
        if (rv == EXR_ERR_SUCCESS)
        {
            ret->do_write = &dispatch_write;

            rv = exr_attr_string_create (
                (exr_context_t) ret, &(ret->filename), filename);

            if (rv == EXR_ERR_SUCCESS)
            {
                if (!inits.write_fn)
                {
                    if (default_mode == EXR_INTERMEDIATE_TEMP_FILE)
                        rv = make_temp_filename (ret);
                    if (rv == EXR_ERR_SUCCESS)
                        rv = default_init_write_file (ret);
                }
            }

            if (rv != EXR_ERR_SUCCESS) exr_finish ((exr_context_t*) &ret);
        }
        else
            rv = EXR_ERR_OUT_OF_MEMORY;
    }
    else
    {
        inits.error_handler_fn (
            NULL,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid filename passed to start_write function");
        rv = EXR_ERR_INVALID_ARGUMENT;
    }

    *ctxt = (exr_context_t) ret;
    return rv;
}

/**************************************/

exr_result_t
exr_start_inplace_header_update (
    exr_context_t*                   ctxt,
    const char*                      filename,
    const exr_context_initializer_t* ctxtdata)
{
    /* TODO: not yet implemented */
    (void) ctxt;
    (void) filename;
    (void) ctxtdata;
    return EXR_ERR_INVALID_ARGUMENT;
}

/**************************************/

exr_result_t exr_start_temporary_context (
    exr_context_t*                   ctxt,
    const char*                      context_name,
    const exr_context_initializer_t* ctxtdata)
{
    exr_result_t              rv    = EXR_ERR_UNKNOWN;
    exr_context_t             ret   = NULL;
    exr_context_initializer_t inits = fill_context_data (ctxtdata);

    if (!ctxt) return EXR_ERR_INVALID_ARGUMENT;

    rv = internal_exr_alloc_context (
        &ret,
        &inits,
        EXR_CONTEXT_TEMPORARY,
        0);

    if (rv == EXR_ERR_SUCCESS)
    {
        rv = exr_attr_string_create (
            (exr_context_t) ret, &(ret->filename), context_name ? context_name : "<temporary>");
        if (rv != EXR_ERR_SUCCESS) exr_finish ((exr_context_t*) &ret);
    }

    *ctxt = (exr_context_t) ret;
    return rv;
}


/**************************************/

exr_result_t
exr_get_file_name (exr_const_context_t ctxt, const char** name)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    /* not changeable after construction, no locking needed */
    if (name)
    {
        *name = ctxt->filename.str;
        if (ctxt->mode == EXR_CONTEXT_WRITE) internal_exr_unlock (ctxt);
        return EXR_ERR_SUCCESS;
    }

    return ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);
}

/**************************************/

exr_result_t
exr_get_file_version_and_flags (exr_const_context_t ctxt, uint32_t* ver)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;
    if (ctxt->mode == EXR_CONTEXT_WRITE) internal_exr_lock (ctxt);

    if (ver)
    {
        exr_result_t ret = EXR_ERR_SUCCESS;

        if (ctxt->orig_version_and_flags != 0)
            *ver = ctxt->orig_version_and_flags;
        else
            ret = internal_exr_calc_header_version_flags (ctxt, ver);

        if (ctxt->mode == EXR_CONTEXT_WRITE) internal_exr_unlock (ctxt);
        return ret;
    }

    if (ctxt->mode == EXR_CONTEXT_WRITE) internal_exr_unlock (ctxt);
    return ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);
}

/**************************************/

exr_result_t
exr_get_user_data (exr_const_context_t ctxt, void** userdata)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;
    if (ctxt->mode == EXR_CONTEXT_WRITE) internal_exr_lock (ctxt);

    /* not changeable after construction, no locking needed */
    if (userdata)
    {
        *userdata = ctxt->real_user_data;
        if (ctxt->mode == EXR_CONTEXT_WRITE) internal_exr_unlock (ctxt);
        return EXR_ERR_SUCCESS;
    }

    if (ctxt->mode == EXR_CONTEXT_WRITE) internal_exr_unlock (ctxt);
    return ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);
}

/**************************************/

exr_result_t
exr_register_attr_type_handler (
    exr_context_t ctxt,
    const char*   type,
    exr_result_t (*unpack_func_ptr) (
        exr_context_t ctxt,
        const void*   data,
        int32_t       attrsize,
        int32_t*      outsize,
        void**        outbuffer),
    exr_result_t (*pack_func_ptr) (
        exr_context_t ctxt,
        const void*   data,
        int32_t       datasize,
        int32_t*      outsize,
        void*         outbuffer),
    void (*destroy_unpacked_func_ptr) (
        exr_context_t ctxt, void* data, int32_t datasize))
{
    exr_attribute_t*      ent;
    exr_result_t          rv;
    int32_t               tlen, mlen = EXR_SHORTNAME_MAXLEN;
    size_t                slen;
    exr_attribute_list_t* curattrs;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;
    internal_exr_lock (ctxt);

    mlen = (int32_t) ctxt->max_name_length;

    if (!type || type[0] == '\0')
        return EXR_UNLOCK_AND_RETURN (ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid type to register_attr_handler"));

    slen = strlen (type);
    if (slen > (size_t) mlen)
        return EXR_UNLOCK_AND_RETURN (ctxt->print_error (
            ctxt,
            EXR_ERR_NAME_TOO_LONG,
            "Provided type name '%s' too long for file (len %d, max %d)",
            type,
            (int) slen,
            mlen));
    tlen = (int32_t) slen;

    if (internal_exr_is_standard_type (type))
        return EXR_UNLOCK_AND_RETURN (ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Provided type name '%s' is a reserved / internal type name",
            type));

    rv =
        exr_attr_list_find_by_name (ctxt, &(ctxt->custom_handlers), type, &ent);
    if (rv == EXR_ERR_SUCCESS)
        return EXR_UNLOCK_AND_RETURN (ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Attribute handler for '%s' previously registered",
            type));

    ent = NULL;
    rv  = exr_attr_list_add_by_type (
        ctxt, &(ctxt->custom_handlers), type, type, 0, NULL, &ent);
    if (rv != EXR_ERR_SUCCESS)
        return EXR_UNLOCK_AND_RETURN (ctxt->print_error (
            ctxt, rv, "Unable to register custom handler for type '%s'", type));

    ent->opaque->unpack_func_ptr           = unpack_func_ptr;
    ent->opaque->pack_func_ptr             = pack_func_ptr;
    ent->opaque->destroy_unpacked_func_ptr = destroy_unpacked_func_ptr;

    for (int p = 0; p < ctxt->num_parts; ++p)
    {
        curattrs = &(ctxt->parts[p]->attributes);
        if (curattrs)
        {
            int nattr = curattrs->num_attributes;
            for (int a = 0; a < nattr; ++a)
            {
                ent = curattrs->entries[a];
                if (ent->type_name_length == tlen &&
                    0 == strcmp (ent->type_name, type))
                {
                    ent->opaque->unpack_func_ptr = unpack_func_ptr;
                    ent->opaque->pack_func_ptr   = pack_func_ptr;
                    ent->opaque->destroy_unpacked_func_ptr =
                        destroy_unpacked_func_ptr;
                }
            }
        }
    }

    return EXR_UNLOCK_AND_RETURN (rv);
}

/**************************************/

exr_result_t
exr_set_longname_support (exr_context_t ctxt, int onoff)
{
    uint8_t oldval, newval;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;
    internal_exr_lock (ctxt);

    if (ctxt->mode != EXR_CONTEXT_WRITE && ctxt->mode != EXR_CONTEXT_TEMPORARY)
        return EXR_UNLOCK_AND_RETURN (
            ctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE));

    oldval = ctxt->max_name_length;
    newval = EXR_SHORTNAME_MAXLEN;
    if (onoff)
    {
        newval        = EXR_LONGNAME_MAXLEN;
        ctxt->version = 2;
    }
    else { ctxt->version = 1; }

    if (oldval > newval)
    {
        for (int p = 0; p < ctxt->num_parts; ++p)
        {
            exr_priv_part_t curp = ctxt->parts[p];
            for (int a = 0; a < curp->attributes.num_attributes; ++a)
            {
                exr_attribute_t* curattr = curp->attributes.entries[a];
                if (curattr->name_length > newval ||
                    curattr->type_name_length > newval)
                {
                    return EXR_UNLOCK_AND_RETURN (ctxt->print_error (
                        ctxt,
                        EXR_ERR_NAME_TOO_LONG,
                        "Part %d, attribute '%s' (type '%s') has a name too long for new longname setting (%d)",
                        curp->part_index,
                        curattr->name,
                        curattr->type_name,
                        (int) newval));
                }
                if (curattr->type == EXR_ATTR_CHLIST)
                {
                    exr_attr_chlist_t* chs = curattr->chlist;
                    for (int c = 0; c < chs->num_channels; ++c)
                    {
                        if (chs->entries[c].name.length > newval)
                        {
                            return EXR_UNLOCK_AND_RETURN (ctxt->print_error (
                                ctxt,
                                EXR_ERR_NAME_TOO_LONG,
                                "Part %d, channel '%s' has a name too long for new longname setting (%d)",
                                curp->part_index,
                                chs->entries[c].name.str,
                                (int) newval));
                        }
                    }
                }
            }
        }
    }
    ctxt->max_name_length = newval;
    return EXR_UNLOCK_AND_RETURN (EXR_ERR_SUCCESS);
}

/**************************************/

exr_result_t
exr_write_header (exr_context_t ctxt)
{
    exr_result_t rv = EXR_ERR_SUCCESS;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;
    internal_exr_lock (ctxt);

    if (ctxt->mode != EXR_CONTEXT_WRITE)
        return EXR_UNLOCK_AND_RETURN (
            ctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE));

    if (ctxt->num_parts == 0)
        return EXR_UNLOCK_AND_RETURN (ctxt->report_error (
            ctxt,
            EXR_ERR_FILE_BAD_HEADER,
            "No parts defined in file prior to writing data"));

    /* add part and set name should have already validated the uniqueness
     * so just ensure the name has been set for multi part files
     */
    for ( int p = ctxt->num_parts > 1 ? 0 : 1; p < ctxt->num_parts; ++p )
    {
        const exr_attribute_t* pname = ctxt->parts[p]->name;
        if (!pname)
        {
            return EXR_UNLOCK_AND_RETURN (
                ctxt->print_error (
                    ctxt,
                    EXR_ERR_INVALID_ARGUMENT,
                    "Part %d missing required name for multi-part file",
                    p));
        }
    }

    for (int p = 0; rv == EXR_ERR_SUCCESS && p < ctxt->num_parts; ++p)
    {
        exr_priv_part_t curp = ctxt->parts[p];

        int32_t ccount = 0;

        if (!curp->channels)
            return EXR_UNLOCK_AND_RETURN (ctxt->print_error (
                ctxt,
                EXR_ERR_MISSING_REQ_ATTR,
                "Part %d is missing channel list",
                p));

        rv = internal_exr_compute_tile_information (ctxt, curp, 0);
        if (rv != EXR_ERR_SUCCESS) break;

        ccount = internal_exr_compute_chunk_offset_size (curp);
        if (ccount < 0)
            return EXR_UNLOCK_AND_RETURN (
                ctxt->report_error (
                    ctxt,
                    EXR_ERR_FILE_BAD_HEADER,
                    "Invalid part specification computing number of chunks in file"));

        curp->chunk_count = ccount;

        if (ctxt->has_nonimage_data || ctxt->is_multipart)
        {
            internal_exr_unlock (ctxt);
            rv = exr_attr_set_int (ctxt, p, EXR_REQ_CHUNK_COUNT_STR, ccount);
            internal_exr_lock (ctxt);
            if (rv != EXR_ERR_SUCCESS) break;
        }

        rv = internal_exr_validate_write_part (ctxt, curp);
    }

    ctxt->output_file_offset = 0;

    if (rv == EXR_ERR_SUCCESS) rv = internal_exr_write_header (ctxt);

    if (rv == EXR_ERR_SUCCESS)
    {
        ctxt->mode               = EXR_CONTEXT_WRITING_DATA;
        ctxt->cur_output_part    = 0;
        ctxt->last_output_chunk  = -1;
        ctxt->output_chunk_count = 0;
        for (int p = 0; rv == EXR_ERR_SUCCESS && p < ctxt->num_parts; ++p)
        {
            exr_priv_part_t curp = ctxt->parts[p];

            curp->chunk_table_offset = ctxt->output_file_offset;
            ctxt->output_file_offset +=
                (uint64_t) (curp->chunk_count) * sizeof (uint64_t);
        }
    }

    return EXR_UNLOCK_AND_RETURN (rv);
}
